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

#define RDF_PREFIX "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDFS_PREFIX "http://www.w3.org/2000/01/rdf-schema#"

#define RDF__value RDF_PREFIX"value"
#define RDF__type RDF_PREFIX"type"

#define RDFS__label RDFS_PREFIX"label"
#define RDFS__range RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

#define MAX_PORTS 18

#ifndef LV2_UI__protocol
#	define LV2_UI__protocol         LV2_UI_PREFIX "protocol"
#	define LV2_UI__floatProtocol    LV2_UI_PREFIX "floatProtocol"
#	define LV2_UI__peakProtocol     LV2_UI_PREFIX "peakProtocol"
#endif

#ifdef LV2_ATOM_TUPLE_FOREACH
#	undef LV2_ATOM_TUPLE_FOREACH
#	define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))
#endif

typedef struct _UI UI;
typedef struct _server_t server_t;
typedef struct _client_t client_t;
typedef struct _url_t url_t;
typedef struct _pmap_t pmap_t;

struct _pmap_t {
	uint32_t index;
	const char *symbol;
	float val;
};

struct _server_t {
	uv_tcp_t http_server;
	client_t *clients;
	http_parser_settings http_settings;
	uv_timer_t timer;
	cJSON *root;
};

struct _client_t {
	client_t *next;
	uv_tcp_t handle;
	http_parser parser;
	uv_write_t req;

	server_t *server;
	char url [1024];
	const char *body;
	int keepalive;
};

struct _UI {
	struct {
		LV2_URID moony_message;
		LV2_URID moony_code;
		LV2_URID moony_selection;
		LV2_URID moony_error;
		LV2_URID moony_trace;
		LV2_URID window_title;

		LV2_URID subject;

		patch_t patch;

		LV2_URID ui_float_protocol;
		LV2_URID ui_peak_protocol;
		LV2_URID atom_event_transfer;
		LV2_URID atom_atom_transfer;
	} uris;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	LV2_Atom_Forge forge;
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	LV2UI_Port_Map *port_map;
	uint32_t control_port;
	pmap_t pmap [MAX_PORTS];

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
	char title [512];

	char bundle_path [512];
	server_t server;
};

struct _url_t {
	const char *alias;
	const char *url;
	int content_type;
};

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

static const char *http_status [] = {
	[STATUS_NOT_FOUND] = "HTTP/1.1 404 Not Found\r\n",
	[STATUS_OK] = "HTTP/1.1 200 OK\r\n"
};

static const char *http_content [] = {
	[CONTENT_APPLICATION_OCTET_STREAM] = "Content-Type: application/octet-stream\r\n",
	[CONTENT_TEXT_HTML] = "Content-Type: text/html\r\n",
	[CONTENT_TEXT_JSON] = "Content-Type: text/json\r\n",
	[CONTENT_TEXT_CSS] = "Content-Type: text/css\r\n",
	[CONTENT_TEXT_JS] = "Content-Type: text/javascript\r\n",
	[CONTENT_TEXT_PLAIN] = "Content-Type: text/plain\r\n",
	[CONTENT_IMAGE_PNG] = "Content-Type: image/png\r\n"
};

static const char *content_length = "Content-Length: %"PRIuPTR"\r\n\r\n%s";

