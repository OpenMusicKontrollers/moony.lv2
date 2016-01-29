/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <moony.h>

#include <uv.h>
#include <lv2_external_ui.h> // kxstudio external-ui extension

#include <http_parser.h>
#include <cJSON.h>

typedef struct _UI UI;
typedef struct _server_t server_t;
typedef struct _client_t client_t;

struct _server_t {
	uv_tcp_t http_server;
	client_t *clients;
	http_parser_settings http_settings;
	uv_timer_t timer;
};

struct _client_t {
	client_t *next;
	uv_tcp_t handle;
	http_parser parser;
	uv_write_t req;

	server_t *server;
	char url [1024];
	char *body;
	int keepalive;
};

struct _UI {
	struct {
		LV2_URID moony_message;
		LV2_URID moony_code;
		LV2_URID moony_error;
		LV2_URID event_transfer;
	} uris;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	LV2_Atom_Forge forge;
	LV2_URID_Map *map;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	LV2UI_Port_Map *port_map;
	uint32_t control_port;
	uint32_t notify_port;

	uv_loop_t loop;
	uv_process_t req;
	uv_process_options_t opts;
	int done;

	struct {
		LV2_External_UI_Widget widget;
		const LV2_External_UI_Host *host;
	} kx;

	uint8_t buf [0x10000];
	char path [512];

	char bundle_path [1024];
	server_t server;
};

static inline client_t *
_client_append(client_t *list, client_t *child)
{
	child->next = list;

	return child;
}

static inline client_t *
_client_remove(client_t *list, client_t *child)
{
	for(client_t *o=NULL, *l=list; l; o=l, l=l->next)
	{
		if(l == child)
		{
			if(o)
			{
				o->next = child->next; // skip child
				return list;
			}
			else // !o
			{
				return l->next; // skip child
			}
		}
	}

	return list;
}

#define SERVER_CLIENT_FOREACH(server, client) \
	for(client_t *(client)=(server)->clients; (client); (client)=(client)->next)

static inline unsigned 
_server_clients_count(server_t *server)
{
	unsigned count = 0;
	SERVER_CLIENT_FOREACH(server, client)
		count++;

	return count;
}

static inline void
_err2(UI *ui, const char *from)
{
	if(ui->log)
		lv2_log_error(&ui->logger, "%s", from);
	else
		fprintf(stderr, "%s\n", from);
}

static inline void
_err(UI *ui, const char *from, int ret)
{
	if(ui->log)
		lv2_log_error(&ui->logger, "%s: %s", from, uv_strerror(ret));
	else
		fprintf(stderr, "%s: %s\n", from, uv_strerror(ret));
}

static void
_moony_message_send(UI *ui, LV2_URID otype, LV2_URID key,
	uint32_t size, const char *str)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
	if(_moony_message_forge(&ui->forge, otype, key, size, str))
	{
		// trigger update
		ui->write_function(ui->controller, ui->control_port, lv2_atom_total_size(&obj->atom),
			ui->uris.event_transfer, ui->buf);
		if(ui->log)
			lv2_log_note(&ui->logger, str && size ? "send code chunk" : "query code chunk");
	}
	else if(ui->log)
		lv2_log_error(&ui->logger, "code chunk too long");
}

static inline void
_hide(UI *ui);

static void
_timeout(uv_timer_t *timer)
{
	UI *ui = timer->data;

	printf("_timeout\n");

	ui->done = 1;

	if(ui->kx.host && ui->kx.host->ui_closed && ui->controller)
	{
		_hide(ui);
		ui->kx.host->ui_closed(ui->controller);
	}
}

static void
_on_client_close(uv_handle_t *handle)
{
	client_t *client = handle->data;
	server_t *server = client->server;

	printf("_on_client_close\n");

	server->clients = _client_remove(server->clients, client);
	free(client);

	if(!server->clients) // no clients any more
	{
		printf("start timer\n");
		uv_timer_start(&server->timer, _timeout, 1000, 0); //TODO check
	}
}

