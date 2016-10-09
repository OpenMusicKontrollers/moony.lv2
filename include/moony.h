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

#ifndef _MOONY_H
#define _MOONY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>

#if !defined(_WIN32)
#	include <sys/mman.h>
#else
#	define mlock(...)
#	define munlock(...)
#endif

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/extensions/units/units.h>

#include <lv2_extensions.h>

#include <api_vm.h>
#include <osc.lv2/osc.h>
#include <xpress.h>

#include <lua.h>
#include <lauxlib.h>

#ifdef BUILD_INLINE_DISPLAY
#	include <cairo.h>
#endif

#ifdef LV2_ATOM_TUPLE_FOREACH
#	undef LV2_ATOM_TUPLE_FOREACH
#	define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))
#endif

#define MOONY_MAX_CHUNK_LEN		0x20000 // 128KB
#define MOONY_MAX_ERROR_LEN		0x800 // 2KB

#define MOONY_URI							"http://open-music-kontrollers.ch/lv2/moony"

#define MOONY_CODE_URI				MOONY_URI"#code"
#define MOONY_SELECTION_URI		MOONY_URI"#selection"
#define MOONY_ERROR_URI				MOONY_URI"#error"
#define MOONY_TRACE_URI				MOONY_URI"#trace"
#define MOONY_STATE_URI				MOONY_URI"#state"

#define MOONY_UI_URI					MOONY_URI"#ui"
#define MOONY_DSP_URI					MOONY_URI"#dsp"
#define MOONY_DESTINATION_URI	MOONY_URI"#destination"

#define MOONY_COMMON_UI_URI		MOONY_URI"#ui_3_common_1_ui"
#define MOONY_COMMON_KX_URI		MOONY_URI"#ui_3_common_2_kx"
#define MOONY_COMMON_EO_URI		MOONY_URI"#ui_3_common_3_eo"

#define MOONY_SIMPLE_UI_URI		MOONY_URI"#ui_2_simple_1_ui"
#define MOONY_SIMPLE_KX_URI		MOONY_URI"#ui_2_simple_2_kx"

#define MOONY_WEB_UI_URI			MOONY_URI"#ui_1_web_1_ui"
#define MOONY_WEB_KX_URI			MOONY_URI"#ui_1_web_2_kx"

#define MOONY_C1XC1_URI				MOONY_URI"#c1xc1"
#define MOONY_C2XC2_URI				MOONY_URI"#c2xc2"
#define MOONY_C4XC4_URI				MOONY_URI"#c4xc4"

#define MOONY_A1XA1_URI				MOONY_URI"#a1xa1"
#define MOONY_A2XA2_URI				MOONY_URI"#a2xa2"
#define MOONY_A4XA4_URI				MOONY_URI"#a4xa4"

#define MOONY_C1A1XC1A1_URI		MOONY_URI"#c1a1xc1a1"
#define MOONY_C2A1XC2A1_URI		MOONY_URI"#c2a1xc2a1"
#define MOONY_C4A1XC4A1_URI		MOONY_URI"#c4a1xc4a1"

extern const LV2_Descriptor c1xc1;
extern const LV2_Descriptor c2xc2;
extern const LV2_Descriptor c4xc4;

extern const LV2_Descriptor a1xa1;
extern const LV2_Descriptor a2xa2;
extern const LV2_Descriptor a4xa4;

extern const LV2_Descriptor c1a1xc1a1;
extern const LV2_Descriptor c2a1xc2a1;
extern const LV2_Descriptor c4a1xc4a1;

extern const LV2UI_Descriptor common_eo;
extern const LV2UI_Descriptor common_ui;
extern const LV2UI_Descriptor common_kx;

extern const LV2UI_Descriptor simple_ui;
extern const LV2UI_Descriptor simple_kx;

extern const LV2UI_Descriptor web_ui;
extern const LV2UI_Descriptor web_kx;

typedef enum _moony_udata_t {
	MOONY_UDATA_ATOM,
	MOONY_UDATA_FORGE,
	MOONY_UDATA_STASH,

	MOONY_UDATA_COUNT
} moony_udata_t;

