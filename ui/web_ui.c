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

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <libwebsockets.h>

#include <moony.h>
#include <private_ui.h>
#include <jsatom.h>

#include <lv2_external_ui.h> // kxstudio external-ui extension

#include <cJSON.h>

#define BUF_SIZE 0x100000 // 1M

#define RDFS_PREFIX "http://www.w3.org/2000/01/rdf-schema#"

#define RDFS__label RDFS_PREFIX"label"
#define RDFS__range RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

#define MAX_PORTS 18

#ifndef LV2_UI__protocol
#	define LV2_UI__protocol         LV2_UI_PREFIX "protocol"
#endif

#ifndef LV2_UI__floatProtocol
#	define LV2_UI__floatProtocol    LV2_UI_PREFIX "floatProtocol"
#endif

#ifndef LV2_UI__peakProtocol
#	define LV2_UI__peakProtocol     LV2_UI_PREFIX "peakProtocol"
#endif

#ifndef LV2_UI__portEvent
#	define LV2_UI__portEvent     LV2_UI_PREFIX "portEvent"
#endif

#ifndef LV2_UI__periodStart
#	define LV2_UI__periodStart     LV2_UI_PREFIX "periodStart"
#endif

#ifndef LV2_UI__periodSize
#	define LV2_UI__periodSize     LV2_UI_PREFIX "periodSize"
#endif

#ifndef LV2_UI__peak
#	define LV2_UI__peak     LV2_UI_PREFIX "peak"
#endif

typedef struct _ui_t ui_t;
typedef struct _client_t client_t;
typedef struct _pmap_t pmap_t;

struct _pmap_t {
	uint32_t index;
	const char *symbol;
	float val;
};

struct _client_t {
	cJSON *root;
};

struct _ui_t {
	struct {
		LV2_URID moony_message;
		LV2_URID moony_code;
		LV2_URID moony_selection;
		LV2_URID moony_error;
		LV2_URID moony_trace;
		LV2_URID moony_ui;
		LV2_URID moony_dsp;
		LV2_URID moony_destination;
		LV2_URID window_title;

		LV2_URID subject;

		patch_t patch;

		LV2_URID core_symbol;
		LV2_URID ui_protocol;
		LV2_URID ui_port_notification;
		LV2_URID ui_port_event;
		LV2_URID ui_float_protocol;
		LV2_URID ui_peak_protocol;
		LV2_URID ui_period_start;
		LV2_URID ui_period_size;
		LV2_URID ui_peak;
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

	int done;
	char *url;

	spawn_t spawn;

	struct {
		LV2_External_UI_Widget widget;
		const LV2_External_UI_Host *host;
	} kx;

	atom_ser_t ser [3];

	char title [512];
	char *bundle_path;

	struct lws_context *context;
	client_t *client;
	atomic_flag lock;