static inline void
_hide(UI *ui)
{
	server_t *server = &ui->server;
	int ret;

	uv_timer_stop(&server->timer);

	if(uv_is_active((uv_handle_t *)&ui->req))
	{
		if((ret = uv_process_kill(&ui->req, SIGKILL)))
			_err(ui, "uv_process_kill", ret);
	}
	uv_close((uv_handle_t *)&ui->req, NULL);

	SERVER_CLIENT_FOREACH(server, client)
	{
		// cancel pending requests
		if(uv_is_active((uv_handle_t *)&client->req))
			uv_cancel((uv_req_t *)&client->req);

		// close client
		if(uv_is_active((uv_handle_t *)&client->handle))
		{
			if((ret = uv_read_stop((uv_stream_t *)&client->handle)))
				fprintf(stderr, "uv_read_stop: %s\n", uv_strerror(ret));
		}
		uv_close((uv_handle_t *)&client->handle, _on_client_close);
	}

	// close server
	uv_close((uv_handle_t *)&server->http_server, NULL);

	uv_stop(&ui->loop);
	uv_run(&ui->loop, UV_RUN_DEFAULT); // cleanup
	
	ui->done = 0;
}

static void
_on_exit(uv_process_t *req, int64_t exit_status, int term_signal)
{
	UI *ui = req ? (void *)req - offsetof(UI, req) : NULL;
	if(!ui)
		return;

	/* FIXME
	ui->done = 1;

	if(ui->kx.host && ui->kx.host->ui_closed && ui->controller)
	{
		_hide(ui);
		ui->kx.host->ui_closed(ui->controller);
	}
	*/
}

static inline char **
_parse_env(char *env, char *path)
{
	unsigned n = 0;
	char **args = malloc((n+1) * sizeof(char *));
	if(!args)
		goto fail;
	args[n] = NULL;

	char *pch = strtok(env," \t");
	while(pch)
	{
		args[n++] = pch;
		args = realloc(args, (n+1) * sizeof(char *));
		if(!args)
			goto fail;
		args[n] = NULL;

		pch = strtok(NULL, " \t");
	}

	args[n++] = path;
	args = realloc(args, (n+1) * sizeof(char *));
	if(!args)
		goto fail;
	args[n] = NULL;

	return args;

fail:
	if(args)
		free(args);
	return NULL;
}

static inline void
_show(UI *ui)
{
#if defined(_WIN32)
	const char *command = "cmd /c start";
#elif defined(__APPLE__)
	const char *command = "open";
#else // Linux/BSD
	const char *command = "xdg-open";
#endif

	// get default editor from environment
	const char *moony_editor = getenv("MOONY_BROWSER");
	if(!moony_editor)
		moony_editor = command;
	char *dup = strdup(moony_editor);
	char **args = dup ? _parse_env(dup, ui->path) : NULL;
	
	ui->opts.exit_cb = _on_exit;
	ui->opts.file = args ? args[0] : NULL;
	ui->opts.args = args;

	if(!uv_is_active((uv_handle_t *)&ui->req))
	{
		int ret;
		if((ret = uv_spawn(&ui->loop, &ui->req, &ui->opts)))
			_err(ui, "uv_spawn", ret);
	}

	if(dup)
		free(dup);
	if(args)
		free(args);
}

// External-UI Interface
static inline void
_kx_run(LV2_External_UI_Widget *widget)
{
	UI *ui = widget ? (void *)widget - offsetof(UI, kx.widget) : NULL;
	if(ui)
		uv_run(&ui->loop, UV_RUN_NOWAIT);
}

static inline void
_kx_hide(LV2_External_UI_Widget *widget)
{
	UI *ui = widget ? (void *)widget - offsetof(UI, kx.widget) : NULL;
	if(ui)
		_hide(ui);
}

static inline void
_kx_show(LV2_External_UI_Widget *widget)
{
	UI *ui = widget ? (void *)widget - offsetof(UI, kx.widget) : NULL;
	if(ui)
		_show(ui);
}

// Show Interface
static inline int
_show_cb(LV2UI_Handle instance)
{
	UI *ui = instance;
	if(ui)
		_show(ui);
	return 0;
}

static inline int
_hide_cb(LV2UI_Handle instance)
{
	UI *ui = instance;
	if(ui)
		_hide(ui);
	return 0;
}

