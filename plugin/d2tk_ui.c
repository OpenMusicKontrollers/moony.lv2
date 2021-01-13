/*
 * Copyright (c) 2019-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)
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
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

#include <moony.h>
#include <props.h>

#define SER_ATOM_IMPLEMENTATION
#include <ser_atom.lv2/ser_atom.h>

#include <d2tk/hash.h>
#include <d2tk/util.h>
#include <d2tk/frontend_pugl.h>

#include <nanovg.h>

#define LV2_CANVAS_RENDER_NANOVG
#include <canvas.lv2/render.h>

#define MAX_NPROPS 16
#define MAX_GRAPH 2048 //FIXME

#define RDF_PREFIX    "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDFS_PREFIX   "http://www.w3.org/2000/01/rdf-schema#"

#define RDF__value    RDF_PREFIX"value"
#define RDFS__label   RDFS_PREFIX"label"
#define RDFS__range   RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

typedef enum _console_t {
	CONSOLE_EDITOR,
	CONSOLE_GRAPH,

	CONSOLE_MAX
} console_t;

typedef struct _plugstate_t plugstate_t;
typedef struct _plughandle_t plughandle_t;
typedef void (*console_cb_t)(plughandle_t *handle, const d2tk_rect_t *rect);
typedef union _dynvalue_t dynvalue_t;
typedef struct _dynparam_t dynparam_t;

union _dynvalue_t {
	uint32_t urid;
	int32_t i32;
	int64_t i64;
	float f32;
	double f64;
};

struct _dynparam_t {
	uint32_t prop;
	const char *uri;

	bool writable;

	LV2_URID range;

	uint32_t size;
	void *val;

	char *label;
	char *comment;
	void *min;
	void *max;
	char *unit;

	LV2_Atom_Tuple *points;
};

struct _plugstate_t {
	char code [MOONY_MAX_CHUNK_LEN];
	char error [MOONY_MAX_ERROR_LEN];
	int32_t font_height;
	int32_t panic;
	int32_t graph_hidden;
	int32_t editor_hidden;
	uint8_t graph_body [MAX_GRAPH];
	float aspect_ratio;
	bool mouse_button_left;
	bool mouse_button_middle;
	bool mouse_button_right;
	double mouse_wheel_x;
	double mouse_wheel_y;
	double mouse_position_x;
	double mouse_position_y;
	bool mouse_focus;
};

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

#ifdef _LV2_HAS_REQUEST_VALUE
	LV2UI_Request_Value *request_code;
#endif

	d2tk_pugl_config_t config;
	d2tk_frontend_t *dpugl;

	LV2UI_Controller *controller;
	LV2UI_Write_Function writer;

	PROPS_T(props, MAX_NPROPS);

	plugstate_t state;
	plugstate_t stash;

	uint64_t hash;

	LV2_URID atom_eventTransfer;
	LV2_URID midi_MidiEvent;

	LV2_URID urid_code;
	LV2_URID urid_error;
	LV2_URID urid_fontHeight;
	LV2_URID urid_panic;
	LV2_URID urid_editorHidden;
	LV2_URID urid_graphHidden;

	LV2_URID urid_rdfs_range;
	LV2_URID urid_rdfs_label;
	LV2_URID urid_rdfs_comment;
	LV2_URID urid_rdf_value;

	LV2_URID urid_lv2_minimum;
	LV2_URID urid_lv2_maximum;
	LV2_URID urid_lv2_scalePoint;

	LV2_URID urid_units_unit;
	LV2_URID urid_units_symbol;
	LV2_URID urid_units_bar;
	LV2_URID urid_units_beat;
	LV2_URID urid_units_bpm;
	LV2_URID urid_units_cent;
	LV2_URID urid_units_cm;
	LV2_URID urid_units_db;
	LV2_URID urid_units_degree;
	LV2_URID urid_units_frame;
	LV2_URID urid_units_hz;
	LV2_URID urid_units_inch;
	LV2_URID urid_units_khz;
	LV2_URID urid_units_km;
	LV2_URID urid_units_m;
	LV2_URID urid_units_mhz;
	LV2_URID urid_units_midiNote;
	LV2_URID urid_units_mile;
	LV2_URID urid_units_min;
	LV2_URID urid_units_mm;
	LV2_URID urid_units_ms;
	LV2_URID urid_units_oct;
	LV2_URID urid_units_pc;
	LV2_URID urid_units_s;
	LV2_URID urid_units_semitone12TET;

	bool reinit;
	char template [24];
	char manual [PATH_MAX] ;
	int fd;
	time_t modtime;

	float scale;
	d2tk_coord_t header_height;
	d2tk_coord_t footer_height;
	d2tk_coord_t tip_height;
	d2tk_coord_t sidebar_width;
	d2tk_coord_t item_height;
	d2tk_coord_t font_height;

	uint32_t max_red;
	uint32_t control;

	int done;

	console_t console;

	LV2_Canvas canvas;

	uint32_t graph_size;
	int kid;

	dynparam_t *dynparams;
	size_t ndynparams;
};

static void
_update_font_height(plughandle_t *handle)
{
	handle->font_height = handle->state.font_height * handle->scale;
}

static void
_intercept_code(void *data, int64_t frames __attribute__((unused)),
	props_impl_t *impl __attribute__((unused)))
{
	plughandle_t *handle = data;

	const ssize_t len = strlen(handle->state.code);
	const uint64_t hash = d2tk_hash(handle->state.code, len);

	if(handle->hash == hash)
	{
		return;
	}

	handle->hash = hash;

	// save code to file
	if(lseek(handle->fd, 0, SEEK_SET) == -1)
	{
		lv2_log_error(&handle->logger, "lseek: %s\n", strerror(errno));
	}
	if(ftruncate(handle->fd, 0) == -1)
	{
		lv2_log_error(&handle->logger, "ftruncate: %s\n", strerror(errno));
	}
	if(fsync(handle->fd) == -1)
	{
		lv2_log_error(&handle->logger, "fsync: %s\n", strerror(errno));
	}
	if(write(handle->fd, handle->state.code, len) == -1)
	{
		lv2_log_error(&handle->logger, "write: %s\n", strerror(errno));
	}
	if(fsync(handle->fd) == -1)
	{
		lv2_log_error(&handle->logger, "fsync: %s\n", strerror(errno));
	}

	// change modification timestamp of file
	struct stat st;
	if(stat(handle->template, &st) == -1)
	{
		lv2_log_error(&handle->logger, "stat: %s\n", strerror(errno));
	}

	handle->modtime = time(NULL);

	const struct utimbuf btime = {
	 .actime = st.st_atime,
	 .modtime = handle->modtime
	};

	if(utime(handle->template, &btime) == -1)
	{
		lv2_log_error(&handle->logger, "utime: %s\n", strerror(errno));
	}

	handle->reinit = true;
}

static void
_intercept_graph(void *data, int64_t frames __attribute__((unused)),
	props_impl_t *impl)
{
	plughandle_t *handle = data;

	handle->graph_size = impl->value.size;
}

static void
_intercept_font_height(void *data, int64_t frames __attribute__((unused)),
	props_impl_t *impl __attribute__((unused)))
{
	plughandle_t *handle = data;

	_update_font_height(handle);
}

static void
_intercept_control(void *data __attribute__((unused)),
	int64_t frames __attribute__((unused)), props_impl_t *impl __attribute__((unused)))
{
	// nothing to do, yet
}

static const props_def_t defs [MAX_NPROPS] = {
	{
		.property = MOONY_CODE_URI,
		.offset = offsetof(plugstate_t, code),
		.type = LV2_ATOM__String,
		.event_cb = _intercept_code,
		.max_size = MOONY_MAX_CHUNK_LEN
	},
	{
		.property = MOONY_ERROR_URI,
		.access = LV2_PATCH__readable,
		.offset = offsetof(plugstate_t, error),
		.type = LV2_ATOM__String,
		.max_size = MOONY_MAX_ERROR_LEN
	},
	{
		.property = MOONY_FONT_HEIGHT_URI,
		.offset = offsetof(plugstate_t, font_height),
		.type = LV2_ATOM__Int,
		.event_cb = _intercept_font_height
	},
	{
		.property = MOONY_PANIC_URI,
		.offset = offsetof(plugstate_t, panic),
		.type = LV2_ATOM__Bool
	},
	{
		.property = MOONY_EDITOR_HIDDEN_URI,
		.offset = offsetof(plugstate_t, editor_hidden),
		.type = LV2_ATOM__Bool
	},
	{
		.property = MOONY_GRAPH_HIDDEN_URI,
		.offset = offsetof(plugstate_t, graph_hidden),
		.type = LV2_ATOM__Bool
	},

	{
		.property = CANVAS__graph,
		.offset = offsetof(plugstate_t, graph_body),
		.type = LV2_ATOM__Tuple,
		.event_cb = _intercept_graph,
		.max_size = MAX_GRAPH
	},
	{
		.property = CANVAS__aspectRatio,
		.offset = offsetof(plugstate_t, aspect_ratio),
		.type = LV2_ATOM__Float
	},
	{
		.property = CANVAS__mouseButtonLeft,
		.offset = offsetof(plugstate_t, mouse_button_left),
		.type = LV2_ATOM__Bool
	},
	{
		.property = CANVAS__mouseButtonMiddle,
		.offset = offsetof(plugstate_t, mouse_button_middle),
		.type = LV2_ATOM__Bool
	},
	{
		.property = CANVAS__mouseButtonRight,
		.offset = offsetof(plugstate_t, mouse_button_right),
		.type = LV2_ATOM__Bool
	},
	{
		.property = CANVAS__mouseWheelX,
		.offset = offsetof(plugstate_t, mouse_wheel_x),
		.type = LV2_ATOM__Double
	},
	{
		.property = CANVAS__mouseWheelY,
		.offset = offsetof(plugstate_t, mouse_wheel_y),
		.type = LV2_ATOM__Double
	},
	{
		.property = CANVAS__mousePositionX,
		.offset = offsetof(plugstate_t, mouse_position_x),
		.type = LV2_ATOM__Double
	},
	{
		.property = CANVAS__mousePositionY,
		.offset = offsetof(plugstate_t, mouse_position_y),
		.type = LV2_ATOM__Double
	},
	{
		.property = CANVAS__mouseFocus,
		.offset = offsetof(plugstate_t, mouse_focus),
		.type = LV2_ATOM__Bool
	}
};

static int
_dynparam_cmp(const void *_a, const void *_b)
{
	const dynparam_t *a = _a;
	const dynparam_t *b = _b;

	return strcmp(a->uri, b->uri);

	return 0;
}

static void
_dynparam_free(dynparam_t *dynparam)
{
	free(dynparam->label);
	free(dynparam->comment);
	free(dynparam->val);
	free(dynparam->min);
	free(dynparam->max);
	free(dynparam->unit);
	free(dynparam->points);
}

static dynparam_t *
_dynparams_get(plughandle_t *handle, LV2_URID prop)
{
	const dynparam_t key = {
		.uri = handle->unmap->unmap(handle->unmap->handle, prop)
	};

	return bsearch(&key, handle->dynparams, handle->ndynparams,
		sizeof(dynparam_t), _dynparam_cmp);
}

static dynparam_t *
_dynparams_add(plughandle_t *handle, LV2_URID prop)
{
	const size_t new_sz = sizeof(dynparam_t) * ++handle->ndynparams;

	handle->dynparams = realloc(handle->dynparams, new_sz); //TODO check

	dynparam_t *dynparam = &handle->dynparams[handle->ndynparams - 1];
	memset(dynparam, 0x0, sizeof(dynparam_t));
	handle->dynparams[handle->ndynparams - 1].prop = prop;
	handle->dynparams[handle->ndynparams - 1].uri = handle->unmap->unmap(
		handle->unmap->handle, prop);

	qsort(handle->dynparams, handle->ndynparams,
		sizeof(dynparam_t), _dynparam_cmp);

	return _dynparams_get(handle, prop);
}

static void
_dynparams_clr(plughandle_t *handle)
{
	if(handle->dynparams)
	{
		for(unsigned i = 0; i < handle->ndynparams; i++)
		{
			dynparam_t *dynparam = &handle->dynparams[i];

			_dynparam_free(dynparam);
		}

		free(handle->dynparams);
		handle->dynparams = NULL;
	}

	handle->ndynparams = 0;
}

static void
_dyn_prop_add(plughandle_t *handle, LV2_URID subj, LV2_URID prop,
	const LV2_Atom *atom)
{
	if(prop == handle->props.urid.patch_writable)
	{
		const LV2_URID tar = ((const LV2_Atom_URID *)(atom))->body; //FIXME check
		dynparam_t *dynparam = _dynparams_add(handle, tar);

		if(dynparam)
		{
			dynparam->writable = true;
		}
	}
	else if(prop == handle->props.urid.patch_readable)
	{
		const LV2_URID tar = ((const LV2_Atom_URID *)(atom))->body; //FIXME check
		dynparam_t *dynparam = _dynparams_add(handle, tar);

		if(dynparam)
		{
			dynparam->writable = false;
		}
	}
	else if(subj)
	{
		dynparam_t *dynparam = _dynparams_get(handle, subj);

		if(dynparam)
		{
			if(prop == handle->urid_rdfs_range)
			{
				const LV2_URID tar = ((const LV2_Atom_URID *)(atom))->body; //FIXME check
				dynparam->range = tar;
			}
			else if(prop == handle->urid_rdfs_label)
			{
				dynparam->label = realloc(dynparam->label, atom->size); //FIXME check
				memcpy(dynparam->label, LV2_ATOM_BODY_CONST(atom), atom->size);
			}
			else if(prop == handle->urid_rdfs_comment)
			{
				dynparam->comment = realloc(dynparam->comment, atom->size); //FIXME check
				memcpy(dynparam->comment, LV2_ATOM_BODY_CONST(atom), atom->size);
			}
			else if(prop == handle->urid_lv2_minimum)
			{
				dynparam->min = realloc(dynparam->min, atom->size); //FIXME check
				memcpy(dynparam->min, LV2_ATOM_BODY_CONST(atom), atom->size);
			}
			else if(prop == handle->urid_lv2_maximum)
			{
				dynparam->max = realloc(dynparam->max, atom->size); //FIXME check
				memcpy(dynparam->max, LV2_ATOM_BODY_CONST(atom), atom->size);
			}
			else if(prop == handle->urid_units_symbol)
			{
				dynparam->unit = realloc(dynparam->unit, atom->size); //FIXME check
				memcpy(dynparam->unit, LV2_ATOM_BODY_CONST(atom), atom->size);
			}
			else if(prop == handle->urid_units_unit)
			{
				const LV2_URID tar = ((const LV2_Atom_URID *)(atom))->body; //FIXME check
				const char *symbol = NULL;

				//TODO use binary lookup
				if(tar == handle->urid_units_bar)
					symbol = "bars";
				else if(tar == handle->urid_units_beat)
					symbol = "beats";
				else if(tar == handle->urid_units_bpm)
					symbol = "BPM";
				else if(tar == handle->urid_units_cent)
					symbol = "ct";
				else if(tar == handle->urid_units_cm)
					symbol = "cm";
				else if(tar == handle->urid_units_db)
					symbol = "dB";
				else if(tar == handle->urid_units_degree)
					symbol = "deg";
				else if(tar == handle->urid_units_frame)
					symbol = "frames";
				else if(tar == handle->urid_units_hz)
					symbol = "Hz";
				else if(tar == handle->urid_units_inch)
					symbol = "in";
				else if(tar == handle->urid_units_khz)
					symbol = "kHz";
				else if(tar == handle->urid_units_km)
					symbol = "km";
				else if(tar == handle->urid_units_m)
					symbol = "m";
				else if(tar == handle->urid_units_mhz)
					symbol = "MHz";
				else if(tar == handle->urid_units_midiNote)
					symbol = "note";
				else if(tar == handle->urid_units_mile)
					symbol = "mi";
				else if(tar == handle->urid_units_min)
					symbol = "min";
				else if(tar == handle->urid_units_mm)
					symbol = "mm";
				else if(tar == handle->urid_units_ms)
					symbol = "ms";
				else if(tar == handle->urid_units_oct)
					symbol = "oct";
				else if(tar == handle->urid_units_pc)
					symbol = "%";
				else if(tar == handle->urid_units_s)
					symbol = "s";
				else if(tar == handle->urid_units_semitone12TET)
					symbol = "semi";

				if(symbol)
				{
					const size_t symbol_len = strlen(symbol) + 1;

					dynparam->unit = realloc(dynparam->unit, symbol_len); //FIXME check
					memcpy(dynparam->unit, symbol, symbol_len);
				}
			}
			else if(prop == handle->urid_lv2_scalePoint)
			{
				const LV2_Atom_Tuple *points = (const LV2_Atom_Tuple *)atom;

				const size_t points_len = lv2_atom_total_size(&points->atom);
				dynparam->points = realloc(dynparam->points, points_len); //FIXME check
				memcpy(dynparam->points, points, points_len);
			}
			//TODO more attributes
		}
	}
}

static void
_dyn_prop_rem(plughandle_t *handle, LV2_URID subj, LV2_URID prop,
	const LV2_Atom *atom)
{
	if(!atom || (atom->type != handle->props.urid.atom_urid) )
	{
		return;
	}

	const LV2_URID tar = ((const LV2_Atom_URID *)(atom))->body;

	if(prop == handle->props.urid.patch_writable)
	{
		if(tar == handle->props.urid.patch_wildcard)
		{
			_dynparams_clr(handle); //FIXME seperate for writables
		}
		else
		{
			//TODO
		}
	}
	else if(prop == handle->props.urid.patch_readable)
	{
		if(tar == handle->props.urid.patch_wildcard)
		{
			_dynparams_clr(handle); //FIXME seperate for readables
		}
		else
		{
			//TODO
		}
	}
	else if(subj)
	{
		dynparam_t *dynparam = _dynparams_get(handle, subj);

		if(dynparam)
		{
			//TODO
		}
	}
}

static void
_dyn_prop_set(plughandle_t *handle, LV2_URID subj, LV2_URID prop,
	const LV2_Atom *atom)
{
	dynparam_t *dynparam = _dynparams_get(handle, prop);

	if(!dynparam)
	{
		return; //TODO log
	}

	if(dynparam->range != atom->type)
	{
		return; //TODO log
	}

	dynparam->size = atom->size;
	dynparam->val = realloc(dynparam->val, atom->size); //FIXME check
	memcpy(dynparam->val, LV2_ATOM_BODY_CONST(atom), atom->size);
}

static void
_dyn_prop(void *data, props_dyn_ev_t ev, LV2_URID subj, LV2_URID prop,
	const LV2_Atom *atom)
{
	plughandle_t *handle = data;

#if 0
	const char *subject = handle->unmap->unmap(handle->unmap->handle, subj);
	const char *property = handle->unmap->unmap(handle->unmap->handle, prop);
	const char *range = handle->unmap->unmap(handle->unmap->handle, atom->type);

	lv2_log_note(&handle->logger, "[%s] (%u) <%s> <%s> <%s>", __func__,
		ev, subject, property, range);
#endif

	switch(ev)
	{
		case PROPS_DYN_EV_ADD:
		{
			_dyn_prop_add(handle, subj, prop, atom);
		} break;
		case PROPS_DYN_EV_REM:
		{
			_dyn_prop_rem(handle, subj, prop, atom);
		} break;
		case PROPS_DYN_EV_SET:
		{
			_dyn_prop_set(handle, subj, prop, atom);
		} break;
	}
}

static const props_dyn_t dyn = {
	.prop = _dyn_prop
};

static void
_message_set_key(plughandle_t *handle, LV2_URID key)
{
	ser_atom_t ser;
	props_impl_t *impl = _props_impl_get(&handle->props, key);
	if(!impl)
	{
		return;
	}

	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	LV2_Atom_Forge_Ref ref = 1;

	props_set(&handle->props, &handle->forge, 0, key, &ref);

	const LV2_Atom_Event *ev = (const LV2_Atom_Event *)ser_atom_get(&ser);
	const LV2_Atom *atom = &ev->body;
	handle->writer(handle->controller, handle->control, lv2_atom_total_size(atom),
		handle->atom_eventTransfer, atom);

	ser_atom_deinit(&ser);
}

static void
_message_set_dynparam(plughandle_t *handle, const dynparam_t *dynparam)
{
	ser_atom_t ser;

	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	LV2_Atom_Forge_Frame frame;

	lv2_atom_forge_object(&handle->forge, &frame, 0, handle->props.urid.patch_set);
	lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_subject);
	lv2_atom_forge_urid(&handle->forge, handle->props.urid.subject);
	lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_property);
	lv2_atom_forge_urid(&handle->forge, dynparam->prop);
	lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_value);
	lv2_atom_forge_atom(&handle->forge, dynparam->size, dynparam->range);
	lv2_atom_forge_write(&handle->forge, dynparam->val, dynparam->size);
	lv2_atom_forge_pop(&handle->forge, &frame);


	const LV2_Atom *atom = (const LV2_Atom *)ser_atom_get(&ser);
	handle->writer(handle->controller, handle->control, lv2_atom_total_size(atom),
		handle->atom_eventTransfer, atom);

	ser_atom_deinit(&ser);
}

static void
_message_get(plughandle_t *handle, LV2_URID key)
{
	ser_atom_t ser;
	props_impl_t *impl = _props_impl_get(&handle->props, key);
	if(!impl)
	{
		return;
	}

	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	LV2_Atom_Forge_Ref ref = 1;

	props_get(&handle->props, &handle->forge, 0, key, &ref);

	const LV2_Atom_Event *ev = (const LV2_Atom_Event *)ser_atom_get(&ser);
	const LV2_Atom *atom = &ev->body;
	handle->writer(handle->controller, handle->control, lv2_atom_total_size(atom),
		handle->atom_eventTransfer, atom);

	ser_atom_deinit(&ser);
}

static void
_expose_header(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	const d2tk_coord_t frac [3] = { 1, 1, 1 };
	D2TK_BASE_LAYOUT(rect, 3, frac, D2TK_FLAG_LAYOUT_X_REL, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				d2tk_base_label(base, -1, "Open•Music•Kontrollers", 0.5f, lrect,
					D2TK_ALIGN_LEFT | D2TK_ALIGN_TOP);
			} break;
			case 1:
			{
				d2tk_base_label(base, -1, "M•O•O•N•Y", 1.f, lrect,
					D2TK_ALIGN_CENTER | D2TK_ALIGN_TOP);
			} break;
			case 2:
			{
				d2tk_base_label(base, -1, "Version "MOONY_VERSION, 0.5f, lrect,
					D2TK_ALIGN_RIGHT | D2TK_ALIGN_TOP);
			} break;
		}
	}
}

#if 0
static void
_expose_vkb(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	static const uint8_t off_black [7] = {
		0, 1, 3, 0, 6, 8, 10,
	};
	static const uint8_t off_white [7] = {
		0, 2, 4, 5, 7, 9, 11
	};

	D2TK_BASE_TABLE(rect, 48, 2, D2TK_FLAG_TABLE_REL, tab)
	{
		const unsigned x = d2tk_table_get_index_x(tab);
		const unsigned y = d2tk_table_get_index_y(tab);
		const unsigned k = d2tk_table_get_index(tab);
		const d2tk_rect_t *trect = d2tk_table_get_rect(tab);

		switch(y)
		{
			case 0:
			{
				d2tk_rect_t trect2 = *trect;
				trect2.x -= trect2.w/2;

				switch(x%7)
				{
					case 0:
					{
						char buf [32];
						const ssize_t len = snprintf(buf, sizeof(buf), "%+i", x/7);

						d2tk_base_label(base, len, buf, 0.35f, &trect2,
							D2TK_ALIGN_TOP| D2TK_ALIGN_RIGHT);
					} break;
					case 1:
						// fall-through
					case 2:
						// fall-through
					case 4:
						// fall-through
					case 5:
						// fall-through
					case 6:
						// fall-through
					{
						const d2tk_state_t state = d2tk_base_button(base, D2TK_ID_IDX(k),
							&trect2);

						if(d2tk_state_is_down(state))
						{
							const uint8_t note = x/7*12 + off_black[x%7];

							_message_midi_note(handle, 0x0, LV2_MIDI_MSG_NOTE_ON, note, 0x7f);
						}
						if(d2tk_state_is_up(state))
						{
							const uint8_t note = x/7*12 + off_black[x%7];

							_message_midi_note(handle, 0x0, LV2_MIDI_MSG_NOTE_OFF, note, 0x0);
						}
					} break;
				}
			} break;
			case 1:
			{
				const d2tk_state_t state = d2tk_base_button(base, D2TK_ID_IDX(k), trect);

				if(d2tk_state_is_down(state))
				{
					const uint8_t note = x/7*12 + off_white[x%7];

					_message_midi_note(handle, 0x0, LV2_MIDI_MSG_NOTE_ON, note, 0x7f);
				}
				if(d2tk_state_is_up(state))
				{
					const uint8_t note = x/7*12 + off_white[x%7];

					_message_midi_note(handle, 0x0, LV2_MIDI_MSG_NOTE_OFF, note, 0x0);
				}
			} break;
		}
	}
}
#endif

static void
_expose_font_height(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);
	
	static const char lbl [] = "font-height•px";

	if(d2tk_base_spinner_int32_is_changed(base, D2TK_ID, rect,
		sizeof(lbl), lbl, 10, &handle->state.font_height, 25, D2TK_FLAG_NONE))
	{
		_message_set_key(handle, handle->urid_fontHeight);
		_update_font_height(handle);
	}
}

static void
_expose_panic(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	static const char path [] = "alert-triangle.png";
	static const char tip [] = "panic";

	const d2tk_state_t state = d2tk_base_button_image(base, D2TK_ID,
		sizeof(path), path, rect);

	if(d2tk_state_is_changed(state))
	{
		handle->state.panic = 1;
		_message_set_key(handle, handle->urid_panic);
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip), tip, handle->tip_height);
	}
}

/* list of tested console editors:
 *
 * e3
 * joe
 * nano
 * vi
 * vis
 * vim
 * neovim
 * emacs
 * zile
 * mg
 * kakoune
 */

