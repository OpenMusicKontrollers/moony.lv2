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
#include <dirent.h>

#include <moony.h>

#define NK_PUGL_IMPLEMENTATION
#include "nk_pugl/nk_pugl.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern int luaopen_lpeg(lua_State *L);

#ifdef Bool
#	undef Bool // Xlib.h interferes with LV2_Atom_Forge.Bool
#endif

#define RDF_PREFIX    "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDFS_PREFIX   "http://www.w3.org/2000/01/rdf-schema#"

#define RDF__value    RDF_PREFIX"value"
#define RDFS__label   RDFS_PREFIX"label"
#define RDFS__range   RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

#define MAX_PATH_LEN 1024

#if defined(__WIN32)
static char *
_strndup(const char *s, size_t n)
{
	size_t len = strnlen(s, n);
	char *new = (char *) malloc(len + 1);

  if(new == NULL)
		return NULL;

  new[len] = '\0';
  return memcpy(new, s, len);
}
#else
#	define _strndup strndup
#endif

#ifdef __unix__
#include <dirent.h>
#include <unistd.h>
#endif

#ifndef _WIN32
# include <pwd.h>
#endif

#ifdef _WIN32
#	define SLASH_CHAR '\\'
#	define SLASH_STRING "\\"
#else
#	define SLASH_CHAR '/'
#	define SLASH_STRING "/"
#endif

typedef union _body_t body_t;
typedef struct _prop_t prop_t;
typedef enum _browser_type_t browser_type_t;
typedef struct _browser_t browser_t;
typedef struct _plughandle_t plughandle_t;

union _body_t {
	int32_t i;
	int64_t h;
	uint32_t u;
	float f;
	double d;
	struct nk_text_edit editor;
};

struct _prop_t {
	LV2_URID key;
	LV2_URID range;
	char *unit;
	char *label;
	char *comment;
	body_t value;
	body_t minimum;
	body_t maximum;
	LV2_Atom_Tuple *points;
	struct nk_color color;
};

enum _browser_type_t {
	BROWSER_NONE = 0,
	BROWSER_IMPORT,
	BROWSER_EXPORT,
	BROWSER_PROPERTY
};

struct _browser_t {
	char file[MAX_PATH_LEN];
	char home[MAX_PATH_LEN];
	char directory[MAX_PATH_LEN];
	char export[MAX_PATH_LEN];

	char **files;
	char **directories;
	size_t file_count;
	size_t dir_count;
	ssize_t selected;
	int return_hidden;
	int return_lua_only;

	struct {
		struct nk_image home;
		struct nk_image computer;
		struct nk_image directory;
		struct nk_image default_file;
	} icons;
};

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	nk_pugl_window_t win;

	LV2UI_Controller *controller;
	LV2UI_Write_Function writer;

	uint32_t control;
	uint32_t notify;
	LV2_URID atom_eventTransfer;
	LV2_URID patch_self;
	LV2_URID patch_Get;
	LV2_URID patch_Set;
	LV2_URID patch_Put;
	LV2_URID patch_Patch;
	LV2_URID patch_Ack;
	LV2_URID patch_Error;
	LV2_URID patch_writable;
	LV2_URID patch_readable;
	LV2_URID patch_add;
	LV2_URID patch_remove;
	LV2_URID patch_wildcard;
	LV2_URID patch_property;
	LV2_URID patch_body;
	LV2_URID patch_value;
	LV2_URID patch_subject;
	LV2_URID patch_sequenceNumber;
	LV2_URID moony_code;
	LV2_URID moony_selection;
	LV2_URID moony_error;
	LV2_URID moony_trace;
	LV2_URID moony_editorHidden;
	LV2_URID moony_logHidden;
	LV2_URID moony_paramHidden;
	LV2_URID moony_paramCols;
	LV2_URID moony_paramRows;
	LV2_URID rdfs_label;
	LV2_URID rdfs_range;
	LV2_URID rdfs_comment;
	LV2_URID rdf_value;
	LV2_URID lv2_minimum;
	LV2_URID lv2_maximum;
	LV2_URID lv2_scalePoint;
	LV2_URID units_symbol;
	LV2_URID units_unit;
	LV2_URID units_bar;
	LV2_URID units_beat;
	LV2_URID units_bpm;
	LV2_URID units_cent;
	LV2_URID units_cm;
	LV2_URID units_db;
	LV2_URID units_degree;
	LV2_URID units_frame;
	LV2_URID units_hz;
	LV2_URID units_inch;
	LV2_URID units_khz;
	LV2_URID units_km;
	LV2_URID units_m;
	LV2_URID units_mhz;
	LV2_URID units_midiNote;
	LV2_URID units_mile;
	LV2_URID units_min;
	LV2_URID units_mm;
	LV2_URID units_ms;
	LV2_URID units_oct;
	LV2_URID units_pc;
	LV2_URID units_s;
	LV2_URID units_semitone12TET;
	LV2_URID canvas_Style;

	atom_ser_t ser;

	float dy;
	browser_type_t browser_type;
	prop_t *browser_target;

	char code [MOONY_MAX_CHUNK_LEN];
	struct nk_text_edit editor;
	char *clipboard;

	char error [MOONY_MAX_ERROR_LEN];
	int error_sz;

	int n_trace;
	char **traces;

	int n_writable;
	prop_t *writables;
	int n_readable;
	prop_t *readables;

	lua_State *L;
	
	browser_t browser;
	const char *bundle_path;

	int32_t editor_hidden;
	int32_t log_hidden;
	int32_t param_hidden;
	int32_t param_rows;
	int32_t param_cols;
};

static void
_patch_set_code(plughandle_t *handle, uint32_t size, const char *body);

static uint8_t *
file_load(const char *path, size_t *siz)
{
	FILE *fd = fopen(path, "rb");
	if(!fd)
		return NULL;

	fseek(fd, 0, SEEK_END);
	*siz = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	uint8_t *buf = calloc(*siz, 1);
	if(buf)
		fread(buf, *siz, 1, fd);

	fclose(fd);
	return buf;
}

static void
dir_free_list(char **list, size_t size)
{
	size_t i;
	for(i = 0; i < size; i++)
		free(list[i]);
	free(list);
}

static int
_cmp(const void *a, const void *b)
{
	const char *const *A = a;
	const char *const *B = b;
	return strcasecmp(*A, *B);
}

static char**
dir_list(const char *dir, int return_subdirs, int return_hidden,
	int return_lua_only, size_t *count)
{
	size_t n = 0;
	char buffer[MAX_PATH_LEN];
	char **results = NULL;
	size_t capacity = 32;
	size_t size;
	DIR *z;
	
	assert(dir);
	assert(count);
	strncpy(buffer, dir, MAX_PATH_LEN);
	n = strlen(buffer);
	
	if(n > 0 && (buffer[n-1] != SLASH_CHAR))
		buffer[n++] = SLASH_CHAR;
	
	size = 0;
	
	z = opendir(dir);
	if(z != NULL)
	{
		struct dirent *data = readdir(z);
		if(data == NULL)
			return NULL;
		
		do {
			DIR *y;
			char *p;
			int is_subdir;
			if(  (data->d_name[0] == '.')
				&& (!return_hidden || ( (data->d_name[1] == '\0') || (data->d_name[1] == '.'))) )
				continue;

			if(return_lua_only)
			{
				char *point = strchr(data->d_name, '.');
				if(!point) // no suffix
					continue;

				if(strcmp(point, ".lua")) // no *.lua suffix
					continue;
			}
			
			strncpy(buffer + n, data->d_name, MAX_PATH_LEN-n);
			y = opendir(buffer);
			is_subdir = (y != NULL);
			if (y != NULL)
				closedir(y);
			
			if((return_subdirs && is_subdir) || (!is_subdir && !return_subdirs))
			{
				if(!size)
				{
					results = (char**)calloc(sizeof(char*), capacity);
				}
				else if(size >= capacity)
				{
					void *old = results;
					capacity = capacity * 2;
					results = (char**)realloc(results, capacity * sizeof(char*));
					assert(results);
					if(!results)
						free(old);
				}
				p = strdup(data->d_name);
				results[size++] = p;
			}
		} while ((data = readdir(z)) != NULL);
		
		qsort(results, size, sizeof(char *), _cmp);
	}
	
	if(z)
		closedir(z);
	*count = size;
	return results;
}

static void
file_browser_reload_directory_content(browser_t *browser, const char *path)
{
	strncpy(browser->directory, path, MAX_PATH_LEN);
	dir_free_list(browser->files, browser->file_count);
	dir_free_list(browser->directories, browser->dir_count);
	browser->files = dir_list(path, 0, browser->return_hidden, browser->return_lua_only, &browser->file_count);
	browser->directories = dir_list(path, 1, browser->return_hidden, 0, &browser->dir_count);
	browser->selected = -1;
}

static void
file_browser_init(browser_t *browser, int return_hidden, int return_lua_only,
	struct nk_image (*icon_load)(void *data, const char *filename), void *data)
{
	memset(browser, 0, sizeof(*browser));
	
	const char *home = getenv("HOME");
#ifdef _WIN32
	if (!home) home = getenv("USERPROFILE");
#else
	if (!home) home = getpwuid(getuid())->pw_dir;
#endif

	size_t l;
	strncpy(browser->home, home, MAX_PATH_LEN);
	l = strlen(browser->home);
	strcpy(browser->home + l, SLASH_STRING);
	strcpy(browser->directory, browser->home);
	
	browser->selected = -1;
	browser->return_hidden = return_hidden;
	browser->return_lua_only = return_lua_only;

	if(icon_load)
	{
		browser->icons.home = icon_load(data, "home.png");
		browser->icons.directory = icon_load(data, "directory.png");
		browser->icons.computer = icon_load(data, "computer.png");
		browser->icons.default_file = icon_load(data, "default.png");
	}
}

static void
file_browser_free(browser_t *browser,
	void (*icon_unload)(void *data, struct nk_image img), void *data)
{
	if(icon_unload)
	{
		icon_unload(data, browser->icons.home);
		icon_unload(data, browser->icons.directory);
		icon_unload(data, browser->icons.computer);
		icon_unload(data, browser->icons.default_file);
	}

	if(browser->files)
		dir_free_list(browser->files, browser->file_count);
	browser->files = NULL;

	if(browser->directories)
		dir_free_list(browser->directories, browser->dir_count);
	browser->directories = NULL;

	memset(browser, 0, sizeof(*browser));
}

