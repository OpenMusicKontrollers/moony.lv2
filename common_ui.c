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

#include <moony.h>

#include <Elementary.h>

#include <lv2_eo_ui.h>

#include <common_ui.h>
#include <encoder.h>

typedef struct _UI UI;

struct _UI {
	eo_ui_t eoui;

	struct {
		LV2_URID moony_message;
		LV2_URID moony_code;
		LV2_URID moony_error;
		LV2_URID event_transfer;
	} uris;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	LV2_Atom_Forge forge;
	LV2_URID_Map *map;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	LV2UI_Port_Map *port_map;
	uint32_t control_port;
	uint32_t notify_port;

	int w, h;
	Evas_Object *table;
	Evas_Object *error;
	Evas_Object *message;
	Evas_Object *entry;
	Evas_Object *notify;
	Evas_Object *load;
	Evas_Object *status;
	Evas_Object *save;
	Evas_Object *compile;
	Evas_Object *external;
	Evas_Object *popup;
	Evas_Object *overlay;
	
	Eina_List *desks;

	Ecore_Exe *exe;
	Ecore_Event_Handler *handler;
	Ecore_File_Monitor *monitor;

	char *logo_path;

	char *chunk;
	char *cache;
	uint8_t buf [0x10000];
};

static void
_moony_message_send(UI *ui, LV2_URID otype, LV2_URID key,
	uint32_t size, const char *str)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
	if(_moony_message_forge(&ui->forge, otype, key, size, str))
	{
		// trigger update
		ui->write_function(ui->controller, ui->control_port, lv2_atom_total_size(&obj->atom),
			ui->uris.event_transfer, ui->buf);
		if(ui->log)
			lv2_log_note(&ui->logger, str && size ? "send code chunk" : "query code chunk");
	}
	else if(ui->log)
		lv2_log_error(&ui->logger, "code chunk too long");
}

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

	if(size <= MOONY_MAX_CHUNK_LEN)
	{
		_moony_message_send(ui, ui->uris.moony_message, ui->uris.moony_code, size, utf8);
	}
	else
	{
		char buf [64];
		sprintf(buf, "script too long by %"PRIi32, size - MOONY_MAX_CHUNK_LEN);
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

	if(size)
		ui->chunk = realloc(ui->chunk, size);

	if(ui->chunk && str)
		strcat(ui->chunk, str);
}

static void
_encoder_end(void *data)
{
	UI *ui = data;

	elm_entry_entry_set(ui->entry, ui->chunk);
	free(ui->chunk);
}

static moony_encoder_t enc = {
	.begin = _encoder_begin,
	.append = _encoder_append,
	.end = _encoder_end,
	.data = NULL
};
moony_encoder_t *encoder = &enc;

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
		_compile(ui);
		fclose(f);
	}
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
_monitor(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
	UI *ui = data;

	switch(event)
	{
		case ECORE_FILE_EVENT_NONE:              /**< No event. */
		case ECORE_FILE_EVENT_CREATED_FILE:      /**< Created file event. */
		case ECORE_FILE_EVENT_CREATED_DIRECTORY: /**< Created directory event. */
		case ECORE_FILE_EVENT_DELETED_FILE:      /**< Deleted file event. */
		case ECORE_FILE_EVENT_DELETED_DIRECTORY: /**< Deleted directory event. */
		case ECORE_FILE_EVENT_DELETED_SELF:      /**< Deleted monitored directory event. */
		case ECORE_FILE_EVENT_CLOSED:            /**< Closed file event */
		{
			// ignore
			break;
		}
		case ECORE_FILE_EVENT_MODIFIED:          /**< Modified file or directory event. */
		{
			//printf("external editor has modified file\n");
			_load_chosen(ui, ui->entry, (void *)path);

			break;
		}
	}
}

static void
_exe_cleanup(UI *ui)
{
	ui->exe = NULL;

	if(ui->handler)
	{
		ecore_event_handler_del(ui->handler);
		ui->handler = NULL;
	}

	if(ui->monitor)
	{
		ecore_file_monitor_del(ui->monitor);
		ui->monitor = NULL;
	}

	elm_object_text_set(ui->overlay, "closing");
	evas_object_hide(ui->overlay);
	elm_entry_editable_set(ui->entry, EINA_TRUE);
	elm_object_disabled_set(ui->load, EINA_FALSE);
	elm_object_disabled_set(ui->save, EINA_FALSE);
	elm_object_disabled_set(ui->compile, EINA_FALSE);
}