/* list of tested graphical editors:
 *
 * acme
 * adie
 * beaver
 * deepin-editor
 * gedit         (does not work properly)
 * gobby
 * howl
 * jedit         (does not work properly)
 * xed           (does not work properly)
 * leafpad
 * mousepad
 * nedit
 * notepadqq
 * pluma         (does not work properly)
 * sublime3      (needs to be started with -w)
 */

static void
_expose_term(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	char *editor = getenv("EDITOR");

	char *args [] = {
		editor ? editor : "vi",
		handle->template,
		NULL
	};

	D2TK_BASE_PTY(base, D2TK_ID, args,
		handle->font_height, rect, handle->reinit, pty)
	{
		const d2tk_state_t state = d2tk_pty_get_state(pty);
		const uint32_t max_red = d2tk_pty_get_max_red(pty);

		if(max_red != handle->max_red)
		{
			handle->max_red = max_red;
			d2tk_frontend_redisplay(handle->dpugl);
		}

		if(d2tk_state_is_close(state))
		{
			handle->done = 1;
		}
	}

	handle->reinit = false;
}

static unsigned
_num_lines(const char *err)
{
	unsigned nlines = 1;

	for(const char *from = err, *to = strchr(from, '\n');
		to;
		from = &to[1],
		to = strchr(from, '\n'))
	{
		nlines += 1;
	}

	return nlines;
}