static const LV2UI_Show_Interface show_ext = {
	.show = _show_cb,
	.hide = _hide_cb
};

// Idle interface
static inline int
_idle_cb(LV2UI_Handle instance)
{
	UI *ui = instance;
	if(!ui)
		return -1;
	uv_run(&ui->loop, UV_RUN_NOWAIT);
	return ui->done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle_cb
};

static void
_after_write(uv_write_t *req, int status)
{
	uv_tcp_t *handle = (uv_tcp_t *)req->handle;
	int ret;

	if(req->data)
		free(req->data);

	if(uv_is_active((uv_handle_t *)handle))
	{
		if((ret = uv_read_stop((uv_stream_t *)handle)))
			fprintf(stderr, "uv_read_stop: %s\n", uv_strerror(ret));
	}
	uv_close((uv_handle_t *)handle, _on_client_close);
}

enum {
	STATUS_NOT_FOUND,
	STATUS_OK
};

enum {
	CONTENT_APPLICATION_OCTET_STREAM,
	CONTENT_TEXT_HTML,
	CONTENT_TEXT_JSON,
	CONTENT_TEXT_CSS,
	CONTENT_TEXT_JS,
	CONTENT_TEXT_PLAIN,
	CONTENT_IMAGE_PNG
};

const char *http_status [] = {
	[STATUS_NOT_FOUND] = "HTTP/1.1 404 Not Found\r\n",
	[STATUS_OK] = "HTTP/1.1 200 OK\r\n"
};

const char *http_content [] = {
	[CONTENT_APPLICATION_OCTET_STREAM] = "Content-Type: application/octet-stream\r\n\r\n",
	[CONTENT_TEXT_HTML] = "Content-Type: text/html\r\n\r\n",
	[CONTENT_TEXT_JSON] = "Content-Type: text/json\r\n\r\n",
	[CONTENT_TEXT_CSS] = "Content-Type: text/css\r\n\r\n",
	[CONTENT_TEXT_JS] = "Content-Type: text/javascript\r\n\r\n",
	[CONTENT_TEXT_PLAIN] = "Content-Type: text/plain\r\n\r\n",
	[CONTENT_IMAGE_PNG] = "Content-Type: image/png\r\n\r\n"
};

static char *
_read_file(const char *bundle_path, const char *file_path, size_t *size)
{
	char *full_path;
	if(asprintf(&full_path, "%s/web_ui/%s", bundle_path, file_path) == -1)
		return NULL;

	FILE *f = fopen(full_path, "rb");
	free(full_path);
	if(f)
	{
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *str = malloc(fsize);
		if(str)
		{
			if(fread(str, fsize, 1, f) == 1)
			{
				*size = fsize;
				return str; // success
			}

			free(str);
		}

		fclose(f);
	}

	return NULL;
}