static int
file_browser_run(browser_t *browser, struct nk_context *ctx, float dy,
	const char *title, browser_type_t browser_type, struct nk_rect bounds)
{
	int ret = 0;

	// delayed initial loading
	if(!browser->files)
		browser->files = dir_list(browser->directory, 0, browser->return_hidden, browser->return_lua_only, &browser->file_count);
	if(!browser->directories)
		browser->directories = dir_list(browser->directory, 1, browser->return_hidden, 0, &browser->dir_count);
	
	if(nk_begin(ctx, title, bounds,
		NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_CLOSABLE))
	{
		static float ratio[] = {0.75f, 0.25f};
		float spacing_x = ctx->style.window.spacing.x;
		
		/* output path directory selector in the menubar */
		ctx->style.window.spacing.x = 0;
		nk_menubar_begin(ctx);
		{
			char *d = browser->directory;
			char *begin = d + 1;
			nk_layout_row_dynamic(ctx, dy, 6);
			while(*d++)
			{
				if(*d == SLASH_CHAR)
				{
					*d = '\0';
					if(nk_button_label(ctx, begin))
					{
						*d++ = SLASH_CHAR; *d = '\0';
						file_browser_reload_directory_content(browser, browser->directory);
						break;
					}
					*d = SLASH_CHAR;
					begin = d + 1;
				}
			}

			nk_layout_row_dynamic(ctx, dy, 1);
			nk_label(ctx, browser->directory, NK_TEXT_LEFT);
			if(browser_type == BROWSER_EXPORT)
			{
				nk_flags flags = NK_EDIT_FIELD | NK_EDIT_SIG_ENTER;
				const nk_flags state = nk_edit_string_zero_terminated(ctx, flags,
					browser->export, MAX_PATH_LEN, nk_filter_ascii);
				if( (state & NK_EDIT_COMMITED) && (strlen(browser->export) > 0) )
				{
					strncpy(browser->file, browser->directory, MAX_PATH_LEN);
					const size_t n = strlen(browser->file);
					strncpy(browser->file + n, browser->export, MAX_PATH_LEN - n);
					ret = 1;
				}
			}
		}
		nk_menubar_end(ctx);
		ctx->style.window.spacing.x = spacing_x;
		
		/* window layout */
		struct nk_rect total_space = nk_window_get_content_region(ctx);
		nk_layout_row(ctx, NK_DYNAMIC, total_space.h, 2, ratio);
		
		/* output directory content window */
		nk_group_begin(ctx, "Content", 0);
		{
			ssize_t count = browser->dir_count + browser->file_count;
			
			nk_layout_row_dynamic(ctx, dy, 1);
			for(ssize_t j = 0; j < count; j++)
			{
				if(j < (ssize_t)browser->dir_count)
				{
					const int selected = (browser->selected == j);
					if(nk_select_image_label(ctx, browser->icons.directory, browser->directories[j], NK_TEXT_LEFT, selected) != selected)
					{
						browser->selected = j;
					}
				}
				else
				{
					const int k = j - browser->dir_count;
					const int selected = (browser->selected == j);
					if(nk_select_image_label(ctx, browser->icons.default_file, browser->files[k], NK_TEXT_LEFT, selected) != selected)
					{
						browser->selected = j;
						strncpy(browser->export, browser->files[k], MAX_PATH_LEN);
					}
				}
			}
			
			nk_group_end(ctx);
		}

		/* special buttons */
		nk_group_begin(ctx, "Special", NK_WINDOW_NO_SCROLLBAR);
		{
			nk_layout_row_dynamic(ctx, 40, 1);
			if(nk_button_image_label(ctx, browser->icons.home, "Home", NK_TEXT_RIGHT))
				file_browser_reload_directory_content(browser, browser->home);
			if(nk_button_image_label(ctx, browser->icons.computer, "Computer", NK_TEXT_RIGHT))
#ifdef _WIN32
				file_browser_reload_directory_content(browser, "C:\\");
#else
				file_browser_reload_directory_content(browser, SLASH_STRING);
#endif

			if(nk_checkbox_label(ctx, "Show hidden", &browser->return_hidden))
				file_browser_reload_directory_content(browser, browser->directory);

			if(nk_checkbox_label(ctx, "Show *.lua only", &browser->return_lua_only))
				file_browser_reload_directory_content(browser, browser->directory);

			if(nk_button_label(ctx, "Select"))
			{
				if( (browser_type == BROWSER_EXPORT) && (strlen(browser->export) > 0) )
				{
					//FIXME remove duplicate code
					strncpy(browser->file, browser->directory, MAX_PATH_LEN);
					const size_t n = strlen(browser->file);
					strncpy(browser->file + n, browser->export, MAX_PATH_LEN - n);
					ret = 1;
				}
				else if(browser->selected >= (ssize_t)browser->dir_count)
				{
					const int k = browser->selected - browser->dir_count;
					strncpy(browser->file, browser->directory, MAX_PATH_LEN);
					const size_t n = strlen(browser->file);
					strncpy(browser->file + n, browser->files[k], MAX_PATH_LEN - n);
					ret = 1;
				}
				else if(browser->selected >= 0)
				{
					const int j = browser->selected;
					size_t n = strlen(browser->directory);
					strncpy(browser->directory + n, browser->directories[j], MAX_PATH_LEN - n);
					n = strlen(browser->directory);
					if(n < MAX_PATH_LEN - 1)
					{
						browser->directory[n] = SLASH_CHAR;
						browser->directory[n+1] = '\0';
					}
					file_browser_reload_directory_content(browser, browser->directory);
				}
			}

			nk_group_end(ctx);
		}
	}
	nk_end(ctx);

	return ret;
}

static inline void
_qsort(prop_t *a, unsigned n, LV2_URID_Unmap *unmap)
{
	if(n < 2)
		return;

	const prop_t *p = &a[n/2];

	unsigned i, j;
	for(i=0, j=n-1; ; i++, j--)
	{
		while(strcmp(unmap->unmap(unmap->handle, a[i].key), unmap->unmap(unmap->handle, p->key)) < 0)
			i++;

		while(strcmp(unmap->unmap(unmap->handle, p->key), unmap->unmap(unmap->handle, a[j].key)) < 0)
			j--;

		if(i >= j)
			break;

		const prop_t t = a[i];
		a[i] = a[j];
		a[j] = t;
	}

	_qsort(a, i, unmap);
	_qsort(&a[i], n - i, unmap);
}

static prop_t *
_prop_get(prop_t **properties, int *n_properties, LV2_URID key)
{
	// we cannot easily speed this up, as properties are ordered according to URI, not URID
	for(int i = 0; i < *n_properties; i++)
	{
		prop_t *prop = &(*properties)[i];

		if(prop->key == key)
			return prop;
	}

	return NULL;
}

static prop_t *
_prop_get_or_add(plughandle_t *handle, prop_t **properties, int *n_properties, LV2_URID key)
{
	prop_t *prop = _prop_get(properties, n_properties, key);
	if(prop)
		return prop;

	*properties = realloc(*properties, (*n_properties + 1)*sizeof(prop_t));
	prop = &(*properties)[*n_properties];
	memset(prop, 0x0, sizeof(prop_t));
	prop->key = key;
	prop->color = nk_white;
	*n_properties += 1;

	// sort properties according to URI string comparison
	_qsort(*properties, *n_properties, handle->unmap);

	return _prop_get(properties, n_properties, key);
}

static void
_prop_free(plughandle_t *handle, prop_t *prop)
{
	if(prop->label)
		free(prop->label);

	if(prop->comment)
		free(prop->comment);

	if(prop->unit)
		free(prop->unit);

	if(  (prop->range == handle->forge.String)
		|| (prop->range == handle->forge.URID) )
	{
		nk_textedit_free(&prop->value.editor);
	}

	if(prop->points)
		free(prop->points);
}

LV2_Atom_Forge_Ref
_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
{
	atom_ser_t *ser = handle;

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;

	const uint32_t new_offset = ser->offset + size;
	if(new_offset > ser->size)
	{
		uint32_t new_size = ser->size << 1;
		while(new_offset > new_size)
			new_size <<= 1;

		if(!(ser->buf = realloc(ser->buf, new_size)))
			return 0; // realloc failed

		ser->size = new_size;
	}

	memcpy(ser->buf + ser->offset, buf, size);
	ser->offset = new_offset;

	return ref;
}

LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

static void
_patch_ack(plughandle_t *handle, int32_t sequence_number)
{
	if(!sequence_number)
		return;

	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Ack);

	lv2_atom_forge_key(forge, handle->patch_subject);
	lv2_atom_forge_urid(forge, handle->patch_self);

	lv2_atom_forge_key(forge, handle->patch_sequenceNumber);
	lv2_atom_forge_int(forge, sequence_number);

	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_patch_error(plughandle_t *handle, int32_t sequence_number)
{
	if(!sequence_number)
		return;

	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Error);

	lv2_atom_forge_key(forge, handle->patch_subject);
	lv2_atom_forge_urid(forge, handle->patch_self);

	lv2_atom_forge_key(forge, handle->patch_sequenceNumber);
	lv2_atom_forge_int(forge, sequence_number);

	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_patch_get(plughandle_t *handle, LV2_URID property)
{
	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Get);
	if(property)
	{
		lv2_atom_forge_key(forge, handle->patch_subject);
		lv2_atom_forge_urid(forge, handle->patch_self);

		lv2_atom_forge_key(forge, handle->patch_sequenceNumber);
		lv2_atom_forge_int(forge, 0);

		lv2_atom_forge_key(forge, handle->patch_property);
		lv2_atom_forge_urid(forge, property);
	}
	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_patch_set(plughandle_t *handle, LV2_URID property, uint32_t size, LV2_URID type,
	const void *body)
{
	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Set);

	lv2_atom_forge_key(forge, handle->patch_subject);
	lv2_atom_forge_urid(forge, handle->patch_self);

	lv2_atom_forge_key(forge, handle->patch_sequenceNumber);
	lv2_atom_forge_int(forge, 0);

	lv2_atom_forge_key(forge, handle->patch_property);
	lv2_atom_forge_urid(forge, property);

	lv2_atom_forge_key(forge, handle->patch_value);
	if(type == forge->String)
	{
		lv2_atom_forge_string(forge, body, size);
	}
	else // !String
	{
		lv2_atom_forge_atom(forge, size, type);
		lv2_atom_forge_write(forge, body, size);
	}

	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_clear_error(plughandle_t *handle)
{
	handle->error[0] = '\0';
}

static void
_submit_selection(plughandle_t *handle, const char *sel, uint32_t sz)
{
	_clear_error(handle);

	_patch_set(handle, handle->moony_selection,
		sz, handle->forge.String, sel);
}

static void
_submit_all(plughandle_t *handle)
{
	_clear_error(handle);

	struct nk_str *str = &handle->editor.string;
	_patch_set(handle, handle->moony_code,
		nk_str_len_char(str), handle->forge.String, nk_str_get_const(str));
}

static void
_submit_line(plughandle_t *handle)
{
	uint32_t newlines = 0;
	uint32_t from = 0;
	for(int i = 0; i < handle->editor.cursor; i++) // count newlines
	{
		if(handle->code[i] == '\n')
		{
			newlines += 1;
			from = i + 1;
		}
	}

	// create selection with prefixed newlines
	struct nk_str *str = &handle->editor.string;
	const char *code = nk_str_get_const(str);
	const int code_sz  = nk_str_len_char(str);
	char *end = memchr(code + from, '\n', code_sz);
	const uint32_t to = end ? end - code : code_sz;
	const uint32_t len = to - from;
	if(len > 0)
	{
		const uint32_t sz = newlines + len + 1;
		char *sel = calloc(sz, sizeof(char));
		if(sel)
		{
			memset(sel, '\n', newlines);
			memcpy(sel + newlines, code + from, len);

			_submit_selection(handle, sel, sz);

			free(sel);
		}
	}
}