static void
_expose_error(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	const char *from = handle->state.error;
	const unsigned nlines = _num_lines(from);

	//FIXME wrap in scroll widget
	D2TK_BASE_TABLE(rect, rect->w, handle->font_height, D2TK_FLAG_TABLE_ABS, tab)
	{
		const d2tk_rect_t *trect = d2tk_table_get_rect(tab);
		const unsigned k = d2tk_table_get_index(tab);

		if(k >= nlines)
		{
			break;
		}

		const char *to = strchr(from, '\n');
		const size_t len = to ? to - from : -1;

		d2tk_base_label(base, len, from, 1.f, trect,
			D2TK_ALIGN_LEFT | D2TK_ALIGN_MIDDLE);

		from = &to[1];
	}
}

static void
_expose_editor(plughandle_t *handle, const d2tk_rect_t *rect)
{
	const size_t err_len = strlen(handle->state.error);
	const unsigned n = err_len > 0 ? 2 : 1;
	const d2tk_coord_t frac [2] = { 2, 1 };
	D2TK_BASE_LAYOUT(rect, n, frac, D2TK_FLAG_LAYOUT_Y_REL, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				_expose_term(handle, lrect);
			} break;
			case 1:
			{
				_expose_error(handle, lrect);
			} break;
		}
	}
}