//TODO keep updated
static const url_t valid_urls [] = {
	{ .alias = NULL, .url = "/favicon.png",         .content_type = CONTENT_IMAGE_PNG },
	{ .alias = NULL, .url = "/jquery-2.2.0.min.js", .content_type = CONTENT_TEXT_JS },
	{ .alias = NULL, .url = "/moony.js",            .content_type = CONTENT_TEXT_JS },
	{ .alias = NULL, .url = "/style.css",           .content_type = CONTENT_TEXT_CSS },
	{ .alias = NULL, .url = "/Chango-Regular.ttf",  .content_type = CONTENT_APPLICATION_OCTET_STREAM },
	{ .alias = NULL, .url = "/ace.js",              .content_type = CONTENT_TEXT_JS },
	{ .alias = NULL, .url = "/mode-lua.js",         .content_type = CONTENT_TEXT_JS },
	{ .alias = NULL, .url = "/theme-chaos.js",      .content_type = CONTENT_TEXT_JS },
	{ .alias = NULL, .url = "/keybinding-vim.js",   .content_type = CONTENT_TEXT_JS },
	{ .alias = NULL, .url = "/keybinding-emacs.js", .content_type = CONTENT_TEXT_JS },
	{ .alias = "/",  .url = "/index.html",          .content_type = CONTENT_TEXT_HTML },

	{ .url = NULL } // sentinel
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

static inline unsigned 
_server_clients_keepalive_count(server_t *server)
{
	unsigned count = 0;
	SERVER_CLIENT_FOREACH(server, client)
		if(client->keepalive)
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

static inline LV2_Atom_Object *
_moony_message_forge(UI *ui, LV2_URID key,
	const char *str, uint32_t size)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
	if(_moony_patch(&ui->uris.patch, &ui->forge, key, str, size))
		return obj;

	if(ui->log)
		lv2_log_error(&ui->logger, "code chunk too long");
	return NULL;
}

static inline LV2_Atom_Forge_Ref
_json_to_atom(UI *ui, cJSON *root, LV2_Atom_Forge *forge)
{
	assert(root->type == cJSON_Object);

	cJSON *range = cJSON_GetObjectItem(root, RDFS__range);
	cJSON *value = cJSON_GetObjectItem(root, RDF__value);

	assert(range->type == cJSON_String);

	if(!strcmp(range->valuestring, LV2_ATOM__Int) && (value->type == cJSON_Number) )
		return lv2_atom_forge_int(forge, value->valueint);
	else if(!strcmp(range->valuestring, LV2_ATOM__Long) && (value->type == cJSON_Number) )
		return lv2_atom_forge_long(forge, value->valueint);
	else if(!strcmp(range->valuestring, LV2_ATOM__Float) && (value->type == cJSON_Number) )
		return lv2_atom_forge_float(forge, value->valuedouble);
	else if(!strcmp(range->valuestring, LV2_ATOM__Double) && (value->type == cJSON_Number))
		return lv2_atom_forge_double(forge, value->valuedouble);
	else if(!strcmp(range->valuestring, LV2_ATOM__Bool) && (value->type == cJSON_True) )
		return lv2_atom_forge_bool(forge, 1);
	else if(!strcmp(range->valuestring, LV2_ATOM__Bool) && (value->type == cJSON_False) )
		return lv2_atom_forge_bool(forge, 0);
	else if(!strcmp(range->valuestring, LV2_ATOM__URID) && (value->type == cJSON_String) )
		return lv2_atom_forge_urid(forge, ui->map->map(ui->map->handle, value->valuestring));
	else if(!strcmp(range->valuestring, LV2_ATOM__String) && (value->type == cJSON_String) )
		return lv2_atom_forge_string(forge, value->valuestring, strlen(value->valuestring));
	else if(!strcmp(range->valuestring, LV2_ATOM__URI) && (value->type == cJSON_String) )
		return lv2_atom_forge_uri(forge, value->valuestring, strlen(value->valuestring));
	else if(!strcmp(range->valuestring, LV2_ATOM__Path) && (value->type == cJSON_String) )
		return lv2_atom_forge_path(forge, value->valuestring, strlen(value->valuestring));
	//TODO literal, chunk
	else if(!strcmp(range->valuestring, LV2_ATOM__Tuple) && (value->type == cJSON_Array) )
	{
		LV2_Atom_Forge_Frame frame;
		LV2_Atom_Forge_Ref ref = lv2_atom_forge_tuple(forge, &frame);
		for(cJSON *child = value->child; ref && child; child = child->next)
		{
			ref = _json_to_atom(ui, child, forge);
		}
		if(ref)
			lv2_atom_forge_pop(forge, &frame);
		return ref;
	}
	else if(!strcmp(range->valuestring, LV2_ATOM__Object) && (value->type == cJSON_Array) )
	{
		cJSON *type = cJSON_GetObjectItem(root, RDF__type);
		const LV2_URID otype = type && (type->type == cJSON_String)
			? ui->map->map(ui->map->handle, type->valuestring)
			: 0;

		LV2_Atom_Forge_Frame frame;
		LV2_Atom_Forge_Ref ref = lv2_atom_forge_object(forge, &frame, 0, otype);
		for(cJSON *item = value->child; ref && item; item = item->next)
		{
			cJSON *_key = cJSON_GetObjectItem(item, LV2_ATOM__Property);
			cJSON *child = cJSON_GetObjectItem(item, RDF__value);

			if(_key && child)
			{
				const LV2_URID key = ui->map->map(ui->map->handle, _key->valuestring);
				ref = lv2_atom_forge_key(forge, key)
					&& _json_to_atom(ui, child, forge);
			}
		}
		if(ref)
			lv2_atom_forge_pop(forge, &frame);
		return ref;
	}
	else if(!strcmp(range->valuestring, LV2_ATOM__Sequence) && (value->type == cJSON_Array) )
	{
		LV2_Atom_Forge_Frame frame;
		LV2_Atom_Forge_Ref ref = lv2_atom_forge_sequence_head(forge, &frame, 0);
		for(cJSON *child = value->child; ref && child; child = child->next)
		{
			assert(child->type == cJSON_Object);
			for(cJSON *item = child->child; ref && item; item = item->next)
			{
				cJSON *frames = cJSON_GetObjectItem(item, LV2_ATOM__frameTime);
				cJSON *beats = cJSON_GetObjectItem(item, LV2_ATOM__beatTime);
				cJSON *event = cJSON_GetObjectItem(item, LV2_ATOM__Event);
				if(ref && frames && (frames->type == cJSON_Number) )
					ref = lv2_atom_forge_frame_time(forge, frames->valueint);
				else if(ref && beats && (beats->type == cJSON_Number) )
					ref = lv2_atom_forge_beat_time(forge, beats->valuedouble);

				if(ref && event)
					ref = _json_to_atom(ui, event, forge);
			}
		}
		if(ref)
			lv2_atom_forge_pop(forge, &frame);
		return ref;
	}
	//TODO vector

	return 0;
}

static inline cJSON *
_atom_to_json(UI *ui, const LV2_Atom *atom)
{
	cJSON *range = NULL;
	cJSON *value = NULL;
	cJSON *type = NULL;

	if(atom->type == ui->forge.Int)
	{
		range = cJSON_CreateString(LV2_ATOM__Int);
		value = cJSON_CreateNumber(((const LV2_Atom_Int *)atom)->body);
	}
	else if(atom->type == ui->forge.Long)
	{
		range = cJSON_CreateString(LV2_ATOM__Long);
		value = cJSON_CreateNumber(((const LV2_Atom_Long *)atom)->body);
	}
	else if(atom->type == ui->forge.Float)
	{
		range = cJSON_CreateString(LV2_ATOM__Float);
		value = cJSON_CreateNumber(((const LV2_Atom_Float *)atom)->body);
	}
	else if(atom->type == ui->forge.Double)
	{
		range = cJSON_CreateString(LV2_ATOM__Double);
		value = cJSON_CreateNumber(((const LV2_Atom_Double *)atom)->body);
	}
	else if(atom->type == ui->forge.Bool)
	{
		range = cJSON_CreateString(LV2_ATOM__Bool);
		if(((const LV2_Atom_Bool *)atom)->body)
			value = cJSON_CreateTrue();
		else
			value = cJSON_CreateFalse();
	}
	else if(atom->type == ui->forge.URID) // URID -> URI
	{
		range = cJSON_CreateString(LV2_ATOM__URID);
		value = cJSON_CreateString(ui->unmap->unmap(ui->unmap->handle, ((const LV2_Atom_URID *)atom)->body));
	}
	else if(atom->type == ui->forge.String)
	{
		range = cJSON_CreateString(LV2_ATOM__String);
		value = cJSON_CreateString(LV2_ATOM_BODY_CONST(atom));
	}
	else if(atom->type == ui->forge.URI)
	{
		range = cJSON_CreateString(LV2_ATOM__URI);
		value = cJSON_CreateString(LV2_ATOM_BODY_CONST(atom));
	}
	else if(atom->type == ui->forge.Path)
	{
		range = cJSON_CreateString(LV2_ATOM__Path);
		value = cJSON_CreateString(LV2_ATOM_BODY_CONST(atom));
	}
	//todo literal, chunk
	else if(atom->type == ui->forge.Tuple)
	{
		const LV2_Atom_Tuple *tup = (const LV2_Atom_Tuple *)atom;
		range = cJSON_CreateString(LV2_ATOM__Tuple);
		value = cJSON_CreateArray();
		LV2_ATOM_TUPLE_FOREACH(tup, item)
		{
			cJSON *child = _atom_to_json(ui, item);
			if(value && child)
				cJSON_AddItemToArray(value, child);
		}
	}
	else if(atom->type == ui->forge.Object)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)atom;
		range = cJSON_CreateString(LV2_ATOM__Object);
		const char *otype = ui->unmap->unmap(ui->unmap->handle, obj->body.otype);
		if(otype)
			type = cJSON_CreateString(otype);
		value = cJSON_CreateArray();
		if(value)
		{
			LV2_ATOM_OBJECT_FOREACH(obj, prop)
			{
				cJSON *item = cJSON_CreateObject();
				if(item)
				{
					const char *key = ui->unmap->unmap(ui->unmap->handle, prop->key);
					cJSON *_key = key ? cJSON_CreateString(key) : NULL;
					cJSON *child = _atom_to_json(ui, &prop->value);
					if(_key)
						cJSON_AddItemToObject(item, LV2_ATOM__Property, _key);
					if(child)
						cJSON_AddItemToObject(item, RDF__value, child);
					cJSON_AddItemToArray(value, item);
				}
			}
		}
	}
	else if(atom->type == ui->forge.Sequence)
	{
		const LV2_Atom_Sequence *seq = (const LV2_Atom_Sequence *)atom;
		range = cJSON_CreateString(LV2_ATOM__Sequence);
		value = cJSON_CreateArray();
		LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
		{
			cJSON *event = cJSON_CreateObject();
			cJSON *time = NULL;
			if(seq->body.unit == 0)
				time = cJSON_CreateNumber(ev->time.frames);
			else
				time = cJSON_CreateNumber(ev->time.beats);
			cJSON *child = _atom_to_json(ui, &ev->body);
			if(value && event && time && child)
			{
				if(seq->body.unit == 0)
					cJSON_AddItemToObject(event, LV2_ATOM__frameTime, time);
				else
					cJSON_AddItemToObject(event, LV2_ATOM__beatTime, time);
				cJSON_AddItemToObject(event, LV2_ATOM__Event, child);
				cJSON_AddItemToArray(value, event);
			}
		}
	}
	//TODO vector

	cJSON *root = cJSON_CreateObject();
	if(root && range && value)
	{
		cJSON_AddItemToObject(root, RDFS__range, range);
		if(type)
			cJSON_AddItemToObject(root, RDF__type, type);
		cJSON_AddItemToObject(root, RDF__value, value);
	}

	return root;
}