static void
_submit_sel(plughandle_t *handle)
{
	struct nk_str *str = &handle->editor.string;
	const char *code = nk_str_get_const(str);

	const uint32_t len = handle->editor.select_end - handle->editor.select_start;
	if(len > 0)
	{
		uint32_t newlines = 0;
		for(int i = 0; i < handle->editor.select_start; i++) // count newlines
		{
			if(code[i] == '\n')
				newlines += 1;
		}

		const uint32_t sz = newlines + len + 1;
		char *sel = calloc(sz, sizeof(char));
		if(sel)
		{
			memset(sel, '\n', newlines);
			memcpy(sel + newlines, code + handle->editor.select_start, len);

			_submit_selection(handle, sel, sz);

			free(sel);
		}
	}
}

static void
_submit_line_or_sel(plughandle_t *handle)
{
	if(handle->editor.select_end == handle->editor.select_start)
		_submit_line(handle);
	else
		_submit_sel(handle);
}

static void
_clear_log(plughandle_t *handle)
{
	for(int i = 0; i < handle->n_trace; i++)
		free(handle->traces[i]);
	free(handle->traces);

	handle->n_trace = 0;
	handle->traces = NULL;

	nk_pugl_post_redisplay(&handle->win);
}

static int
_dial_bool(struct nk_context *ctx, int32_t *val, struct nk_color color, bool editable)
{
	const int32_t tmp = *val;
	struct nk_rect bounds = nk_layout_space_bounds(ctx);
	const bool left_mouse_click_in_cursor = nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT);
	const enum nk_widget_layout_states layout_states = nk_widget(&bounds, ctx);

	if(layout_states != NK_WIDGET_INVALID)
	{
		enum nk_widget_states states = NK_WIDGET_STATE_INACTIVE;

		if(editable && (layout_states == NK_WIDGET_VALID) )
		{
			struct nk_input *in = &ctx->input;
			bool mouse_has_scrolled = false;

			if(left_mouse_click_in_cursor)
			{
				states = NK_WIDGET_STATE_ACTIVED;
			}
			else if(nk_input_is_mouse_hovering_rect(in, bounds))
			{
				if(in->mouse.scroll_delta != 0.f) // has scrolling
				{
					mouse_has_scrolled = true;
					in->mouse.scroll_delta = 0.f;
				}

				states = NK_WIDGET_STATE_HOVER;
			}

			if(left_mouse_click_in_cursor || mouse_has_scrolled)
			{
				*val = !*val;
			}
		}

		const struct nk_style_item *fg = NULL;
		const struct nk_style_item *bg = NULL;

		switch(states)
		{
			case NK_WIDGET_STATE_HOVER:
			{
				bg = &ctx->style.progress.hover;
				fg = &ctx->style.progress.cursor_hover;
			}	break;
			case NK_WIDGET_STATE_ACTIVED:
			{
				bg = &ctx->style.progress.active;
				fg = &ctx->style.progress.cursor_active;
			}	break;
			default:
			{
				bg = &ctx->style.progress.normal;
				fg = &ctx->style.progress.cursor_normal;
			}	break;
		}

		const struct nk_color bg_color = bg->data.color;
		struct nk_color fg_color = fg->data.color;

		fg_color.r = (int)fg_color.r * color.r / 0xff;
		fg_color.g = (int)fg_color.g * color.g / 0xff;
		fg_color.b = (int)fg_color.b * color.b / 0xff;
		fg_color.a = (int)fg_color.a * color.a / 0xff;

		struct nk_command_buffer *canv= nk_window_get_canvas(ctx);
		const float w2 = bounds.w/2;
		const float h2 = bounds.h/2;
		const float r1 = h2;
		const float r2 = r1 / 2;
		const float cx = bounds.x + w2;
		const float cy = bounds.y + h2;

		nk_fill_arc(canv, cx, cy, r2, 0.f, 2*M_PI, fg_color);
		nk_fill_arc(canv, cx, cy, r2 - 2, 0.f, 2*M_PI, ctx->style.window.background);
		nk_fill_arc(canv, cx, cy, r2 - 4, 0.f, 2*M_PI,
			*val ? fg_color : bg_color);
	}

	return tmp != *val;
}

static float
_dial_numeric_behavior(struct nk_context *ctx, struct nk_rect bounds,
	enum nk_widget_states *states, int *divider)
{
	struct nk_input *in = &ctx->input;

	const struct nk_mouse_button *btn = &in->mouse.buttons[NK_BUTTON_LEFT];;
	const bool left_mouse_down = btn->down;
	const bool left_mouse_click_in_cursor = nk_input_has_mouse_click_down_in_rect(in,
		NK_BUTTON_LEFT, bounds, nk_true);

	float dd = 0.f;
	if(left_mouse_down && left_mouse_click_in_cursor)
	{
		const float dx = in->mouse.delta.x;
		const float dy = in->mouse.delta.y;
		dd = fabs(dx) > fabs(dy) ? dx : -dy;

		*states = NK_WIDGET_STATE_ACTIVED;
	}
	else if(nk_input_is_mouse_hovering_rect(in, bounds))
	{
		if(in->mouse.scroll_delta != 0.f) // has scrolling
		{
			dd = in->mouse.scroll_delta;
			in->mouse.scroll_delta = 0.f;
		}

		*states = NK_WIDGET_STATE_HOVER;
	}

	if(nk_input_is_key_down(in, NK_KEY_CTRL))
		*divider *= 4;
	if(nk_input_is_key_down(in, NK_KEY_SHIFT))
		*divider *= 4;

	return dd;
}

static void
_dial_numeric_draw(struct nk_context *ctx, struct nk_rect bounds,
	enum nk_widget_states states, float perc, struct nk_color color)
{
	struct nk_command_buffer *canv= nk_window_get_canvas(ctx);
	const struct nk_style_item *bg = NULL;
	const struct nk_style_item *fg = NULL;

	switch(states)
	{
		case NK_WIDGET_STATE_HOVER:
		{
			bg = &ctx->style.progress.hover;
			fg = &ctx->style.progress.cursor_hover;
		}	break;
		case NK_WIDGET_STATE_ACTIVED:
		{
			bg = &ctx->style.progress.active;
			fg = &ctx->style.progress.cursor_active;
		}	break;
		default:
		{
			bg = &ctx->style.progress.normal;
			fg = &ctx->style.progress.cursor_normal;
		}	break;
	}

	const struct nk_color bg_color = bg->data.color;
	struct nk_color fg_color = fg->data.color;

	fg_color.r = (int)fg_color.r * color.r / 0xff;
	fg_color.g = (int)fg_color.g * color.g / 0xff;
	fg_color.b = (int)fg_color.b * color.b / 0xff;
	fg_color.a = (int)fg_color.a * color.a / 0xff;

	const float w2 = bounds.w/2;
	const float h2 = bounds.h/2;
	const float r1 = h2;
	const float r2 = r1 / 2;
	const float cx = bounds.x + w2;
	const float cy = bounds.y + h2;
	const float aa = M_PI/6;
	const float a1 = M_PI/2 + aa;
	const float a2 = 2*M_PI + M_PI/2 - aa;
	const float a3 = a1 + (a2 - a1)*perc;

	nk_fill_arc(canv, cx, cy, r1, a1, a2, bg_color);
	nk_fill_arc(canv, cx, cy, r1, a1, a3, fg_color);
	nk_fill_arc(canv, cx, cy, r2, 0.f, 2*M_PI, ctx->style.window.background);
}

static int
_dial_double(struct nk_context *ctx, double min, double *val, double max, float mul,
	struct nk_color color, bool editable)
{
	const double tmp = *val;
	struct nk_rect bounds = nk_layout_space_bounds(ctx);
	const enum nk_widget_layout_states layout_states = nk_widget(&bounds, ctx);

	if(layout_states != NK_WIDGET_INVALID)
	{
		enum nk_widget_states states = NK_WIDGET_STATE_INACTIVE;
		const double range = max - min;

		if(editable && (layout_states == NK_WIDGET_VALID) )
		{
			int divider = 1;
			const float dd = _dial_numeric_behavior(ctx, bounds, &states, &divider);

			if(dd != 0.f) // update value
			{
				const double per_pixel_inc = mul * range / bounds.w / divider;

				*val += dd * per_pixel_inc;
				*val = NK_CLAMP(min, *val, max);
			}
		}

		const float perc = (*val - min) / range;
		_dial_numeric_draw(ctx, bounds, states, perc, color);
	}

	return tmp != *val;
}

static int
_dial_long(struct nk_context *ctx, int64_t min, int64_t *val, int64_t max, float mul,
	struct nk_color color, bool editable)
{
	const int64_t tmp = *val;
	struct nk_rect bounds = nk_layout_space_bounds(ctx);
	const enum nk_widget_layout_states layout_states = nk_widget(&bounds, ctx);

	if(layout_states != NK_WIDGET_INVALID)
	{
		enum nk_widget_states states = NK_WIDGET_STATE_INACTIVE;
		const int64_t range = max - min;

		if(editable && (layout_states == NK_WIDGET_VALID) )
		{
			int divider = 1;
			const float dd = _dial_numeric_behavior(ctx, bounds, &states, &divider);

			if(dd != 0.f) // update value
			{
				const double per_pixel_inc = mul * range / bounds.w / divider;

				const double diff = dd * per_pixel_inc;
				*val += diff < 0.0 ? floor(diff) : ceil(diff);
				*val = NK_CLAMP(min, *val, max);
			}
		}

		const float perc = (float)(*val - min) / range;
		_dial_numeric_draw(ctx, bounds, states, perc, color);
	}

	return tmp != *val;
}

static int
_dial_float(struct nk_context *ctx, float min, float *val, float max, float mul,
	struct nk_color color, bool editable)
{
	double tmp = *val;
	const int res = _dial_double(ctx, min, &tmp, max, mul, color, editable);
	*val = tmp;

	return res;
}

static int
_dial_int(struct nk_context *ctx, int32_t min, int32_t *val, int32_t max, float mul,
	struct nk_color color, bool editable)
{
	int64_t tmp = *val;
	const int res = _dial_long(ctx, min, &tmp, max, mul, color, editable);
	*val = tmp;

	return res;
}