static void
_aspect_correction(d2tk_rect_t *rect, float aspect_ratio)
{
	const d2tk_coord_t lon = rect->w >= rect->h
		? rect->w
		: rect->h;
	const d2tk_coord_t sho = rect->w < rect->h
		? rect->w
		: rect->h;

	if(aspect_ratio <= 0.f)
	{
		aspect_ratio = 1.f;
	}

	if(aspect_ratio < 1.f)
	{
		rect->w = sho * aspect_ratio;
		rect->h = sho;
	}
	else if(aspect_ratio == 1.f)
	{
		rect->w = sho;
		rect->h = sho;
	}
	else //if(aspect_ratio > 1.f)
	{
		rect->w = lon;
		rect->h = lon / aspect_ratio;
	}
}

static void
_render_graph(void *_ctx, const d2tk_rect_t *rect, const void *data)
{
	plughandle_t *handle = (plughandle_t *)data;
	NVGcontext *ctx = _ctx;

	nvgSave(ctx);
	nvgTranslate(ctx, -rect->x, -rect->y);
	nvgScale(ctx, rect->w, rect->h);

	lv2_canvas_render_body(&handle->canvas, ctx, handle->forge.Tuple,
		handle->graph_size, (const LV2_Atom *)handle->state.graph_body);

	nvgRestore(ctx);
}

static void
_expose_canvas_graph(plughandle_t *handle, const d2tk_rect_t *_rect)
{
	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);

	if(handle->graph_size == 0)
	{
		return;
	}

	d2tk_rect_t rect = *_rect;
	_aspect_correction(&rect, handle->state.aspect_ratio);

	const uint64_t dhash = d2tk_hash(handle->state.graph_body, handle->graph_size);

	d2tk_base_custom(base, dhash, handle, &rect, _render_graph);

	const d2tk_state_t state = d2tk_base_is_active_hot(base, D2TK_ID, &rect,
		D2TK_FLAG_SCROLL);

	// user mouse feedback
	if(d2tk_state_is_over(state))
	{
		if(!handle->state.mouse_focus)
		{
			handle->state.mouse_focus = true;
			_message_set_key(handle, handle->canvas.urid.Canvas_mouseFocus);
		}

		d2tk_coord_t mx, my;
		d2tk_base_get_mouse_pos(base, &mx, &my);

		double x = (double)(mx - rect.x) / rect.w;
		double y = (double)(my - rect.y) / rect.h;

		d2tk_clip_double(0.0, &x, 1.0);
		d2tk_clip_double(0.0, &y, 1.0);

		if(x != handle->state.mouse_position_x)
		{
			handle->state.mouse_position_x = x;
			_message_set_key(handle, handle->canvas.urid.Canvas_mousePositionX);
		}

		if(y != handle->state.mouse_position_y)
		{
			handle->state.mouse_position_y = y;
			_message_set_key(handle, handle->canvas.urid.Canvas_mousePositionY);
		}
	}
	else
	{
		if(handle->state.mouse_focus)
		{
			handle->state.mouse_focus = false;
			_message_set_key(handle, handle->canvas.urid.Canvas_mouseFocus);
		}
	}

	if(d2tk_state_is_scroll_up(state))
	{
		handle->state.mouse_wheel_y = -1.0;
		_message_set_key(handle, handle->canvas.urid.Canvas_mouseWheelY);
	}
	if(d2tk_state_is_scroll_down(state))
	{
		handle->state.mouse_wheel_y = +1.0;
		_message_set_key(handle, handle->canvas.urid.Canvas_mouseWheelY);
	}

	if(d2tk_state_is_scroll_left(state))
	{
		handle->state.mouse_wheel_x = -1.0;
		_message_set_key(handle, handle->canvas.urid.Canvas_mouseWheelX);
	}
	if(d2tk_state_is_scroll_right(state))
	{
		handle->state.mouse_wheel_x = +1.0;
		_message_set_key(handle, handle->canvas.urid.Canvas_mouseWheelX);
	}

	if(d2tk_state_is_down(state))
	{
		handle->state.mouse_button_left = true;
		_message_set_key(handle, handle->canvas.urid.Canvas_mouseButtonLeft);
	}
	if(d2tk_state_is_up(state))
	{
		handle->state.mouse_button_left = false;
		_message_set_key(handle, handle->canvas.urid.Canvas_mouseButtonLeft);
	}
}