static int
_pmap_cmp(const void *data1, const void *data2)
{
	const pmap_t *pmap1 = data1;
	const pmap_t *pmap2 = data2;

	if(pmap1->index > pmap2->index)
		return 1;
	else if(pmap1->index < pmap2->index)
		return -1;
	return 0;
}

static inline pmap_t *
_pmap_get(UI *ui, uint32_t idx)
{
	const pmap_t tar = { .index = idx };
	pmap_t *pmap = bsearch(&tar, ui->pmap, MAX_PORTS, sizeof(pmap_t), _pmap_cmp);
	return pmap;
}

static inline void
_moony_dsp(UI *ui, const char *json)
{
	cJSON *root = cJSON_Parse(json);
	if(root)
	{
		cJSON *protocol = cJSON_GetObjectItem(root, LV2_UI__protocol);
		cJSON *symbol = cJSON_GetObjectItem(root, LV2_CORE__symbol);
		cJSON *value = cJSON_GetObjectItem(root, RDF__value);

		if(  protocol && symbol && value
			&& (protocol->type == cJSON_String)
			&& (symbol->type == cJSON_String) )
		{
			if(!strcmp(protocol->valuestring, LV2_UI__floatProtocol)
				&& (value->type == cJSON_Number) )
			{
				uint32_t idx = ui->port_map->port_index(ui->port_map->handle, symbol->valuestring);
				const float val = value->valuedouble;
				ui->write_function(ui->controller, idx, sizeof(float),
					0, &val);

				// intercept control port changes
				pmap_t *pmap = _pmap_get(ui, idx);
				if(pmap)
					pmap->val = val;
			}
			else if(!strcmp(protocol->valuestring, LV2_ATOM__eventTransfer)
				&& (value->type == cJSON_Object) )
			{
				uint32_t idx = ui->port_map->port_index(ui->port_map->handle, symbol->valuestring);
				lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
				if(_json_to_atom(ui, value, &ui->forge))
				{
					const LV2_Atom *atom = (const LV2_Atom *)ui->buf;
					ui->write_function(ui->controller, idx, lv2_atom_total_size(atom),
						ui->uris.atom_event_transfer, atom);
				}
				else if(ui->log)
					lv2_log_error(&ui->logger, "_moony_dsp: forge buffer overflow");
			}
			else if(!strcmp(protocol->valuestring, LV2_ATOM__atomTransfer)
				&& (value->type == cJSON_Object) )
			{
				//FIXME
			}
		}
		else if(ui->log)
			lv2_log_error(&ui->logger, "_moony_dsp: missing protocol, symbol or value");
		cJSON_Delete(root);
	}
}

