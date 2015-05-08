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
	Evas_Object *hbox;
	Evas_Object *error;
	Evas_Object *message;
	Evas_Object *entry;
	Evas_Object *notify;
	Evas_Object *load;
	Evas_Object *status;
	Evas_Object *save;
	Evas_Object *compile;

	char *chunk;
	char *cache;
};

static char *
_entry_cache(UI *ui, int update)
{
	if(update || !ui->cache || !strlen(ui->cache))
	{
		if(ui->cache)
			free(ui->cache);

		const char *chunk = elm_entry_entry_get(ui->entry);
		ui->cache = elm_entry_markup_to_utf8(chunk);
	}

	return ui->cache;
}

static void
_compile(UI *ui)
{
	// clear error
	elm_object_text_set(ui->error, "");
	evas_object_hide(ui->message);

	evas_object_show(ui->notify);

	// get code from entry
	char *utf8 = _entry_cache(ui, 1);
	uint32_t size = strlen(utf8) + 1;

	if(size <= MAX_CHUNK_LEN)
	{
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

		free(atom);
	}
	else
	{
		char buf [64];
		sprintf(buf, "script too long by %i", size - MAX_CHUNK_LEN);
		elm_object_text_set(ui->error, buf);
		evas_object_show(ui->message);
	}
}

static void
_lua_markup(void *data, Evas_Object *obj, char **txt)
{
	UI *ui = data;

	// replace tab with two spaces
	if(!strcmp(*txt, "<tab/>"))
	{
		free(*txt);
		*txt = strdup("  ");
	}
	// intercept shift-enter
	else if(!strcmp(*txt, "<br/>"))
	{
		const Evas_Modifier *mods = evas_key_modifier_get(evas_object_evas_get(obj));
		if(evas_key_modifier_is_set(mods, "Shift"))
		{
			free(*txt);
			*txt = strdup("");

			_compile(ui);
			//TODO give some visual feedback here
		}
	}
}

static void
_encoder_begin(void *data)
{
	UI *ui = data;

	ui->chunk = strdup("");
}

static void
_encoder_append(const char *str, void *data)
{
	UI *ui = data;

	size_t size = 0;
	size += ui->chunk ? strlen(ui->chunk) : 0;
	size += str ? strlen(str) + 1 : 0;

	ui->chunk = realloc(ui->chunk, size);

	strncat(ui->chunk, str, size);
}

static void
_encoder_end(void *data)
{
	UI *ui = data;

	elm_entry_entry_set(ui->entry, ui->chunk);
	free(ui->chunk);
}

static encoder_t enc = {
	.begin = _encoder_begin,
	.append = _encoder_append,
	.end = _encoder_end,
	.data = NULL
};
encoder_t *encoder = &enc;

static void
_load_chosen(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	const char *path = event_info;
	if(!path)
		return;
	
	// load file
	FILE *f = fopen(path, "rb");
	if(f)
	{
		enc.data = ui;
		lua_to_markup(NULL, f);
		elm_entry_cursor_pos_set(ui->entry, 0);
	}
	fclose(f);
}

static void
_save_chosen(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	const char *path = event_info;
	if(!path)
		return;
	
	// get code from entry
	const char *chunk = elm_entry_entry_get(ui->entry);
	char *utf8 = elm_entry_markup_to_utf8(chunk);
	uint32_t size = strlen(utf8);

	// save to file
	FILE *f = fopen(path, "wb");
	if(f)
		fwrite(utf8, size, 1, f);
	fclose(f);

	// cleanup
	free(utf8);
}

static void
_compile_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	_compile(ui);
}

static void
_changed(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;
	
	int pos = elm_entry_cursor_pos_get(ui->entry);
	char *utf8 = _entry_cache(ui, 1);

	enc.data = ui;
	lua_to_markup(utf8, NULL);
	elm_entry_cursor_pos_set(ui->entry, pos);
}

static void
_cursor(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	int pos = elm_entry_cursor_pos_get(ui->entry);
	char *utf8 = _entry_cache(ui, 0);

	// count lines and chars
	const char *end = utf8 + pos;
	int l = 0;
	int c = pos;
	for(const char *ptr = utf8, *start = utf8; ptr < end; ptr++)
	{
		if(*ptr == '\n')
		{
			l += 1;
			c -= ptr + 1 - start;
			start = ptr + 1;
		}
	}

	char buf [64];
	sprintf(buf, "%i / %i", l + 1, c + 1);

	elm_object_text_set(ui->status, buf);
}

