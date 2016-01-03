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

#ifndef _EO_UI_H
#define _EO_UI_H

#include <Elementary.h>

#if defined(X11_UI_WRAP)
#	include <Ecore_X.h>
#endif

#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2_external_ui.h> // kxstudio external-ui extension

typedef enum _eo_ui_driver_t eo_ui_driver_t;
typedef struct _eo_ui_t eo_ui_t;
typedef Evas_Object *(*eo_ui_content_get)(eo_ui_t *eoui);

enum _eo_ui_driver_t {
	EO_UI_DRIVER_NONE	= 0,
	EO_UI_DRIVER_EO,
	EO_UI_DRIVER_X11,
	EO_UI_DRIVER_UI,
	EO_UI_DRIVER_KX
};

struct _eo_ui_t {
	eo_ui_driver_t driver;
  LV2UI_Controller controller;
	int w, h;

	Evas_Object *win; // main window
	Evas_Object *bg; // background

	Evas_Object *content;
	eo_ui_content_get content_get;

	union {
		// eo iface
		struct {
			Evas_Object *parent;
			LV2UI_Resize *resize;
		} eo;

		// show iface
		struct {
			volatile int done;
		} ui;

#if defined(X11_UI_WRAP)
		// X11 iface
		struct {
			Ecore_X_Window parent;
			Ecore_X_Window child;
			LV2UI_Resize *resize;

			Ecore_Evas *ee;
		} x11;
#endif

		// external-ui iface
		struct {
			LV2_External_UI_Widget widget;
			const LV2_External_UI_Host *host;
		} kx;
	};
};

// Idle interface
static inline int
_idle_cb(LV2UI_Handle instance)
{
	eo_ui_t *eoui = instance;
	if(!eoui)
		return -1;

	ecore_main_loop_iterate();

	return eoui->ui.done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle_cb
};

static inline void
_show_delete_request(void *data, Evas_Object *obj, void *event_info)
{
	eo_ui_t *eoui = data;
	if(!eoui)
		return;

	// set done flag, host will then call _hide_cb
	eoui->ui.done = 1;
}