static void
_expose_graph_minimize(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);

	static const char *path [2] = { "eye-off.png", "eye.png" };
	static const char tip [2][5] = { "show", "hide" };

	bool visible = !handle->state.graph_hidden;
	const d2tk_state_t state = d2tk_base_toggle_label_image(base, D2TK_ID,
		0, NULL, D2TK_ALIGN_CENTERED, -1, path[visible], rect, &visible);

	if(d2tk_state_is_changed(state))
	{
		handle->state.graph_hidden = !handle->state.graph_hidden;
		_message_set_key(handle, handle->urid_graphHidden);
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip[visible]), tip[visible], handle->tip_height);
	}
}

static void
_expose_manual(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	static const char path [] = "info.png";
	static const char tip [] = "open manual";

	if(!handle->request_code)
	{
		return;
	}

	const d2tk_state_t state = d2tk_base_button_image(base, D2TK_ID,
		sizeof(path), path, rect);

	if(d2tk_state_is_changed(state))
	{
		char *argv [] = {
			"xdg-open",
			handle->manual,
			NULL
		};

		d2tk_util_kill(&handle->kid);
		handle->kid = d2tk_util_spawn(argv);
		if(handle->kid <= 0)
		{
			lv2_log_error(&handle->logger, "[%s] failed to spawn: %s '%s'", __func__,
				argv[0], argv[1]);
		}
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip), tip, handle->tip_height);
	}

	d2tk_util_wait(&handle->kid);
}

static void
_expose_graph_footer(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);

	const d2tk_coord_t frac [3] = {
		0, rect->h, rect->h
	};
	D2TK_BASE_LAYOUT(rect, 3, frac, D2TK_FLAG_LAYOUT_X_ABS, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				static char lbl [] = "canvas•graph";

				d2tk_base_label(base, sizeof(lbl), lbl, 0.5f, lrect,
					D2TK_ALIGN_LEFT | D2TK_ALIGN_MIDDLE);
			} break;
			case 1:
			{
				_expose_manual(handle, lrect);
			} break;
			case 2:
			{
				_expose_graph_minimize(handle, lrect);
			} break;
		}
	}
}

static void
_expose_left(plughandle_t *handle, const d2tk_rect_t *rect)
{
	const d2tk_coord_t frac [2] = {
		handle->state.graph_hidden ? 1 : 0,
		handle->state.editor_hidden ? 1 : 0
	};
	D2TK_BASE_LAYOUT(rect, 2, frac, D2TK_FLAG_LAYOUT_Y_ABS, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				if(!handle->state.graph_hidden)
				{
					_expose_canvas_graph(handle, lrect);
				}
			} break;
			case 1:
			{
				if(!handle->state.editor_hidden)
				{
					_expose_editor(handle, lrect);
				}
			} break;
		}
	}
}

static d2tk_flag_t
_dynparam_flag(dynparam_t *dynparam)
{
	return dynparam->writable
		? D2TK_FLAG_NONE
		: D2TK_FLAG_INACTIVE;
}

static double
_body_as_double(plughandle_t *handle, const void *data, LV2_URID range)
{
	if(range == handle->forge.Bool)
	{
		return *(const int32_t *)data;
	}
	else if(range == handle->forge.Int)
	{
		return *(const int32_t *)data;
	}
	else if(range == handle->forge.URID)
	{
		return *(const uint32_t*)data;
	}
	else if(range == handle->forge.Long)
	{
		return *(const int64_t *)data;
	}
	else if(range == handle->forge.Float)
	{
		return *(const float *)data;
	}
	else if(range == handle->forge.Double)
	{
		return *(const double *)data;
	}

	return 0.0;
}

static void
_expose_slot_enum(plughandle_t *handle, dynparam_t *dynparam,
	const d2tk_rect_t *rect, size_t lbl_len, const char *lbl, unsigned k)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	if(!dynparam->val)
	{
		return;
	}

	d2tk_base_label(base, lbl_len, lbl, 0.5f, rect, D2TK_ALIGN_CENTERED);

	// derive item number
	size_t nitms = 0;
	LV2_ATOM_TUPLE_FOREACH(dynparam->points, itm)
	{
		nitms++;
	}

	// allocate item label string array
	const char **itms = alloca(nitms * sizeof(char *));

	// initialize array index
	int32_t idx = 0;
	double lst_val = _body_as_double(handle, dynparam->val, dynparam->range);

	// fill item label string array
	nitms = 0;
	LV2_ATOM_TUPLE_FOREACH(dynparam->points, itm)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)itm;
		const LV2_Atom *label = NULL;
		const LV2_Atom *value = NULL;

		lv2_atom_object_get(obj,
			handle->urid_rdfs_label, &label,
			handle->urid_rdf_value, &value,
			0);

		const double itm_val = _body_as_double(handle, LV2_ATOM_BODY_CONST(value),
			dynparam->range);

		if(itm_val <= lst_val)
		{
			idx = nitms;
		}

		itms[nitms++] = LV2_ATOM_BODY_CONST(label);
	}

	const d2tk_state_t state = d2tk_base_combo(base, D2TK_ID_IDX(k),
		nitms, itms, rect, &idx);

	if(d2tk_state_is_changed(state))
	{
		// set current value to tuple index value
		nitms = 0;
		LV2_ATOM_TUPLE_FOREACH(dynparam->points, itm)
		{
			const LV2_Atom_Object *obj = (const LV2_Atom_Object *)itm;

			if(nitms++ != idx)
			{
				continue;
			}

			const LV2_Atom *value = NULL;

			lv2_atom_object_get(obj,
				handle->urid_rdf_value, &value,
				0);

			memcpy(dynparam->val, LV2_ATOM_BODY_CONST(value), dynparam->size);

			break;
		}

		_message_set_dynparam(handle, dynparam);
	}
	if(d2tk_state_is_over(state) && dynparam->comment)
	{
		d2tk_base_set_tooltip(base, -1, dynparam->comment, handle->tip_height);
	}
}

static void
_expose_slot_bool(plughandle_t *handle, dynparam_t *dynparam,
	const d2tk_rect_t *rect, size_t lbl_len, const char *lbl, unsigned k)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	if(!dynparam->val)
	{
		return;
	}

	bool val = *(int32_t *)dynparam->val;

	const d2tk_state_t state = d2tk_base_spinner_bool(base,
		D2TK_ID_IDX(k), rect, lbl_len, lbl, &val, _dynparam_flag(dynparam));

	if(d2tk_state_is_changed(state))
	{
		*(int32_t *)dynparam->val = val;
		_message_set_dynparam(handle, dynparam);
	}
	if(d2tk_state_is_over(state) && dynparam->comment)
	{
		d2tk_base_set_tooltip(base, -1, dynparam->comment, handle->tip_height);
	}
}

static void
_expose_slot_int(plughandle_t *handle, dynparam_t *dynparam,
	const d2tk_rect_t *rect, size_t lbl_len, const char *lbl, unsigned k)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	if(!dynparam->val || !dynparam->min || !dynparam->max)
	{
		return;
	}

	const int32_t min = *(int32_t *)dynparam->min;
	const int32_t max = *(int32_t *)dynparam->max;
	int32_t *val = dynparam->val;

	const d2tk_state_t state = d2tk_base_spinner_int32(base,
		D2TK_ID_IDX(k), rect, lbl_len, lbl, min, val, max, _dynparam_flag(dynparam));

	if(d2tk_state_is_changed(state))
	{
		_message_set_dynparam(handle, dynparam);
	}
	if(d2tk_state_is_over(state) && dynparam->comment)
	{
		d2tk_base_set_tooltip(base, -1, dynparam->comment, handle->tip_height);
	}
}