static inline void
_hide(UI *ui);

static void
_timeout(uv_timer_t *timer)
{
	UI *ui = timer->data;

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
	UI *ui = (void *)server - offsetof(UI, server);

	server->clients = _client_remove(server->clients, client);
	free(client);

	if(!server->clients) // no clients any more
	{
		int ret;
		if((ret = uv_timer_start(&server->timer, _timeout, 5000, 0)))
			_err(ui, "uv_timer_start", ret);
	}
}

static void
_after_write(uv_write_t *req, int status)
{
	uv_tcp_t *handle = (uv_tcp_t *)req->handle;
	client_t *client = handle->data;
	server_t *server = client->server;
	UI *ui = (void *)server - offsetof(UI, server);

	int ret;

	if(req->data)
		free(req->data);

	if(uv_is_active((uv_handle_t *)handle))
	{
		if((ret = uv_read_stop((uv_stream_t *)handle)))
			_err(ui, "uv_read_stop", ret);
	}
	if(!uv_is_closing((uv_handle_t *)handle))
		uv_close((uv_handle_t *)handle, _on_client_close);
}

static inline void
_keepalive(server_t *server)
{
	const char *stat = http_status[STATUS_OK];
	const char *cont = http_content[CONTENT_TEXT_JSON];

	if(server->root && (_server_clients_keepalive_count(server) > 0) )
	{
		SERVER_CLIENT_FOREACH(server, client)
		{
			if(!client->keepalive)
				continue;

			char *json = cJSON_PrintUnformatted(server->root);
			if(json)
			{
				char *chunk = NULL;
				if(asprintf(&chunk, content_length, strlen(json), json) != -1)
				{
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
							.len = strlen(chunk) 
						}
					};

					uv_write(&client->req, (uv_stream_t *)&client->handle, msg, 3, _after_write);
				}
				free(json);
			}

			client->keepalive = false;
		}

		cJSON_Delete(server->root);
		server->root = NULL;
	}
}

static inline void
_schedule(UI *ui, cJSON *job)
{
	server_t *server = &ui->server;

	if(!server->root)
	{
		// create root object
		server->root = cJSON_CreateObject();
		cJSON *jobs = cJSON_CreateArray();
		if(server->root && jobs)
			cJSON_AddItemToObject(server->root, "jobs", jobs);
	}

	if(server->root)
	{
		// add job to jobs array
		cJSON *jobs = cJSON_GetObjectItem(server->root, "jobs");
		if(jobs)
			cJSON_AddItemToArray(jobs, job);
	}

	_keepalive(server); // answer keep alives
}