static int
_on_message_complete(http_parser *parser)
{
	client_t *client = parser->data;
	server_t *server = client->server;
	UI *ui = (void *)server - offsetof(UI, server);

	//printf("_on_message_complete\n");
	//FIXME
	
	const char *stat = NULL;
	const char *cont = NULL;
	char *chunk = NULL;

	size_t size;
	if(  !strcmp(client->url, "/")
		|| !strcmp(client->url, "/index.html") )
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_HTML];
		chunk = _read_file(ui->bundle_path, "/index.html", &size);
	}
	else if(!strcmp(client->url, "/favicon.png"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_IMAGE_PNG];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/jquery-2.2.0.min.js"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/moony.js"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/style.css"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_CSS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/Chango-Regular.ttf"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_APPLICATION_OCTET_STREAM];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/ace.js"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/mode-lua.js"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/keybinding-vim.js"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/keybinding-emacs.js"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/theme-chaos.js"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JS];
		chunk = _read_file(ui->bundle_path, client->url, &size);
	}
	else if(!strcmp(client->url, "/keepalive"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JSON];
		client->keepalive = 1;
		// keepalive
	}
	else if(!strcmp(client->url, "/code/get"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JSON];
		_moony_message_send(ui, ui->uris.moony_message, ui->uris.moony_code, 0, NULL);
		chunk = strdup("{}");
	}
	else if(!strcmp(client->url, "/code/set"))
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JSON];
		cJSON *root = cJSON_Parse(client->body);
		if(root)
		{
			cJSON *code = cJSON_GetObjectItem(root, "code");
			if(code)
			{
				_moony_message_send(ui, ui->uris.moony_message, ui->uris.moony_code, strlen(code->valuestring)+1, code->valuestring);
			}
			cJSON_Delete(root);
		}
		free(client->body);
		chunk = strdup("{}");
	}
	else
	{
		stat = http_status[STATUS_NOT_FOUND];
	}

	//printf("chunk: %zu\n", size);

	client->req.data = chunk;

	uv_buf_t msg [3] = {
		[0] = {
			.base = (char *)stat,
			.len = stat ? strlen(stat) : 0
		},
		[1] = {
			.base = (char *)cont,
			.len = cont ? strlen(cont) : 0
		},
		[2] = {
			.base = client->req.data,
			.len = size
		}
	};

	if(stat == http_status[STATUS_NOT_FOUND])
		uv_write(&client->req, (uv_stream_t *)&client->handle, msg, 1, _after_write);
	else if(chunk)
		uv_write(&client->req, (uv_stream_t *)&client->handle, msg, 3, _after_write);
	// else keepalive

	return 0;
}

static int
_on_url(http_parser *parser, const char *at, size_t len)
{
	client_t *client = parser->data;

	snprintf(client->url, len+1, "%s", at);

	printf("_on_url: %s\n", client->url);
	//FIXME

	return 0;
}

static int
_on_body(http_parser *parser, const char *at, size_t len)
{
	client_t *client = parser->data;

	//printf("_on_body\n");
	//FIXME
	client->body = strndup(at, len);

	return 0;
}

static void
_on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	//client_t *client = handle->data;

	buf->base = malloc(suggested_size);
	buf->len = buf->base ? suggested_size : 0;
}

static void
_on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
	uv_tcp_t *handle = (uv_tcp_t *)stream;
	client_t *client = handle->data;
	server_t *server = client->server;
	int ret;

	if(nread >= 0)
	{
		ssize_t parsed = http_parser_execute(&client->parser, &server->http_settings, buf->base, nread);

		if(parsed < nread)
		{
			fprintf(stderr, "_on_read: http parse error\n");
			if(uv_is_active((uv_handle_t *)handle))
			{
				if((ret = uv_read_stop((uv_stream_t *)handle)))
					fprintf(stderr, "uv_read_stop: %s\n", uv_strerror(ret));
			}
			uv_close((uv_handle_t *)handle, _on_client_close);
		}
	}
	else
	{
		if(nread != UV_EOF)
			fprintf(stderr, "_on_read: %s\n", uv_strerror(nread));
		if(uv_is_active((uv_handle_t *)handle))
		{
			if((ret = uv_read_stop((uv_stream_t *)handle)))
				fprintf(stderr, "uv_read_stop: %s\n", uv_strerror(ret));
		}
		uv_close((uv_handle_t *)handle, _on_client_close);
	}

	if(buf->base)
		free(buf->base);
}

