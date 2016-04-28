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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#if !defined(_WIN32)
#	include <sys/wait.h>
#endif
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <moony.h>

#include <lv2_external_ui.h> // kxstudio external-ui extension

#include <cJSON.h>

#include <libwebsockets.h>

#define BUF_SIZE 0x100000 // 1M

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

typedef struct _ui_t ui_t;
typedef struct _client_t client_t;
typedef struct _pmap_t pmap_t;

struct _pmap_t {
	uint32_t index;
	const char *symbol;
	float val;
};

struct _client_t {
	client_t *next;
	cJSON *root;
};

struct _ui_t {
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

	int done;
	char *executable;
	char *url;

#if defined(_WIN32)
	PROCESS_INFORMATION pi;
#else
	pid_t pid;
#endif

	struct {
		LV2_External_UI_Widget widget;
		const LV2_External_UI_Host *host;
	} kx;

	uint8_t buf [BUF_SIZE];

	char title [512];
	char *bundle_path;

	struct lws_context *context;
	client_t *clients;
};

#if defined(_WIN32)
static inline int 
_spawn(ui_t *ui)
{
	STARTUPINFO si;
	memset(&si, 0x0, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);

	if(!CreateProcess(
		NULL,           // No module name (use command line)
		ui->url,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&ui->pi )       // Pointer to PROCESS_INFORMATION structure
	)
	{
		printf( "CreateProcess failed (%d).\n", GetLastError() );
		return -1;
	}

	return 0;
}

static inline int
_waitpid(ui_t *ui, int *status, bool blocking)
{
	if(blocking)
	{
		WaitForSingleObject(ui->pi.hProcess, INFINITE);
		return 0;
	}

	// !blocking
	switch(WaitForSingleObject(ui->pi.hProcess, 0))
	{
		case WAIT_TIMEOUT: // non-signaled, e.g. still running
			return 0;
		case WAIT_OBJECT_0: // signaled, e.g. not running anymore
			return -1;
		case WAIT_ABANDONED: // failed
		case WAIT_FAILED: // failed
			return -1;
	}

	return 0;
}

static inline int
_kill(ui_t *ui, int sig)
{
	return !(CloseHandle(ui->pi.hProcess) && CloseHandle(ui->pi.hThread));
}

static inline bool
_has_child(ui_t *ui)
{
	return (ui->pi.hProcess != 0x0) && (ui->pi.hThread != 0);
}

static inline void
_invalidate_child(ui_t *ui)
{
	memset(&ui->pi, 0x0, sizeof(PROCESS_INFORMATION));
}
#else
static inline int 
_spawn(ui_t *ui)
{
	ui->pid = fork();
	if(ui->pid == 0) // child
	{
		char *const argv [] = {
			ui->executable,
			ui->url,
			NULL
		};
		execvp(ui->executable, argv); // p = search PATH for executable

		printf("fork child failed\n");
		exit(-1);
	}
	else if(ui->pid < 0)
	{
		printf("fork failed\n");
		return -1;
	}

	return 0;
}

static inline int
_waitpid(ui_t *ui, int *status, bool blocking)
{
	if(blocking)
	{
		waitpid(ui->pid, status, 0);
		return 0;
	}

	int res;
	if( (res = waitpid(ui->pid, status, WNOHANG)) < 0) // error?
	{
		if(errno == ECHILD) // child not existing
		{
			return -1;
		}
	}
	else if( (res > 0) && WIFEXITED(status) ) // child exited?
	{
		return -1;
	}

	return 0;
}

static inline int
_kill(ui_t *ui, int sig)
{
	return kill(ui->pid, SIGINT);
}

static inline bool
_has_child(ui_t *ui)
{
	return ui->pid > 0;
}

static inline void
_invalidate_child(ui_t *ui)
{
	ui->pid = -1;
}
#endif

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

#define CLIENTS_FOREACH(clients, client) \
	for(client_t *(client)=(clients); (client); (client)=(client)->next)

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

			/* this server has no concept of directories */
			if(strchr(in_str+ 1, '/'))
			{
				lws_return_http_status(wsi, HTTP_STATUS_FORBIDDEN, NULL);

				goto try_to_reuse;
			}

			/* if a legal POST URL, let it continue and accept data */
			if(lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI))
				return 0;

			const char *file_path= strcmp(in_str, "/")
				? in_str
				: "/index.html";

			/* refuse to serve files we don't understand */
			const char *mimetype = get_mimetype(file_path);
			if(!mimetype)
			{
				lwsl_err("Unknown mimetype for %s\n", file_path);
				lws_return_http_status(wsi, HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE, NULL);
				return -1;
			}