static struct nk_token *
_lex(void *data, const char *code, int code_sz)
{
	lua_State *L = data;
	struct nk_token *tokens = NULL;
	const int top = lua_gettop(L);

	//printf("_lex: %i\n", code_sz);

	lua_getglobal(L, "lexer");
	lua_getfield(L, -1, "lex");
	lua_getglobal(L, "moony");
	lua_pushlstring(L, code, code_sz);
	if(lua_pcall(L, 2, 1, 0))
	{
		fprintf(stderr, "err: %s\n", lua_tostring(L, -1));
	}
	else
	{
		const int len = luaL_len(L, -1);

		tokens = calloc(len/2 + 1, sizeof(struct nk_token)); //FIXME use realloc
		if(tokens)
		{
			for(int i = 0; i < len; i += 2)
			{
				struct nk_token *token = &tokens[i/2];

				lua_rawgeti(L, -1, i + 1);
				const char *token_str = luaL_checkstring(L, -1);
				lua_pop(L, 1);

				//TODO cache colors
				if(!strcmp(token_str, "whitespace"))
					token->color = nk_rgb(0xdd, 0xdd, 0xdd);
				else if(!strcmp(token_str, "keyword"))
					token->color = nk_rgb(0x00, 0x69, 0x8f);
				else if(!strcmp(token_str, "function"))
					token->color = nk_rgb(0x00, 0xae, 0xef);
				else if(!strcmp(token_str, "constant"))
					token->color = nk_rgb(0xff, 0x66, 0x00);
				else if(!strcmp(token_str, "library"))
					token->color = nk_rgb(0x8d, 0xff, 0x0a);
				else if(!strcmp(token_str, "table"))
					token->color = nk_rgb(0x8d, 0xff, 0x0a);
				else if(!strcmp(token_str, "identifier"))
					token->color = nk_rgb(0xdd, 0xdd, 0xdd);
				else if(!strcmp(token_str, "string"))
					token->color = nk_rgb(0x58, 0xc5, 0x54);
				else if(!strcmp(token_str, "comment"))
					token->color = nk_rgb(0x55, 0x55, 0x55);
				else if(!strcmp(token_str, "number"))
					token->color = nk_rgb(0xfb, 0xfb, 0x00);
				else if(!strcmp(token_str, "label"))
					token->color = nk_rgb(0xfd, 0xc2, 0x51);
				else if(!strcmp(token_str, "binop"))
					token->color = nk_rgb(0xcc, 0x00, 0x00);
				else if(!strcmp(token_str, "operator"))
					token->color = nk_rgb(0xcc, 0x00, 0x00);
				else if(!strcmp(token_str, "brace"))
					token->color = nk_rgb(0xff, 0xff, 0xff);
				else
					token->color = nk_rgb(0xdd, 0xdd, 0xdd);

				lua_rawgeti(L, -1, i + 2);
				const int token_offset = luaL_checkinteger(L, -1);
				lua_pop(L, 1);

				token->offset = token_offset - 1;
			}

			struct nk_token *token = &tokens[len/2];
			token->offset = INT32_MAX;
		}
	}

	lua_settop(L, top);
	return tokens;
}

static void
_paste(nk_handle userdata, struct nk_text_edit* editor)
{
	plughandle_t *handle = userdata.ptr;

	if(handle->clipboard)
		nk_textedit_paste(editor, handle->clipboard, strlen(handle->clipboard));
}

static void
_copy(nk_handle userdata, const char *buf, int len)
{
	plughandle_t *handle = userdata.ptr;

	if(handle->clipboard)
		free(handle->clipboard);

	handle->clipboard = _strndup(buf, len);
}

static bool
_tooltip_visible(struct nk_context *ctx)
{
	return nk_widget_has_mouse_click_down(ctx, NK_BUTTON_RIGHT, nk_true)
		|| (nk_widget_is_hovered(ctx) && nk_input_is_key_down(&ctx->input, NK_KEY_CTRL));
}

static void
_select_draw_end(struct nk_command_buffer *canv, nk_handle userdata)
{
	struct nk_context *ctx = userdata.ptr;
	struct nk_rect bounds = nk_layout_space_bounds(ctx);

	const float x0 = bounds.x;
	const float x1 = bounds.x + bounds.w;
	const float y = bounds.y + bounds.h;
	nk_stroke_line(canv, x0, y, x1, y, ctx->style.button.border*2, ctx->style.button.border_color);
}

const char *lab = "#"; //FIXME

static void
_parameter_widget_enum(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	const char *header = NULL;
	float diff = HUGE_VAL;
	LV2_ATOM_TUPLE_FOREACH(prop->points, itm)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)itm;

		if(!lv2_atom_forge_is_object_type(&handle->forge, obj->atom.type))
			continue;

		const LV2_Atom *label = NULL;
		const LV2_Atom *value = NULL;

		lv2_atom_object_get(obj,
			handle->rdfs_label, &label,
			handle->rdf_value, &value,
			0);

		if(label && (label->type == handle->forge.String)
			&& value && (value->type == prop->range))
		{
			float d = HUGE_VAL;

			if(value->type == handle->forge.Int)
				d = abs(((const LV2_Atom_Int *)value)->body - prop->value.i);
			else if(value->type == handle->forge.Bool)
				d = abs(((const LV2_Atom_Bool *)value)->body - prop->value.i);
			else if(value->type == handle->forge.Long)
				d = labs(((const LV2_Atom_Long *)value)->body - prop->value.h);
			else if(value->type == handle->forge.Float)
				d = fabs(((const LV2_Atom_Float *)value)->body - prop->value.f);
			else if(value->type == handle->forge.Double)
				d = fabs(((const LV2_Atom_Double *)value)->body - prop->value.d);

			if(d < diff)
			{
				header = LV2_ATOM_BODY(label);
				diff = d;
			}
		}
	}

	nk_layout_row_dynamic(ctx, dy, 1);
	if(header && nk_combo_begin_label(ctx, header, nk_vec2(nk_widget_width(ctx), 7*dy)))
	{
		nk_layout_row_dynamic(ctx, dy, 1);
		LV2_ATOM_TUPLE_FOREACH(prop->points, itm)
		{
			const LV2_Atom_Object *obj = (const LV2_Atom_Object *)itm;

			if(!lv2_atom_forge_is_object_type(&handle->forge, obj->atom.type))
				continue;

			const LV2_Atom *label = NULL;
			const LV2_Atom *value = NULL;

			lv2_atom_object_get(obj,
				handle->rdfs_label, &label,
				handle->rdf_value, &value,
				0);

			if(label && (label->type == handle->forge.String)
				&& value && (value->type == prop->range))
			{
				if(nk_combo_item_label(ctx, LV2_ATOM_BODY_CONST(label), NK_TEXT_LEFT) && editable)
				{
					if(value->type == handle->forge.Int)
					{
						prop->value.i = ((const LV2_Atom_Int *)value)->body;
						_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &prop->value.i);
					}
					else if (value->type == handle->forge.Bool)
					{
						prop->value.i = ((const LV2_Atom_Bool *)value)->body;
						_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &prop->value.i);
					}
					else if (value->type == handle->forge.Long)
					{
						prop->value.h = ((const LV2_Atom_Long *)value)->body;
						_patch_set(handle, prop->key, sizeof(int64_t), prop->range, &prop->value.h);
					}
					else if (value->type == handle->forge.Float)
					{
						prop->value.f = ((const LV2_Atom_Float *)value)->body;
						_patch_set(handle, prop->key, sizeof(float), prop->range, &prop->value.f);
					}
					else if (value->type == handle->forge.Double)
					{
						prop->value.d = ((const LV2_Atom_Double *)value)->body;
						_patch_set(handle, prop->key, sizeof(double), prop->range, &prop->value.d);
					}
				}
			}
		}

		nk_combo_end(ctx);
	}
}

static void
_parameter_widget_int(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	// draw dial
	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	if(_dial_int(ctx, prop->minimum.i, &prop->value.i, prop->maximum.i, 1.f,
		prop->color, editable))
	{
		_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &prop->value.i);
	}

	nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
	nk_layout_row_push(ctx, 0.9);
	if(editable)
	{
		const int32_t val = nk_propertyi(ctx, lab,
			prop->minimum.i, prop->value.i, prop->maximum.i, 1.f, 0.f);
		if(val != prop->value.i)
		{
			prop->value.i = val;
			_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &val);
		}
	}
	else // !editable
	{
		nk_labelf(ctx, NK_TEXT_RIGHT, "%"PRIi32, prop->value.i);
	}
	nk_layout_row_push(ctx, 0.1);
	if(prop->unit)
		nk_label(ctx, prop->unit, NK_TEXT_RIGHT);
	else
		nk_spacing(ctx, 1);
}

static void
_parameter_widget_long(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	if(_dial_long(ctx, prop->minimum.h, &prop->value.h, prop->maximum.h, 1.f,
		prop->color, editable))
	{
		_patch_set(handle, prop->key, sizeof(int64_t), prop->range, &prop->value.h);
	}

	nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
	nk_layout_row_push(ctx, 0.9);
	if(editable)
	{
		const int64_t val = nk_propertyi(ctx, lab,
			prop->minimum.h, prop->value.h, prop->maximum.h, 1.f, 0.f);
		if(val != prop->value.h)
		{
			prop->value.h = val;
			_patch_set(handle, prop->key, sizeof(int64_t), prop->range, &val);
		}
	}
	else // !editable
	{
		nk_labelf(ctx, NK_TEXT_RIGHT, "%"PRIi64, prop->value.h);
	}
	nk_layout_row_push(ctx, 0.1);
	if(prop->unit)
		nk_label(ctx, prop->unit, NK_TEXT_RIGHT);
	else
		nk_spacing(ctx, 1);
}

static void
_parameter_widget_float(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	if(_dial_float(ctx, prop->minimum.f, &prop->value.f, prop->maximum.f, 1.f,
		prop->color, editable))
	{
		_patch_set(handle, prop->key, sizeof(float), prop->range, &prop->value.f);
	}

	nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
	nk_layout_row_push(ctx, 0.9);
	if(editable)
	{
		const float val = nk_propertyf(ctx, lab,
			prop->minimum.f, prop->value.f, prop->maximum.f, 0.05f, 0.f);
		if(val != prop->value.f)
		{
			prop->value.f = val;
			_patch_set(handle, prop->key, sizeof(float), prop->range, &val);
		}
	}
	else // !editable
	{
		nk_labelf(ctx, NK_TEXT_RIGHT, "%f", prop->value.f);
	}
	nk_layout_row_push(ctx, 0.1);
	if(prop->unit)
		nk_label(ctx, prop->unit, NK_TEXT_RIGHT);
	else
		nk_spacing(ctx, 1);
}

static void
_parameter_widget_double(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	if(_dial_double(ctx, prop->minimum.d, &prop->value.d, prop->maximum.d, 1.f,
		prop->color, editable))
	{
		_patch_set(handle, prop->key, sizeof(double), prop->range, &prop->value.d);
	}

	nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
	nk_layout_row_push(ctx, 0.9);
	if(editable)
	{
		const double val = nk_propertyd(ctx, lab,
			prop->minimum.d, prop->value.d, prop->maximum.d, 0.05f, 0.f);
		if(val != prop->value.d)
		{
			prop->value.d = val;
			_patch_set(handle, prop->key, sizeof(double), prop->range, &val);
		}
	}
	else // !editable
	{
		nk_labelf(ctx, NK_TEXT_RIGHT, "%lf", prop->value.d);
	}
	nk_layout_row_push(ctx, 0.1);
	if(prop->unit)
		nk_label(ctx, prop->unit, NK_TEXT_RIGHT);
	else
		nk_spacing(ctx, 1);
}

static void
_parameter_widget_bool(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	if(_dial_bool(ctx, &prop->value.i, prop->color, editable))
	{
		_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &prop->value.i);
	}
}

static void
_parameter_widget_urid(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	bool prop_commited = false;

	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	nk_flags flags = NK_EDIT_BOX;
	if(has_shift_enter)
		flags |= NK_EDIT_SIG_ENTER;
	if(!editable)
		flags |= NK_EDIT_READ_ONLY;
	const nk_flags state = nk_edit_buffer(ctx, flags, &prop->value.editor, nk_filter_default);
	if(state & NK_EDIT_COMMITED)
		prop_commited = true;

	if(!editable)
		return;

	nk_layout_row_dynamic(ctx, dy, 1);
	nk_style_push_style_item(ctx, &ctx->style.button.normal, prop_commited
		? nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE])
		: nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]));
	if(nk_button_label(ctx, "Submit") || prop_commited)
	{
		struct nk_str *str = &prop->value.editor.string;

		LV2_URID urid = 0;
		char *uri = _strndup(nk_str_get_const(str), nk_str_len_char(str));
		if(uri)
		{
			urid = handle->map->map(handle->map->handle, uri);
			free(uri);
		}
		if(urid)
			_patch_set(handle, prop->key, sizeof(uint32_t), prop->range, &urid);
	}
	nk_style_pop_style_item(ctx);
}

