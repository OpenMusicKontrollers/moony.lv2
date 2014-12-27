/*
 * Copyright (c) 2014 Hanspeter Portner (dev@open-music-kontrollers.ch)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
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
	Evas_Object *theme;
	Evas_Object *vbox;
	Evas_Object *entry;

	char theme_path [512];
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

	ecore_evas_show(ui->ee);

	return 0;
}

static int
_hide_cb(LV2UI_Handle handle)
{
	UI *ui = handle;

	if(!ui)
		return -1;

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

	ecore_evas_resize(ui->ee, ui->w, ui->h);
	evas_object_resize(ui->theme, ui->w, ui->h);
  
  return 0;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri, const char *bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget *widget, const LV2_Feature *const *features)
{
	elm_init(1, (char **)&plugin_uri);

	//edje_frametime_set(0.04);

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
		else if (!strcmp(features[i]->URI, LV2_UI__resize))
			resize = (LV2UI_Resize *)features[i]->data;
  }

	ui->ee = ecore_evas_gl_x11_new(NULL, (Ecore_X_Window)parent, 0, 0, ui->w, ui->h);
	if(!ui->ee)
		ui->ee = ecore_evas_software_x11_new(NULL, (Ecore_X_Window)parent, 0, 0, ui->w, ui->h);
	if(!ui->ee)
		printf("could not start evas\n");
	ui->e = ecore_evas_get(ui->ee);
	ecore_evas_show(ui->ee);

	if(resize)
    resize->ui_resize(resize->handle, ui->w, ui->h);
	
	sprintf(ui->theme_path, "%s/lua.edj", bundle_path);
	
	ui->theme = edje_object_add(ui->e);
	edje_object_file_set(ui->theme, ui->theme_path, LUA_COMMON_UI_URI"/theme");
	evas_object_resize(ui->theme, ui->w, ui->h);
	evas_object_show(ui->theme);

	ui->vbox = elm_box_add(ui->theme);
	evas_object_size_hint_weight_set(ui->vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->vbox);
	evas_object_resize(ui->vbox, 800, 450);
	edje_object_part_swallow(ui->theme, "content", ui->vbox);

	ui->entry = elm_entry_add(ui->vbox);
	elm_entry_single_line_set(ui->entry, EINA_FALSE);
	elm_entry_scrollable_set(ui->entry, EINA_TRUE);
	elm_entry_editable_set(ui->entry, EINA_TRUE);
	elm_object_focus_set(ui->entry, EINA_TRUE);
	evas_object_size_hint_weight_set(ui->entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ui->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(ui->entry);
	elm_box_pack_end(ui->vbox, ui->entry);

	elm_entry_entry_set(ui->entry, 
		"<code>function run(...)<br/>"
		"</tab>return ...<br/>"
		"end</code>");

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;
	
	if(ui)
	{
		ecore_evas_hide(ui->ee);

		elm_box_clear(ui->vbox);
		edje_object_part_unswallow(ui->theme, ui->vbox);
		evas_object_del(ui->vbox);
		evas_object_del(ui->theme);
		ecore_evas_free(ui->ee);
		
		free(ui);
	}

	elm_shutdown();
}

static void
port_event(LV2UI_Handle handle, uint32_t i, uint32_t buffer_size, uint32_t format, const void *buffer)
{
	UI *ui = handle;
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