static void
_on_connected(uv_stream_t *handle, int status)
{
	server_t *server = handle->data;
	int ret;

	client_t *client = calloc(1, sizeof(client_t)); //TODO check
	client->server = server;
	server->clients = _client_append(server->clients, client);
	printf("clients: %u\n", _server_clients_count(server));

	if((ret = uv_tcp_init(handle->loop, &client->handle)))
	{
		free(client);
		fprintf(stderr, "uv_tcp_init: %s\n", uv_strerror(ret));
		return;
	}

	if((ret = uv_accept(handle, (uv_stream_t *)&client->handle)))
	{
		free(client);
		fprintf(stderr, "uv_accept: %s\n", uv_strerror(ret));
		return;
	}

	client->handle.data = client;
	client->parser.data = client;

	http_parser_init(&client->parser, HTTP_REQUEST);

	if((ret = uv_read_start((uv_stream_t *)&client->handle, _on_alloc, _on_read)))
	{
		free(client);
		fprintf(stderr, "uv_read_start: %s\n", uv_strerror(ret));
		return;
	}

	uv_timer_stop(&server->timer);
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	if(		strcmp(plugin_uri, MOONY_C1XC1_URI)
		&&	strcmp(plugin_uri, MOONY_C2XC2_URI)
		&&	strcmp(plugin_uri, MOONY_C4XC4_URI)

		&&	strcmp(plugin_uri, MOONY_A1XA1_URI)
		&&	strcmp(plugin_uri, MOONY_A2XA2_URI)
		&&	strcmp(plugin_uri, MOONY_A4XA4_URI)

		&&	strcmp(plugin_uri, MOONY_C1A1XC1A1_URI)
		&&	strcmp(plugin_uri, MOONY_C2A1XC2A1_URI)
		&&	strcmp(plugin_uri, MOONY_C4A1XC4A1_URI) )
	{
		return NULL;
	}

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			ui->port_map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			ui->log = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) && (descriptor == &web_kx))
			ui->kx.host = features[i]->data;
	}

	sprintf(ui->bundle_path, "%s", bundle_path);

	ui->kx.widget.run = _kx_run;
	ui->kx.widget.show = _kx_show;
	ui->kx.widget.hide = _kx_hide;

	if(descriptor == &web_kx)
		*(LV2_External_UI_Widget **)widget = &ui->kx.widget;
	else
		*(void **)widget = NULL;

	if(!ui->map)
	{
		fprintf(stderr, "%s: Host does not support urid:map\n", descriptor->URI);
		free(ui);
		return NULL;
	}
	if(!ui->port_map)
	{
		fprintf(stderr, "%s: Host does not support ui:portMap\n", descriptor->URI);
		free(ui);
		return NULL;
	}

	// query port index of "control" port
	ui->control_port = ui->port_map->port_index(ui->port_map->handle, "control");
	ui->notify_port = ui->port_map->port_index(ui->port_map->handle, "notify");

	ui->uris.moony_message = ui->map->map(ui->map->handle, MOONY_MESSAGE_URI);
	ui->uris.moony_code = ui->map->map(ui->map->handle, MOONY_CODE_URI);
	ui->uris.moony_error = ui->map->map(ui->map->handle, MOONY_ERROR_URI);
	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	lv2_atom_forge_init(&ui->forge, ui->map);
	if(ui->log)
		lv2_log_logger_init(&ui->logger, ui->map, ui->log);

	ui->write_function = write_function;
	ui->controller = controller;

	int ret;
	if((ret = uv_loop_init(&ui->loop)))
	{
		fprintf(stderr, "%s: %s\n", descriptor->URI, uv_strerror(ret));
		free(ui);
		return NULL;
	}

	uint16_t port = 9091; //FIXME
	//sprintf(ui->path, "file://%s/index.html", bundle_path);
	sprintf(ui->path, "http://localhost:9091");

	server_t *server = &ui->server;
	server->http_settings.on_message_begin = NULL;
	server->http_settings.on_message_complete= _on_message_complete;
	server->http_settings.on_headers_complete= NULL;

	server->http_settings.on_url = _on_url;
	server->http_settings.on_header_field = NULL;
	server->http_settings.on_header_value = NULL;
	server->http_settings.on_body = _on_body;
	server->http_server.data = server;

	if((ret = uv_timer_init(&ui->loop, &server->timer)))
	{
		fprintf(stderr, "uv_timer_init: %s\n", uv_strerror(ret));
		free(ui);
		return NULL;
	}
	server->timer.data = ui;

	if((ret = uv_tcp_init(&ui->loop, &server->http_server)))
	{
		fprintf(stderr, "uv_tcp_init: %s\n", uv_strerror(ret));
		free(ui);
		return NULL;
	}

	struct sockaddr_in addr_ip4;
	struct sockaddr *addr = (struct sockaddr *)&addr_ip4;

	if((ret = uv_ip4_addr("0.0.0.0", port, &addr_ip4)))
	{
		fprintf(stderr, "uv_ip4_addr: %s\n", uv_strerror(ret));
		free(ui);
		return NULL;
	}

	if((ret = uv_tcp_bind(&server->http_server, addr, 0)))
	{
		fprintf(stderr, "bind: %s\n", uv_strerror(ret));
		free(ui);
		return NULL;
	}

	if((ret = uv_listen((uv_stream_t *)&server->http_server, 128, _on_connected)))
	{
		fprintf(stderr, "listen %s\n", uv_strerror(ret));
		free(ui);
		return NULL;
	}

	printf("start timer\n");
	uv_timer_start(&server->timer, _timeout, 10000, 0); //TODO check

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	uv_loop_close(&ui->loop);
}