static void
_expose_slot_long(plughandle_t *handle, dynparam_t *dynparam,
	const d2tk_rect_t *rect, size_t lbl_len, const char *lbl, unsigned k)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	if(!dynparam->val || !dynparam->min || !dynparam->max)
	{
		return;
	}

	const int64_t min = *(int64_t *)dynparam->min;
	const int64_t max = *(int64_t *)dynparam->max;
	int64_t *val = dynparam->val;

	const d2tk_state_t state = d2tk_base_spinner_int64(base,
		D2TK_ID_IDX(k), rect, lbl_len, lbl, min, val, max, _dynparam_flag(dynparam));

	if(d2tk_state_is_changed(state))
	{
		_message_set_dynparam(handle, dynparam);
	}
	if(d2tk_state_is_over(state) && dynparam->comment)
	{
		d2tk_base_set_tooltip(base, -1, dynparam->comment, handle->tip_height);
	}
}

static void
_expose_slot_float(plughandle_t *handle, dynparam_t *dynparam,
	const d2tk_rect_t *rect, size_t lbl_len, const char *lbl, unsigned k)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	if(!dynparam->val || !dynparam->min || !dynparam->max)
	{
		return;
	}

	const float min = *(float *)dynparam->min;
	const float max = *(float *)dynparam->max;
	float *val = dynparam->val;

	const d2tk_state_t state = d2tk_base_spinner_float(base,
		D2TK_ID_IDX(k), rect, lbl_len, lbl, min, val, max, _dynparam_flag(dynparam));

	if(d2tk_state_is_changed(state))
	{
		_message_set_dynparam(handle, dynparam);
	}
	if(d2tk_state_is_over(state) && dynparam->comment)
	{
		d2tk_base_set_tooltip(base, -1, dynparam->comment, handle->tip_height);
	}
}

static void
_expose_slot_double(plughandle_t *handle, dynparam_t *dynparam,
	const d2tk_rect_t *rect, size_t lbl_len, const char *lbl, unsigned k)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	if(!dynparam->val || !dynparam->min || !dynparam->max)
	{
		return;
	}

	const double min = *(double *)dynparam->min;
	const double max = *(double *)dynparam->max;
	double *val = dynparam->val;

	const d2tk_state_t state = d2tk_base_spinner_double(base,
		D2TK_ID_IDX(k), rect, lbl_len, lbl, min, val, max, _dynparam_flag(dynparam));

	if(d2tk_state_is_changed(state))
	{
		_message_set_dynparam(handle, dynparam);
	}
	if(d2tk_state_is_over(state) && dynparam->comment)
	{
		d2tk_base_set_tooltip(base, -1, dynparam->comment, handle->tip_height);
	}
}

static void
_expose_slot(plughandle_t *handle, const d2tk_rect_t *rect, unsigned k)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	dynparam_t *dynparam = &handle->dynparams[k];

	if(!dynparam->label)
	{
		return;
	}

	char buf[256];
	const char *lbl;
	size_t lbl_len;

	if(dynparam->unit)
	{
		lbl_len = snprintf(buf, sizeof(buf), "%s•%s", dynparam->label, dynparam->unit);
		lbl = buf;
	}
	else
	{
		lbl_len = -1;
		lbl = dynparam->label;
	}

	if(dynparam->points)
	{
		_expose_slot_enum(handle, dynparam, rect, lbl_len, lbl, k);
	}
	else if(dynparam->range == handle->props.urid.atom_bool)
	{
		_expose_slot_bool(handle, dynparam, rect, lbl_len, lbl, k);
	}
	else if(dynparam->range == handle->props.urid.atom_int)
	{
		_expose_slot_int(handle, dynparam, rect, lbl_len, lbl, k);
	}
	else if(dynparam->range == handle->props.urid.atom_long)
	{
		_expose_slot_long(handle, dynparam, rect, lbl_len, lbl, k);
	}
	else if(dynparam->range == handle->props.urid.atom_float)
	{
		_expose_slot_float(handle, dynparam, rect, lbl_len, lbl, k);
	}
	else if(dynparam->range == handle->props.urid.atom_double)
	{
		_expose_slot_double(handle, dynparam, rect, lbl_len, lbl, k);
	}
	/*
	else if(dynparam->range == handle->props.urid.atom_urid)
	{
		//TODO
	}
	*/
	else
	{
		if(d2tk_base_button_label_is_changed(base, D2TK_ID_IDX(k),
			-1, dynparam->label, D2TK_ALIGN_LEFT | D2TK_ALIGN_MIDDLE, rect))
		{
			//TODO
		}
	}
}

static void
_expose_right(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	const uint32_t numv = rect->h / handle->item_height;
	const uint32_t max [2] = { 0, handle->ndynparams};
	const uint32_t num [2] = { 0, numv };
	D2TK_BASE_SCROLLBAR(base, rect, D2TK_ID, D2TK_FLAG_SCROLL_Y, max, num, vscroll)
	{
		const unsigned offy = d2tk_scrollbar_get_offset_y(vscroll);
		const d2tk_rect_t *vrect = d2tk_scrollbar_get_rect(vscroll);

		D2TK_BASE_TABLE(vrect, 1, numv,  D2TK_FLAG_TABLE_REL, tab)
		{
			const unsigned k = d2tk_table_get_index(tab) + offy;
			const d2tk_rect_t *trect = d2tk_table_get_rect(tab);

			if(k >= handle->ndynparams)
			{
				break;
			}

			_expose_slot(handle, trect, k);
		}
	}
}

static void
_expose_body(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);

	const d2tk_coord_t frac [3] = { 0, 5, handle->sidebar_width };
	D2TK_BASE_LAYOUT(rect, 3, frac, D2TK_FLAG_LAYOUT_X_ABS, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				_expose_left(handle, lrect);
			} break;
			case 1:
			{
				d2tk_base_separator(base, lrect, D2TK_FLAG_SEPARATOR_X);
			} break;
			case 2:
			{
				_expose_right(handle, lrect);
			} break;
		}
	}
}

static const char *
_text_basename(plughandle_t *handle)
{
	return basename(handle->template);
}

static void
_expose_text_link(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);

	static const char tip [] = "open externally";
	char lbl [PATH_MAX];
	const size_t lbl_len = snprintf(lbl, sizeof(lbl), "%s",
		_text_basename(handle));

	const d2tk_state_t state = d2tk_base_link(base, D2TK_ID, lbl_len, lbl, .5f,
		rect, D2TK_ALIGN_MIDDLE | D2TK_ALIGN_LEFT);

	if(d2tk_state_is_changed(state))
	{
		char *argv [] = {
			"xdg-open",
			handle->template,
			NULL
		};

		d2tk_util_kill(&handle->kid);
		handle->kid = d2tk_util_spawn(argv);
		if(handle->kid <= 0)
		{
			lv2_log_error(&handle->logger, "[%s] failed to spawn: %s '%s'", __func__,
				argv[0], argv[1]);
		}
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip), tip, handle->tip_height);
	}

	d2tk_util_wait(&handle->kid);
}

static void
_update_code(plughandle_t *handle, const char *txt, size_t txt_len)
{
	ser_atom_t ser;
	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	lv2_atom_forge_string(&handle->forge, txt, txt_len);

	const LV2_Atom *atom = ser_atom_get(&ser);

	props_impl_t *impl = _props_impl_get(&handle->props, handle->urid_code);
	_props_impl_set(&handle->props, impl, atom->type, atom->size,
		LV2_ATOM_BODY_CONST(atom));

	ser_atom_deinit(&ser);

	_message_set_key(handle, handle->urid_code);
}

static void
_expose_text_clear(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	static const char path [] = "delete.png";
	static const char none [] = "";
	static const char tip [] = "clear";

	const d2tk_state_t state = d2tk_base_button_image(base, D2TK_ID,
		sizeof(path), path, rect);

	if(d2tk_state_is_changed(state))
	{
		_update_code(handle, none, sizeof(none) - 1);
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip), tip, handle->tip_height);
	}
}

static void
_expose_text_copy(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	static const char path [] = "copy.png";
	static const char tip [] = "copy to clipboard";

	const d2tk_state_t state = d2tk_base_button_image(base, D2TK_ID,
		sizeof(path), path, rect);

	if(d2tk_state_is_changed(state))
	{
		d2tk_frontend_set_clipboard(dpugl, "UTF8_STRING",
			handle->state.code, strlen(handle->state.code) + 1);
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip), tip, handle->tip_height);
	}
}

