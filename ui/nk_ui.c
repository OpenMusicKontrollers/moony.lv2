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
	LV2_Atom_Forge forge;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	nk_pugl_window_t win;

	LV2UI_Controller *controller;
	LV2UI_Write_Function writer;

	float dy;
	struct nk_text_edit text_edit;
	bool code_hidden;
	bool prop_hidden;
};

static void
_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	const float dy = handle->dy;
	const struct nk_vec2 window_padding = ctx->style.window.padding;
	const struct nk_vec2 group_padding = ctx->style.window.group_padding;
	const float header_h = 1*dy + 2*window_padding.y;
	const float footer_h = 1*dy + 2*window_padding.y;
	const float body_h = wbounds.h - header_h - footer_h;

	if(nk_begin(ctx, "Moony", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);

		nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
		{
			nk_layout_row_push(ctx, 0.6);
			handle->code_hidden = !nk_select_label(ctx, "Code", NK_TEXT_LEFT, !handle->code_hidden);

			nk_layout_row_push(ctx, 0.4);
			handle->prop_hidden = !nk_select_label(ctx, "Properties", NK_TEXT_LEFT, !handle->prop_hidden);
		}

		nk_layout_row_begin(ctx, NK_DYNAMIC, body_h, !handle->code_hidden + !handle->prop_hidden);
		{
			if(!handle->code_hidden)
			{
				nk_layout_row_push(ctx, handle->prop_hidden ? 1.0 : 0.6);
				const nk_flags state = nk_edit_buffer(ctx, NK_EDIT_BOX, &handle->text_edit, nk_filter_default);
			}

			if(!handle->prop_hidden)
			{
				nk_layout_row_push(ctx, handle->code_hidden ? 1.0 : 0.4);
				if(nk_group_begin(ctx, "Properties", NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER))
				{
					//FIXME

					nk_group_end(ctx);
				}
			}
		}
		nk_layout_row_end(ctx);

		nk_layout_row_dynamic(ctx, dy, 3);
		{
			nk_button_label(ctx, "left");
			nk_button_label(ctx, "right");
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
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
			host_resize = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
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
	if(!handle->map)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	if(handle->log)
		lv2_log_logger_init(&handle->logger, handle->map, handle->log);

	lv2_atom_forge_init(&handle->forge, handle->map);

	handle->controller = controller;
	handle->writer = write_function;

	const char *NK_SCALE = getenv("NK_SCALE");
	const float scale = NK_SCALE ? atof(NK_SCALE) : 1.f;
	handle->dy = 20.f * scale;

	nk_pugl_config_t *cfg = &handle->win.cfg;
	cfg->width = 1280 * scale;
	cfg->height = 720 * scale;
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

	nk_textedit_init_default(&handle->text_edit);

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	nk_textedit_free(&handle->text_edit);

	nk_pugl_hide(&handle->win);
	nk_pugl_shutdown(&handle->win);

	free(handle);
}

static void
port_event(LV2UI_Handle instance, uint32_t index, uint32_t size,
	uint32_t protocol, const void *buf)
{
	plughandle_t *handle = instance;

	//FIXME
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