	jsatom_t jsatom;
};

static LV2_Atom_Forge_Ref
_sink_ui(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
{
	atom_ser_t *ser = handle;

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;

	const uint32_t new_offset = ser->offset + size;
	if(new_offset > ser->size)
	{
		uint32_t new_size = ser->size << 1;
		while(new_offset > new_size)
			new_size <<= 1;

		if(!(ser->buf = realloc(ser->buf, new_size)))
			return 0; // realloc failed

		ser->size = new_size;
	}

	memcpy(ser->buf + ser->offset, buf, size);
	ser->offset = new_offset;

	return ref;
}

static LV2_Atom *
_deref_ui(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

static int
callback_lv2(struct lws *wsi, enum lws_callback_reasons reason,
	void *user, void *in, size_t len);

static int
callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	void *in, size_t len);

enum demo_protocols {
	PROTOCOL_HTTP = 0,
	PROTOCOL_LV2,
	DEMO_PROTOCOL_COUNT
};

static const struct lws_protocols protocols [] = {
	[PROTOCOL_HTTP] = {
		.name = "http-only",
		.callback = callback_http,
		.per_session_data_size = 0,
		.rx_buffer_size = 0x10000 // 64K
	},
	[PROTOCOL_LV2] = {
		.name = "lv2-protocol",
		.callback = callback_lv2,
		.per_session_data_size = sizeof(client_t),
		.rx_buffer_size = 0x100000 // 1M
	},
	{ .name = NULL, .callback = NULL, .per_session_data_size = 0, .rx_buffer_size = 0 }
};

static inline const char *
get_mimetype(const char *file)
{
	int n = strlen(file);

	if(n < 5)
		return NULL;

	if(!strcmp(&file[n - 4], ".ico"))
		return "image/x-icon";

	if(!strcmp(&file[n - 4], ".png"))
		return "image/png";

	if(!strcmp(&file[n - 5], ".html"))
		return "text/html";

	if(!strcmp(&file[n - 3], ".js"))
		return "text/javascript";

	if(!strcmp(&file[n - 5], ".json"))
		return "text/json";

	if(!strcmp(&file[n - 4], ".css"))
		return "text/css";

	if(!strcmp(&file[n - 4], ".ttf"))
		return "application/octet-stream";

	return NULL;
}

static int
callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
	void *in, size_t len)
{
	ui_t *ui = lws_context_user(lws_get_context(wsi));
	const char *in_str = in;

	switch(reason)
	{
		case LWS_CALLBACK_HTTP:
		{
			if(len < 1)
			{
				lws_return_http_status(wsi, HTTP_STATUS_BAD_REQUEST, NULL);

				goto try_to_reuse;
			}

			// skip root slash if present
			if(in_str[0] == '/')
			{
				in_str++;
				len--;
			}

			/* this server has no concept of directories */
			if(strchr(in_str, '/'))
			{
				lws_return_http_status(wsi, HTTP_STATUS_NOT_ACCEPTABLE, NULL);

				goto try_to_reuse;
			}

			/* if a legal POST URL, let it continue and accept data */
			if(lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
				return 0;

			const char *file_path = len > 0
				? in_str
				: "index.html";

			/* refuse to serve files we don't understand */
			const char *mimetype = get_mimetype(file_path);
			if(!mimetype)
			{
				lwsl_err("Unknown mimetype for %s\n", file_path);
				lws_return_http_status(wsi, HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL);

				return -1;
			}

#if 0
			const char *sep = ui->bundle_path[strlen(ui->bundle_path) - 1] == '\\' ? "" : "\\";
			const char *sep2 = "\\";
#else
			const char *sep = ui->bundle_path[strlen(ui->bundle_path) - 1] == '/' ? "" : "/";
			const char *sep2 = "/";
#endif

			char *full_path;
			if(asprintf(&full_path, "%s%sweb_ui%s%s", ui->bundle_path, sep, sep2, file_path) == -1)
			{
				lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL);

				return -1;
			}

			const int n = lws_serve_http_file(wsi, full_path, mimetype, NULL, 0);
			free(full_path);

			if( (n < 0) || ( (n > 0) && lws_http_transaction_completed(wsi)))
			{
				return -1; /* error or can't reuse connection: close the socket */
			}

			// we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback asynchronously
			break;
		}

		case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		{
			lws_return_http_status(wsi, HTTP_STATUS_OK, NULL);

			goto try_to_reuse;
		}

		case LWS_CALLBACK_HTTP_FILE_COMPLETION:
		{
			goto try_to_reuse;
		}

		case LWS_CALLBACK_LOCK_POLL:
		{
			while(atomic_flag_test_and_set_explicit(&ui->lock, memory_order_acquire))
			{
				// spin
			}
			break;
		}
		case LWS_CALLBACK_UNLOCK_POLL:
		{
			atomic_flag_clear_explicit(&ui->lock, memory_order_release);
			break;
		}

		default:
		{
			break;
		}
	}

	return 0;

	/* if we're on HTTP1.1 or 2.0, will keep the idle connection alive */
try_to_reuse:
	if(lws_http_transaction_completed(wsi))
		return -1;

	return 0;
}

static inline void
_moony_cb(ui_t *ui, const char *json);

static int
callback_lv2(struct lws *wsi, enum lws_callback_reasons reason,
	void *user, void *in, size_t len)
{
	ui_t *ui = lws_context_user(lws_get_context(wsi));
	client_t *client = user;
	const char *in_str = in;

	switch(reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
		{
			//lwsl_notice("LWS_CALLBACK_ESTABLISHED:\n");

			if(ui->client)
				return -1; // refuse to connect to a second websocket

			memset(client, 0x0, sizeof(client_t));

			client->root = cJSON_CreateArray();
			if(!client->root)
				return -1;

			ui->client = client;

			break;
		}

		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			const int num = cJSON_GetArraySize(client->root);
			if(num > 0)
			{
				cJSON *job = cJSON_DetachItemFromArray(client->root, 0); // get next job
				if(job)
				{
					char *json = cJSON_PrintUnformatted(job);
					if(json)
					{
						const int n = strlen(json);

						json = realloc(json, n + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING);
						memmove(&json[LWS_SEND_BUFFER_PRE_PADDING], json, n);

						const int m = lws_write(wsi, (uint8_t *)&json[LWS_SEND_BUFFER_PRE_PADDING], n, LWS_WRITE_TEXT);
						free(json);

						if(m < n)
						{
							lwsl_err("ERROR %d writing to di socket\n", n);
							return -1;
						}
					}
					cJSON_Delete(job);

					if(num > 1) // there are remaining jobs, schedule an other write callback
						lws_callback_on_writable_all_protocol(ui->context, &protocols[PROTOCOL_LV2]);
				}
			}

			break;
		}

		case LWS_CALLBACK_RECEIVE:
		{
			if(len < 6)
				break;

			_moony_cb(ui, in_str);

			break;
		}

		case LWS_CALLBACK_CLOSED:
		{
			//lwsl_notice("LWS_CALLBACK_CLOSE:\n");

			if(client->root)
			{
				cJSON_Delete(client->root);
				client->root = NULL;
			}

			ui->client = NULL;
			ui->done = 1;

			break;
		}

		case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
		{
			//lwsl_notice("LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: len %d\n", len);

			if(client->root)
			{
				cJSON_Delete(client->root);
				client->root = NULL;
			}

			ui->client = NULL;
			ui->done = 1;

			break;
		}

		case LWS_CALLBACK_LOCK_POLL:
		{
			while(atomic_flag_test_and_set_explicit(&ui->lock, memory_order_acquire))
			{
				// spin
			}
			break;
		}
		case LWS_CALLBACK_UNLOCK_POLL:
		{
			atomic_flag_clear_explicit(&ui->lock, memory_order_release);
			break;
		}

		default:
		{
			break;
		}
	}