static void
_expose_text_paste(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	static const char path [] = "clipboard.png";
	static const char tip [] = "paste from clipboard";

	const d2tk_state_t state = d2tk_base_button_image(base, D2TK_ID,
		sizeof(path), path, rect);

	if(d2tk_state_is_changed(state))
	{
		size_t txt_len = 0;
		const char *mime = "UTF8_STRING";
		const char *txt = d2tk_frontend_get_clipboard(dpugl, &mime, &txt_len);

		if(txt && txt_len && mime && !strcmp(mime, "UTF8_STRING"))
		{
			_update_code(handle, txt, txt_len);
		}
		else
		{
			lv2_log_error(&handle->logger, "[%s] failed to paste text: %s", __func__,
				mime);
		}
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip), tip, handle->tip_height);
	}
}

#ifdef _LV2_HAS_REQUEST_VALUE
static void
_expose_text_load(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_frontend_t *dpugl = handle->dpugl;
	d2tk_base_t *base = d2tk_frontend_get_base(dpugl);

	static const char path [] = "save.png";
	static const char tip [] = "load from file";

	if(!handle->request_code)
	{
		return;
	}

	const d2tk_state_t state = d2tk_base_button_image(base, D2TK_ID,
		sizeof(path), path, rect);

	if(d2tk_state_is_changed(state))
	{
		const LV2_URID key = handle->urid_code;
		const LV2_URID type = handle->forge.String;

		const LV2UI_Request_Value_Status status = handle->request_code->request(
			handle->request_code->handle, key, type, NULL);

		if(  (status != LV2UI_REQUEST_VALUE_SUCCESS)
			&& (status != LV2UI_REQUEST_VALUE_BUSY) )
		{
			lv2_log_error(&handle->logger, "[%s] requestValue failed: %i", __func__, status);

			if(status == LV2UI_REQUEST_VALUE_ERR_UNSUPPORTED)
			{
				handle->request_code = NULL;
			}
		}
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip), tip, handle->tip_height);
	}
}
#endif

static void
_expose_text_minimize(plughandle_t *handle, const d2tk_rect_t *rect)
{
	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);

	static const char *path [2] = { "eye-off.png", "eye.png" };
	static const char tip [2][5] = { "show", "hide" };

	bool visible = !handle->state.editor_hidden;
	const d2tk_state_t state = d2tk_base_toggle_label_image(base, D2TK_ID,
		0, NULL, D2TK_ALIGN_CENTERED, -1, path[visible], rect, &visible);

	if(d2tk_state_is_changed(state))
	{
		handle->state.editor_hidden = !handle->state.editor_hidden;
		_message_set_key(handle, handle->urid_editorHidden);
	}
	if(d2tk_state_is_over(state))
	{
		d2tk_base_set_tooltip(base, sizeof(tip[visible]), tip[visible], handle->tip_height);
	}
}

static void
_expose_text_footer(plughandle_t *handle, const d2tk_rect_t *rect)
{
	const d2tk_coord_t frac [8] = {
		rect->h, 0, 0, rect->h, rect->h, rect->h, rect->h, rect->h
	};
	D2TK_BASE_LAYOUT(rect, 8, frac, D2TK_FLAG_LAYOUT_X_ABS, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				_expose_panic(handle, lrect);
			} break;
			case 1:
			{
				_expose_text_link(handle, lrect);
			} break;
			case 2:
			{
				_expose_font_height(handle, lrect);
			} break;
			case 3:
			{
				_expose_text_clear(handle, lrect);
			} break;
			case 4:
			{
				_expose_text_copy(handle, lrect);
			} break;
			case 5:
			{
				_expose_text_paste(handle, lrect);
			} break;
			case 6:
			{
#ifdef _LV2_HAS_REQUEST_VALUE
				_expose_text_load(handle, lrect);
#endif
			} break;
			case 7:
			{
				_expose_text_minimize(handle, lrect);
			} break;
		}
	}
}

