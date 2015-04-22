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

#include <lua_lv2.h>

#include <Elementary.h>

#include <lv2_eo_ui.h>

#include <encoder.h>

typedef struct _UI UI;

struct _UI {
	eo_ui_t eoui;

	struct {
		LV2_URID lua_message;
		LV2_URID lua_code;
		LV2_URID lua_error;
		LV2_URID event_transfer;
	} uris;

	LV2_Atom_Forge forge;
	LV2_URID_Map *map;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	int w, h;
	Evas_Object *vbox;
	Evas_Object *entry;
	Evas_Object *error;
	Evas_Object *compile;
};

static void
_lua_markup(void *data, Evas_Object *entry, char **txt)
{
	// replace tab with two spaces
	if(!strcmp(*txt, "<tab/>"))
	{
		free(*txt);
		*txt = strdup("  ");
	}
}

static void
_compile_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	// clear error
	elm_entry_entry_set(ui->error, "");

	// get code from entry
	const char *chunk = elm_entry_entry_get(ui->entry);
	char *utf8 = elm_entry_markup_to_utf8(chunk);
	uint32_t size = strlen(utf8) + 1;
	uint32_t atom_size = sizeof(LV2_Atom) + size;
	LV2_Atom *atom = calloc(1, atom_size);
	if(!atom)
	{
		free(utf8);
		return;
	}

	atom->size = size;
	atom->type = ui->forge.String;
	strcpy(LV2_ATOM_BODY(atom), utf8);

	ui->write_function(ui->controller, 0, atom_size, ui->uris.event_transfer, atom);

	free(utf8);
	free(atom);
}

static void
_encoder_append(const char *str, void *data)
{
	UI *ui = data;

	elm_entry_entry_append(ui->entry, str);
}

static encoder_t enc = {
	.append = _encoder_append,
	.data = NULL
};
encoder_t *encoder = &enc;

static void
_changed(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;
	
	int pos = elm_entry_cursor_pos_get(ui->entry);
	const char *chunk = elm_entry_entry_get(ui->entry);
	char *utf8 = elm_entry_markup_to_utf8(chunk);
	elm_entry_entry_set(ui->entry, "<code>");

	enc.data = ui;
	lua_to_markup(utf8);

	elm_entry_entry_append(ui->entry, "</code>");
	elm_entry_cursor_pos_set(ui->entry, pos);
	free(utf8);
}

static Evas_Object *
_content_get(eo_ui_t *eoui)
{
	UI *ui = (void *)eoui - offsetof(UI, eoui);
	
	ui->vbox = elm_box_add(eoui->win);
	elm_box_horizontal_set(ui->vbox, EINA_FALSE);
	elm_box_homogeneous_set(ui->vbox, EINA_FALSE);
	elm_box_padding_set(ui->vbox, 0, 0);
	
	ui->entry = elm_entry_add(ui->vbox);
	elm_entry_entry_set(ui->entry, "");
	elm_entry_single_line_set(ui->entry, EINA_FALSE);
	elm_entry_scrollable_set(ui->entry, EINA_TRUE);
	elm_entry_editable_set(ui->entry, EINA_TRUE);
	elm_entry_markup_filter_append(ui->entry, _lua_markup, NULL);
	elm_entry_cnp_mode_set(ui->entry, ELM_CNP_MODE_PLAINTEXT);
	elm_object_focus_set(ui->entry, EINA_TRUE);
	evas_object_smart_callback_add(ui->entry, "changed,user", _changed, ui);
	evas_object_size_hint_weight_set(ui->entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->entry);
	elm_box_pack_end(ui->vbox, ui->entry);
	
	ui->error = elm_entry_add(ui->vbox);
	elm_entry_entry_set(ui->error, "");
	elm_entry_single_line_set(ui->error, EINA_FALSE);
	elm_entry_scrollable_set(ui->error, EINA_TRUE);
	elm_entry_editable_set(ui->error, EINA_FALSE);
	elm_entry_cnp_mode_set(ui->error, ELM_CNP_MODE_PLAINTEXT);
	evas_object_size_hint_weight_set(ui->error, EVAS_HINT_EXPAND, 0.125);
	evas_object_size_hint_align_set(ui->error, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->error);
	elm_box_pack_end(ui->vbox, ui->error);

	ui->compile = elm_button_add(ui->vbox);
	elm_object_part_text_set(ui->compile, "default", "Inject");
	evas_object_smart_callback_add(ui->compile, "clicked", _compile_clicked, ui);
	evas_object_size_hint_align_set(ui->compile, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->compile);
	elm_box_pack_end(ui->vbox, ui->compile);

	return ui->vbox;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{

	if(		strcmp(plugin_uri, LUA_CONTROL_URI)
		&&	strcmp(plugin_uri, LUA_ATOM_URI) )
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

	for(int i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = (LV2_URID_Map *)features[i]->data;

	if(!ui->map)
	{
		fprintf(stderr, "%s: Host does not support urid:map\n", descriptor->URI);
		free(ui);
		return NULL;
	}

	ui->uris.lua_message = ui->map->map(ui->map->handle, LUA_MESSAGE_URI);
	ui->uris.lua_code = ui->map->map(ui->map->handle, LUA_CODE_URI);
	ui->uris.lua_error = ui->map->map(ui->map->handle, LUA_ERROR_URI);
	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	lv2_atom_forge_init(&ui->forge, ui->map);

	eo_ui_t *eoui = &ui->eoui;
	eoui->driver = driver;
	eoui->content_get = _content_get;
	eoui->w = 720,
	eoui->h = 720;

	ui->write_function = write_function;
	ui->controller = controller;

	if(eoui_instantiate(eoui, descriptor, plugin_uri, bundle_path, write_function,
		controller, widget, features))
	{
		free(ui);
		return NULL;
	}

	LV2_Atom empty = {
		.size = 0,
		.type = ui->forge.String
	};
	// trigger update
	ui->write_function(ui->controller, 0, sizeof(LV2_Atom),
		ui->uris.event_transfer, &empty);

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

	if( (i == 1) && (format == ui->uris.event_transfer) )
	{
		const LV2_Atom_Object *obj = buffer;
		if(obj->body.otype != ui->uris.lua_message)
			return;
		
		const LV2_Atom_Property_Body *prop = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Object, obj);

		if(prop->key == ui->uris.lua_code)
		{
			const char *chunk = LV2_ATOM_BODY_CONST(&prop->value);
			elm_entry_entry_set(ui->entry, "<code>");

			enc.data = ui;
			lua_to_markup(chunk);

			elm_entry_entry_append(ui->entry, "</code>");
		}
		else if(prop->key == ui->uris.lua_error)
		{
			const char *error = LV2_ATOM_BODY_CONST(&prop->value);
			elm_entry_entry_set(ui->error, "<code><string>");
			elm_entry_entry_append(ui->error, error);
			elm_entry_entry_append(ui->error, "</string></code>");
		}
	}
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