static Eina_Bool
_exe_del(void *data, int type, void *event)
{
	UI *ui = data;
	Ecore_Exe_Event_Del *ev = event;
		
	// is this event of our concern?
	if(ui->exe && (ev->exe == ui->exe) )
	{
		//printf("external editor has been closed\n");

		if(ui->log)
			switch(ev->exit_code)
			{
				case 0: // success
					break;
#if !defined(_WIN32) && !defined(__APPLE__)
				case 1:
					lv2_log_error(&ui->logger, "xdg-open: Error in command line syntax.");
					break;
				case 2:
					lv2_log_error(&ui->logger, "xdg-open: One of the files passed on the command line did not exist.");
					break;
				case 3:
					lv2_log_error(&ui->logger, "xdg-open: A required tool could not be found.");
					break;
				case 4:
					lv2_log_error(&ui->logger, "xdg-open: The action failed.");
					break;
#endif
				default:
					lv2_log_error(&ui->logger, "opening the file in an external editor failed.");
					break;
			}

		_exe_cleanup(ui);

		return EINA_TRUE;
	}

	return EINA_FALSE;
}

static void
_editor_chosen(void *data, Evas_Object *obj, void *event_info)
{
	Efreet_Desktop *desk = data;
	UI *ui = evas_object_data_get(obj, "ui");
	
	if(!ui)
		return;

	Eina_Tmpstr *path;
	int fd = eina_file_mkstemp("moony_XXXXXX.lua", &path);
	if(fd == -1)
	{
		if(ui->log)
			lv2_log_error(&ui->logger, "creation of temporary file failed");
		return;
	}

	// write to file
	close(fd); //FIXME this is dangerous, better fdopen
	_save_chosen(ui, ui->entry, (void *)path);

	// open file in external editor
	char *command = NULL;
	if(desk)
	{
		char *exec = strdup(desk->exec);
		char *repl = NULL;

		if(exec)
		{
			// replace %F with %s
			if((repl=strstr(exec, "%F")))
				repl[1] = 's';
			
			asprintf(&command, exec, path);

			free(exec);
		}
	}
	else // !desk
	{
#if defined(_WIN32)
		asprintf(&command, "START %s", path);
#elif defined(__APPLE__)
		asprintf(&command, "open %s", path);
#else // Linux/BSD
		asprintf(&command, "xdg-open %s", path);
#endif
	}

	if(command)
	{
		ui->exe = ecore_exe_run(command, ui);
		free(command);

		if(ui->exe)
		{
			// add file and exe monitoring callback
			ui->handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _exe_del, ui);
			ui->monitor = ecore_file_monitor_add(path, _monitor, ui);
		}
		else if(ui->log)
			lv2_log_error(&ui->logger, "spawning of external editor failed");
	}
	else if(ui->log)
		lv2_log_error(&ui->logger, "memory allocation for external editor command failed");

	eina_tmpstr_del(path);
	
	elm_object_text_set(ui->overlay, "external editor in use");
}

static void
_external_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	if(evas_object_visible_get(ui->overlay))
	{
		if(ui->exe)
			ecore_exe_quit(ui->exe);

		//if(ui->exe)
			_exe_cleanup(ui);
	}
	else // !visible
	{
		Eina_List *l;
		Efreet_Desktop *desk;
		EINA_LIST_FOREACH(ui->desks, l, desk)
		{
			if(desk->exec)
			{
				Evas_Object *icon = NULL;
				if(desk->icon && (icon = elm_icon_add(ui->overlay)) )
				{
					elm_icon_standard_set(icon, desk->icon);
					evas_object_show(icon);
				}

				elm_popup_item_append(ui->overlay, desk->name, icon, _editor_chosen, desk);
			}
		}
		elm_popup_item_append(ui->overlay, "Use system default", NULL, _editor_chosen, NULL);

		evas_object_show(ui->overlay);
		elm_entry_editable_set(ui->entry, EINA_FALSE);
		elm_object_disabled_set(ui->load, EINA_TRUE);
		elm_object_disabled_set(ui->save, EINA_TRUE);
		elm_object_disabled_set(ui->compile, EINA_TRUE);
	}
}