	return 0;
}

static inline const LV2_Atom_Object *
_moony_message_forge(ui_t *ui, LV2_URID key,
	const char *str, uint32_t size)
{
	atom_ser_t *ser = &ui->ser[0];

	ser->offset = 0;
	lv2_atom_forge_set_sink(&ui->forge, _sink_ui, _deref_ui, ser);
	memset(ser->buf, 0x0, sizeof(LV2_Atom));

	if(_moony_patch(&ui->uris.patch, &ui->forge, key, str, size))
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)ser->buf;
		return obj;
	}

	if(ui->log)
		lv2_log_error(&ui->logger, "code chunk too long");
	return NULL;
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
_pmap_get(ui_t *ui, uint32_t idx)
{
	const pmap_t tar = { .index = idx };
	pmap_t *pmap = bsearch(&tar, ui->pmap, MAX_PORTS, sizeof(pmap_t), _pmap_cmp);
	return pmap;
}

static inline void
_moony_dsp(ui_t *ui, const LV2_Atom_Object *obj)
{
	const LV2_Atom_URID *protocol = NULL;
	const LV2_Atom_String *symbol = NULL;
	const LV2_Atom *event = NULL;

	LV2_Atom_Object_Query q [] = {
		{ ui->uris.ui_protocol, (const LV2_Atom **)&protocol },
		{ ui->uris.core_symbol, (const LV2_Atom **)&symbol },
		{ ui->uris.ui_port_event, &event },
		{ 0, NULL}
	};

	lv2_atom_object_query(obj, q);

	if(  !protocol || (protocol->atom.type != ui->forge.URID)
		|| !symbol || (symbol->atom.type != ui->forge.String)
		|| !event)
	{
		return;
	}
	
	const uint32_t idx = ui->port_map->port_index(ui->port_map->handle, LV2_ATOM_BODY_CONST(symbol));

	if(  (protocol->body == ui->uris.ui_float_protocol)
		&& (event->type == ui->forge.Float) )
	{
		const float val = ((const LV2_Atom_Float *)event)->body;
		ui->write_function(ui->controller, idx, sizeof(float),
			0, &val);

		// intercept control port changes
		pmap_t *pmap = _pmap_get(ui, idx);
		if(pmap)
			pmap->val = val;
	}
	else if(protocol->body == ui->uris.atom_event_transfer)
	{
		ui->write_function(ui->controller, idx, lv2_atom_total_size(event),
			ui->uris.atom_event_transfer, event);
	}
	else if(protocol->body == ui->uris.atom_atom_transfer)
	{
		ui->write_function(ui->controller, idx, lv2_atom_total_size(event),
			ui->uris.atom_atom_transfer, event);
	}
	else if(ui->log)
		lv2_log_error(&ui->logger, "_moony_dsp: missing protocol, symbol or value");
}