static void
_parameter_widget_string(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	bool prop_commited = false;

	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	nk_flags flags = NK_EDIT_BOX;
	if(!editable)
		flags |= NK_EDIT_READ_ONLY;
	if(has_shift_enter)
		flags |= NK_EDIT_SIG_ENTER;
	const nk_flags state = nk_edit_buffer(ctx, flags,
		&prop->value.editor, nk_filter_default);
	if(state & NK_EDIT_COMMITED)
		prop_commited = true;

	if(!editable)
		return;

	nk_layout_row_dynamic(ctx, dy, 1);
	nk_style_push_style_item(ctx, &ctx->style.button.normal, prop_commited
		? nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE])
		: nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]));
	if(nk_button_label(ctx, "Submit") || prop_commited)
	{
		struct nk_str *str = &prop->value.editor.string;

		_patch_set(handle, prop->key, nk_str_len_char(str), prop->range, nk_str_get_const(str));
	}
	nk_style_pop_style_item(ctx);
}

static void
_parameter_widget_chunk(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	nk_layout_row_dynamic(ctx, dy*(ndy-1), 1);
	nk_labelf(ctx, NK_TEXT_CENTERED, "%"PRIu32" bytes", prop->value.u);

	if(!editable)
		return;

	nk_layout_row_dynamic(ctx, dy, 1);
	if(nk_button_label(ctx, "Load"))
	{
		handle->browser_type = BROWSER_PROPERTY;
		handle->browser_target = prop;
	}
}

static void
_parameter_widget(plughandle_t *handle, struct nk_context *ctx, prop_t *prop,
	bool editable, bool has_shift_enter, float dy, int ndy)
{
	if(prop->comment && _tooltip_visible(ctx))
		nk_tooltip(ctx, prop->comment);
	if(nk_group_begin(ctx, prop->label, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
	{
		if(prop->points) // an Enumerator
		{
			_parameter_widget_enum(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.Int)
		{
			_parameter_widget_int(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.Long)
		{
			_parameter_widget_long(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.Float)
		{
			_parameter_widget_float(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.Double)
		{
			_parameter_widget_double(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.Bool)
		{
			_parameter_widget_bool(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.URID)
		{
			_parameter_widget_urid(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.String)
		{
			_parameter_widget_string(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else if(prop->range == handle->forge.Chunk)
		{
			_parameter_widget_chunk(handle, ctx, prop, editable, has_shift_enter, dy, ndy);
		}
		else
		{
			// ignore unsupported types
		}

		nk_group_end(ctx);
	}
}

static void
_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	ctx->clip.paste = _paste;
	ctx->clip.copy = _copy;
	ctx->clip.userdata.ptr = handle;

	struct nk_input *in = &ctx->input;
	const float dy = handle->dy;
	const struct nk_vec2 window_padding = ctx->style.window.padding;
	const struct nk_vec2 group_padding = ctx->style.window.group_padding;
	const float header_h = 1*dy + 2*window_padding.y;
	const float footer_h = 1*dy + 2*window_padding.y;
	const float body_h = wbounds.h - header_h - footer_h;
	const float header_height = ctx->style.font->height + 2*ctx->style.window.header.padding.y
		+ 2*ctx->style.window.header.label_padding.y;

	if(nk_begin(ctx, "Moony", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);

		const bool has_control_down = nk_input_is_key_down(in, NK_KEY_CTRL);
		const bool has_shift_down = nk_input_is_key_down(in, NK_KEY_SHIFT);
		const bool has_enter_pressed = nk_input_is_key_pressed(in, NK_KEY_ENTER);
		char control_letter = 0;
		if(has_control_down && (in->keyboard.text_len == 1) )
		{
			control_letter = in->keyboard.text[0];
			in->keyboard.text_len = 0;
		}
		const bool has_shift_enter = has_shift_down && has_enter_pressed;
		const bool has_control_enter = has_control_down && has_enter_pressed;
		const bool has_control_l = control_letter == 'l';
		const bool has_control_d = control_letter == 'd';
		const bool has_control_e = control_letter == 'e';
		const bool has_control_p = control_letter == 'p';
		const bool has_control_f = control_letter == 'f';
		const bool has_control_t = control_letter == 't';
		bool has_commited = false;

		nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 3);
		{
			ctx->style.selectable.normal = nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]);
			ctx->style.selectable.hover = nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_HOVER]);
			ctx->style.selectable.pressed = nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE]);

			ctx->style.selectable.normal_active = nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE]);
			ctx->style.selectable.hover_active = nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_HOVER]);
			ctx->style.selectable.pressed_active = nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE]);

			ctx->style.selectable.userdata = nk_handle_ptr(ctx);
			ctx->style.selectable.draw_end = _select_draw_end;

			nk_layout_row_push(ctx, 0.6);
			if(_tooltip_visible(ctx))
				nk_tooltip(ctx, "Ctrl-E");
			if(has_control_e)
				handle->editor_hidden = !handle->editor_hidden;
			const bool editor_hidden = handle->editor_hidden;
			handle->editor_hidden = !nk_select_label(ctx, "- Editor -", NK_TEXT_CENTERED, !handle->editor_hidden);
			if( (editor_hidden != handle->editor_hidden) || has_control_e)
				_patch_set(handle, handle->moony_editorHidden, sizeof(int32_t), handle->forge.Bool, &handle->editor_hidden);

			nk_layout_row_push(ctx, 0.2);
			if(_tooltip_visible(ctx))
				nk_tooltip(ctx, "Ctrl-L");
			if(has_control_l)
				handle->log_hidden = !handle->log_hidden;
			const bool log_hidden = handle->log_hidden;
			handle->log_hidden = !nk_select_label(ctx, "- Log -", NK_TEXT_CENTERED, !handle->log_hidden);
			if( (log_hidden != handle->log_hidden) || has_control_l)
				_patch_set(handle, handle->moony_logHidden, sizeof(int32_t), handle->forge.Bool, &handle->log_hidden);

			nk_layout_row_push(ctx, 0.2);
			if(_tooltip_visible(ctx))
				nk_tooltip(ctx, "Ctrl-P");
			if(has_control_p)
				handle->param_hidden = !handle->param_hidden;
			const bool param_hidden = handle->param_hidden;
			handle->param_hidden = !nk_select_label(ctx, "- Parameters -", NK_TEXT_CENTERED, !handle->param_hidden);
			if( (param_hidden != handle->param_hidden) || has_control_p)
				_patch_set(handle, handle->moony_paramHidden, sizeof(int32_t), handle->forge.Bool, &handle->param_hidden);

			ctx->style.selectable.draw_end = NULL;
		}
		nk_layout_row_end(ctx);

		nk_layout_row_begin(ctx, NK_DYNAMIC, body_h, !handle->editor_hidden + !handle->log_hidden + !handle->param_hidden);
		{
			const struct nk_style *style = &ctx->style;
			const float box_h = body_h - 1*dy - 3*group_padding.y;

			if(!handle->editor_hidden)
			{
				nk_layout_row_push(ctx, (handle->log_hidden && handle->param_hidden) ? 1.0 : 0.6);
				if(nk_group_begin(ctx, "Logic", NK_WINDOW_NO_SCROLLBAR))
				{
					const float left_width = style->font->width(style->font->userdata,
						style->font->height, "     ", 5); //TODO calculate only once?
					const float left_rel = left_width / nk_window_get_content_region_size(ctx).x;

					const float ratio [2] = {left_rel, NK_UNDEFINED};
					nk_layout_row(ctx, NK_DYNAMIC, box_h, 2, ratio);

					// reserve space for line numbers
					struct nk_rect line_number_bounds = nk_layout_space_bounds(ctx);
					const enum nk_widget_layout_states line_number_states = nk_widget(&line_number_bounds, ctx);

					nk_flags flags = NK_EDIT_BOX;
					if(has_shift_enter || has_control_enter)
						flags |= NK_EDIT_SIG_ENTER;
					const nk_flags state = nk_edit_buffer(ctx, flags,
						&handle->editor, nk_filter_default);
					if(state & NK_EDIT_COMMITED)
					{
						if(has_control_down)
							_submit_line_or_sel(handle);
						else if(has_shift_down)
							_submit_all(handle);

						has_commited = true;
					}

					// actually draw line numbers (after editor window)
					if(line_number_states != NK_WIDGET_INVALID)
					{
						const float y0 = line_number_bounds.y + style->edit.padding.y + style->edit.border - handle->editor.scrollbar.y;
						struct nk_command_buffer *canv = nk_window_get_canvas(ctx);

						struct nk_rect old_clip = canv->clip;
						nk_push_scissor(canv, line_number_bounds);
						for(int i = 0; ; i++)
						{
							struct nk_rect bb = line_number_bounds;
							bb.h = style->font->height + style->edit.row_padding;
							bb.y = y0 + bb.h * i;

							if(bb.y + bb.h < line_number_bounds.y)
								continue; // skip, not visible
							else if(bb.y > line_number_bounds.y + line_number_bounds.h)
								break; // break out of loop, rest not visible

							struct nk_window *win = ctx->current;
							struct nk_vec2 item_padding = style->text.padding;
							struct nk_text text;
							text.padding.x = item_padding.x;
							text.padding.y = item_padding.y;
							text.background = style->window.background;
							text.text = style->text.color;

							char tmp [16];
							snprintf(tmp, 16, "%i", i + 1);
							nk_widget_text(&win->buffer, bb, tmp, strlen(tmp), &text, NK_TEXT_RIGHT, style->font);
						}
						nk_push_scissor(canv, old_clip);
					}

					nk_layout_row_dynamic(ctx, dy, 4);
					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Shift-Enter");
					nk_style_push_style_item(ctx, &ctx->style.button.normal, has_shift_enter && has_commited
						? nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE])
						: nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]));
					if(nk_button_label(ctx, "Run all"))
						_submit_all(handle);
					nk_style_pop_style_item(ctx);

					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Ctrl-Enter");
					nk_style_push_style_item(ctx, &ctx->style.button.normal, has_control_enter && has_commited
						? nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE])
						: nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]));
					if(nk_button_label(ctx, "Run line/sel."))
						_submit_line_or_sel(handle);
					nk_style_pop_style_item(ctx);

					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Ctrl-F");
					nk_style_push_style_item(ctx, &ctx->style.button.normal, has_control_f
						? nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE])
						: nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]));
					if(nk_button_label(ctx, "Import from") || has_control_f)
						handle->browser_type = BROWSER_IMPORT;
					nk_style_pop_style_item(ctx);

					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Ctrl-E");
					nk_style_push_style_item(ctx, &ctx->style.button.normal, has_control_t
						? nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE])
						: nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]));
					if(nk_button_label(ctx, "Export to") || has_control_t)
						handle->browser_type = BROWSER_EXPORT;
					nk_style_pop_style_item(ctx);

					nk_group_end(ctx);
				}
			}

			if(!handle->log_hidden)
			{
				nk_layout_row_push(ctx, (handle->param_hidden ? 0.4 : 0.2) * (handle->editor_hidden ? 2.5 : 1) );
				if(nk_group_begin(ctx, "Utils", NK_WINDOW_NO_SCROLLBAR))
				{
					nk_layout_row_dynamic(ctx, box_h, 1);
					struct nk_list_view lview;
					const float dh = style->font->height + style->edit.row_padding;
					if(nk_list_view_begin(ctx, &lview, "Traces", NK_WINDOW_BORDER, dh, handle->n_trace))
					{
						nk_layout_row_dynamic(ctx, dh, 1);
						for(int l = lview.begin; (l < lview.end) && (l < handle->n_trace); l++)
						{
							if(l % 2)
							{
								struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
								struct nk_rect b1 = nk_widget_bounds(ctx);
								nk_fill_rect(canvas, b1, 0.f, nk_rgb(0x37, 0x37, 0x37));
							}
							nk_label(ctx, handle->traces[l], NK_TEXT_LEFT);
						}

						nk_list_view_end(&lview);
					}

					nk_layout_row_dynamic(ctx, dy, 1);
					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Ctrl-D");
					nk_style_push_style_item(ctx, &ctx->style.button.normal, has_control_d
						? nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON_ACTIVE])
						: nk_style_item_color(nk_default_color_style[NK_COLOR_BUTTON]));
					if(nk_button_label(ctx, "Clear log") || has_control_d)
						_clear_log(handle);
					nk_style_pop_style_item(ctx);

					nk_group_end(ctx);
				}
			}

			if(!handle->param_hidden)
			{
				nk_layout_row_push(ctx, (handle->log_hidden ? 0.4 : 0.2) * (handle->editor_hidden ? 2.5 : 1) );
				if(nk_group_begin(ctx, "Parameters", NK_WINDOW_NO_SCROLLBAR))
				{
					const float prop_h = (box_h - (handle->param_rows + 1) * group_padding.y) / handle->param_rows;
					const float rest = header_height + 3*group_padding.y + 2*ctx->style.window.border;
					const int ndy = ceilf(prop_h - rest) / dy;
					const float prop_h_2 = ndy*dy + rest;

					nk_layout_row_dynamic(ctx, box_h, 1);
					if(nk_group_begin(ctx, "Widgets", 0))
					{
						nk_layout_row_dynamic(ctx, prop_h_2, handle->param_cols);
						for(int p = 0; p < handle->n_writable; p++)
						{
							prop_t *prop = &handle->writables[p];
							if(!prop->key || !prop->range || !prop->label) // marked for removal
								continue;

							_parameter_widget(handle, ctx, prop, true, has_shift_enter, dy, ndy);
						}
						for(int p = 0; p < handle->n_readable; p++)
						{
							prop_t *prop = &handle->readables[p];
							if(!prop->key || !prop->range || !prop->label) // marked for removal
								continue;

							_parameter_widget(handle, ctx, prop, false, has_shift_enter, dy, ndy);
						}

						nk_group_end(ctx);
					}

					nk_layout_row_dynamic(ctx, dy, 2);

					const int param_rows = handle->param_rows;
					handle->param_rows = nk_propertyi(ctx, "Rows", 1, handle->param_rows, 16, 1, 0.f);
					if(param_rows != handle->param_rows)
						_patch_set(handle, handle->moony_paramRows, sizeof(int32_t), handle->forge.Int, &handle->param_rows);

					const int param_cols = handle->param_cols;
					handle->param_cols = nk_propertyi(ctx, "Columns", 1, handle->param_cols, 16, 1, 0.f);
					if(param_cols != handle->param_cols)
						_patch_set(handle, handle->moony_paramCols, sizeof(int32_t), handle->forge.Int, &handle->param_cols);

					nk_group_end(ctx);
				}
			}
		}
		nk_layout_row_end(ctx);

		nk_layout_row_dynamic(ctx, dy, 2);
		{
			if(handle->error[0] == 0)
				nk_spacing(ctx, 1);
			else
				nk_labelf_colored(ctx, NK_TEXT_LEFT, nk_yellow, "Stopped: %s", handle->error);
			nk_label(ctx, "Moony.lv2: "MOONY_VERSION, NK_TEXT_RIGHT);
		}
	}
	nk_end(ctx);

	if(handle->browser_type == BROWSER_NONE)
		return;

	const struct nk_vec2 browser_padding = nk_vec2(wbounds.w/4, wbounds.h/4);
	if(file_browser_run(&handle->browser, ctx, dy, "File browser",
		handle->browser_type, nk_pad_rect(wbounds, browser_padding)))
	{
		switch(handle->browser_type)
		{
			case BROWSER_NONE:
				break;
			case BROWSER_IMPORT:
			{
				size_t sz;
				uint8_t *chunk = file_load(handle->browser.file, &sz);
				if(chunk)
				{
					_patch_set_code(handle, sz, (const char *)chunk);
					free(chunk);
				}
			} break;
			case BROWSER_EXPORT:
			{
				FILE *f = fopen(handle->browser.file, "wb");
				if(f)
				{
					struct nk_str *str = &handle->editor.string;
					if(fwrite(nk_str_get_const(str), nk_str_len_char(str), 1, f) != 1)
						; //TODO handle error
					fclose(f);
				}
			} break;
			case BROWSER_PROPERTY:
			{
				prop_t *prop = handle->browser_target;

				size_t sz;
				uint8_t *chunk = file_load(handle->browser.file, &sz);
				if(chunk)
				{
					prop->value.u = sz;
					_patch_set(handle, prop->key, sz, prop->range, &chunk);
					free(chunk);
				}
			} break;
		}

		nk_window_close(ctx, "File browser");
	}

	if(  !nk_window_is_active(ctx, "File browser")
		|| nk_input_is_key_pressed(in, NK_KEY_TEXT_RESET_MODE)) // or escape
	{
		nk_window_close(ctx, "File browser");
	}

	if(nk_window_is_closed(ctx, "File browser"))
	{
		// reset browser type and target
		handle->browser_type = BROWSER_NONE;
		handle->browser_target = NULL;
	}
}