static void
_unfocused(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	elm_object_text_set(ui->status, "");
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
	elm_entry_autosave_set(ui->entry, EINA_FALSE);
	elm_entry_entry_set(ui->entry, "");
	elm_entry_single_line_set(ui->entry, EINA_FALSE);
	elm_entry_scrollable_set(ui->entry, EINA_TRUE);
	elm_entry_editable_set(ui->entry, EINA_TRUE);
	elm_entry_markup_filter_append(ui->entry, _lua_markup, ui);
	elm_entry_cnp_mode_set(ui->entry, ELM_CNP_MODE_PLAINTEXT);
	elm_object_focus_set(ui->entry, EINA_TRUE);
	evas_object_smart_callback_add(ui->entry, "changed,user", _changed, ui);
	evas_object_smart_callback_add(ui->entry, "cursor,changed", _cursor, ui);
	evas_object_smart_callback_add(ui->entry, "cursor,changed,manual", _cursor, ui);
	evas_object_smart_callback_add(ui->entry, "unfocused", _unfocused, ui);
	evas_object_size_hint_weight_set(ui->entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->entry);
	elm_box_pack_end(ui->vbox, ui->entry);

	ui->error = elm_label_add(ui->vbox);
	elm_object_text_set(ui->error, "");
	evas_object_show(ui->error);

	ui->message = elm_notify_add(ui->vbox);
	elm_notify_timeout_set(ui->message, 0);
	elm_notify_align_set(ui->message, 0.5, 0.0);
	elm_notify_allow_events_set(ui->message, EINA_TRUE);
	evas_object_size_hint_weight_set(ui->message, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_content_set(ui->message, ui->error);

	Evas_Object *label = elm_label_add(ui->vbox);
	elm_object_text_set(label, "compiling ...");
	evas_object_show(label);

	ui->notify = elm_notify_add(ui->vbox);
	elm_notify_timeout_set(ui->notify, 1);
	elm_notify_align_set(ui->notify, 0.5, 0.5);
	elm_notify_allow_events_set(ui->notify, EINA_FALSE);
	evas_object_size_hint_weight_set(ui->notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_content_set(ui->notify, label);
	
	ui->hbox = elm_box_add(eoui->win);
	elm_box_horizontal_set(ui->hbox, EINA_TRUE);
	elm_box_homogeneous_set(ui->hbox, EINA_TRUE);
	elm_box_padding_set(ui->hbox, 0, 0);
	evas_object_size_hint_align_set(ui->hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->hbox);
	elm_box_pack_end(ui->vbox, ui->hbox);

	ui->load = elm_fileselector_button_add(ui->hbox);
	elm_fileselector_button_inwin_mode_set(ui->load, EINA_FALSE);
	elm_fileselector_button_window_title_set(ui->load, "Load Lua script from file");
	elm_fileselector_is_save_set(ui->save, EINA_FALSE);
	elm_object_part_text_set(ui->load, "default", "Load");
	evas_object_smart_callback_add(ui->load, "file,chosen", _load_chosen, ui);
	evas_object_size_hint_weight_set(ui->load, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->load, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->load);
	elm_box_pack_end(ui->hbox, ui->load);

	ui->save = elm_fileselector_button_add(ui->hbox);
	elm_fileselector_button_inwin_mode_set(ui->save, EINA_FALSE);
	elm_fileselector_button_window_title_set(ui->save, "Save Lua script to file");
	elm_fileselector_is_save_set(ui->save, EINA_TRUE);
	elm_object_part_text_set(ui->save, "default", "Save");
	evas_object_smart_callback_add(ui->save, "file,chosen", _save_chosen, ui);
	evas_object_size_hint_weight_set(ui->save, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->save, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->save);
	elm_box_pack_end(ui->hbox, ui->save);

	ui->compile = elm_button_add(ui->hbox);
	elm_object_part_text_set(ui->compile, "default", "Compile");
	evas_object_smart_callback_add(ui->compile, "clicked", _compile_clicked, ui);
	evas_object_size_hint_weight_set(ui->compile, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->compile, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->compile);
	elm_box_pack_end(ui->hbox, ui->compile);

	ui->status = elm_label_add(ui->vbox);
	elm_object_text_set(ui->status, "");
	evas_object_show(ui->status);
	elm_box_pack_end(ui->hbox, ui->status);

	return ui->vbox;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{

	if(		strcmp(plugin_uri, LUA_C1XC1_URI)
		&&	strcmp(plugin_uri, LUA_C2XC2_URI)
		&&	strcmp(plugin_uri, LUA_C4XC4_URI)

		&&	strcmp(plugin_uri, LUA_A1XA1_URI)
		&&	strcmp(plugin_uri, LUA_A2XA2_URI)
		&&	strcmp(plugin_uri, LUA_A4XA4_URI)

		&&	strcmp(plugin_uri, LUA_A1XC1_URI)
		&&	strcmp(plugin_uri, LUA_A1XC2_URI)
		&&	strcmp(plugin_uri, LUA_A1XC4_URI)

		&&	strcmp(plugin_uri, LUA_C1XA1_URI)
		&&	strcmp(plugin_uri, LUA_C2XA1_URI)
		&&	strcmp(plugin_uri, LUA_C4XA1_URI)
		
		&&	strcmp(plugin_uri, LUA_C4A1XC4A1_URI) )
	{
		return NULL;
	}

	eo_ui_driver_t driver;
	if(descriptor == &common_eo)
		driver = EO_UI_DRIVER_EO;
	else if(descriptor == &common_ui)
		driver = EO_UI_DRIVER_UI;
	else if(descriptor == &common_x11)
		driver = EO_UI_DRIVER_X11;
	else if(descriptor == &common_kx)
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

	if(ui->cache)
		free(ui->cache);
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

			enc.data = ui;
			lua_to_markup(chunk, NULL);
			elm_entry_cursor_pos_set(ui->entry, 0);
		}
		else if(prop->key == ui->uris.lua_error)
		{
			const char *error = LV2_ATOM_BODY_CONST(&prop->value);
			elm_object_text_set(ui->error, error);
			evas_object_show(ui->message);
		}
	}
}

const LV2UI_Descriptor common_eo = {
	.URI						= LUA_COMMON_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor common_ui = {
	.URI						= LUA_COMMON_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor common_x11 = {
	.URI						= LUA_COMMON_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor common_kx = {
	.URI						= LUA_COMMON_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};