static inline void
_schedule(ui_t *ui, cJSON *job)
{
	client_t *client;

	if( (client = ui->client) )
	{
		// add job to jobs array
		cJSON_AddItemToArray(client->root, job);
	}

	lws_callback_on_writable_all_protocol(ui->context, &protocols[PROTOCOL_LV2]);
}

static inline cJSON * 
_moony_send(ui_t *ui, uint32_t idx, uint32_t size, uint32_t prot, const void *buf)
{
	atom_ser_t *ser = &ui->ser[1];

	ser->offset = 0;
	lv2_atom_forge_set_sink(&ui->forge, _sink_ui, _deref_ui, ser);
	memset(ser->buf, 0x0, sizeof(LV2_Atom));

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(&ui->forge, &frame, 0, ui->uris.ui_port_notification);

	if(prot == 0)
		prot = ui->uris.ui_float_protocol;

	lv2_atom_forge_key(&ui->forge, ui->uris.ui_protocol);
	lv2_atom_forge_urid(&ui->forge, prot);

	pmap_t *pmap = _pmap_get(ui, idx);
	if(pmap)
	{
		lv2_atom_forge_key(&ui->forge, ui->uris.core_symbol);
		lv2_atom_forge_string(&ui->forge, pmap->symbol, strlen(pmap->symbol));
	}

	lv2_atom_forge_key(&ui->forge, ui->uris.ui_port_event);
	if( (prot == ui->uris.ui_float_protocol) && (size == sizeof(float)) )
	{
		const float *val = buf;
		lv2_atom_forge_float(&ui->forge, *val);
	}
	else if( (prot == ui->uris.ui_peak_protocol) && (size == sizeof(LV2UI_Peak_Data)) )
	{
		const LV2UI_Peak_Data *peak_data = buf;
		LV2_Atom_Forge_Frame peak_frame;
		lv2_atom_forge_object(&ui->forge, &peak_frame, 0, 0);
		lv2_atom_forge_key(&ui->forge, ui->uris.ui_period_start);
			lv2_atom_forge_int(&ui->forge, peak_data->period_start);
		lv2_atom_forge_key(&ui->forge, ui->uris.ui_period_size);
			lv2_atom_forge_int(&ui->forge, peak_data->period_size);
		lv2_atom_forge_key(&ui->forge, ui->uris.ui_peak);
			lv2_atom_forge_float(&ui->forge, peak_data->peak);
		lv2_atom_forge_pop(&ui->forge, &peak_frame);
	}
	else if(prot == ui->uris.atom_event_transfer)
	{
		const LV2_Atom *_atom = buf;
		lv2_atom_forge_write(&ui->forge, _atom, lv2_atom_total_size(_atom));
	}
	else if(prot == ui->uris.atom_atom_transfer)
	{
		const LV2_Atom *_atom = buf;
		lv2_atom_forge_write(&ui->forge, _atom, lv2_atom_total_size(_atom));
	}
	else
	{
		return NULL;
	}

	lv2_atom_forge_pop(&ui->forge, &frame);

	const LV2_Atom *atom = (const LV2_Atom *)ser->buf;
	return jsatom_encode(&ui->jsatom, atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));
}

