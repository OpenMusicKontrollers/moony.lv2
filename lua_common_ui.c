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

typedef struct _UI UI;

struct _UI {
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	int w, h;
	Ecore_Evas *ee;
	Evas *e;
	Evas_Object *parent;
	Evas_Object *vbox;
	Evas_Object *entry;
};

// Idle interface
static int
idle_cb(LV2UI_Handle handle)
{
	UI *ui = handle;

	if(!ui)
		return -1;

	ecore_main_loop_iterate();
	
	return 0;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = idle_cb
};

// Show Interface
static int
_show_cb(LV2UI_Handle handle)
{
	UI *ui = handle;

	if(!ui)
		return -1;

	if(ui->ee)
		ecore_evas_show(ui->ee);

	return 0;
}

static int
_hide_cb(LV2UI_Handle handle)
{
	UI *ui = handle;

	if(!ui)
		return -1;

	if(ui->ee)
		ecore_evas_hide(ui->ee);

	return 0;
}

static const LV2UI_Show_Interface show_ext = {
	.show = _show_cb,
	.hide = _hide_cb
};

// Resize Interface
static int
resize_cb(LV2UI_Feature_Handle handle, int w, int h)
{
	UI *ui = handle;

	if(!ui)
		return -1;

	ui->w = w;
	ui->h = h;

	if(ui->ee)
	{
		ecore_evas_resize(ui->ee, ui->w, ui->h);
		evas_object_resize(ui->parent, ui->w, ui->h);
	}

	evas_object_resize(ui->vbox, ui->w, ui->h);
	evas_object_size_hint_min_set(ui->vbox, ui->w, ui->h);
  
  return 0;
}

static void
_delete(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	elm_box_clear(ui->vbox);
	//evas_object_del(ui->entry);
}

void
_lua_markup(void *data, Evas_Object *entry, char **txt)
{
	//TODO
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	elm_init(1, (char **)&plugin_uri);

	if(		strcmp(plugin_uri, LUA_CONTROL_URI)
		&&	strcmp(plugin_uri, LUA_MIDI_URI)
		&&	strcmp(plugin_uri, LUA_OSC_URI) )
		return NULL;

	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	ui->w = 400;
	ui->h = 400;
	ui->write_function = write_function;
	ui->controller = controller;

	void *parent = NULL;
	LV2UI_Resize *resize = NULL;
	
	int i, j;
	for(i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
			resize = (LV2UI_Resize *)features[i]->data;
  }

	if(descriptor == &lv2_lua_common_ui)
	{
		ui->ee = ecore_evas_gl_x11_new(NULL, (Ecore_X_Window)parent, 0, 0, ui->w, ui->h);
		if(!ui->ee)
			ui->ee = ecore_evas_software_x11_new(NULL, (Ecore_X_Window)parent, 0, 0, ui->w, ui->h);
		if(!ui->ee)
			printf("could not start evas\n");
		ui->e = ecore_evas_get(ui->ee);
		ecore_evas_show(ui->ee);
		
		ui->parent = evas_object_rectangle_add(ui->e);
		evas_object_color_set(ui->parent, 48, 48, 48, 255);
		evas_object_resize(ui->parent, ui->w, ui->h);
		evas_object_show(ui->parent);
	}
	else if(descriptor == &lv2_lua_common_eo)
	{
		ui->ee = NULL;
		ui->parent = (Evas_Object *)parent;
		ui->e = evas_object_evas_get((Evas_Object *)parent);
	}

	if(resize)
    resize->ui_resize(resize->handle, ui->w, ui->h);
	
	ui->vbox = elm_box_add(ui->parent);
	elm_box_horizontal_set(ui->vbox, EINA_FALSE);
	elm_box_homogeneous_set(ui->vbox, EINA_FALSE);
	elm_box_padding_set(ui->vbox, 0, 10);
	evas_object_event_callback_add(ui->vbox, EVAS_CALLBACK_DEL, _delete, ui);
	evas_object_size_hint_weight_set(ui->vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_min_set(ui->vbox, ui->w, ui->h);
	evas_object_size_hint_aspect_set(ui->vbox, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
	evas_object_resize(ui->vbox, ui->w, ui->h);
	evas_object_show(ui->vbox);
	
	ui->entry = elm_entry_add(ui->vbox);
	elm_entry_single_line_set(ui->entry, EINA_FALSE);
	elm_entry_scrollable_set(ui->entry, EINA_TRUE);
	elm_entry_editable_set(ui->entry, EINA_FALSE);
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
	
	if(ui->ee) // X11 UI
		*(Evas_Object **)widget = NULL;
	else // Eo UI
		*(Evas_Object **)widget = ui->vbox;

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;
	
	if(ui)
	{
		if(ui->ee)
		{
			ecore_evas_hide(ui->ee);
			
			evas_object_del(ui->vbox);
			evas_object_del(ui->parent);

			ecore_evas_free(ui->ee);
		}
		
		free(ui);
	}

	elm_shutdown();
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	UI *ui = handle;

	// TODO read notify

	//printf("port_event: %u %u %u\n", i, buffer_size, format);
}

static const void *
extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__showInterface))
		return &show_ext;
		
	return NULL;
}

const LV2UI_Descriptor lv2_lua_common_ui = {
	.URI						= LUA_COMMON_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= extension_data
};

const LV2UI_Descriptor lv2_lua_common_eo = {
	.URI						= LUA_COMMON_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= NULL
};
