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

#include <lua_lv2.h>

#include <Elementary.h>

#include <lv2_eo_ui.h>

typedef struct _UI UI;

struct _UI {
	eo_ui_t eoui;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	int w, h;
	Ecore_Evas *ee;
	Evas *e;
	Evas_Object *parent;
	Evas_Object *vbox;
	Evas_Object *entry;
};

void
_lua_markup(void *data, Evas_Object *entry, char **txt)
{
	//TODO
}

static Evas_Object *
_content_get(eo_ui_t *eoui)
{
	UI *ui = (void *)eoui - offsetof(UI, eoui);
	
	ui->vbox = elm_box_add(eoui->win);
	elm_box_horizontal_set(ui->vbox, EINA_FALSE);
	elm_box_homogeneous_set(ui->vbox, EINA_FALSE);
	elm_box_padding_set(ui->vbox, 0, 10);
	evas_object_size_hint_weight_set(ui->vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_min_set(ui->vbox, ui->w, ui->h);
	evas_object_size_hint_aspect_set(ui->vbox, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
	evas_object_resize(ui->vbox, ui->w, ui->h);
	evas_object_show(ui->vbox);
	
	ui->entry = elm_entry_add(ui->vbox);
	elm_entry_single_line_set(ui->entry, EINA_FALSE);
	elm_entry_scrollable_set(ui->entry, EINA_TRUE);
	//elm_entry_editable_set(ui->entry, EINA_FALSE);
	elm_entry_editable_set(ui->entry, EINA_TRUE);
	elm_entry_markup_filter_append(ui->entry, _lua_markup, NULL);
	elm_entry_cnp_mode_set(ui->entry, ELM_CNP_MODE_PLAINTEXT);
	elm_object_focus_set(ui->entry, EINA_TRUE);
	evas_object_size_hint_weight_set(ui->entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->entry);
	elm_box_pack_end(ui->vbox, ui->entry);

	elm_entry_entry_set(ui->entry, 
		"<code>"
		"<keyword>function</keyword> <function>run</function><brace>(</brace><param>seq</param><brace>)</brace><br/>"
		"<tab/><keyword>for</keyword> frames, atom <keyword>in</keyword> sequence_foreach<brace>(</brace>seq<brace>)</brace> <keyword>do</keyword><br/>"
		"<tab/><tab/><function>print</function><brace>(</brace>frames, atom.size<brace>)</brace><br/>"
		"<tab/><keyword>end</keyword><br/>"
		"<keyword>end</keyword>"
		"</code>");

	return ui->vbox;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{

	if(		strcmp(plugin_uri, LUA_CONTROL_URI)
		&&	strcmp(plugin_uri, LUA_MIDI_URI)
		&&	strcmp(plugin_uri, LUA_OSC_URI) )
		return NULL;

	eo_ui_driver_t driver;
	if(descriptor == &lv2_lua_common_eo)
		driver = EO_UI_DRIVER_EO;
	else if(descriptor == &lv2_lua_common_ui)
		driver = EO_UI_DRIVER_UI;
	else if(descriptor == &lv2_lua_common_x11)
		driver = EO_UI_DRIVER_X11;
	else if(descriptor == &lv2_lua_common_kx)
		driver = EO_UI_DRIVER_KX;
	else
		return NULL;

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	eo_ui_t *eoui = &ui->eoui;
	eoui->driver = driver;
	eoui->content_get = _content_get;
	eoui->w = 400,
	eoui->h = 400;

	ui->write_function = write_function;
	ui->controller = controller;

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
port_event(LV2UI_Handle handle, uint32_t i, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	UI *ui = handle;

	// TODO read notify

	//printf("port_event: %u %u %u\n", i, buffer_size, format);
}

const LV2UI_Descriptor lv2_lua_common_eo = {
	.URI						= LUA_COMMON_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor lv2_lua_common_ui = {
	.URI						= LUA_COMMON_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor lv2_lua_common_x11 = {
	.URI						= LUA_COMMON_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor lv2_lua_common_kx = {
	.URI						= LUA_COMMON_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};