static inline void
_moony_ui(ui_t *ui, const LV2_Atom_Object *obj)
{
	const LV2_Atom_URID *protocol = NULL;
	const LV2_Atom *event = NULL;

	LV2_Atom_Object_Query q [] = {
		{ ui->uris.ui_protocol, (const LV2_Atom **)&protocol },
		{ ui->uris.ui_port_event, &event },
		{ 0, NULL}
	};

	lv2_atom_object_query(obj, q);

	if(  !protocol || (protocol->atom.type != ui->forge.URID)
		|| !event)
	{
		return;
	}

	if(  (protocol->body == ui->uris.atom_event_transfer)
		&& lv2_atom_forge_is_object_type(&ui->forge, event->type) )
	{
		const LV2_Atom_Object *eobj = (const LV2_Atom_Object *)event;

		if(lv2_atom_forge_is_object_type(&ui->forge, eobj->atom.type))
		{
			if(eobj->body.otype == ui->uris.patch.get)
			{
				const LV2_Atom_URID *property = NULL;

				lv2_atom_object_get(eobj,
					ui->uris.patch.property, &property,
					0);

				if(property && (property->atom.type == ui->forge.URID) )
				{
					if(property->body == ui->uris.window_title)
					{
						const LV2_Atom_Object *obj_out = _moony_message_forge(ui, ui->uris.window_title,
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
						const LV2_Atom_Object *obj_out = _moony_message_forge(ui, ui->uris.patch.subject,
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
		lv2_log_error(&ui->logger, "_moony_ui: missing protocol, symbol or value");
}

static inline void
_moony_cb(ui_t *ui, const char *json)
{
	cJSON *root = cJSON_Parse(json);
	if(!root)
		return;

	atom_ser_t *ser = &ui->ser[2];

	ser->offset = 0;
	lv2_atom_forge_set_sink(&ui->forge, _sink_ui, _deref_ui, ser);
	memset(ser->buf, 0x0, sizeof(LV2_Atom));

	const LV2_Atom_Forge_Ref ref = jsatom_decode(&ui->jsatom, &ui->forge, root);
	cJSON_Delete(root);
	if(!ref)
		return;

	const LV2_Atom_Object *obj = (const LV2_Atom_Object *)ser->buf;
	if(  !lv2_atom_forge_is_object_type(&ui->forge, obj->atom.type)
		|| (obj->body.otype != ui->uris.ui_port_notification) )
	{
		return;
	}

	const LV2_Atom_URID *destination = NULL;

	LV2_Atom_Object_Query q [] = {
		{ ui->uris.moony_destination, (const LV2_Atom **)&destination},
		{ 0, NULL }
	};

	lv2_atom_object_query(obj, q);

	if(!destination || (destination->atom.type != ui->forge.URID) )
		return;

	if(destination->body == ui->uris.moony_ui)
		_moony_ui(ui, obj);
	else if(destination->body == ui->uris.moony_dsp)
		_moony_dsp(ui, obj);
}

// Show Interface
static inline int
_show_cb(LV2UI_Handle instance)
{
	ui_t *ui = instance;

	if(!ui->done)
		return 0; // already showing

#if defined(_WIN32)
	const char *command = "cmd /c start";
#elif defined(__APPLE__)
	const char *command = "open";
#else // Linux/BSD
	const char *command = "xdg-open";
#endif

	// get default browser from environment
	const char *moony_browser = getenv("MOONY_BROWSER");
	if(!moony_browser)
		moony_browser = getenv("BROWSER");
	if(!moony_browser)
		moony_browser = command;
	char *dup = strdup(moony_browser);
	char **args = dup ? _spawn_parse_env(dup, ui->url) : NULL;

	const int status = _spawn_spawn(&ui->spawn, args);

	if(args)
		free(args);
	if(dup)
		free(dup);

	if(status)
		return -1; // failed to spawn

	ui->done = 0;

	return 0;
}

static inline int
_hide_cb(LV2UI_Handle instance)
{
	ui_t *ui = instance;

	if(_spawn_has_child(&ui->spawn))
	{
		_spawn_kill(&ui->spawn);

		_spawn_waitpid(&ui->spawn, true);

		_spawn_invalidate_child(&ui->spawn);
	}

	ui->done = 1;

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
	ui_t *ui = instance;

	if(_spawn_has_child(&ui->spawn))
	{
		int res;
		if((res = _spawn_waitpid(&ui->spawn, false)) < 0)
		{
			_spawn_invalidate_child(&ui->spawn);
			//ui->done = 1; // xdg-open, START return immediately, we wait for websocket to terminate
		}
	}

	if(!ui->done)
	{
		if(lws_service(ui->context, 0))
			ui->done = 1; // lws errored
	}

	return ui->done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle_cb
};

// External-UI Interface
static inline void
_kx_run(LV2_External_UI_Widget *widget)
{
	ui_t *handle = (void *)widget - offsetof(ui_t, kx.widget);

	if(_idle_cb(handle))
	{
		if(handle->kx.host && handle->kx.host->ui_closed)
			handle->kx.host->ui_closed(handle->controller);
		_hide_cb(handle);
	}
}

static inline void
_kx_hide(LV2_External_UI_Widget *widget)
{
	ui_t *handle = (void *)widget - offsetof(ui_t, kx.widget);

	_hide_cb(handle);
}

static inline void
_kx_show(LV2_External_UI_Widget *widget)
{
	ui_t *handle = (void *)widget - offsetof(ui_t, kx.widget);

	_show_cb(handle);
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

	ui_t *ui = calloc(1, sizeof(ui_t));
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

	ui->bundle_path = strdup(bundle_path); //TODO check

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
	ui->uris.moony_ui = ui->map->map(ui->map->handle, MOONY_UI_URI);
	ui->uris.moony_dsp = ui->map->map(ui->map->handle, MOONY_DSP_URI);
	ui->uris.moony_destination = ui->map->map(ui->map->handle, MOONY_DESTINATION_URI);
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
	ui->uris.patch.destination = ui->map->map(ui->map->handle, LV2_PATCH__destination);

	ui->uris.core_symbol = ui->map->map(ui->map->handle, LV2_CORE__symbol);
	ui->uris.ui_protocol = ui->map->map(ui->map->handle, LV2_UI__protocol);
	ui->uris.ui_port_notification= ui->map->map(ui->map->handle, LV2_UI__portNotification);
	ui->uris.ui_port_event = ui->map->map(ui->map->handle, LV2_UI__portEvent);
	ui->uris.ui_float_protocol = ui->map->map(ui->map->handle, LV2_UI__floatProtocol);
	ui->uris.ui_peak_protocol = ui->map->map(ui->map->handle, LV2_UI__peakProtocol);
	ui->uris.ui_period_start = ui->map->map(ui->map->handle, LV2_UI__periodStart);
	ui->uris.ui_period_size = ui->map->map(ui->map->handle, LV2_UI__periodSize);
	ui->uris.ui_peak = ui->map->map(ui->map->handle, LV2_UI__peak);
	ui->uris.atom_atom_transfer = ui->map->map(ui->map->handle, LV2_ATOM__atomTransfer);
	ui->uris.atom_event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	lv2_atom_forge_init(&ui->forge, ui->map);
	if(ui->log)
		lv2_log_logger_init(&ui->logger, ui->map, ui->log);

	ui->spawn.logger = ui->log ? &ui->logger : NULL;

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

	// LWS
	atomic_flag_clear_explicit(&ui->lock, memory_order_relaxed);
#if defined(USE_VERBOSE_LOG)
	lws_set_log_level( (1 << LLL_COUNT) - 1, NULL);
#else
	lws_set_log_level(0, NULL);
#endif

	struct lws_context_creation_info info;
	memset(&info, 0x0, sizeof(info));

	info.user = ui;
	info.port = 0;
	info.iface = NULL;
	info.protocols = protocols;

	info.gid = -1;
	info.uid = -1;
	info.max_http_header_pool = 32;
	info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;
	info.ka_time = 10; // TCP keepalive period
	info.ka_probes = 3; // TCP keepalive number of retries
	info.ka_interval = 1; // TCP keepalive interval between retries
	info.timeout_secs = 60; // roundtrip timeout

	ui->context = lws_create_context(&info);
	if(!ui->context)
	{
		free(ui);
		return NULL;
	}

	if(asprintf(&ui->url, "http://localhost:%d", info.port) == -1)
	{
		free(ui);
		return NULL;
	}

	if(ui->log)
		lv2_log_note(&ui->logger, "moony web_ui url: %s", ui->url);

	jsatom_init(&ui->jsatom, ui->map, ui->unmap);

	_spawn_invalidate_child(&ui->spawn);
	ui->done = 1;

	for(unsigned i=0; i<3; i++)
	{
		ui->ser[i].moony = NULL;
		ui->ser[i].size = 1024;
		ui->ser[i].buf = malloc(ui->ser[i].size);
	}

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	ui_t *ui = handle;

	for(unsigned i=0; i<3; i++)
	{
		if(ui->ser[i].buf)
			free(ui->ser[i].buf);
	}

	if(ui->url)
		free(ui->url);

	if(ui->bundle_path)
		free(ui->bundle_path);

	if(ui->context)
		lws_context_destroy(ui->context);

	if(ui->client)
	{
		if(ui->client->root)
			cJSON_Delete(ui->client->root);

		free(ui->client);
	}

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	ui_t *ui = handle;

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