static inline cJSON * 
_moony_send(UI *ui, uint32_t idx, uint32_t size, uint32_t prot, const void *buf)
{
	cJSON *protocol = NULL;
	cJSON *value = NULL;

	if( ((prot == ui->uris.ui_float_protocol) || (prot == 0)) && (size == sizeof(float)) )
	{
		const float *val = buf;
		protocol = cJSON_CreateString(LV2_UI__floatProtocol);
		value = cJSON_CreateNumber(*val);
	}
	else if( (prot == ui->uris.ui_peak_protocol) && (size == sizeof(LV2UI_Peak_Data)) )
	{
		const LV2UI_Peak_Data *peak_data = buf;
		protocol = cJSON_CreateString(LV2_UI__peakProtocol);
		value = cJSON_CreateObject();
		if(value)
		{
			cJSON *period_start = cJSON_CreateNumber(peak_data->period_start);
			cJSON *period_size = cJSON_CreateNumber(peak_data->period_size);
			cJSON *peak = cJSON_CreateNumber(peak_data->peak);
			if(period_start)
				cJSON_AddItemToObject(value, LV2_UI_PREFIX"periodStart", period_start);
			if(period_size)
				cJSON_AddItemToObject(value, LV2_UI_PREFIX"periodSize", period_size);
			if(peak)
				cJSON_AddItemToObject(value, LV2_UI_PREFIX"peak", peak);
		}
	}
	else if(prot == ui->uris.atom_event_transfer)
	{
		const LV2_Atom *atom = buf;
		protocol = cJSON_CreateString(LV2_ATOM__eventTransfer);
		value = _atom_to_json(ui, atom);
	}
	else if(prot == ui->uris.atom_atom_transfer)
	{
		const LV2_Atom *atom = buf;
		protocol = cJSON_CreateString(LV2_ATOM__atomTransfer);
		value = _atom_to_json(ui, atom);
	}
	else
		return NULL;

	cJSON *root = cJSON_CreateObject();
	if(root)
	{
		cJSON *symbol = NULL;

		pmap_t *pmap = _pmap_get(ui, idx);
		if(pmap)
			symbol = cJSON_CreateString(pmap->symbol);

		if(symbol)
			cJSON_AddItemToObject(root, LV2_CORE__symbol, symbol);

		cJSON_AddItemToObject(root, LV2_UI__protocol, protocol);
		cJSON_AddItemToObject(root, RDF__value, value);
	}

	return root;
}

static inline void
_moony_ui(UI *ui, const char *json)
{
	cJSON *root = cJSON_Parse(json);
	if(root)
	{
		cJSON *protocol = cJSON_GetObjectItem(root, LV2_UI__protocol);
		cJSON *value = cJSON_GetObjectItem(root, RDF__value);

		if(  protocol && value
			&& (protocol->type == cJSON_String) )
		{
			if(!strcmp(protocol->valuestring, LV2_ATOM__eventTransfer)
				&& (value->type == cJSON_Object) )
			{
				lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
				if(_json_to_atom(ui, value, &ui->forge))
				{
					const LV2_Atom_Object *obj = (const LV2_Atom_Object *)ui->buf;

					if(obj->atom.type == ui->forge.Object)
					{
						if(obj->body.otype == ui->uris.patch.get)
						{
							const LV2_Atom_URID *property = NULL;

							lv2_atom_object_get(obj,
								ui->uris.patch.property, &property,
								0);
							
							if(property && (property->atom.type == ui->forge.URID) )
							{
								if(property->body == ui->uris.window_title)
								{
									LV2_Atom_Object *obj_out = _moony_message_forge(ui, ui->uris.window_title,
										ui->title, strlen(ui->title));
									if(obj_out)
									{
										cJSON *job = _moony_send(ui, ui->control_port,
											lv2_atom_total_size(&obj_out->atom), ui->uris.atom_event_transfer, obj_out);
										if(job)
											_schedule(ui, job);
									}

									// resend control port values
									for(unsigned i=0; i<MAX_PORTS; i++) // iterate over all ports
									{
										if(  (ui->pmap[i].index != LV2UI_INVALID_PORT_INDEX )
											&& (ui->pmap[i].val != HUGE_VAL) ) // skip invalid ports
										{
											cJSON *job = _moony_send(ui, ui->pmap[i].index, sizeof(float),
												ui->uris.ui_float_protocol, &ui->pmap[i].val);
											if(job)
												_schedule(ui, job);
										}
									}
								}
								if(property->body == ui->uris.patch.subject) //FIXME rename property?
								{
									const char *self = ui->unmap->unmap(ui->unmap->handle, ui->uris.patch.self);
									LV2_Atom_Object *obj_out = _moony_message_forge(ui, ui->uris.patch.subject,
										self, strlen(self)); //FIXME URI
									if(obj_out)
									{
										cJSON *job = _moony_send(ui, ui->control_port,
											lv2_atom_total_size(&obj_out->atom), ui->uris.atom_event_transfer, obj_out);
										if(job)
											_schedule(ui, job);
									}
								}
							}
						}
					}
				}
				else if(ui->log)
					lv2_log_error(&ui->logger, "_moony_ui: forge buffer overflow");
			}
		}
		else if(ui->log)
			lv2_log_error(&ui->logger, "_moony_ui: missing protocol or value");
		cJSON_Delete(root);
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
	if(!uv_is_closing((uv_handle_t *)&ui->req))
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
				_err(ui, "uv_read_stop", ret);
		}
		if(!uv_is_closing((uv_handle_t *)&client->handle))
			uv_close((uv_handle_t *)&client->handle, _on_client_close);
	}

	// close server
	if(!uv_is_closing((uv_handle_t *)&server->http_server))
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
	char **oldargs = NULL;
	if(!args)
		goto fail;
	args[n] = NULL;

	char *pch = strtok(env," \t");
	while(pch)
	{
		args[n++] = pch;
		oldargs = args;
		args = realloc(args, (n+1) * sizeof(char *));
		if(!args)
			goto fail;
		oldargs = NULL;
		args[n] = NULL;

		pch = strtok(NULL, " \t");
	}

	args[n++] = path;
	oldargs = args;
	args = realloc(args, (n+1) * sizeof(char *));
	if(!args)
		goto fail;
	oldargs = NULL;
	args[n] = NULL;

	return args;