static void
port_event(LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	UI *ui = handle;

	if( (port_index == ui->notify_port) && (format == ui->uris.event_transfer) )
	{
		const LV2_Atom_Object *obj = buffer;

		if(  lv2_atom_forge_is_object_type(&ui->forge, obj->atom.type)
			&& (obj->body.otype == ui->uris.moony_message) )
		{
			const LV2_Atom_String *moony_error = NULL;
			const LV2_Atom_String *moony_code = NULL;
			
			LV2_Atom_Object_Query q[] = {
				{ ui->uris.moony_error, (const LV2_Atom **)&moony_error },
				{ ui->uris.moony_code, (const LV2_Atom **)&moony_code },
				LV2_ATOM_OBJECT_QUERY_END
			};
			lv2_atom_object_query(obj, q);

			if(moony_code)
			{
				const char *str = LV2_ATOM_BODY_CONST(&moony_code->atom);

				//printf("code is: %s\n", str);

				SERVER_CLIENT_FOREACH(&ui->server, client)
				{
					if(!client->keepalive)
						continue;

					const char *stat = http_status[STATUS_OK];
					const char *cont = http_content[CONTENT_TEXT_JSON];

					cJSON *root = cJSON_CreateObject();
					cJSON *code = cJSON_CreateString(str);
					cJSON_AddItemToObject(root, "code", code);

					client->req.data = cJSON_PrintUnformatted(root);
					cJSON_Delete(root);
					//printf("json: %s\n", client->req.data);
					if(!client->req.data)
						continue;

					uv_buf_t msg [5] = {
						[0] = {
							.base = (char *)stat,
							.len = stat ? strlen(stat) : 0
						},
						[1] = {
							.base = (char *)cont,
							.len = cont ? strlen(cont) : 0
						},
						[2] = {
							.base = client->req.data,
							.len = strlen(client->req.data)
						}
					};

					uv_write(&client->req, (uv_stream_t *)&client->handle, msg, 3, _after_write);
				}
			}
			else if(moony_error)
			{
				const char *str = LV2_ATOM_BODY_CONST(&moony_error->atom);

				//printf("error is: %s\n", str);

				SERVER_CLIENT_FOREACH(&ui->server, client)
				{
					if(!client->keepalive)
						continue;

					const char *stat = http_status[STATUS_OK];
					const char *cont = http_content[CONTENT_TEXT_JSON];

					cJSON *root = cJSON_CreateObject();
					cJSON *err = cJSON_CreateString(str);
					cJSON_AddItemToObject(root, "error", err);

					client->req.data = cJSON_PrintUnformatted(root);
					cJSON_Delete(root);
					//printf("json: %s\n", client->req.data);
					if(!client->req.data)
						continue;

					uv_buf_t msg [5] = {
						[0] = {
							.base = (char *)stat,
							.len = stat ? strlen(stat) : 0
						},
						[1] = {
							.base = (char *)cont,
							.len = cont ? strlen(cont) : 0
						},
						[2] = {
							.base = client->req.data,
							.len = strlen(client->req.data)
						}
					};

					uv_write(&client->req, (uv_stream_t *)&client->handle, msg, 3, _after_write);
				}
			}
		}
	}
}

static const void *
ui_extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__showInterface))
		return &show_ext;
		
	return NULL;
}

const LV2UI_Descriptor web_ui = {
	.URI						= MOONY_WEB_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= ui_extension_data
};

const LV2UI_Descriptor web_kx = {
	.URI						= MOONY_WEB_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= NULL
};
