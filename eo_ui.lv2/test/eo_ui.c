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

#include <Elementary.h>

#include <lv2_eo_ui.h>

#define EO_PREFIX		"http://open-music-kontrollers.ch/lv2/eo#"
#define EO_TEST_URI	EO_PREFIX"test"
#define EO_UI_URI		EO_PREFIX"ui"
#define EO_KX_URI		EO_PREFIX"kx"
#define EO_X11_URI	EO_PREFIX"x11"
#define EO_EO_URI		EO_PREFIX"eo"

typedef struct _UI UI;

struct _UI {
	eo_ui_t eoui;
	LV2_URID_Map *map;

	LV2_URID float_protocol;
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	LV2UI_Port_Map *port_map;
	uint32_t x_in_port;
	uint32_t y_in_port;

	int w, h;
	Evas_Object *widget;
	char img_src [512];
};

const LV2UI_Descriptor eo_eo;
const LV2UI_Descriptor eo_ui;
const LV2UI_Descriptor eo_x11;
const LV2UI_Descriptor eo_kx;

static void
_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	UI *ui = data;
	Evas_Event_Mouse_Move *ev = event_info;

	int w, h;
	evas_object_geometry_get(obj, NULL, NULL, &w, &h);

	Evas_Coord x = ev->cur.canvas.x;
	Evas_Coord y = ev->cur.canvas.y;

	const float X = (float)x / w;
	const float Y = (float)y / h;

	ui->write_function(ui->controller, ui->x_in_port, sizeof(float),
		ui->float_protocol, &X);
	ui->write_function(ui->controller, ui->y_in_port, sizeof(float),
		ui->float_protocol, &Y);
}

static Evas_Object *
_content_get(eo_ui_t *eoui)
{
	UI *ui = (void *)eoui - offsetof(UI, eoui);

	ui->widget = elm_bg_add(eoui->win);
	if(ui->widget)
	{
		elm_bg_file_set(ui->widget, ui->img_src, NULL);
		evas_object_event_callback_add(ui->widget, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, ui);
	}

	return ui->widget;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	if(strcmp(plugin_uri, EO_TEST_URI))
		return NULL;

	eo_ui_driver_t driver;
	if(descriptor == &eo_eo)
		driver = EO_UI_DRIVER_EO;
	else if(descriptor == &eo_ui)
		driver = EO_UI_DRIVER_UI;
	else if(descriptor == &eo_x11)
		driver = EO_UI_DRIVER_X11;
	else if(descriptor == &eo_kx)
		driver = EO_UI_DRIVER_KX;
	else
		return NULL;

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			ui->port_map = (LV2UI_Port_Map *)features[i]->data;
	}

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

	ui->float_protocol = ui->map->map(ui->map->handle, LV2_UI__floatProtocol);

	// query port index of "control" port
	ui->x_in_port = ui->port_map->port_index(ui->port_map->handle, "x_in");
	ui->y_in_port = ui->port_map->port_index(ui->port_map->handle, "y_in");

	eo_ui_t *eoui = &ui->eoui;
	eoui->driver = driver;
	eoui->content_get = _content_get;
	eoui->w = 100,
	eoui->h = 63;

	ui->write_function = write_function;
	ui->controller = controller;

	snprintf(ui->img_src, 512, "%s/lv2_white.png", bundle_path);

	if(eoui_instantiate(eoui, descriptor, plugin_uri, bundle_path, write_function,
		controller, widget, features))
	{
		free(ui);
		return NULL;
	}

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	eoui_cleanup(&ui->eoui);
	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	UI *ui = handle;

	if(port_index == ui->x_in_port)
	{
		//TODO
	}
	else if(port_index == ui->y_in_port)
	{
		//TODO
	}
}

const LV2UI_Descriptor eo_eo = {
	.URI						= EO_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor eo_ui = {
	.URI						= EO_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor eo_x11 = {
	.URI						= EO_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor eo_kx = {
	.URI						= EO_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &eo_eo;
		case 1:
			return &eo_ui;
		case 2:
			return &eo_x11;
		case 3:
			return &eo_kx;
		default:
			return NULL;
	}
}