fail:
	if(oldargs)
		free(oldargs);
	if(args)
		free(args);
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
	UI *ui = (void *)server - offsetof(UI, server);
	int ret;

	if(nread >= 0)
	{
		ssize_t parsed = http_parser_execute(&client->parser, &server->http_settings, buf->base, nread);

		if(parsed < nread)
		{
			_err2(ui, "_on_read: http parse error");
			if(uv_is_active((uv_handle_t *)handle))
			{
				if((ret = uv_read_stop((uv_stream_t *)handle)))
					_err(ui, "uv_read_stop", ret);
			}
			if(!uv_is_closing((uv_handle_t *)handle))
				uv_close((uv_handle_t *)handle, _on_client_close);
		}
	}
	else if(nread < 0)
	{
		if(nread != UV_EOF)
			_err(ui, "_on_read", nread);
		if(uv_is_active((uv_handle_t *)handle))
		{
			if((ret = uv_read_stop((uv_stream_t *)handle)))
				_err(ui, "uv_read_stop", ret);
		}
		if(!uv_is_closing((uv_handle_t *)handle))
			uv_close((uv_handle_t *)handle, _on_client_close);
	}
	else // nread == 0
	{
		// this is only reached in non-blocking read from pipes
	}

	if(buf->base)
		free(buf->base);
}

static void
_on_connected(uv_stream_t *handle, int status)
{
	server_t *server = handle->data;
	UI *ui = (void *)server - offsetof(UI, server);
	int ret;

	client_t *client = calloc(1, sizeof(client_t)); //TODO check
	client->server = server;
	server->clients = _client_append(server->clients, client);

	if((ret = uv_tcp_init(handle->loop, &client->handle)))
	{
		free(client);
		_err(ui, "uv_tcp_init", ret);
		return;
	}

	if((ret = uv_accept(handle, (uv_stream_t *)&client->handle)))
	{
		free(client);
		_err(ui, "uv_accept", ret);
		return;
	}

	client->handle.data = client;
	client->parser.data = client;

	http_parser_init(&client->parser, HTTP_REQUEST);

	if((ret = uv_read_start((uv_stream_t *)&client->handle, _on_alloc, _on_read)))
	{
		free(client);
		_err(ui, "uv_read_start", ret);
		return;
	}

	uv_timer_stop(&server->timer);
}