// Show Interface
static inline int
_show_cb(LV2UI_Handle instance)
{
	eo_ui_t *eoui = instance;
	if(!eoui)
		return -1;

	// create main window
	eoui->win = elm_win_add(NULL, "EoUI", ELM_WIN_BASIC);
	if(!eoui->win)
		return -1;
	evas_object_smart_callback_add(eoui->win, "delete,request",
		_show_delete_request, eoui);
	evas_object_resize(eoui->win, eoui->w, eoui->h);
	evas_object_show(eoui->win);

	eoui->bg = elm_bg_add(eoui->win);
	if(eoui->bg)
	{
		elm_bg_color_set(eoui->bg, 64, 64, 64);
		evas_object_size_hint_weight_set(eoui->bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(eoui->bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(eoui->bg);
		elm_win_resize_object_add(eoui->win, eoui->bg);
	}

	eoui->content = eoui->content_get(eoui);
	if(eoui->content)
	{
		evas_object_size_hint_weight_set(eoui->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(eoui->content, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(eoui->content);
		elm_win_resize_object_add(eoui->win, eoui->content);
	}

	return 0;
}

static inline int
_hide_cb(LV2UI_Handle instance)
{
	eo_ui_t *eoui = instance;
	if(!eoui)
		return -1;

	// hide & delete bg&main window
	if(eoui->win)
	{
		if(eoui->content)
		{
			elm_win_resize_object_del(eoui->win, eoui->content);
			evas_object_del(eoui->content);
			eoui->content = NULL;
		}
		if(eoui->bg)
		{
			elm_win_resize_object_del(eoui->win, eoui->bg);
			evas_object_del(eoui->bg);
			eoui->bg = NULL;
		}
		evas_object_del(eoui->win);
		eoui->win = NULL;
	}

	// reset done flag
	eoui->ui.done = 0;

	return 0;
}

static const LV2UI_Show_Interface show_ext = {
	.show = _show_cb,
	.hide = _hide_cb
};

// External-UI Interface
static inline void
_kx_run(LV2_External_UI_Widget *widget)
{
	eo_ui_t *eoui = widget
		? (void *)widget - offsetof(eo_ui_t, kx.widget)
		: NULL;
	if(!eoui)
		return;

	ecore_main_loop_iterate();
}

static inline void
_kx_hide(LV2_External_UI_Widget *widget)
{
	eo_ui_t *eoui = widget
		? (void *)widget - offsetof(eo_ui_t, kx.widget)
		: NULL;
	if(!eoui)
		return;

	// hide & delete bg & main window
	if(eoui->win)
	{
		if(eoui->content)
		{
			elm_win_resize_object_del(eoui->win, eoui->content);
			evas_object_del(eoui->content);
			eoui->content = NULL;
		}
		if(eoui->bg)
		{
			elm_win_resize_object_del(eoui->win, eoui->bg);
			evas_object_del(eoui->bg);
			eoui->bg = NULL;
		}
		evas_object_del(eoui->win); // will call _kx_free
		eoui->win = NULL;
	}
}

static inline void
_kx_free(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	eo_ui_t *eoui = data;
	if(!eoui)
		return;

	eoui->content = NULL;
	eoui->bg = NULL;
	eoui->win = NULL;

	if(eoui->kx.host->ui_closed && eoui->controller)
		eoui->kx.host->ui_closed(eoui->controller);
}

static inline void
_kx_show(LV2_External_UI_Widget *widget)
{
	eo_ui_t *eoui = widget
		? (void *)widget - offsetof(eo_ui_t, kx.widget)
		: NULL;
	if(!eoui || eoui->win)
		return;

	// create main window
	const char *title = eoui->kx.host->plugin_human_id
		? eoui->kx.host->plugin_human_id
		: "EoUI";
	eoui->win = elm_win_add(NULL, title, ELM_WIN_BASIC);
	if(!eoui->win)
		return;
	elm_win_autodel_set(eoui->win, EINA_TRUE);
	evas_object_event_callback_add(eoui->win, EVAS_CALLBACK_FREE, _kx_free, eoui);
	evas_object_resize(eoui->win, eoui->w, eoui->h);
	evas_object_show(eoui->win);

	eoui->bg = elm_bg_add(eoui->win);
	if(eoui->bg)
	{
		elm_bg_color_set(eoui->bg, 64, 64, 64);
		evas_object_size_hint_weight_set(eoui->bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(eoui->bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(eoui->bg);
		elm_win_resize_object_add(eoui->win, eoui->bg);
	}

	eoui->content = eoui->content_get(eoui);
	if(eoui->content)
	{
		evas_object_size_hint_weight_set(eoui->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(eoui->content, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(eoui->content);
		elm_win_resize_object_add(eoui->win, eoui->content);
	}
}

// Resize Interface
static inline int
_ui_resize_cb(LV2UI_Feature_Handle instance, int w, int h)
{
	eo_ui_t *eoui = instance;
	if(!eoui)
		return -1;

	// check whether size actually needs any update
	if( (eoui->w == w) && (eoui->h == h) )
		return 0;

	// update size
	eoui->w = w;
	eoui->h = h;

	// resize main window
#if defined(X11_UI_WRAP)
	if(eoui->x11.ee)
		ecore_evas_resize(eoui->x11.ee, eoui->w, eoui->h);
#endif
	if(eoui->win)
		evas_object_resize(eoui->win, eoui->w, eoui->h);
	if(eoui->bg)
		evas_object_resize(eoui->bg, eoui->w, eoui->h);
	if(eoui->content)
		evas_object_resize(eoui->content, eoui->w, eoui->h);
  
  return 0;
}

static const LV2UI_Resize resize_ext = {
	.handle = NULL,
	.ui_resize = _ui_resize_cb
};

#if defined(X11_UI_WRAP)
static void
_x11_ui_wrap_mouse_in(Ecore_Evas *ee)
{
	eo_ui_t *eoui = ecore_evas_data_get(ee, "eoui");
	if(!eoui)
		return;

	if(eoui->x11.parent)
		ecore_x_window_focus(eoui->x11.parent);
}
#endif

static inline int
eoui_instantiate(eo_ui_t *eoui, const LV2UI_Descriptor *descriptor,
	const char *plugin_uri, const char *bundle_path,
	LV2UI_Write_Function write_function, LV2UI_Controller controller,
	LV2UI_Widget *widget, const LV2_Feature *const *features)
{
	//eoui->driver = NULL; set by ui plugin
	eoui->controller = controller;
	eoui->w = eoui->w > 0 ? eoui->w : 400; // fall-back if w == 0
	eoui->h = eoui->h > 0 ? eoui->h : 400; // fall-back if h == 0

	*widget = NULL;

	switch(eoui->driver)
	{
		case EO_UI_DRIVER_EO:
		{
			eoui->eo.parent = NULL; // mandatory
			eoui->eo.resize = NULL; // optional
			for(int i=0; features[i]; i++)
			{
				if(!strcmp(features[i]->URI, LV2_UI__parent))
					eoui->eo.parent = features[i]->data;
				else if(!strcmp(features[i]->URI, LV2_UI__resize))
					eoui->eo.resize = (LV2UI_Resize *)features[i]->data;
			}
			if(!eoui->eo.parent)
				return -1;

			eoui->win = eoui->eo.parent;

			eoui->content = eoui->content_get(eoui);

			*(Evas_Object **)widget = eoui->content;

			if(eoui->eo.resize)
				eoui->eo.resize->ui_resize(eoui->eo.resize->handle, eoui->w, eoui->h);

			break;
		}

		case EO_UI_DRIVER_UI:
		{
			// according to the LV2 spec, the host MUST signal availability of
			// idle interface via features, thus we test for it here
			int host_provides_idle_iface = 0; // mandatory
			for(int i=0; features[i]; i++)
			{
				if(!strcmp(features[i]->URI, LV2_UI__idleInterface))
					host_provides_idle_iface = 1;
			}
			if(!host_provides_idle_iface)
				return -1;

			// initialize elementary library
			_elm_startup_time = ecore_time_unix_get();
			elm_init(0, NULL);

			break;
		}

#if defined(X11_UI_WRAP)
		case EO_UI_DRIVER_X11:
		{
			eoui->x11.parent = 0; // mandatory
			eoui->x11.resize = NULL; // optional
			int host_provides_idle_iface = 0; // mandatory
			for(int i=0; features[i]; i++)
			{
				if(!strcmp(features[i]->URI, LV2_UI__parent))
					eoui->x11.parent = (Ecore_X_Window)(uintptr_t)features[i]->data;
				else if(!strcmp(features[i]->URI, LV2_UI__resize))
					eoui->x11.resize = (LV2UI_Resize *)features[i]->data;
				else if(!strcmp(features[i]->URI, LV2_UI__idleInterface))
					host_provides_idle_iface = 1;
			}
			if(!eoui->x11.parent || !host_provides_idle_iface)
				return -1;

			// initialize elementary library
			_elm_startup_time = ecore_time_unix_get();
			elm_init(0, NULL);

			eoui->x11.ee = ecore_evas_gl_x11_new(NULL, eoui->x11.parent, 0, 0,
				eoui->w, eoui->h);
			if(!eoui->x11.ee)
				eoui->x11.ee = ecore_evas_software_x11_new(NULL, eoui->x11.parent, 0, 0,
					eoui->w, eoui->h);
			if(!eoui->x11.ee)
			{
				//elm_shutdown();
				return -1;
			}
			ecore_evas_data_set(eoui->x11.ee, "eoui", eoui);
			ecore_evas_callback_mouse_in_set(eoui->x11.ee, _x11_ui_wrap_mouse_in);
			ecore_evas_show(eoui->x11.ee);
	
			eoui->win = elm_win_fake_add(eoui->x11.ee);
			if(eoui->win)
			{
				evas_object_resize(eoui->win, eoui->w, eoui->h);
				evas_object_show(eoui->win);

				eoui->bg = elm_bg_add(eoui->win);
				if(eoui->bg)
				{
					elm_bg_color_set(eoui->bg, 64, 64, 64);
					evas_object_size_hint_weight_set(eoui->bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
					evas_object_size_hint_align_set(eoui->bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
					evas_object_resize(eoui->bg, eoui->w, eoui->h);
					evas_object_show(eoui->bg);
				}

				eoui->content = eoui->content_get(eoui);
				if(eoui->content)
				{
					evas_object_size_hint_weight_set(eoui->content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
					evas_object_size_hint_align_set(eoui->content, EVAS_HINT_FILL, EVAS_HINT_FILL);
					evas_object_resize(eoui->content, eoui->w, eoui->h);
					evas_object_show(eoui->content);
				}
			}

			eoui->x11.child = elm_win_xwindow_get(eoui->win);
			*(uintptr_t *)widget = eoui->x11.child;

			if(eoui->x11.resize)
				eoui->x11.resize->ui_resize(eoui->x11.resize->handle, eoui->w, eoui->h);

			break;
		}
#endif

		case EO_UI_DRIVER_KX:
		{
			eoui->kx.host = NULL; // mandatory
			for(int i=0; features[i]; i++)
			{
				if(!strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host))
					eoui->kx.host = features[i]->data;
			}
			if(!eoui->kx.host)
				return -1;

			// initialize elementary library
			_elm_startup_time = ecore_time_unix_get();
			elm_init(0, NULL);

			eoui->kx.widget.run = _kx_run;
			eoui->kx.widget.show = _kx_show;
			eoui->kx.widget.hide = _kx_hide;

			*(LV2_External_UI_Widget **)widget = &eoui->kx.widget;

			break;
		}

		default:
			break;
	}

	return 0;
}

static inline void
eoui_cleanup(eo_ui_t *eoui)
{
	switch(eoui->driver)
	{
		case EO_UI_DRIVER_EO:
		{
			eoui->content = NULL;

			break;
		}

		case EO_UI_DRIVER_UI:
		{
			//elm_shutdown();

			break;
		}

#if defined(X11_UI_WRAP)
		case EO_UI_DRIVER_X11:
		{
			if(eoui->win)
			{
				if(eoui->content)
				{
					elm_win_resize_object_del(eoui->win, eoui->content);
					evas_object_del(eoui->content);
				}
				if(eoui->bg)
				{
					elm_win_resize_object_del(eoui->win, eoui->bg);
					evas_object_del(eoui->bg);
				}
				evas_object_del(eoui->win);
			}
			if(eoui->x11.ee)
			{
				//ecore_evas_free(eoui->x11.ee);
			}

			//elm_shutdown();

			break;
		}
#endif

		case EO_UI_DRIVER_KX:
		{
			//elm_shutdown();

			break;
		}

		default:
			break;
	}

	// clear eoui
	memset(eoui, 0, sizeof(eo_ui_t));
}

// extension data callback for EoUI
static inline const void *
eoui_eo_extension_data(const char *uri)
{
	return NULL;
}

// extension data callback for show interface UI
static inline const void *
eoui_ui_extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__showInterface))
		return &show_ext;
		
	return NULL;
}

// extension data callback for X11UI
static inline const void *
eoui_x11_extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__resize))
		return &resize_ext;
		
	return NULL;
}

// extension data callback for external-ui
static inline const void *
eoui_kx_extension_data(const char *uri)
{
	return NULL;
}

#endif // _EO_UI_H