#if defined(_WIN32)
			const char *sep = ui->bundle_path[strlen(ui->bundle_path) - 1] == '\\' ? "" : "\\";
			const char *sep2 = "\\";
#else
			const char *sep = ui->bundle_path[strlen(ui->bundle_path) - 1] == '/' ? "" : "/";
			const char *sep2 = "/";
#endif

			char *full_path;
			if(asprintf(&full_path, "%s%sweb_ui%s%s", ui->bundle_path, sep, sep2, &file_path[1]) == -1)
			{
				lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL);
				return -1;
			}

			const int n = lws_serve_http_file(wsi, full_path, mimetype, NULL, 0);
			free(full_path);

			if( (n < 0) || ( (n > 0) && lws_http_transaction_completed(wsi)))
				return -1; /* error or can't reuse connection: close the socket */

			// we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback asynchronously
			break;
		}

		case LWS_CALLBACK_HTTP_BODY:
		{
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

		case LWS_CALLBACK_HTTP_WRITEABLE:
			// fall-through
		case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
			// fall-through
		case LWS_CALLBACK_LOCK_POLL:
			// fall-through
		case LWS_CALLBACK_UNLOCK_POLL:
			// fall-through
		case LWS_CALLBACK_GET_THREAD_ID:
			// fall-through
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
			memset(client, 0x0, sizeof(client_t));

			ui->clients = _client_append(ui->clients, client);

			break;
		}

		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			if(client->root)
			{
				char *json = cJSON_PrintUnformatted(client->root);
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

				cJSON_Delete(client->root);
				client->root = NULL;
			}

			break;
		}

		case LWS_CALLBACK_RECEIVE:
		{
			if(len < 6)
				break;

			_moony_cb(ui, in_str);

			/* FIXME
			if(!strcmp(in_str, "closeme\n"))
			{
				lwsl_notice("lv2: closing as requested\n");
				lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY, (unsigned char *)"seeya", 5);
				//TODO client remove?
				return -1;
			}
			*/

			break;
		}

		case LWS_CALLBACK_CLOSED:
		{
			lwsl_notice("LWS_CALLBACK_CLOSE:\n");

			ui->clients = _client_remove(ui->clients, client);
			ui->done = 1;

			break;
		}

		case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
		{
			lwsl_notice("LWS_CALLBACK_WS_PEER_INITIATED_CLOSE: len %d\n", len);

			ui->clients = _client_remove(ui->clients, client);
			ui->done = 1;

			break;
		}

		default:
		{
			break;
		}
	}

	return 0;
}

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
		.rx_buffer_size = 0,
	},
	[PROTOCOL_LV2] = {
		.name = "lv2-protocol",
		.callback = callback_lv2,
		.per_session_data_size = sizeof(client_t),
		.rx_buffer_size = BUF_SIZE,
	},
	{ .name = NULL, .callback = NULL, .per_session_data_size = 0, .rx_buffer_size = 0 }
};

static inline void
_err2(ui_t *ui, const char *from)
{
	if(ui->log)
		lv2_log_error(&ui->logger, "%s", from);
	else
		fprintf(stderr, "%s\n", from);
}

static inline LV2_Atom_Object *
_moony_message_forge(ui_t *ui, LV2_URID key,
	const char *str, uint32_t size)
{
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, BUF_SIZE);
	if(_moony_patch(&ui->uris.patch, &ui->forge, key, str, size))
		return obj;

	if(ui->log)
		lv2_log_error(&ui->logger, "code chunk too long");
	return NULL;
}

static inline LV2_Atom_Forge_Ref
_json_to_atom(ui_t *ui, cJSON *root, LV2_Atom_Forge *forge)
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
_atom_to_json(ui_t *ui, const LV2_Atom *atom)
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
_pmap_get(ui_t *ui, uint32_t idx)
{
	const pmap_t tar = { .index = idx };
	pmap_t *pmap = bsearch(&tar, ui->pmap, MAX_PORTS, sizeof(pmap_t), _pmap_cmp);
	return pmap;
}

static inline void
_moony_dsp(ui_t *ui, cJSON *root)
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
			lv2_atom_forge_set_buffer(&ui->forge, ui->buf, BUF_SIZE);
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
}