static inline void
_show(UI *ui)
{
	server_t *server = &ui->server;
	int ret;

	struct sockaddr_in addr_ip4;
	struct sockaddr *addr = (struct sockaddr *)&addr_ip4;

	if((ret = uv_ip4_addr("0.0.0.0", 0, &addr_ip4))) // let OS choose unused port
	{
		_err(ui, "uv_ip4_addr", ret);
	}

	if(!uv_is_active((uv_handle_t *)&server->http_server))
	{
		if((ret = uv_tcp_bind(&server->http_server, addr, 0)))
			_err(ui, "uv_tcp_bind", ret);
		if((ret = uv_listen((uv_stream_t *)&server->http_server, 128, _on_connected)))
			_err(ui, "uv_listen", ret);

		// get chosen port
		struct sockaddr_storage storage;
		struct sockaddr_in *storage_ip4 = (struct sockaddr_in *)&storage;
		int namelen = sizeof(struct sockaddr_storage);
		if((ret = uv_tcp_getsockname(&server->http_server, (struct sockaddr *)&storage, &namelen)))
			_err(ui, "uv_tcp_get_sockname", ret);

		const uint16_t port = ntohs(storage_ip4->sin_port);
		sprintf(ui->path, "http://localhost:%"PRIu16, port);
	}

	/* FIXME
	if(!uv_is_active((uv_handle_t *)&server->timer))
	{
		if((ret = uv_timer_start(&server->timer, _timeout, 10000, 0)))
			_err(ui, "uv_timer_start", ret);
	}
	*/

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
		moony_editor = getenv("BROWSER");
	if(!moony_editor)
		moony_editor = command;
	char *dup = strdup(moony_editor);
	char **args = dup ? _parse_env(dup, ui->path) : NULL;
	
	ui->opts.exit_cb = _on_exit;
	ui->opts.file = args ? args[0] : NULL;
	ui->opts.args = args;

	if(!uv_is_active((uv_handle_t *)&ui->req))
	{
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

static char *
_read_file(UI *ui, const char *bundle_path, const char *file_path, size_t *size)
{
#if defined(_WIN32)
	const char *sep = bundle_path[strlen(bundle_path) - 1] == '\\' ? "" : "\\";
	const char *sep2 = "\\";
#else
	const char *sep = bundle_path[strlen(bundle_path) - 1] == '/' ? "" : "/";
	const char *sep2 = "/";
#endif

	char *full_path;
	if(asprintf(&full_path, "%s%sweb_ui%s%s", bundle_path, sep, sep2, &file_path[1]) == -1)
		return NULL;

	/* FIXME
	if(ui->log)
		lv2_log_note(&ui->logger, "full_path: %s", full_path);
	*/

	FILE *f = fopen(full_path, "rb");
	free(full_path);
	if(f)
	{
		fseek(f, 0, SEEK_END);
		size_t fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *str = malloc(128 + fsize);
		if(str)
		{
			sprintf(str, content_length, fsize, "");
			size_t len = strlen(str);
			char *ptr = str + len;
			if(fread(ptr, fsize, 1, f) == 1)
			{
				*size = len + fsize;
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

	const char *stat = http_status[STATUS_NOT_FOUND];
	const char *cont = NULL;
	char *chunk = NULL;
	size_t size = 0;

	if(strstr(client->url, "/keepalive") == client->url)
	{
		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JSON];
		client->keepalive = 1;
		// keepalive
	}
	else if(strstr(client->url, "/lv2/ui") == client->url)
	{
		_moony_ui(ui, client->body);

		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JSON];
		chunk = strdup("Content-Length: 2\r\n\r\n{}");
		size = chunk ? strlen(chunk) : 0;
	}
	else if(strstr(client->url, "/lv2/dsp") == client->url)
	{
		_moony_dsp(ui, client->body);

		stat = http_status[STATUS_OK];
		cont = http_content[CONTENT_TEXT_JSON];
		chunk = strdup("Content-Length: 2\r\n\r\n{}");
		size = chunk ? strlen(chunk) : 0;
	}
	else
	{
		for(const url_t *valid = valid_urls; valid->url; valid++)
		{
			if(  (strstr(client->url, valid->url) == client->url)
				|| (valid->alias && (strstr(client->url, valid->alias) == client->url)) )
			{
				stat = http_status[STATUS_OK];
				cont = http_content[valid->content_type];
				chunk = _read_file(ui, ui->bundle_path, valid->url, &size);
				break;
			}
		}
	}

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
	else
		_keepalive(server); // answer keep alives

	return 0;
}

static int
_on_url(http_parser *parser, const char *at, size_t len)
{
	client_t *client = parser->data;
	//server_t *server = client->server;
	//UI *ui = (void *)server - offsetof(UI, server);

	snprintf(client->url, len+1, "%s", at); //FIXME

	return 0;
}

static int
_on_body(http_parser *parser, const char *at, size_t len)
{
	client_t *client = parser->data;

	client->body = at;

	return 0;
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

	LV2_Options_Option *opts = NULL;

	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			ui->unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			ui->port_map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			ui->log = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) && (descriptor == &web_kx))
			ui->kx.host = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_OPTIONS__options))
			opts = features[i]->data;
	}

	snprintf(ui->bundle_path, 512, "%s", bundle_path);

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
	if(!ui->unmap)
	{
		fprintf(stderr, "%s: Host does not support urid:unmap\n", descriptor->URI);
		free(ui);
		return NULL;
	}
	if(!ui->port_map)
	{
		fprintf(stderr, "%s: Host does not support ui:portMap\n", descriptor->URI);
		free(ui);
		return NULL;
	}
	if(!ui->kx.host && (descriptor == &web_kx) )
	{
		fprintf(stderr, "%s: Host does not support kx:Host\n", descriptor->URI);
		free(ui);
		return NULL;
	}

	// query port index of "control" port
	ui->pmap[0].symbol = "input_1";
		ui->pmap[0].val = 0.f;
	ui->pmap[1].symbol = "input_2";
		ui->pmap[1].val = 0.f;
	ui->pmap[2].symbol = "input_3";
		ui->pmap[2].val = 0.f;
	ui->pmap[3].symbol = "input_4";
		ui->pmap[3].val = 0.f;
	ui->pmap[4].symbol = "output_1";
		ui->pmap[4].val = 0.f;
	ui->pmap[5].symbol = "output_2";
		ui->pmap[5].val = 0.f;
	ui->pmap[6].symbol = "output_3";
		ui->pmap[6].val = 0.f;
	ui->pmap[7].symbol = "output_4";
		ui->pmap[7].val = 0.f;
	ui->pmap[8].symbol = "event_in_1";
		ui->pmap[8].val = HUGE_VAL;
	ui->pmap[9].symbol = "event_in_2";
		ui->pmap[9].val = HUGE_VAL;
	ui->pmap[10].symbol = "event_in_3";
		ui->pmap[10].val = HUGE_VAL;
	ui->pmap[11].symbol = "event_in_4";
		ui->pmap[11].val = HUGE_VAL;
	ui->pmap[12].symbol = "event_out_1";
		ui->pmap[12].val = HUGE_VAL;
	ui->pmap[13].symbol = "event_out_2";
		ui->pmap[13].val = HUGE_VAL;
	ui->pmap[14].symbol = "event_out_3";
		ui->pmap[14].val = HUGE_VAL;
	ui->pmap[15].symbol = "event_out_4";
		ui->pmap[15].val = HUGE_VAL;
	ui->pmap[16].symbol = "control";
		ui->pmap[16].val = HUGE_VAL;
	ui->pmap[17].symbol = "notify";
		ui->pmap[17].val = HUGE_VAL;

	for(unsigned i=0; i<MAX_PORTS; i++)
		ui->pmap[i].index = ui->port_map->port_index(ui->port_map->handle, ui->pmap[i].symbol);
	ui->control_port = ui->pmap[16].index;

	qsort(ui->pmap, MAX_PORTS, sizeof(pmap_t), _pmap_cmp);

	ui->uris.moony_code = ui->map->map(ui->map->handle, MOONY_CODE_URI);
	ui->uris.moony_selection = ui->map->map(ui->map->handle, MOONY_SELECTION_URI);
	ui->uris.moony_error = ui->map->map(ui->map->handle, MOONY_ERROR_URI);
	ui->uris.moony_trace = ui->map->map(ui->map->handle, MOONY_TRACE_URI);
	ui->uris.window_title = ui->map->map(ui->map->handle, LV2_UI__windowTitle);

	ui->uris.patch.self = ui->map->map(ui->map->handle, plugin_uri);
	ui->uris.patch.get = ui->map->map(ui->map->handle, LV2_PATCH__Get);
	ui->uris.patch.set = ui->map->map(ui->map->handle, LV2_PATCH__Set);
	ui->uris.patch.put = ui->map->map(ui->map->handle, LV2_PATCH__Put);
	ui->uris.patch.patch = ui->map->map(ui->map->handle, LV2_PATCH__Patch);
	ui->uris.patch.body = ui->map->map(ui->map->handle, LV2_PATCH__body);
	ui->uris.patch.subject = ui->map->map(ui->map->handle, LV2_PATCH__subject);
	ui->uris.patch.property = ui->map->map(ui->map->handle, LV2_PATCH__property);
	ui->uris.patch.value = ui->map->map(ui->map->handle, LV2_PATCH__value);
	ui->uris.patch.add = ui->map->map(ui->map->handle, LV2_PATCH__add);
	ui->uris.patch.remove = ui->map->map(ui->map->handle, LV2_PATCH__remove);
	ui->uris.patch.wildcard = ui->map->map(ui->map->handle, LV2_PATCH__wildcard);
	ui->uris.patch.writable = ui->map->map(ui->map->handle, LV2_PATCH__writable);
	ui->uris.patch.readable = ui->map->map(ui->map->handle, LV2_PATCH__readable);

	ui->uris.ui_float_protocol = ui->map->map(ui->map->handle, LV2_UI__floatProtocol);
	ui->uris.ui_peak_protocol = ui->map->map(ui->map->handle, LV2_UI__peakProtocol);
	ui->uris.atom_atom_transfer = ui->map->map(ui->map->handle, LV2_ATOM__atomTransfer);
	ui->uris.atom_event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	lv2_atom_forge_init(&ui->forge, ui->map);
	if(ui->log)
		lv2_log_logger_init(&ui->logger, ui->map, ui->log);

	// set window title
	snprintf(ui->title, 512, "%s", descriptor->URI);
	if(opts)
	{
		for(LV2_Options_Option *opt = opts;
			(opt->key != 0) && (opt->value != NULL);
			opt++)
		{
			if( (opt->key == ui->uris.window_title) && (opt->type == ui->forge.String) )
				snprintf(ui->title, 512, "%s", opt->value);
		}
	}
	if( (descriptor == &web_kx) && (ui->kx.host->plugin_human_id))
		snprintf(ui->title, 512, "%s", ui->kx.host->plugin_human_id);

	ui->write_function = write_function;
	ui->controller = controller;

	int ret;
	if((ret = uv_loop_init(&ui->loop)))
	{
		fprintf(stderr, "%s: %s\n", descriptor->URI, uv_strerror(ret));
		free(ui);
		return NULL;
	}

	server_t *server = &ui->server;
	server->http_settings.on_message_begin = NULL;
	server->http_settings.on_message_complete= _on_message_complete;
	server->http_settings.on_headers_complete= NULL;

	server->http_settings.on_url = _on_url;
	server->http_settings.on_header_field = NULL;
	server->http_settings.on_header_value = NULL;
	server->http_settings.on_body = _on_body;
	server->http_server.data = server;

	if((ret = uv_tcp_init(&ui->loop, &server->http_server)))
	{
		fprintf(stderr, "%s: %s\n", descriptor->URI, uv_strerror(ret));
		free(ui);
		return NULL;
	}

	if((ret = uv_timer_init(&ui->loop, &server->timer)))
	{
		fprintf(stderr, "%s: %s\n", descriptor->URI, uv_strerror(ret));
		free(ui);
		return NULL;
	}
	server->timer.data = ui;

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

	if(format == 0)
		format = ui->uris.ui_float_protocol;

	// intercept control port notifications
	if(format == ui->uris.ui_float_protocol)
	{
		pmap_t *pmap = _pmap_get(ui, port_index);
		if(pmap)
			pmap->val = *(const float *)buffer;
	}

	cJSON *job = _moony_send(ui, port_index, buffer_size, format, buffer);
	if(job)
		_schedule(ui, job);
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