static void
_compile_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	_compile(ui);
}

static void
_info_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	// toggle popup
	if(ui->popup)
	{
		if(evas_object_visible_get(ui->popup))
			evas_object_hide(ui->popup);
		else
			evas_object_show(ui->popup);
	}
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
	sprintf(buf, "%"PRIi32" / %"PRIi32, l + 1, c + 1);

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

	ui->table = elm_table_add(eoui->win);
	if(ui->table)
	{
		elm_table_homogeneous_set(ui->table, EINA_FALSE);
		elm_table_padding_set(ui->table, 0, 0);

		ui->entry = elm_entry_add(ui->table);
		if(ui->entry)
		{
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
			elm_table_pack(ui->table, ui->entry, 0, 0, 6, 1);
		}

		ui->error = elm_label_add(ui->table);
		if(ui->error)
		{
			elm_object_text_set(ui->error, "");
			evas_object_show(ui->error);
		}

		ui->message = elm_notify_add(ui->table);
		if(ui->message)
		{
			elm_notify_timeout_set(ui->message, 0);
			elm_notify_align_set(ui->message, 0.5, 0.0);
			elm_notify_allow_events_set(ui->message, EINA_TRUE);
			evas_object_size_hint_weight_set(ui->message, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			if(ui->error)
				elm_object_content_set(ui->message, ui->error);
		}

		Evas_Object *label = elm_label_add(ui->table);
		if(label)
		{
			elm_object_text_set(label, "compiling ...");
			evas_object_show(label);
		}

		ui->notify = elm_notify_add(ui->table);
		if(ui->notify)
		{
			elm_notify_timeout_set(ui->notify, 0.5);
			elm_notify_align_set(ui->notify, 0.5, 0.75);
			elm_notify_allow_events_set(ui->notify, EINA_TRUE);
			evas_object_size_hint_weight_set(ui->notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			if(label)
				elm_object_content_set(ui->notify, label);
		}
		
		ui->overlay = elm_popup_add(ui->entry);
		if(ui->overlay)
		{
			evas_object_size_hint_weight_set(ui->overlay, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			evas_object_data_set(ui->overlay, "ui", ui);
		}

		ui->load = elm_fileselector_button_add(ui->table);
		if(ui->load)
		{
			elm_fileselector_button_inwin_mode_set(ui->load, EINA_FALSE);
			elm_fileselector_button_window_title_set(ui->load, "Import Lua script from file");
			elm_fileselector_is_save_set(ui->load, EINA_FALSE);
			elm_object_part_text_set(ui->load, "default", "Import");
			evas_object_smart_callback_add(ui->load, "file,chosen", _load_chosen, ui);
			evas_object_size_hint_weight_set(ui->load, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->load, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->load);
			elm_table_pack(ui->table, ui->load, 0, 1, 1, 1);

			Evas_Object *icon = elm_icon_add(ui->load);
			if(icon)
			{
				elm_icon_standard_set(icon, "document-import");
				evas_object_show(icon);
				elm_object_content_set(ui->load, icon);
			}
		}

		ui->save = elm_fileselector_button_add(ui->table);
		if(ui->save)
		{
			elm_fileselector_button_inwin_mode_set(ui->save, EINA_FALSE);
			elm_fileselector_button_window_title_set(ui->save, "Export Lua script to file");
			elm_fileselector_is_save_set(ui->save, EINA_TRUE);
			elm_object_part_text_set(ui->save, "default", "Export");
			evas_object_smart_callback_add(ui->save, "file,chosen", _save_chosen, ui);
			evas_object_size_hint_weight_set(ui->save, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->save, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->save);
			elm_table_pack(ui->table, ui->save, 1, 1, 1, 1);

			Evas_Object *icon = elm_icon_add(ui->save);
			if(icon)
			{
				elm_icon_standard_set(icon, "document-export");
				evas_object_show(icon);
				elm_object_content_set(ui->save, icon);
			}
		}

		ui->external = elm_button_add(ui->table);
		if(ui->external)
		{
			elm_object_part_text_set(ui->external, "default", "Ext. Editor");
			evas_object_smart_callback_add(ui->external, "clicked", _external_clicked, ui);
			evas_object_size_hint_weight_set(ui->external, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->external, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->external);
			elm_table_pack(ui->table, ui->external, 2, 1, 1, 1);

			Evas_Object *icon = elm_icon_add(ui->external);
			if(icon)
			{
				elm_icon_standard_set(icon, "view-fullscreen");
				evas_object_show(icon);
				elm_object_content_set(ui->external, icon);
			}
		}

		ui->compile = elm_button_add(ui->table);
		if(ui->compile)
		{
			elm_object_part_text_set(ui->compile, "default", "Compile");
			evas_object_smart_callback_add(ui->compile, "clicked", _compile_clicked, ui);
			evas_object_size_hint_weight_set(ui->compile, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->compile, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->compile);
			elm_table_pack(ui->table, ui->compile, 3, 1, 1, 1);
			elm_object_tooltip_text_set(ui->compile, "Shift + Enter");
#if defined(ELM_1_9)
			elm_object_tooltip_orient_set(ui->compile, ELM_TOOLTIP_ORIENT_TOP);
#endif

			Evas_Object *icon = elm_icon_add(ui->compile);
			if(icon)
			{
				elm_icon_standard_set(icon, "system-run");
				evas_object_show(icon);
				elm_object_content_set(ui->compile, icon);
			}
		}

		ui->status = elm_label_add(ui->table);
		if(ui->status)
		{
			elm_object_text_set(ui->status, "");
			evas_object_size_hint_weight_set(ui->status, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->status, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->status);
			elm_table_pack(ui->table, ui->status, 4, 1, 1, 1);
		}

		Evas_Object *info = elm_button_add(ui->table);
		if(info)
		{
			evas_object_smart_callback_add(info, "clicked", _info_clicked, ui);
			evas_object_size_hint_weight_set(info, 0.f, 0.f);
			evas_object_size_hint_align_set(info, 1.f, EVAS_HINT_FILL);
			evas_object_show(info);
			elm_table_pack(ui->table, info, 5, 1, 1, 1);

			Evas_Object *icon = elm_icon_add(info);
			if(icon)
			{
				elm_image_file_set(icon, ui->logo_path, NULL);
				evas_object_size_hint_min_set(icon, 20, 20);
				evas_object_size_hint_max_set(icon, 32, 32);
				//evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
				evas_object_show(icon);
				elm_object_part_content_set(info, "icon", icon);
			}
		}

		ui->popup = elm_popup_add(ui->table);
		if(ui->popup)
		{
			elm_popup_allow_events_set(ui->popup, EINA_TRUE);

			Evas_Object *hbox = elm_box_add(ui->popup);
			if(hbox)
			{
				elm_box_horizontal_set(hbox, EINA_TRUE);
				elm_box_homogeneous_set(hbox, EINA_FALSE);
				elm_box_padding_set(hbox, 10, 0);
				evas_object_show(hbox);
				elm_object_content_set(ui->popup, hbox);

				Evas_Object *icon = elm_icon_add(hbox);
				if(icon)
				{
					elm_image_file_set(icon, ui->logo_path, NULL);
					evas_object_size_hint_min_set(icon, 128, 128);
					evas_object_size_hint_max_set(icon, 256, 256);
					evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_BOTH, 1, 1);
					evas_object_show(icon);
					elm_box_pack_end(hbox, icon);
				}

				Evas_Object *label2 = elm_label_add(hbox);
				if(label2)
				{
					elm_object_text_set(label2,
						"<color=#b00 shadow_color=#fff font_size=20>"
						"Moony - Logic Glue for LV2"
						"</color></br><align=left>"
						"Version "MOONY_VERSION"</br></br>"
						"Copyright (c) 2015 Hanspeter Portner</br></br>"
						"This is free and libre software</br>"
						"Released under Artistic License 2.0</br>"
						"By Open Music Kontrollers</br></br>"
						"<color=#bbb>"
						"http://open-music-kontrollers.ch/lv2/moony</br>"
						"dev@open-music-kontrollers.ch"
						"</color></align>");

					evas_object_show(label2);
					elm_box_pack_end(hbox, label2);
				}
			}
		}
	}

	return ui->table;
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
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			ui->port_map = (LV2UI_Port_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			ui->log = (LV2_Log_Log *)features[i]->data;
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

	// query port index of "control" port
	ui->control_port = ui->port_map->port_index(ui->port_map->handle, "control");
	ui->notify_port = ui->port_map->port_index(ui->port_map->handle, "notify");

	ui->uris.moony_message = ui->map->map(ui->map->handle, MOONY_MESSAGE_URI);
	ui->uris.moony_code = ui->map->map(ui->map->handle, MOONY_CODE_URI);
	ui->uris.moony_error = ui->map->map(ui->map->handle, MOONY_ERROR_URI);
	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	lv2_atom_forge_init(&ui->forge, ui->map);
	if(ui->log)
		lv2_log_logger_init(&ui->logger, ui->map, ui->log);

	eo_ui_t *eoui = &ui->eoui;
	eoui->driver = driver;
	eoui->content_get = _content_get;
	eoui->w = 640,
	eoui->h = 720;

	ui->write_function = write_function;
	ui->controller = controller;

	asprintf(&ui->logo_path, "%s/omk_logo_256x256.png", bundle_path);

	if(eoui_instantiate(eoui, descriptor, plugin_uri, bundle_path, write_function,
		controller, widget, features))
	{
		free(ui);
		return NULL;
	}

	_moony_message_send(ui, ui->uris.moony_message, ui->uris.moony_code, 0, NULL);
	
	efreet_init();
	ui->desks = efreet_util_desktop_mime_list("text/plain");

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	Efreet_Desktop *desk;
	EINA_LIST_FREE(ui->desks, desk)
		efreet_desktop_free(desk);
	efreet_shutdown();

	if(ui->exe)
		ecore_exe_quit(ui->exe); //TODO does not seem to work with xdg-open
	if(ui->handler)
		ecore_event_handler_del(ui->handler);
	if(ui->monitor)
		ecore_file_monitor_del(ui->monitor);

	eoui_cleanup(&ui->eoui);

	if(ui->logo_path)
		free(ui->logo_path);
	if(ui->cache)
		free(ui->cache);
	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	UI *ui = handle;

	if( (port_index == ui->notify_port) && (format == ui->uris.event_transfer) )
	{
		/*FIXME
		const moony_message_t *msg = buffer;

		if(  (msg->obj.atom.type == ui->forge.Object)
			&& (msg->obj.body.otype == ui->uris.moony_message) )
		{
			if(msg->prop.key == ui->uris.moony_code)
			{
				const char *chunk = msg->body;

				enc.data = ui;
				lua_to_markup(chunk, NULL);
				elm_entry_cursor_pos_set(ui->entry, 0);
			}
			else if(msg->prop.key == ui->uris.moony_error)
			{
				elm_object_text_set(ui->error, msg->body);
				evas_object_show(ui->message);
			}
		}
		*/
		const LV2_Atom_Object *obj = buffer;

		if(  lv2_atom_forge_is_object_type(&ui->forge, obj->atom.type)
			&& (obj->body.otype == ui->uris.moony_message) )
		{
			const LV2_Atom_String *moony_error = NULL;
			const LV2_Atom_String *moony_code = NULL;
			
			LV2_Atom_Object_Query q[] = {
				{ ui->uris.moony_error, (const LV2_Atom **)&moony_error },
				{ ui->uris.moony_code, (const LV2_Atom **)&moony_code },
				{ 0, NULL}
			};
			lv2_atom_object_query(obj, q);

			if(moony_code)
			{
				const char *str = LV2_ATOM_BODY_CONST(&moony_code->atom);

				enc.data = ui;
				lua_to_markup(str, NULL);
				elm_entry_cursor_pos_set(ui->entry, 0);
			}
			else if(moony_error)
			{
				const char *str = LV2_ATOM_BODY_CONST(&moony_error->atom);

				elm_object_text_set(ui->error, str);
				evas_object_show(ui->message);
			}
		}
	}
}

const LV2UI_Descriptor common_eo = {
	.URI						= MOONY_COMMON_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_eo_extension_data
};

const LV2UI_Descriptor common_ui = {
	.URI						= MOONY_COMMON_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_ui_extension_data
};

const LV2UI_Descriptor common_x11 = {
	.URI						= MOONY_COMMON_X11_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_x11_extension_data
};

const LV2UI_Descriptor common_kx = {
	.URI						= MOONY_COMMON_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= eoui_kx_extension_data
};