static struct nk_image
_icon_load(void *data, const char *file)
{
	plughandle_t *handle = data;

	struct nk_image img;
	char *path;
	if(asprintf(&path, "%s%s", handle->bundle_path, file) != -1)
	{
		img = nk_pugl_icon_load(&handle->win, path);
		free(path);
	}
	else
		nk_image_id(0);

	return img;
}

static void
_icon_unload(void *data, struct nk_image img)
{
	plughandle_t *handle = data;

	nk_pugl_icon_unload(&handle->win, img);
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

	handle->bundle_path = bundle_path;

	void *parent = NULL;
	LV2UI_Resize *host_resize = NULL;
	LV2UI_Port_Map *port_map = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
			host_resize = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			port_map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			handle->unmap = features[i]->data;
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
	if(!port_map)
	{
		fprintf(stderr,
			"%s: Host does not support ui:portMap\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!handle->map || !handle->unmap)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map/unmap\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	if(handle->log)
		lv2_log_logger_init(&handle->logger, handle->map, handle->log);

	lv2_atom_forge_init(&handle->forge, handle->map);

	handle->controller = controller;
	handle->writer = write_function;

	handle->control = port_map->port_index(port_map->handle, "control");
	handle->notify = port_map->port_index(port_map->handle, "notify");

	handle->atom_eventTransfer = handle->map->map(handle->map->handle, LV2_ATOM__eventTransfer);

	handle->patch_self = handle->map->map(handle->map->handle, plugin_uri);
	handle->patch_Get = handle->map->map(handle->map->handle, LV2_PATCH__Get);
	handle->patch_Set = handle->map->map(handle->map->handle, LV2_PATCH__Set);
	handle->patch_Put = handle->map->map(handle->map->handle, LV2_PATCH__Put);
	handle->patch_Patch = handle->map->map(handle->map->handle, LV2_PATCH__Patch);
	handle->patch_Ack = handle->map->map(handle->map->handle, LV2_PATCH__Ack);
	handle->patch_Error = handle->map->map(handle->map->handle, LV2_PATCH__Error);
	handle->patch_writable = handle->map->map(handle->map->handle, LV2_PATCH__writable);
	handle->patch_readable = handle->map->map(handle->map->handle, LV2_PATCH__readable);
	handle->patch_add = handle->map->map(handle->map->handle, LV2_PATCH__add);
	handle->patch_remove = handle->map->map(handle->map->handle, LV2_PATCH__remove);
	handle->patch_wildcard = handle->map->map(handle->map->handle, LV2_PATCH__wildcard);
	handle->patch_property = handle->map->map(handle->map->handle, LV2_PATCH__property);
	handle->patch_body = handle->map->map(handle->map->handle, LV2_PATCH__body);
	handle->patch_value = handle->map->map(handle->map->handle, LV2_PATCH__value);
	handle->patch_subject = handle->map->map(handle->map->handle, LV2_PATCH__subject);
	handle->patch_sequenceNumber = handle->map->map(handle->map->handle, LV2_PATCH__sequenceNumber);
	handle->moony_code = handle->map->map(handle->map->handle, MOONY_CODE_URI);
	handle->moony_selection = handle->map->map(handle->map->handle, MOONY_SELECTION_URI);
	handle->moony_error = handle->map->map(handle->map->handle, MOONY_ERROR_URI);
	handle->moony_trace = handle->map->map(handle->map->handle, MOONY_TRACE_URI);
	handle->moony_editorHidden = handle->map->map(handle->map->handle, MOONY_EDITOR_HIDDEN_URI);
	handle->moony_logHidden = handle->map->map(handle->map->handle, MOONY_LOG_HIDDEN_URI);
	handle->moony_paramHidden = handle->map->map(handle->map->handle, MOONY_PARAM_HIDDEN_URI);
	handle->moony_paramCols = handle->map->map(handle->map->handle, MOONY_PARAM_COLS_URI);
	handle->moony_paramRows = handle->map->map(handle->map->handle, MOONY_PARAM_ROWS_URI);
	handle->rdfs_label = handle->map->map(handle->map->handle, RDFS__label);
	handle->rdfs_range= handle->map->map(handle->map->handle, RDFS__range);
	handle->rdfs_comment = handle->map->map(handle->map->handle, RDFS__comment);
	handle->rdf_value = handle->map->map(handle->map->handle, RDF__value);
	handle->lv2_minimum = handle->map->map(handle->map->handle, LV2_CORE__minimum);
	handle->lv2_maximum = handle->map->map(handle->map->handle, LV2_CORE__maximum);
	handle->lv2_scalePoint = handle->map->map(handle->map->handle, LV2_CORE__scalePoint);

	handle->units_symbol = handle->map->map(handle->map->handle, LV2_UNITS__symbol);
	handle->units_unit = handle->map->map(handle->map->handle, LV2_UNITS__unit);
	handle->units_bar = handle->map->map(handle->map->handle, LV2_UNITS__bar);
	handle->units_beat = handle->map->map(handle->map->handle, LV2_UNITS__beat);
	handle->units_bpm = handle->map->map(handle->map->handle, LV2_UNITS__bpm);
	handle->units_cent = handle->map->map(handle->map->handle, LV2_UNITS__cent);
	handle->units_cm = handle->map->map(handle->map->handle, LV2_UNITS__cm);
	handle->units_db = handle->map->map(handle->map->handle, LV2_UNITS__db);
	handle->units_degree = handle->map->map(handle->map->handle, LV2_UNITS__degree);
	handle->units_frame = handle->map->map(handle->map->handle, LV2_UNITS__frame);
	handle->units_hz = handle->map->map(handle->map->handle, LV2_UNITS__hz);
	handle->units_inch = handle->map->map(handle->map->handle, LV2_UNITS__inch);
	handle->units_khz = handle->map->map(handle->map->handle, LV2_UNITS__khz);
	handle->units_km = handle->map->map(handle->map->handle, LV2_UNITS__km);
	handle->units_m = handle->map->map(handle->map->handle, LV2_UNITS__m);
	handle->units_mhz = handle->map->map(handle->map->handle, LV2_UNITS__mhz);
	handle->units_midiNote = handle->map->map(handle->map->handle, LV2_UNITS__midiNote);
	handle->units_mile = handle->map->map(handle->map->handle, LV2_UNITS__mile);
	handle->units_min = handle->map->map(handle->map->handle, LV2_UNITS__min);
	handle->units_mm = handle->map->map(handle->map->handle, LV2_UNITS__mm);
	handle->units_ms = handle->map->map(handle->map->handle, LV2_UNITS__ms);
	handle->units_oct = handle->map->map(handle->map->handle, LV2_UNITS__oct);
	handle->units_pc = handle->map->map(handle->map->handle, LV2_UNITS__pc);
	handle->units_s = handle->map->map(handle->map->handle, LV2_UNITS__s);
	handle->units_semitone12TET = handle->map->map(handle->map->handle, LV2_UNITS__semitone12TET);

	handle->canvas_Style = handle->map->map(handle->map->handle, CANVAS__Style);

	const char *NK_SCALE = getenv("NK_SCALE");
	const float scale = NK_SCALE ? atof(NK_SCALE) : 1.f;
	handle->dy = 20.f * scale;

	nk_pugl_config_t *cfg = &handle->win.cfg;
	cfg->width = 960 * scale;
	cfg->height = 960 * scale;
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

	handle->ser.moony = NULL;
	handle->ser.size = 1024;
	handle->ser.buf = malloc(1024);
	handle->ser.offset = 0;

	nk_textedit_init_fixed(&handle->editor, handle->code, MOONY_MAX_CHUNK_LEN);

	_patch_get(handle, handle->moony_code);
	_patch_get(handle, handle->moony_error);
	_patch_get(handle, handle->moony_editorHidden);
	_patch_get(handle, handle->moony_logHidden);
	_patch_get(handle, handle->moony_paramHidden);
	_patch_get(handle, handle->moony_paramCols);
	_patch_get(handle, handle->moony_paramRows);
	_patch_get(handle, 0);

	handle->L = luaL_newstate();
	if(handle->L)
	{
		luaL_requiref(handle->L, "base", luaopen_base, 0);
		luaL_requiref(handle->L, "coroutine", luaopen_coroutine, 1);
		luaL_requiref(handle->L, "table", luaopen_table, 1);
		luaL_requiref(handle->L, "string", luaopen_string, 1);
		luaL_requiref(handle->L, "math", luaopen_math, 1);
		luaL_requiref(handle->L, "utf8", luaopen_utf8, 1);
		luaL_requiref(handle->L, "debug", luaopen_debug, 1);
		luaL_requiref(handle->L, "lpeg", luaopen_lpeg, 1);
		lua_pop(handle->L, 8);

		//luaL_requiref(handle->L, "io", luaopen_io, 1);
		//luaL_requiref(handle->L, "os", luaopen_os, 1);
		//luaL_requiref(handle->L, "bit32", luaopen_bit32, 1);
		luaL_requiref(handle->L, "package", luaopen_package, 1);

		// automatic garbage collector
		lua_gc(handle->L, LUA_GCRESTART, 0); // enable automatic garbage collection
		lua_gc(handle->L, LUA_GCSETPAUSE, 105); // next step when memory increased by 5%
		lua_gc(handle->L, LUA_GCSETSTEPMUL, 105); // run 5% faster than memory allocation

		// set package.path
		lua_getglobal(handle->L, "package");
		lua_pushfstring(handle->L, "%s?.lua", bundle_path);
		lua_setfield(handle->L, -2, "path");
		lua_pop(handle->L, 1); // package

		path = NULL;
		if(asprintf(&path, "%s%s", bundle_path, "lexer.lua") != -1)
		{
			if(luaL_dofile(handle->L, path))
			{
				fprintf(stderr, "err: %s\n", lua_tostring(handle->L, -1));
				lua_pop(handle->L, 1); // err
			}
			else
			{
				lua_setglobal(handle->L, "lexer");
			}
			free(path);
		}

		lua_getglobal(handle->L, "lexer");
		lua_getfield(handle->L, -1, "load");
		lua_pushstring(handle->L, "moony");
		if(lua_pcall(handle->L, 1, 1, 0))
		{
			fprintf(stderr, "err: %s\n", lua_tostring(handle->L, -1));
			lua_pop(handle->L, 1);
		}
		else
		{
			lua_setglobal(handle->L, "moony");
		}
		lua_pop(handle->L, 1); // lexer
	}

	handle->editor.lexer.lex = _lex;
	handle->editor.lexer.data = handle->L;

	file_browser_init(&handle->browser, 0, 1, _icon_load, handle);

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	file_browser_free(&handle->browser, _icon_unload, handle);

	if(handle->clipboard)
		free(handle->clipboard);

	if(handle->L)
		lua_close(handle->L);

	nk_textedit_free(&handle->editor);

	if(handle->editor.lexer.tokens)
		free(handle->editor.lexer.tokens);

	_clear_log(handle);

	for(int p = 0; p < handle->n_writable; p++)
		_prop_free(handle, &handle->writables[p]);
	if(handle->writables)
		free(handle->writables);

	for(int p = 0; p < handle->n_readable; p++)
		_prop_free(handle, &handle->readables[p]);
	if(handle->readables)
		free(handle->readables);

	if(handle->ser.buf)
		free(handle->ser.buf);

	nk_pugl_hide(&handle->win);
	nk_pugl_shutdown(&handle->win);

	free(handle);
}

static void
_patch_set_parameter_value(plughandle_t *handle, LV2_URID property,
	const LV2_Atom *value)
{
	const char *body = LV2_ATOM_BODY_CONST(value);

	prop_t *prop = _prop_get(&handle->readables, &handle->n_readable, property);
	if(!prop)
		prop = _prop_get(&handle->writables, &handle->n_writable, property);
	if(prop && (prop->range == value->type) )
	{
		if(prop->range == handle->forge.Int)
		{
			prop->value.i = ((const LV2_Atom_Int *)value)->body;
		}
		else if(prop->range == handle->forge.Long)
		{
			prop->value.h = ((const LV2_Atom_Long *)value)->body;
		}
		else if(prop->range == handle->forge.Float)
		{
			prop->value.f = ((const LV2_Atom_Float *)value)->body;
		}
		else if(prop->range == handle->forge.Double)
		{
			prop->value.d = ((const LV2_Atom_Double *)value)->body;
		}
		else if(prop->range == handle->forge.Bool)
		{
			prop->value.i = ((const LV2_Atom_Bool *)value)->body;
		}
		else if(prop->range == handle->forge.URID)
		{
			const LV2_URID urid = ((const LV2_Atom_URID *)value)->body;
			const char *uri = handle->unmap->unmap(handle->unmap->handle, urid);

			struct nk_str *str = &prop->value.editor.string;
			nk_str_clear(str);
			nk_str_append_text_utf8(str, uri, strlen(uri));
		}
		else if(prop->range == handle->forge.String)
		{
			struct nk_str *str = &prop->value.editor.string;
			nk_str_clear(str);
			nk_str_append_text_utf8(str, LV2_ATOM_BODY_CONST(value), value->size - 1);
		}
		else if(prop->range == handle->forge.Chunk)
		{
			prop->value.u = value->size; // store only chunk size
		}
		else
		{
			//FIXME
		}

		nk_pugl_post_redisplay(&handle->win);
	}
}

static void
_patch_set_parameter_property(plughandle_t *handle, LV2_URID subject, LV2_URID property,
	const LV2_Atom *value)
{
	const char *body = LV2_ATOM_BODY_CONST(value);

	prop_t *prop = _prop_get(&handle->readables, &handle->n_readable, subject);
	if(!prop)
		prop = _prop_get(&handle->writables, &handle->n_writable, subject);
	if(prop)
	{
		if(  (property == handle->rdfs_range)
			&& (value->type == handle->forge.URID) )
		{
			const LV2_Atom_URID *range = (const LV2_Atom_URID *)value;

			prop->range = range->body;

			if( (prop->range == handle->forge.String)
				|| (prop->range == handle->forge.URID) )
			{
				nk_textedit_init_default(&prop->value.editor);
			}

			_patch_get(handle, subject); // rdfs:range is mandatory, so will always be sent
		}
		else if( (property == handle->rdfs_label)
			&& (value->type == handle->forge.String) )
		{
			prop->label = strdup(LV2_ATOM_BODY_CONST(value));
		}
		else if( (property == handle->rdfs_comment)
			&& (value->type == handle->forge.String) )
		{
			prop->comment= strdup(LV2_ATOM_BODY_CONST(value));
		}
		else if( (property == handle->units_symbol)
			&& (value->type == handle->forge.String) )
		{
			prop->unit= strdup(LV2_ATOM_BODY_CONST(value));
		}
		else if( (property == handle->units_unit)
			&& (value->type == handle->forge.URID) )
		{
			const LV2_Atom_URID *unit = (const LV2_Atom_URID *)value;

			//TODO use binary lookup
			if(unit->body == handle->units_bar)
				prop->unit = strdup("bars");
			else if(unit->body == handle->units_beat)
				prop->unit = strdup("beats");
			else if(unit->body == handle->units_bpm)
				prop->unit = strdup("BPM");
			else if(unit->body == handle->units_cent)
				prop->unit = strdup("ct");
			else if(unit->body == handle->units_cm)
				prop->unit = strdup("cm");
			else if(unit->body == handle->units_db)
				prop->unit = strdup("dB");
			else if(unit->body == handle->units_degree)
				prop->unit = strdup("deg");
			else if(unit->body == handle->units_frame)
				prop->unit = strdup("frames");
			else if(unit->body == handle->units_hz)
				prop->unit = strdup("Hz");
			else if(unit->body == handle->units_inch)
				prop->unit = strdup("in");
			else if(unit->body == handle->units_khz)
				prop->unit = strdup("kHz");
			else if(unit->body == handle->units_km)
				prop->unit = strdup("km");
			else if(unit->body == handle->units_m)
				prop->unit = strdup("m");
			else if(unit->body == handle->units_mhz)
				prop->unit = strdup("MHz");
			else if(unit->body == handle->units_midiNote)
				prop->unit = strdup("note");
			else if(unit->body == handle->units_mile)
				prop->unit = strdup("mi");
			else if(unit->body == handle->units_min)
				prop->unit = strdup("min");
			else if(unit->body == handle->units_mm)
				prop->unit = strdup("mm");
			else if(unit->body == handle->units_ms)
				prop->unit = strdup("ms");
			else if(unit->body == handle->units_oct)
				prop->unit = strdup("oct");
			else if(unit->body == handle->units_pc)
				prop->unit = strdup("%");
			else if(unit->body == handle->units_s)
				prop->unit = strdup("s");
			else if(unit->body == handle->units_semitone12TET)
				prop->unit = strdup("semi");
		}
		else if( (property == handle->canvas_Style)
			&& (value->type == handle->forge.Long) )
		{
			const LV2_Atom_Long *style = (const LV2_Atom_Long *)value;

			prop->color = nk_rgba(
				(style->body >> 24) & 0xff,
				(style->body >> 16) & 0xff,
				(style->body >>  8) & 0xff,
				(style->body >>  0) & 0xff);
		}
		else if( (property == handle->lv2_minimum)
			&& (!prop->range || (prop->range == value->type)) )
		{
			if(value->type == handle->forge.Int)
				prop->minimum.i = ((const LV2_Atom_Int *)(value))->body;
			else if(value->type == handle->forge.Long)
				prop->minimum.h = ((const LV2_Atom_Long *)(value))->body;
			else if(value->type == handle->forge.Float)
				prop->minimum.f = ((const LV2_Atom_Float *)(value))->body;
			else if(value->type == handle->forge.Double)
				prop->minimum.d = ((const LV2_Atom_Double *)(value))->body;
			//FIXME handle more types?
		}
		else if( (property == handle->lv2_maximum)
			&& (!prop->range || (prop->range == value->type)) )
		{
			if(value->type == handle->forge.Int)
				prop->maximum.i = ((const LV2_Atom_Int *)(value))->body;
			else if(value->type == handle->forge.Long)
				prop->maximum.h = ((const LV2_Atom_Long *)(value))->body;
			else if(value->type == handle->forge.Float)
				prop->maximum.f = ((const LV2_Atom_Float *)(value))->body;
			else if(value->type == handle->forge.Double)
				prop->maximum.d = ((const LV2_Atom_Double *)(value))->body;
			//FIXME handle more types?
		}
		else if( (property == handle->lv2_scalePoint)
			&& (value->type == handle->forge.Tuple) )
		{
			const LV2_Atom_Tuple *points = (const LV2_Atom_Tuple *)value;

			const uint32_t sz = lv2_atom_total_size(&points->atom);
			prop->points = malloc(sz);
			if(prop->points)
				memcpy(prop->points, points, sz);
		}

		nk_pugl_post_redisplay(&handle->win);
	}
}

static void
_patch_set_code(plughandle_t *handle, uint32_t size, const char *body)
{
	if(size <= MOONY_MAX_CHUNK_LEN)
	{
		struct nk_str *str = &handle->editor.string;
		nk_str_clear(str);

		// replace tab with 2 spaces
		const char *end = body + size - 1;
		const char *from = body;
		for(const char *to = strchr(from, '\t');
			to && (to < end);
			from = to + 1, to = strchr(from, '\t'))
		{
			nk_str_append_text_utf8(str, from, to-from);
			nk_str_append_text_utf8(str, "  ", 2);
		}
		nk_str_append_text_utf8(str, from, end-from);

		handle->editor.lexer.needs_refresh = 1;

		nk_pugl_post_redisplay(&handle->win);
	}
}

static void
_patch_set_self(plughandle_t *handle, LV2_URID property, const LV2_Atom *value)
{
	const char *body = LV2_ATOM_BODY_CONST(value);

	if(property == handle->moony_code)
	{
		_patch_set_code(handle, value->size, body);
	}
	else if(property == handle->moony_trace)
	{
		if(value->size <= MOONY_MAX_TRACE_LEN)
		{
			char *trace = strdup(body);
			if(trace)
			{
				handle->traces = realloc(handle->traces, (handle->n_trace + 1)*sizeof(char *));
				handle->traces[handle->n_trace++] = trace;

				// replace tab with 1 space
				const char *end = trace + value->size - 1;
				const char *from = trace;
				for(char *to = strchr(from, '\t');
					to && (to < end);
					from = to + 1, to = strchr(from, '\t'))
				{
					*to = ' ';
				}
			}

			nk_pugl_post_redisplay(&handle->win);
		}
	}
	else if(property == handle->moony_error)
	{
		if(value->size <= MOONY_MAX_ERROR_LEN)
		{
			if(handle->error[0] == 0) // do not overwrite previously received error message
			{
				strncpy(handle->error, body, value->size);
				handle->error_sz = value->size - 1;

				nk_pugl_post_redisplay(&handle->win);
			}
		}
	}
	else if(property == handle->moony_editorHidden)
	{
		if(value->type == handle->forge.Bool)
		{
			handle->editor_hidden = ((const LV2_Atom_Bool *)value)->body;

			nk_pugl_post_redisplay(&handle->win);
		}
	}
	else if(property == handle->moony_logHidden)
	{
		if(value->type == handle->forge.Bool)
		{
			handle->log_hidden = ((const LV2_Atom_Bool *)value)->body;

			nk_pugl_post_redisplay(&handle->win);
		}
	}
	else if(property == handle->moony_paramHidden)
	{
		if(value->type == handle->forge.Bool)
		{
			handle->param_hidden = ((const LV2_Atom_Bool *)value)->body;

			nk_pugl_post_redisplay(&handle->win);
		}
	}
	else if(property == handle->moony_paramCols)
	{
		if(value->type == handle->forge.Int)
		{
			handle->param_cols = ((const LV2_Atom_Int *)value)->body;

			nk_pugl_post_redisplay(&handle->win);
		}
	}
	else if(property == handle->moony_paramRows)
	{
		if(value->type == handle->forge.Int)
		{
			handle->param_rows = ((const LV2_Atom_Int *)value)->body;

			nk_pugl_post_redisplay(&handle->win);
		}
	}
	else
	{
		_patch_set_parameter_value(handle, property, value);
	}
}

static void
port_event(LV2UI_Handle instance, uint32_t index, uint32_t size,
	uint32_t protocol, const void *buf)
{
	plughandle_t *handle = instance;

	if( (index == handle->notify) && (protocol == handle->atom_eventTransfer) )
	{
		const LV2_Atom_Object *obj = buf;

		if(lv2_atom_forge_is_object_type(&handle->forge, obj->atom.type))
		{
			if(obj->body.otype == handle->patch_Set)
			{
				const LV2_Atom_URID *subj = NULL;
				const LV2_Atom_Int *seqn = NULL;
				const LV2_Atom_URID *property = NULL;
				const LV2_Atom *value = NULL;

				lv2_atom_object_get(obj,
					handle->patch_subject, &subj,
					handle->patch_sequenceNumber, &seqn,
					handle->patch_property, &property,
					handle->patch_value, &value,
					0);

				int32_t sequence_number = 0;
				if(seqn && (seqn->atom.type == handle->forge.Int) )
					sequence_number = seqn->body;

				if(  property && (property->atom.type == handle->forge.URID)
					&& (!subj || (subj->atom.type == handle->forge.URID))
					&& value)
				{
					if(!subj || (subj->body == handle->patch_self) ) // destined for plugin
					{
						_patch_set_self(handle, property->body, value);
					}
					else // destined for parameter
					{
						_patch_set_parameter_property(handle, subj->body, property->body, value);
					}

					_patch_ack(handle, sequence_number); //TODO handle errors in _patch_set_*
				}
				else
				{
					_patch_error(handle, sequence_number);
				}
			}
			else if(obj->body.otype == handle->patch_Put)
			{
				const LV2_Atom_URID *subj = NULL;
				const LV2_Atom_Int *seqn = NULL;
				const LV2_Atom_Object *body= NULL;

				lv2_atom_object_get(obj,
					handle->patch_subject, &subj,
					handle->patch_sequenceNumber, &seqn,
					handle->patch_body, &body,
					0);

				int32_t sequence_number = 0;
				if(seqn && (seqn->atom.type == handle->forge.Int) )
					sequence_number = seqn->body;

				if(  (!subj || (subj->atom.type == handle->forge.URID))
					&& body && lv2_atom_forge_is_object_type(&handle->forge, body->atom.type) )
				{
					if(!subj || (subj->body == handle->patch_self) ) // destined for plugin
					{
						LV2_ATOM_OBJECT_FOREACH(body, pro)
						{
							_patch_set_self(handle, pro->key, &pro->value);
						}
					}
					else // destined for parameter
					{
						LV2_ATOM_OBJECT_FOREACH(body, pro)
						{
							_patch_set_parameter_property(handle, subj->body, pro->key, &pro->value);
						}
					}

					_patch_ack(handle, sequence_number); //TODO handle errors in _patch_set_*
				}
				else
				{
					_patch_error(handle, sequence_number);
				}
			}
			else if(obj->body.otype == handle->patch_Patch)
			{
				const LV2_Atom_URID *subj = NULL;
				const LV2_Atom_Int *seqn = NULL;
				const LV2_Atom_Object *add = NULL;
				const LV2_Atom_Object *rem = NULL;

				lv2_atom_object_get(obj,
					handle->patch_subject, &subj,
					handle->patch_sequenceNumber, &seqn,
					handle->patch_add, &add,
					handle->patch_remove, &rem,
					0);

				int32_t sequence_number = 0;
				if(seqn && (seqn->atom.type == handle->forge.Int) )
					sequence_number = seqn->body;

				if(  (!subj || (subj->atom.type == handle->forge.URID))
					&& add && lv2_atom_forge_is_object_type(&handle->forge, add->atom.type)
					&& rem && lv2_atom_forge_is_object_type(&handle->forge, rem->atom.type) )
				{
					LV2_ATOM_OBJECT_FOREACH(rem, pro)
					{
						if(!subj || (subj->body == handle->patch_self))
						{
							const LV2_Atom_URID *property = (const LV2_Atom_URID *)&pro->value;

							if(  (pro->key == handle->patch_writable)
								&& (property->body == handle->patch_wildcard) )
							{
								//printf("clearing patch:writable\n");
								for(int p = 0; p < handle->n_writable; p++)
									_prop_free(handle, &handle->writables[p]);

								if(handle->writables)
									free(handle->writables);

								handle->writables = NULL;
								handle->n_writable = 0;
							}
							else if( (pro->key == handle->patch_readable)
								&& (property->body == handle->patch_wildcard) )
							{
								//printf("clearing patch:readable\n");
								for(int p = 0; p < handle->n_readable; p++)
									_prop_free(handle, &handle->readables[p]);

								if(handle->readables)
									free(handle->readables);

								handle->readables = NULL;
								handle->n_readable = 0;
							}
						}
					}

					LV2_ATOM_OBJECT_FOREACH(add, pro)
					{
						if(!subj || (subj->body == handle->patch_self))
						{
							const LV2_Atom_URID *property = (const LV2_Atom_URID *)&pro->value; //FIXME check atom.type

							if(pro->key == handle->patch_writable)
							{
								prop_t *prop = _prop_get_or_add(handle, &handle->writables, &handle->n_writable, property->body);
								(void)prop;
							}
							else if(pro->key == handle->patch_readable)
							{
								prop_t *prop = _prop_get_or_add(handle, &handle->readables, &handle->n_readable, property->body);
								(void)prop;
							}
						}
						else
						{
							_patch_set_parameter_property(handle, subj->body, pro->key, &pro->value);
						}
					}

					_patch_ack(handle, sequence_number); //TODO handle errors in _patch_get/set_*
				}
				else
				{
					_patch_error(handle, sequence_number);
				}
			}
		}
	}
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
