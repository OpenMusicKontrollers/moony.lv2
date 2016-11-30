/*
 * Copyright (c) 2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <inttypes.h>

#include <moony.h>

#define NK_PUGL_IMPLEMENTATION
#include "nk_pugl/nk_pugl.h"

typedef struct _plughandle_t plughandle_t;

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	nk_pugl_window_t win;

	LV2UI_Controller *controller;
	LV2UI_Write_Function writer;

	uint32_t control;
	uint32_t notify;
	LV2_URID atom_eventTransfer;
	LV2_URID patch_Get;
	LV2_URID patch_Set;
	LV2_URID patch_property;
	LV2_URID patch_value;
	LV2_URID moony_code;
	LV2_URID moony_error;
	LV2_URID moony_trace;

	atom_ser_t ser;

	float dy;
	bool code_hidden;
	bool prop_hidden;

	char code [MOONY_MAX_CHUNK_LEN];
	int code_sz;

	char error [MOONY_MAX_ERROR_LEN];
	int error_sz;

	int n_trace;
	char **traces;
};

LV2_Atom_Forge_Ref
_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
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

LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

static void
_patch_get(plughandle_t *handle, LV2_URID property)
{
	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Get);
	if(property)
	{
		lv2_atom_forge_key(forge, handle->patch_property);
		lv2_atom_forge_urid(forge, property);
	}
	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_patch_set(plughandle_t *handle, LV2_URID property, uint32_t size, LV2_URID type, const void *body)
{
	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Set);

	lv2_atom_forge_key(forge, handle->patch_property);
	lv2_atom_forge_urid(forge, property);

	lv2_atom_forge_key(forge, handle->patch_value);
	lv2_atom_forge_atom(forge, size, type);
	lv2_atom_forge_write(forge, body, size);

	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	struct nk_input *in = &ctx->input;
	const float dy = handle->dy;
	const struct nk_vec2 window_padding = ctx->style.window.padding;
	const struct nk_vec2 group_padding = ctx->style.window.group_padding;
	const float header_h = 1*dy + 2*window_padding.y;
	const float footer_h = 1*dy + 2*window_padding.y;
	const float body_h = wbounds.h - header_h - footer_h;

	if(nk_begin(ctx, "Moony", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);

		bool has_enter = false;
		if(  nk_input_is_key_pressed(in, NK_KEY_ENTER)
			&& nk_input_is_key_down(in, NK_KEY_SHIFT) )
		{
			handle->error[0] = '\0';
			_patch_set(handle, handle->moony_code,
				handle->code_sz, handle->forge.String, handle->code);
			has_enter = true;
		}

		nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
		{
			nk_layout_row_push(ctx, 0.6);
			handle->code_hidden = !nk_select_label(ctx, "Logic", NK_TEXT_LEFT, !handle->code_hidden);

			nk_layout_row_push(ctx, 0.4);
			handle->prop_hidden = !nk_select_label(ctx, "Properties", NK_TEXT_LEFT, !handle->prop_hidden);
		}

		nk_layout_row_begin(ctx, NK_DYNAMIC, body_h, !handle->code_hidden + !handle->prop_hidden);
		{
			if(!handle->code_hidden)
			{
				nk_layout_row_push(ctx, handle->prop_hidden ? 1.0 : 0.6);
				if(nk_group_begin(ctx, "Logic", NK_WINDOW_NO_SCROLLBAR))
				{
					const float editor_h = body_h - 1*dy - 4*group_padding.y;

					nk_layout_row_dynamic(ctx, editor_h*0.9, 1);
					nk_flags flags = NK_EDIT_BOX;
					if(has_enter)
						flags |= NK_EDIT_SIG_ENTER;
					const nk_flags state = nk_edit_string(ctx, flags,
						handle->code, &handle->code_sz, MOONY_MAX_CHUNK_LEN, nk_filter_default);

					nk_layout_row_dynamic(ctx, editor_h*0.1, 1);
					struct nk_list_view lview;
					if(nk_list_view_begin(ctx, &lview, "Traces", 0, dy, handle->n_trace))
					{
						nk_layout_row_dynamic(ctx, dy, 1);
						for(int l = lview.begin; (l < lview.end) && (l < handle->n_trace); l++)
						{
							nk_labelf(ctx, NK_TEXT_LEFT, "%i: %s", l + 1, handle->traces[l]);
						}

						nk_list_view_end(&lview);
					}

					nk_layout_row_dynamic(ctx, dy, 4);
					if(nk_button_label(ctx, "Submit all"))
					{
						handle->error[0] = '\0';
						_patch_set(handle, handle->moony_code,
							handle->code_sz, handle->forge.String, handle->code);
					}
					if(nk_button_label(ctx, "Submit line"))
					{
						//FIXME
					}
					if(nk_button_label(ctx, "Submit selection"))
					{
						//FIXME
					}
					if(nk_button_label(ctx, "Clear log"))
					{
						for(int i = 0; i < handle->n_trace; i++)
							free(handle->traces[i]);
						free(handle->traces);

						handle->n_trace = 0;
						handle->traces = NULL;
					}

					nk_group_end(ctx);
				}
			}

			if(!handle->prop_hidden)
			{
				nk_layout_row_push(ctx, handle->code_hidden ? 1.0 : 0.4);
				if(nk_group_begin(ctx, "Properties", NK_WINDOW_BORDER))
				{
					nk_layout_row_dynamic(ctx, dy, 3);
					for(int i = 0; i < 99; i++)
						nk_button_label(ctx, "Property");
					//FIXME

					nk_group_end(ctx);
				}
			}
		}
		nk_layout_row_end(ctx);

		nk_layout_row_dynamic(ctx, dy, 2);
		{
			if(handle->error[0] == 0)
				nk_spacing(ctx, 1);
			else
				nk_labelf_colored(ctx, NK_TEXT_LEFT, nk_yellow, "Stopped: %s", handle->error);
			nk_label(ctx, "Moony.lv2: "MOONY_VERSION, NK_TEXT_RIGHT);
		}
	}
	nk_end(ctx);
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	void *parent = NULL;
	LV2UI_Resize *host_resize = NULL;
	LV2UI_Port_Map *port_map = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
			host_resize = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			port_map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			handle->unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			handle->log = features[i]->data;
	}

	if(!parent)
	{
		fprintf(stderr,
			"%s: Host does not support ui:parent\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!port_map)
	{
		fprintf(stderr,
			"%s: Host does not support ui:portMap\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!handle->map || !handle->unmap)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map/unmap\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	if(handle->log)
		lv2_log_logger_init(&handle->logger, handle->map, handle->log);

	lv2_atom_forge_init(&handle->forge, handle->map);

	handle->controller = controller;
	handle->writer = write_function;

	handle->control = port_map->port_index(port_map->handle, "control");
	handle->notify = port_map->port_index(port_map->handle, "notify");

	handle->atom_eventTransfer = handle->map->map(handle->map->handle, LV2_ATOM__eventTransfer);
	handle->patch_Get = handle->map->map(handle->map->handle, LV2_PATCH__Get);
	handle->patch_Set = handle->map->map(handle->map->handle, LV2_PATCH__Set);
	handle->patch_property = handle->map->map(handle->map->handle, LV2_PATCH__property);
	handle->patch_value = handle->map->map(handle->map->handle, LV2_PATCH__value);
	handle->moony_code = handle->map->map(handle->map->handle, MOONY_CODE_URI);
	handle->moony_error = handle->map->map(handle->map->handle, MOONY_ERROR_URI);
	handle->moony_trace = handle->map->map(handle->map->handle, MOONY_TRACE_URI);

	const char *NK_SCALE = getenv("NK_SCALE");
	const float scale = NK_SCALE ? atof(NK_SCALE) : 1.f;
	handle->dy = 20.f * scale;

	nk_pugl_config_t *cfg = &handle->win.cfg;
	cfg->width = 960 * scale;
	cfg->height = 960 * scale;
	cfg->resizable = true;
	cfg->ignore = false;
	cfg->class = "tracker";
	cfg->title = "Tracker";
	cfg->parent = (intptr_t)parent;
	cfg->data = handle;
	cfg->expose = _expose;

	char *path;
	if(asprintf(&path, "%sCousine-Regular.ttf", bundle_path) == -1)
		path = NULL;

	cfg->font.face = path;
	cfg->font.size = 13 * scale;
	
	*(intptr_t *)widget = nk_pugl_init(&handle->win);
	nk_pugl_show(&handle->win);

	if(path)
		free(path);

	if(host_resize)
		host_resize->ui_resize(host_resize->handle, cfg->width, cfg->height);

	handle->ser.moony = NULL;
	handle->ser.size = 1024;
	handle->ser.buf = malloc(1024);
	handle->ser.offset = 0;

	_patch_get(handle, handle->moony_code);

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	for(int i = 0; i < handle->n_trace; i++)
		free(handle->traces[i]);
	free(handle->traces);

	if(handle->ser.buf)
		free(handle->ser.buf);

	nk_pugl_hide(&handle->win);
	nk_pugl_shutdown(&handle->win);

	free(handle);
}

static void
port_event(LV2UI_Handle instance, uint32_t index, uint32_t size,
	uint32_t protocol, const void *buf)
{
	plughandle_t *handle = instance;

	if( (index == handle->notify) && (protocol == handle->atom_eventTransfer) )
	{
		const LV2_Atom_Object *obj = buf;

		if(lv2_atom_forge_is_object_type(&handle->forge, obj->atom.type))
		{
			if(obj->body.otype == handle->patch_Set)
			{
				const LV2_Atom_URID *property = NULL;
				const LV2_Atom *value = NULL;

				lv2_atom_object_get(obj,
					handle->patch_property, &property,
					handle->patch_value, &value,
					0);

				if(  property && (property->atom.type == handle->forge.URID)
					&& value)
				{
					const char *body = LV2_ATOM_BODY_CONST(value);

					if(property->body == handle->moony_code)
					{
						if(value->size <= MOONY_MAX_CHUNK_LEN)
						{
							strncpy(handle->code, body, value->size);
							handle->code_sz = value->size - 1;

							// replace tab with space
							const char *end = handle->code + handle->code_sz;
							for(char *ptr = handle->code; ptr < end; ptr++)
							{
								if(*ptr == '\t')
									*ptr = ' ';
							}

							nk_pugl_post_redisplay(&handle->win);
						}
					}
					else if(property->body == handle->moony_trace)
					{
						if(value->size <= MOONY_MAX_CHUNK_LEN)
						{
							handle->traces = realloc(handle->traces, (handle->n_trace + 1)*sizeof(char *));
							handle->traces[handle->n_trace++] = strdup(body);

							nk_pugl_post_redisplay(&handle->win);
						}
					}
					else if(property->body == handle->moony_error)
					{
						if(value->size <= MOONY_MAX_CHUNK_LEN)
						{
							strncpy(handle->error, body, value->size);
							handle->error_sz = value->size - 1;

							nk_pugl_post_redisplay(&handle->win);
						}
					}
				}
			}
		}
	}
}

static int
_idle(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	return nk_pugl_process_events(&handle->win);
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle
};

static const void *
ext_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
		
	return NULL;
}

const LV2UI_Descriptor nk_ui= {
	.URI            = MOONY_NK_URI,
	.instantiate    = instantiate,
	.cleanup        = cleanup,
	.port_event     = port_event,
	.extension_data = ext_data
};