typedef enum _moony_cclosure_t {
	MOONY_CCLOSURE_STASH,
	MOONY_CCLOSURE_APPLY,
	MOONY_CCLOSURE_SAVE,
	MOONY_CCLOSURE_RESTORE,

	MOONY_CCLOSURE_TIME_STASH,
	MOONY_CCLOSURE_TIME_APPLY,

	MOONY_CCLOSURE_LIT_UNPACK,
	MOONY_CCLOSURE_TUPLE_UNPACK,
	MOONY_CCLOSURE_VECTOR_UNPACK,
	MOONY_CCLOSURE_CHUNK_UNPACK,

	MOONY_CCLOSURE_TUPLE_FOREACH,
	MOONY_CCLOSURE_VECTOR_FOREACH,
	MOONY_CCLOSURE_OBJECT_FOREACH,
	MOONY_CCLOSURE_SEQUENCE_FOREACH,

	MOONY_CCLOSURE_CLONE,
	MOONY_CCLOSURE_WRITE,

	MOONY_CCLOSURE_COUNT
} moony_cclosure_t;

typedef enum _moony_upclosure_t {
	MOONY_UPCLOSURE_TUPLE_FOREACH,
	MOONY_UPCLOSURE_VECTOR_FOREACH,
	MOONY_UPCLOSURE_OBJECT_FOREACH,
	MOONY_UPCLOSURE_SEQUENCE_FOREACH,
	MOONY_UPCLOSURE_SEQUENCE_MULTIPLEX,

	MOONY_UPCLOSURE_COUNT
} moony_upclosure_t;

// from api_atom.c
typedef struct _lheader_t lheader_t;
typedef struct _latom_driver_t latom_driver_t;
typedef struct _latom_driver_hash_t latom_driver_hash_t;

struct _lheader_t {
	moony_udata_t type;
	bool cache;
};

struct _latom_driver_hash_t {
	LV2_URID type;
	const latom_driver_t *driver;
};

#define UDATA_OFFSET (LUA_RIDX_LAST + 1)
#define DRIVER_HASH_MAX 15

// from moony.c
typedef struct _patch_t patch_t;
typedef struct _moony_t moony_t;

struct _patch_t {
	LV2_URID self;

	LV2_URID get;
	LV2_URID set;
	LV2_URID put;
	LV2_URID patch;
	LV2_URID body;
	LV2_URID subject;
	LV2_URID property;
	LV2_URID value;
	LV2_URID add;
	LV2_URID remove;
	LV2_URID wildcard;
	LV2_URID writable;
	LV2_URID readable;
	LV2_URID destination;
	LV2_URID sequence;
	LV2_URID error;
	LV2_URID ack;
};

struct _moony_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge state_forge;
	LV2_Atom_Forge stash_forge;
	LV2_Atom_Forge notify_forge;

	LV2_Atom_Forge_Frame notify_frame;
	LV2_Atom_Forge_Ref notify_ref;

	struct {
		LV2_URID moony_code;
		LV2_URID moony_selection;
		LV2_URID moony_error;
		LV2_URID moony_trace;
		LV2_URID moony_state;

		LV2_URID midi_event;

		LV2_URID bufsz_max_block_length;
		LV2_URID bufsz_min_block_length;
		LV2_URID bufsz_sequence_size;

		patch_t patch;

		LV2_URID rdfs_label;
		LV2_URID rdfs_range;
		LV2_URID rdfs_comment;

		LV2_URID rdf_value;

		LV2_URID core_minimum;
		LV2_URID core_maximum;
		LV2_URID core_scale_point;
		LV2_URID core_minor_version;
		LV2_URID core_micro_version;

		LV2_URID units_unit;

		LV2_URID atom_beat_time;
		LV2_URID atom_frame_time;

		LV2_URID canvas_canvas;
		LV2_URID canvas_draw;
		LV2_URID canvas_body;
		LV2_URID canvas_beginPath;
		LV2_URID canvas_moveTo;
		LV2_URID canvas_lineTo;
		LV2_URID canvas_rectangle;
		LV2_URID canvas_arc;
		LV2_URID canvas_curveTo;
		LV2_URID canvas_color;
		LV2_URID canvas_lineWidth;
		LV2_URID canvas_closePath;
		LV2_URID canvas_stroke;
		LV2_URID canvas_fill;
		LV2_URID canvas_fontSize;
		LV2_URID canvas_showText;
	} uris;

	struct {
		uint32_t max_block_length;
		uint32_t min_block_length;
		uint32_t sequence_size;
		uint32_t sample_rate;
	} opts;

	LV2_OSC_URID osc_urid;
	LV2_OSC_Schedule *osc_sched;
	
	LV2_Worker_Schedule *sched;
	volatile int working;
	volatile int fully_extended;
	
	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	xpress_map_t *voice_map;

	LV2_Inline_Display *queue_draw;
	LV2_Inline_Display_Image_Surface image_surface;
#ifdef BUILD_INLINE_DISPLAY
	struct {
		cairo_surface_t *surface;
		cairo_t *ctx;
	} cairo;