static int
_expose(void *data, d2tk_coord_t w, d2tk_coord_t h)
{
	plughandle_t *handle = data;
	d2tk_base_t *base = d2tk_frontend_get_base(handle->dpugl);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, w, h);

	const d2tk_style_t *old_style = d2tk_base_get_style(base);
	d2tk_style_t style = *old_style;
	const uint32_t light = handle->max_red;
	const uint32_t dark = (light & ~0xff) | 0x7f;
	style.fill_color[D2TK_TRIPLE_ACTIVE]           = dark;
	style.fill_color[D2TK_TRIPLE_ACTIVE_HOT]       = light;
	style.fill_color[D2TK_TRIPLE_ACTIVE_FOCUS]     = dark;
	style.fill_color[D2TK_TRIPLE_ACTIVE_HOT_FOCUS] = light;
	style.text_stroke_color[D2TK_TRIPLE_HOT]       = light;
	style.text_stroke_color[D2TK_TRIPLE_HOT_FOCUS] = light;

	d2tk_base_set_style(base, &style);

	const d2tk_coord_t frac [4] = {
		handle->header_height,
		handle->footer_height,
		0,
		handle->footer_height
	};
	D2TK_BASE_LAYOUT(&rect, 4, frac, D2TK_FLAG_LAYOUT_Y_ABS, lay)
	{
		const unsigned k = d2tk_layout_get_index(lay);
		const d2tk_rect_t *lrect = d2tk_layout_get_rect(lay);

		switch(k)
		{
			case 0:
			{
				_expose_header(handle, lrect);
			} break;
			case 1:
			{
				_expose_graph_footer(handle, lrect);
			} break;
			case 2:
			{
				_expose_body(handle, lrect);
			} break;
			case 3:
			{
				_expose_text_footer(handle, lrect);
			} break;
		}
	}

	d2tk_base_set_style(base, old_style);

	return 0;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor,
	const char *plugin_uri, const char *bundle_path,
	LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	void *parent = NULL;
	LV2UI_Resize *host_resize = NULL;
	LV2UI_Port_Map *port_map = NULL;
	LV2_Options_Option *opts = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_UI__parent))
		{
			parent = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
		{
			host_resize = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_URID__map))
		{
			handle->map = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
		{
			handle->unmap = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
		{
			handle->log = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
		{
			port_map = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_OPTIONS__options))
		{
			opts = features[i]->data;
		}
#ifdef _LV2_HAS_REQUEST_VALUE
		else if(!strcmp(features[i]->URI, LV2_UI__requestValue))
		{
			handle->request_code = features[i]->data;
		}
#endif
	}

	if(!parent)
	{
		fprintf(stderr,
			"%s: Host does not support ui:parent\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	if(!handle->map)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map\n", descriptor->URI);
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

	if(handle->log)
	{
		lv2_log_logger_init(&handle->logger, handle->map, handle->log);
	}

	lv2_canvas_init(&handle->canvas, handle->map);

	handle->control = port_map->port_index(port_map->handle, "control");

	lv2_atom_forge_init(&handle->forge, handle->map);

	handle->atom_eventTransfer = handle->map->map(handle->map->handle,
		LV2_ATOM__eventTransfer);
	handle->midi_MidiEvent = handle->map->map(handle->map->handle,
		LV2_MIDI__MidiEvent);

	handle->urid_code = handle->map->map(handle->map->handle,
		MOONY_CODE_URI);
	handle->urid_error = handle->map->map(handle->map->handle,
		MOONY_ERROR_URI);
	handle->urid_fontHeight = handle->map->map(handle->map->handle,
		MOONY_FONT_HEIGHT_URI);
	handle->urid_panic = handle->map->map(handle->map->handle,
		MOONY_PANIC_URI);
	handle->urid_editorHidden = handle->map->map(handle->map->handle,
		MOONY_EDITOR_HIDDEN_URI);
	handle->urid_graphHidden = handle->map->map(handle->map->handle,
		MOONY_GRAPH_HIDDEN_URI);

	handle->urid_rdfs_label = handle->map->map(handle->map->handle,
		RDFS__label);
	handle->urid_rdfs_range= handle->map->map(handle->map->handle,
		RDFS__range);
	handle->urid_rdfs_comment = handle->map->map(handle->map->handle,
		RDFS__comment);
	handle->urid_rdf_value = handle->map->map(handle->map->handle,
		RDF__value);

	handle->urid_lv2_minimum = handle->map->map(handle->map->handle,
		LV2_CORE__minimum);
	handle->urid_lv2_maximum = handle->map->map(handle->map->handle,
		LV2_CORE__maximum);
	handle->urid_lv2_scalePoint = handle->map->map(handle->map->handle,
		LV2_CORE__scalePoint);

	handle->urid_units_symbol = handle->map->map(handle->map->handle, LV2_UNITS__symbol);
	handle->urid_units_unit = handle->map->map(handle->map->handle, LV2_UNITS__unit);
	handle->urid_units_bar = handle->map->map(handle->map->handle, LV2_UNITS__bar);
	handle->urid_units_beat = handle->map->map(handle->map->handle, LV2_UNITS__beat);
	handle->urid_units_bpm = handle->map->map(handle->map->handle, LV2_UNITS__bpm);
	handle->urid_units_cent = handle->map->map(handle->map->handle, LV2_UNITS__cent);
	handle->urid_units_cm = handle->map->map(handle->map->handle, LV2_UNITS__cm);
	handle->urid_units_db = handle->map->map(handle->map->handle, LV2_UNITS__db);
	handle->urid_units_degree = handle->map->map(handle->map->handle, LV2_UNITS__degree);
	handle->urid_units_frame = handle->map->map(handle->map->handle, LV2_UNITS__frame);
	handle->urid_units_hz = handle->map->map(handle->map->handle, LV2_UNITS__hz);
	handle->urid_units_inch = handle->map->map(handle->map->handle, LV2_UNITS__inch);
	handle->urid_units_khz = handle->map->map(handle->map->handle, LV2_UNITS__khz);
	handle->urid_units_km = handle->map->map(handle->map->handle, LV2_UNITS__km);
	handle->urid_units_m = handle->map->map(handle->map->handle, LV2_UNITS__m);
	handle->urid_units_mhz = handle->map->map(handle->map->handle, LV2_UNITS__mhz);
	handle->urid_units_midiNote = handle->map->map(handle->map->handle, LV2_UNITS__midiNote);
	handle->urid_units_mile = handle->map->map(handle->map->handle, LV2_UNITS__mile);
	handle->urid_units_min = handle->map->map(handle->map->handle, LV2_UNITS__min);
	handle->urid_units_mm = handle->map->map(handle->map->handle, LV2_UNITS__mm);
	handle->urid_units_ms = handle->map->map(handle->map->handle, LV2_UNITS__ms);
	handle->urid_units_oct = handle->map->map(handle->map->handle, LV2_UNITS__oct);
	handle->urid_units_pc = handle->map->map(handle->map->handle, LV2_UNITS__pc);
	handle->urid_units_s = handle->map->map(handle->map->handle, LV2_UNITS__s);
	handle->urid_units_semitone12TET = handle->map->map(handle->map->handle, LV2_UNITS__semitone12TET);

	if(!props_init(&handle->props, plugin_uri,
		defs, MAX_NPROPS, &handle->state, &handle->stash,
		handle->map, handle))
	{
		fprintf(stderr, "failed to initialize property structure\n");
		free(handle);
		return NULL;
	}

	props_dyn(&handle->props, &dyn);

	handle->controller = controller;
	handle->writer = write_function;

	const d2tk_coord_t w = 1024;
	const d2tk_coord_t h = 720;

	d2tk_pugl_config_t *config = &handle->config;
	config->parent = (uintptr_t)parent;
	config->bundle_path = bundle_path;
	config->min_w = w/2;
	config->min_h = h/2;
	config->w = w;
	config->h = h;
	config->fixed_size = false;
	config->fixed_aspect = false;
	config->expose = _expose;
	config->data = handle;

	handle->dpugl = d2tk_pugl_new(config, (uintptr_t *)widget);
	if(!handle->dpugl)
	{
		free(handle);
		return NULL;
	}

	const LV2_URID ui_scaleFactor = handle->map->map(handle->map->handle,
		LV2_UI__scaleFactor);

	for(LV2_Options_Option *opt = opts;
		opt && (opt->key != 0) && (opt->value != NULL);
		opt++)
	{
		if( (opt->key == ui_scaleFactor) && (opt->type == handle->forge.Float) )
		{
			handle->scale = *(float*)opt->value;
		}
	}

	if(handle->scale == 0.f)
	{
		handle->scale = d2tk_frontend_get_scale(handle->dpugl);
	}

	handle->scale = d2tk_frontend_get_scale(handle->dpugl);
	handle->header_height = 32 * handle->scale;
	handle->footer_height = 32 * handle->scale;
	handle->tip_height = 20 * handle->scale;
	handle->sidebar_width = 256 * handle->scale;
	handle->item_height = 40 * handle->scale;

	handle->state.font_height = 16;
	_update_font_height(handle);

	if(host_resize)
	{
		host_resize->ui_resize(host_resize->handle, w, h);
	}

	strncpy(handle->template, "/tmp/XXXXXX.lua", sizeof(handle->template));
	handle->fd = mkstemps(handle->template, 4);
	if(handle->fd == -1)
	{
		free(handle);
		return NULL;
	}

	snprintf(handle->manual, sizeof(handle->manual), "%s%s",
		bundle_path, "manual.html");

	lv2_log_note(&handle->logger, "template: %s\n", handle->template);

	_message_get(handle, handle->urid_code);
	_message_get(handle, handle->urid_error);
	_message_get(handle, handle->urid_fontHeight);
	_message_get(handle, handle->urid_panic);
	_message_get(handle, handle->urid_editorHidden);
	_message_get(handle, handle->urid_graphHidden);
	_message_get(handle, handle->canvas.urid.Canvas_graph);
	_message_get(handle, handle->canvas.urid.Canvas_aspectRatio);

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	_dynparams_clr(handle);

	d2tk_util_kill(&handle->kid);
	d2tk_frontend_free(handle->dpugl);

	unlink(handle->template);
	close(handle->fd);
	free(handle);
}

static void
port_event(LV2UI_Handle instance, uint32_t index __attribute__((unused)),
	uint32_t size __attribute__((unused)), uint32_t protocol, const void *buf)
{
	plughandle_t *handle = instance;

	if(protocol != handle->atom_eventTransfer)
	{
		return;
	}

	const LV2_Atom_Object *obj = buf;

	ser_atom_t ser;
	ser_atom_init(&ser);
	ser_atom_reset(&ser, &handle->forge);

	LV2_Atom_Forge_Ref ref = 0;
	props_advance(&handle->props, &handle->forge, 0, obj, &ref);

	ser_atom_deinit(&ser);

	d2tk_frontend_redisplay(handle->dpugl);
}

static void
_file_read(plughandle_t *handle)
{
	lseek(handle->fd, 0, SEEK_SET);
	const size_t len = lseek(handle->fd, 0, SEEK_END);
	lseek(handle->fd, 0, SEEK_SET);

	char *txt = alloca(len + 1);
	if(!txt)
	{
		lv2_log_error(&handle->logger, "alloca failed\n");
		return;
	}

	read(handle->fd, txt, len);
	txt[len] = '\0';

	handle->hash = d2tk_hash(txt, len);

	_update_code(handle, txt, len);
}

static int
_idle(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	struct stat st;
	if(stat(handle->template, &st) == -1)
	{
		lv2_log_error(&handle->logger, "stat: %s\n", strerror(errno));
	}

	if( (st.st_mtime > handle->modtime) && (handle->modtime > 0) )
	{
		_file_read(handle);

		handle->modtime = st.st_mtime;
	}

	if(d2tk_frontend_step(handle->dpugl))
	{
		handle->done = 1;
	}

	return handle->done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle
};

static int
_resize(LV2UI_Handle instance, int width, int height)
{
	plughandle_t *handle = instance;

	return d2tk_frontend_set_size(handle->dpugl, width, height);
}

static const LV2UI_Resize resize_ext = {
	.ui_resize = _resize
};

static const void *
_extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__resize))
		return &resize_ext;
		
	return NULL;
}

static const LV2UI_Descriptor moony_ui= {
	.URI            = MOONY_D2TK_URI,
	.instantiate    = instantiate,
	.cleanup        = cleanup,
	.port_event     = port_event,
	.extension_data = _extension_data
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &moony_ui;
		default:
			return NULL;
	}
}