static inline void
_schedule(ui_t *ui, cJSON *job)
{
	CLIENTS_FOREACH(ui->clients, client)
	{
		if(!client->root)
		{
			// create root object
			client->root = cJSON_CreateObject();
			cJSON *jobs = cJSON_CreateArray();
			if(client->root && jobs)
				cJSON_AddItemToObject(client->root, "jobs", jobs);
		}

		if(client->root)
		{
			// add job to jobs array
			cJSON *jobs = cJSON_GetObjectItem(client->root, "jobs");
			if(jobs)
				cJSON_AddItemToArray(jobs, job);
		}
	}

	lws_callback_on_writable_all_protocol(ui->context, &protocols[PROTOCOL_LV2]);
}

static inline cJSON * 
_moony_send(ui_t *ui, uint32_t idx, uint32_t size, uint32_t prot, const void *buf)
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
_moony_ui(ui_t *ui, cJSON *root)
{
	cJSON *protocol = cJSON_GetObjectItem(root, LV2_UI__protocol);
	cJSON *value = cJSON_GetObjectItem(root, RDF__value);

	if(  protocol && value
		&& (protocol->type == cJSON_String) )
	{
		if(!strcmp(protocol->valuestring, LV2_ATOM__eventTransfer)
			&& (value->type == cJSON_Object) )
		{
			lv2_atom_forge_set_buffer(&ui->forge, ui->buf, BUF_SIZE);
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
}

static inline void
_moony_cb(ui_t *ui, const char *json)
{
	cJSON *root = cJSON_Parse(json);
	if(root)
	{
		cJSON *destination= cJSON_GetObjectItem(root, LV2_PATCH__destination);
		if(destination)
		{
			if( (destination->type == cJSON_String) && !strcmp(destination->valuestring, MOONY_UI_URI))
				_moony_ui(ui, root);
			else if( (destination->type == cJSON_String) && !strcmp(destination->valuestring, MOONY_DSP_URI))
				_moony_dsp(ui, root);
		}

		cJSON_Delete(root);
	}
}

/*
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
}
*/

// Show Interface
static inline int
_show_cb(LV2UI_Handle instance)
{
printf("_show_cb\n");
	ui_t *handle = instance;

	if(!handle->done)
		return 0; // already showing

	if(_spawn(handle))
		return -1; // failed to spawn

	handle->done = 0;

	return 0;
}

static inline int
_hide_cb(LV2UI_Handle instance)
{
printf("_hide_cb\n");
	ui_t *handle = instance;

	if(_has_child(handle))
	{
		_kill(handle, SIGINT);

		int status;
		_waitpid(handle, &status, true);

		_invalidate_child(handle);
	}

	if(handle->kx.host && handle->kx.host->ui_closed)
		handle->kx.host->ui_closed(handle->controller);

	handle->done = 1;

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
	ui_t *handle = instance;

	if(_has_child(handle))
	{
		int status;
		int res;
		if((res = _waitpid(handle, &status, false)) < 0)
		{
			_invalidate_child(handle);
			//_hide_cb(ui);
			//handle->done = 1; FIXME xdg-open, START return immediately
		}
	}

	if(!handle->done)
	{
		if(lws_service(handle->context, 0))
			handle->done = 1; // lws errored
	}

	return handle->done;
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
		_hide_cb(handle);
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

	// LWS

	struct lws_context_creation_info info;
	memset(&info, 0x0, sizeof(info));

	info.user = ui;
	info.port = 0;
	info.iface = NULL;
	info.protocols = protocols;

	info.gid = -1;
	info.uid = -1;
	info.max_http_header_pool = 1;
	info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;
	ui->context = lws_create_context(&info);
	if(!ui->context)
	{
		free(ui);
		return NULL;
	}

	if(asprintf(&ui->executable, "%s", "luakit") == -1)
	{
		free(ui);
		return NULL;
	}

#if defined(_WIN32)
	if(asprintf(&ui->url, "cmd /c start http://localhost:%d", info.port) == -1)
#else
	if(asprintf(&ui->url, "http://localhost:%d", info.port) == -1)
#endif
	{
		free(ui);
		return NULL;
	}

	_invalidate_child(ui);
	ui->done = 1;

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	ui_t *ui = handle;

	if(ui->executable)
		free(ui->executable);

	if(ui->url)
		free(ui->url);

	if(ui->bundle_path)
		free(ui->bundle_path);

	if(ui->context)
		lws_context_destroy(ui->context);
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