#endif

	moony_vm_t vm;

	volatile int props_out;
	volatile int dirty_out;
	volatile int error_out;
	volatile int trace_out;

	// udata cache
	int itr [MOONY_UDATA_COUNT];
	int upc [MOONY_UPCLOSURE_COUNT];

	struct {
		atomic_flag state;
		atomic_flag render;
	} lock;

	LV2_Atom *state_atom;
	LV2_Atom *stash_atom;
	uint32_t stash_size;

	struct {
		LV2_Atom_Forge forge;
		LV2_Atom *atom;
		uint32_t size;
	} render;

	latom_driver_hash_t atom_driver_hash [DRIVER_HASH_MAX];

	char error [MOONY_MAX_ERROR_LEN];
	char trace [MOONY_MAX_ERROR_LEN];
	char chunk [MOONY_MAX_CHUNK_LEN];
};

// in api.c
int moony_init(moony_t *moony, const char *subject, double sample_rate,
	const LV2_Feature *const *features);
void moony_deinit(moony_t *moony);
void moony_open(moony_t *moony, lua_State *L, bool use_assert);
void moony_pre(moony_t *moony, LV2_Atom_Sequence *notify);
bool moony_in(moony_t *moony, const LV2_Atom_Sequence *control, LV2_Atom_Sequence *notify);
void moony_out(moony_t *moony, LV2_Atom_Sequence *notify, uint32_t frames);
const void* extension_data(const char* uri);
void *moony_newuserdata(lua_State *L, moony_t *moony, moony_udata_t type, bool cache);

static inline void
moony_freeuserdata(moony_t *moony)
{
	for(unsigned i=0; i<MOONY_UDATA_COUNT; i++)
		moony->itr[i] = 1; // reset iterator
	for(unsigned i=0; i<MOONY_UPCLOSURE_COUNT; i++)
		moony->upc[i] = 1; // reset iterator
}

static inline int
moony_bypass(moony_t *moony)
{
	return moony->error[0] != '\0';
}

static inline void
moony_err(moony_t *moony, const char *msg)
{
	const char *error = msg;
	const char *err = strstr(error, "\"]:"); // search end mark of header [string ""]:
	err = err
		? err + 3 // skip header end mark
		: error; // use whole error string alternatively

	if(moony->log)
		lv2_log_trace(&moony->logger, "%s", err);

	snprintf(moony->error, MOONY_MAX_ERROR_LEN, "%s", err);

	moony->error_out = 1;
}

static inline void
moony_error(moony_t *moony)
{
	lua_State *L = moony->vm.L;

	const char *msg = lua_tostring(L, -1);
	if(msg)
		moony_err(moony, msg);
	lua_pop(L, 1);
}

#define _spin_lock(FLAG) while(atomic_flag_test_and_set_explicit((FLAG), memory_order_acquire)) {}
#define _try_lock(FLAG) !atomic_flag_test_and_set_explicit((FLAG), memory_order_acquire)
#define _unlock(FLAG) atomic_flag_clear_explicit((FLAG), memory_order_release)

static inline LV2_Atom_Forge_Ref
_moony_patch(patch_t *patch, LV2_Atom_Forge *forge, LV2_URID key,
	const char *str, uint32_t size)
{
	LV2_Atom_Forge_Frame frame;
	
	LV2_Atom_Forge_Ref ref = lv2_atom_forge_object(forge, &frame, 0, str ? patch->set : patch->get)
		&& lv2_atom_forge_key(forge, patch->subject)
		&& lv2_atom_forge_urid(forge, patch->self)
		&& lv2_atom_forge_key(forge, patch->property)
		&& lv2_atom_forge_urid(forge, key);

	if(ref && str)
	{
		ref = lv2_atom_forge_key(forge, patch->value)
			&& lv2_atom_forge_string(forge, str, size);
	}

	if(ref)
	{
		lv2_atom_forge_pop(forge, &frame);
		return 1; // success
	}

	return 0; // overflow
}

void *
moony_alloc(moony_t *moony, size_t nsize);

void *
moony_realloc(moony_t *moony, void *buf, size_t osize, size_t nsize);

void
moony_free(moony_t *moony, void *buf, size_t osize);

LV2_Atom_Forge_Ref
_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size);

LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref);

typedef struct _atom_ser_t atom_ser_t;

struct _atom_ser_t {
	moony_t *moony; // use rt-memory pool?
	uint32_t size;
	uint8_t *buf;
	uint32_t offset;
};

#endif // _MOONY_H
