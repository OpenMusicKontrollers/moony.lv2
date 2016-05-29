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

#include <common_ui.h>
#include <encoder.h>

typedef struct _UI UI;

struct _UI {
	struct {
		LV2_URID moony_code;
		LV2_URID moony_selection;
		LV2_URID moony_error;
		LV2_URID moony_trace;
		LV2_URID event_transfer;
		patch_t patch;
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
	Evas_Object *widget;
	Evas_Object *table;
	Evas_Object *error;
	Evas_Object *message;
	Evas_Object *entry;
	Evas_Object *notify;
	Evas_Object *load;
	Evas_Object *status;
	Evas_Object *save;
	Evas_Object *compile;
	Evas_Object *line;
	Evas_Object *clrlog;
	Evas_Object *popup;
	Evas_Object *overlay;
	Evas_Object *trace;

	unsigned tracecnt;
	
	char *logo_path;

	char *chunk;
	char *cache;
	uint8_t buf [0x10000];
};

static void
_moony_message_send(UI *ui, LV2_URID key, const char *str, uint32_t size)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
	if(_moony_patch(&ui->uris.patch, &ui->forge, key, str, size))
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
_count_lines_and_chars(UI *ui, int *L, int *C)
{
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

	*L = l;
	*C = c;
}

static inline void
_clear_error(UI *ui)
{
	// clear error
	elm_object_text_set(ui->error, "");
	if(evas_object_visible_get(ui->message))
		evas_object_hide(ui->message);

	if(evas_object_visible_get(ui->notify))
		evas_object_hide(ui->notify);
	evas_object_show(ui->notify);
}

static inline void
_compile_selection(UI *ui)
{
	_clear_error(ui);

	const char *chunk= elm_entry_selection_get(ui->entry);
	char *utf8 = elm_entry_markup_to_utf8(chunk);
	elm_entry_select_none(ui->entry);

	if(!utf8)
		return;

	uint32_t size = strlen(utf8) + 1;

	int l = 0;
	int c = 0;
	_count_lines_and_chars(ui, &l, &c);
	size += l;

	char *sel = malloc(size);
	if(!sel)
		return;

	memset(sel, '\n', l);
	strcpy(&sel[l], utf8);

	if(size <= MOONY_MAX_CHUNK_LEN)
	{
		_moony_message_send(ui, ui->uris.moony_selection, sel, size);
	}
	else
	{
		char buf [64];
		sprintf(buf, "script too long by %"PRIi32, size - MOONY_MAX_CHUNK_LEN);
		elm_object_text_set(ui->error, buf);
		evas_object_show(ui->message);
	}

	free(sel);
	free(utf8);
}

static inline void
_compile_all(UI *ui)
{
	_clear_error(ui);

	// get code from entry
	char *utf8 = _entry_cache(ui, 1);
	if(utf8)
	{
		uint32_t size = strlen(utf8) + 1;

		if(size <= MOONY_MAX_CHUNK_LEN)
		{
			_moony_message_send(ui, ui->uris.moony_code, utf8, size);
		}
		else
		{
			char buf [64];
			sprintf(buf, "script too long by %"PRIi32, size - MOONY_MAX_CHUNK_LEN);
			elm_object_text_set(ui->error, buf);
			evas_object_show(ui->message);
		}
	}
}

static void
_compile(UI *ui)
{
	const char *sel = elm_entry_selection_get(ui->entry);

	if(sel) //FIXME is never reached if called via _lua_makrup, as selection is replaced by a new line
		_compile_selection(ui);
	else
		_compile_all(ui);
}

static void
_compile_line(UI *ui)
{
	const int pos = elm_entry_cursor_pos_get(ui->entry);

	elm_entry_cursor_line_begin_set(ui->entry);
	elm_entry_cursor_selection_begin(ui->entry);

	elm_entry_cursor_line_end_set(ui->entry);
	elm_entry_cursor_selection_end(ui->entry);

	_compile_selection(ui);

	elm_entry_cursor_pos_set(ui->entry, pos);
}

static void
_clear_log(UI *ui)
{
	ui->tracecnt = 0;
	elm_entry_entry_set(ui->trace, "");
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

			if(evas_key_modifier_is_set(mods, "Control"))
				_compile_line(ui);
			else
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
_compile_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	_compile(ui);
}

static void
_line_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	_compile_line(ui);
}

static void
_clrlog_clicked(void *data, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	_clear_log(ui);
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

	int l = 0;
	int c = 0;
	_count_lines_and_chars(ui, &l, &c);

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

static void
_content_free(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	ui->widget = NULL;
}

static void
_content_del(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	UI *ui = data;

	evas_object_del(ui->widget);
}

static void
_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	UI *ui = data;
	const Evas_Event_Key_Down *ev = event_info;

	const Eina_Bool cntrl = evas_key_modifier_is_set(ev->modifiers, "Control");
	const Eina_Bool shift = evas_key_modifier_is_set(ev->modifiers, "Shift");
	(void)shift;
	
	//printf("_key_down: %s %i %i\n", ev->key, cntrl, shift);

	if(cntrl)
	{
		if(!strcmp(ev->key, "l"))
		{
			_clear_log(ui);
		}
		else if(!strcmp(ev->key, "s"))
		{
			_compile(ui);
		}
	}
}

static Evas_Object *
_content_get(UI *ui, Evas_Object *parent)
{
	ui->table = elm_table_add(parent);
	if(ui->table)
	{
		evas_object_event_callback_add(ui->table, EVAS_CALLBACK_KEY_DOWN, _key_down, ui);

		const Eina_Bool exclusive = EINA_TRUE;
		const Evas_Modifier_Mask ctrl_mask = evas_key_modifier_mask_get(
			evas_object_evas_get(ui->table), "Control");
		const Evas_Modifier_Mask shift_mask = evas_key_modifier_mask_get(
			evas_object_evas_get(ui->table), "Shift");

		if(!evas_object_key_grab(ui->table, "l", ctrl_mask, 0, exclusive))
			fprintf(stderr, "could not grab 'l' key\n");
		if(!evas_object_key_grab(ui->table, "s", ctrl_mask, 0, exclusive))
			fprintf(stderr, "could not grab 's' key\n");
		//TODO add more bindings

		elm_table_homogeneous_set(ui->table, EINA_FALSE);
		elm_table_padding_set(ui->table, 0, 0);
		evas_object_size_hint_min_set(ui->table, 1280, 800);
		evas_object_event_callback_add(ui->table, EVAS_CALLBACK_FREE, _content_free, ui);
		evas_object_event_callback_add(ui->table, EVAS_CALLBACK_DEL, _content_del, ui);

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
			elm_table_pack(ui->table, ui->entry, 0, 0, 7, 1);
		}

		ui->trace = elm_entry_add(ui->table);
		if(ui->trace)
		{
			elm_entry_autosave_set(ui->trace, EINA_FALSE);
			elm_entry_entry_set(ui->trace, "");
			elm_entry_single_line_set(ui->trace, EINA_FALSE);
			elm_entry_scrollable_set(ui->trace, EINA_TRUE);
			elm_entry_editable_set(ui->trace, EINA_FALSE);
			elm_entry_cnp_mode_set(ui->trace, ELM_CNP_MODE_PLAINTEXT);
			elm_object_focus_set(ui->trace, EINA_FALSE);
			evas_object_size_hint_weight_set(ui->trace, EVAS_HINT_EXPAND, 0.1);
			evas_object_size_hint_align_set(ui->trace, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->trace);
			elm_table_pack(ui->table, ui->trace, 0, 1, 7, 1);
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
			elm_table_pack(ui->table, ui->load, 0, 2, 1, 1);

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
			elm_table_pack(ui->table, ui->save, 1, 2, 1, 1);

			Evas_Object *icon = elm_icon_add(ui->save);
			if(icon)
			{
				elm_icon_standard_set(icon, "document-export");
				evas_object_show(icon);
				elm_object_content_set(ui->save, icon);
			}
		}

		ui->clrlog = elm_button_add(ui->table);
		if(ui->clrlog)
		{
			elm_object_part_text_set(ui->clrlog, "default", "Clear Log");
			evas_object_smart_callback_add(ui->clrlog, "clicked", _clrlog_clicked, ui);
			evas_object_size_hint_weight_set(ui->clrlog, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->clrlog, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->clrlog);
			elm_table_pack(ui->table, ui->clrlog, 2, 2, 1, 1);
			elm_object_tooltip_text_set(ui->clrlog, "Control + L");
#if defined(ELM_1_9)
			elm_object_tooltip_orient_set(ui->clrlog, ELM_TOOLTIP_ORIENT_TOP);
#endif

			Evas_Object *icon = elm_icon_add(ui->clrlog);
			if(icon)
			{
				elm_icon_standard_set(icon, "edit-clear");
				evas_object_show(icon);
				elm_object_content_set(ui->clrlog, icon);
			}
		}

		ui->compile = elm_button_add(ui->table);
		if(ui->compile)
		{
			elm_object_part_text_set(ui->compile, "default", "Compile Sel/All");
			evas_object_smart_callback_add(ui->compile, "clicked", _compile_clicked, ui);
			evas_object_size_hint_weight_set(ui->compile, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->compile, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->compile);
			elm_table_pack(ui->table, ui->compile, 3, 2, 1, 1);
			elm_object_tooltip_text_set(ui->compile, "Shift + Enter / Control + S");
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

		ui->line = elm_button_add(ui->table);
		if(ui->line)
		{
			elm_object_part_text_set(ui->line, "default", "Compile Line");
			evas_object_smart_callback_add(ui->line, "clicked", _line_clicked, ui);
			evas_object_size_hint_weight_set(ui->line, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->line, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->line);
			elm_table_pack(ui->table, ui->line, 4, 2, 1, 1);
			elm_object_tooltip_text_set(ui->line, "Shift + Control + Enter");
#if defined(ELM_1_9)
			elm_object_tooltip_orient_set(ui->line, ELM_TOOLTIP_ORIENT_TOP);
#endif

			Evas_Object *icon = elm_icon_add(ui->line);
			if(icon)
			{
				elm_icon_standard_set(icon, "system-run");
				evas_object_show(icon);
				elm_object_content_set(ui->line, icon);
			}
		}

		ui->status = elm_label_add(ui->table);
		if(ui->status)
		{
			elm_object_text_set(ui->status, "");
			evas_object_size_hint_weight_set(ui->status, 0.25, 0.f);
			evas_object_size_hint_align_set(ui->status, EVAS_HINT_FILL, EVAS_HINT_FILL);
			evas_object_show(ui->status);
			elm_table_pack(ui->table, ui->status, 5, 2, 1, 1);
		}

		Evas_Object *info = elm_button_add(ui->table);
		if(info)
		{
			evas_object_smart_callback_add(info, "clicked", _info_clicked, ui);
			evas_object_size_hint_weight_set(info, 0.f, 0.f);
			evas_object_size_hint_align_set(info, 1.f, EVAS_HINT_FILL);
			evas_object_show(info);
			elm_table_pack(ui->table, info, 6, 2, 1, 1);

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
	UI *ui = calloc(1, sizeof(UI));
	if(!ui)
		return NULL;

	Evas_Object *parent = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			ui->port_map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			ui->log = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
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
	if(!parent)
	{
		free(ui);
		return NULL;
	}

	// query port index of "control" port
	ui->control_port = ui->port_map->port_index(ui->port_map->handle, "control");
	ui->notify_port = ui->port_map->port_index(ui->port_map->handle, "notify");

	ui->uris.moony_code = ui->map->map(ui->map->handle, MOONY_CODE_URI);
	ui->uris.moony_selection = ui->map->map(ui->map->handle, MOONY_SELECTION_URI);
	ui->uris.moony_error = ui->map->map(ui->map->handle, MOONY_ERROR_URI);
	ui->uris.moony_trace  = ui->map->map(ui->map->handle, MOONY_TRACE_URI);
	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	ui->uris.patch.self = ui->map->map(ui->map->handle, plugin_uri);
	ui->uris.patch.get = ui->map->map(ui->map->handle, LV2_PATCH__Get);
	ui->uris.patch.set = ui->map->map(ui->map->handle, LV2_PATCH__Set);
	ui->uris.patch.put = ui->map->map(ui->map->handle, LV2_PATCH__Put);
	ui->uris.patch.patch = ui->map->map(ui->map->handle, LV2_PATCH__Patch);
	ui->uris.patch.body = ui->map->map(ui->map->handle, LV2_PATCH__body);
	ui->uris.patch.subject = ui->map->map(ui->map->handle, LV2_PATCH__subject);
	ui->uris.patch.property = ui->map->map(ui->map->handle, LV2_PATCH__property);
	ui->uris.patch.value = ui->map->map(ui->map->handle, LV2_PATCH__value);
	ui->uris.patch.add = ui->map->map(ui->map->handle, LV2_PATCH__add);
	ui->uris.patch.remove = ui->map->map(ui->map->handle, LV2_PATCH__remove);
	ui->uris.patch.wildcard = ui->map->map(ui->map->handle, LV2_PATCH__wildcard);
	ui->uris.patch.writable = ui->map->map(ui->map->handle, LV2_PATCH__writable);
	ui->uris.patch.readable = ui->map->map(ui->map->handle, LV2_PATCH__readable);
	ui->uris.patch.destination = ui->map->map(ui->map->handle, LV2_PATCH__destination);

	lv2_atom_forge_init(&ui->forge, ui->map);
	if(ui->log)
		lv2_log_logger_init(&ui->logger, ui->map, ui->log);

	ui->write_function = write_function;
	ui->controller = controller;

	asprintf(&ui->logo_path, "%s/omk_logo_256x256.png", bundle_path);

	_moony_message_send(ui, ui->uris.moony_code, NULL, 0);
	
	ui->widget = _content_get(ui, parent);
	if(!ui->widget)
	{
		free(ui);
		return NULL;
	}
	*(Evas_Object **)widget = ui->widget;

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	if(ui->widget)
		evas_object_del(ui->widget);

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
		const LV2_Atom_Object *obj = buffer;

		if(  lv2_atom_forge_is_object_type(&ui->forge, obj->atom.type)
			&& (obj->body.otype == ui->uris.patch.set) )
		{
			const LV2_Atom_URID *subject = NULL;
			const LV2_Atom_URID *property = NULL;
			const LV2_Atom_String *value = NULL;
			
			LV2_Atom_Object_Query q[] = {
				{ ui->uris.patch.subject, (const LV2_Atom **)&subject },
				{ ui->uris.patch.property, (const LV2_Atom **)&property },
				{ ui->uris.patch.value, (const LV2_Atom **)&value },
				{ 0, NULL }
			};
			lv2_atom_object_query(obj, q);

			//FIXME check subject

			if(  property && value
				&& (property->atom.type == ui->forge.URID)
				&& (value->atom.type == ui->forge.String) )
			{
				const char *str = LV2_ATOM_BODY_CONST(value);

				if(property->body == ui->uris.moony_code)
				{
					enc.data = ui;
					lua_to_markup(str, NULL);
					elm_entry_cursor_pos_set(ui->entry, 0);
				}
				else if(property->body == ui->uris.moony_error)
				{
					elm_object_text_set(ui->error, str);
					if(evas_object_visible_get(ui->message))
						evas_object_hide(ui->message);
					evas_object_show(ui->message);
				}
				else if(property->body == ui->uris.moony_trace)
				{
					char _tracecnt [64];
					sprintf(_tracecnt, "<font=Mono><color=#AAAAAA>%u</color> ", ui->tracecnt++);
					elm_entry_entry_append(ui->trace, _tracecnt);
					elm_entry_entry_append(ui->trace, str);
					elm_entry_entry_append(ui->trace, "<br></font>");
					elm_entry_cursor_end_set(ui->trace);
				}
			}
		}
	}
}

const LV2UI_Descriptor common_eo = {
	.URI						= MOONY_COMMON_EO_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= NULL 
};
