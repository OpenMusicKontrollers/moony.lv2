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
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/extensions/units/units.h>

typedef struct _atom_ser_t atom_ser_t;

struct _atom_ser_t {
	void *data; // e.g. use rt-memory pool?
	uint32_t size;
	union {
		uint8_t *buf;
		const LV2_Atom *atom;
	};
	uint32_t offset;
};

#include <api_vm.h>
#include <osc.lv2/osc.h>
#include <xpress.lv2/xpress.h>
#include <varchunk.h>

#include <lua.h>
#include <lauxlib.h>

#include <canvas.lv2/forge.h>

#define __realtime __attribute__((annotate("realtime")))
#define __non_realtime __attribute__((annotate("non-realtime")))

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
#define MOONY_PREFIX					MOONY_URI"#"

#define MOONY_CODE_URI				MOONY_URI"#code"
#define MOONY_ERROR_URI				MOONY_URI"#error"
#define MOONY_TRACE_URI				MOONY_URI"#trace"
#define MOONY_STATE_URI				MOONY_URI"#state"
#define MOONY_PANIC_URI				MOONY_URI"#panic"

#define MOONY__color					MOONY_URI"#color"
#define MOONY__syntax					MOONY_URI"#syntax"

#define MOONY_EDITOR_HIDDEN_URI	MOONY_URI"#editorHidden"
#define MOONY_LOG_HIDDEN_URI	MOONY_URI"#logHidden"
#define MOONY_LOG_FOLLOW_URI	MOONY_URI"#logFollow"
#define MOONY_LOG_RESET_URI	MOONY_URI"#logReset"
#define MOONY_PARAM_HIDDEN_URI	MOONY_URI"#paramHidden"

#define MOONY_PARAM_COLS_URI	MOONY_URI"#paramCols"
#define MOONY_PARAM_ROWS_URI	MOONY_URI"#paramRows"

#define MOONY_NK_URI					MOONY_URI"#moony_ui"
#define MOONY_SIMPLE_UI_URI		MOONY_URI"#moony_zimple_ui"
#define MOONY_SIMPLE_KX_URI		MOONY_URI"#moony_zimple_kx"

#define MOONY_C1XC1_URI				MOONY_URI"#c1xc1"
#define MOONY_C2XC2_URI				MOONY_URI"#c2xc2"
#define MOONY_C4XC4_URI				MOONY_URI"#c4xc4"

#define MOONY_A1XA1_URI				MOONY_URI"#a1xa1"
#define MOONY_A2XA2_URI				MOONY_URI"#a2xa2"
#define MOONY_A4XA4_URI				MOONY_URI"#a4xa4"

#define MOONY_C1A1XC1A1_URI		MOONY_URI"#c1a1xc1a1"
#define MOONY_C2A1XC2A1_URI		MOONY_URI"#c2a1xc2a1"
#define MOONY_C4A1XC4A1_URI		MOONY_URI"#c4a1xc4a1"

#define LUA__lang							"http://lua.org#lang"

extern const LV2_Descriptor c1xc1;
extern const LV2_Descriptor c2xc2;
extern const LV2_Descriptor c4xc4;

extern const LV2_Descriptor a1xa1;
extern const LV2_Descriptor a2xa2;
extern const LV2_Descriptor a4xa4;

extern const LV2_Descriptor c1a1xc1a1;
extern const LV2_Descriptor c2a1xc2a1;
extern const LV2_Descriptor c4a1xc4a1;

extern const LV2UI_Descriptor nk_ui;
extern const LV2UI_Descriptor simple_ui;
extern const LV2UI_Descriptor simple_kx;

typedef enum _moony_udata_t {
	MOONY_UDATA_ATOM,
	MOONY_UDATA_FORGE,
	MOONY_UDATA_STASH,

	MOONY_UDATA_COUNT
} moony_udata_t;

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
	LV2_URID delete;
	LV2_URID copy;
	LV2_URID move;
	LV2_URID insert;
};

struct _moony_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Options_Option *opts;

	LV2_Atom_Forge forge;
	LV2_Atom_Forge state_forge;
	LV2_Atom_Forge stash_forge;
	LV2_Atom_Forge notify_forge;

	LV2_Atom_Forge_Frame notify_frame;
	LV2_Atom_Forge_Ref notify_ref;
	LV2_Atom_Forge notify_snapshot;

	LV2_Atom_Float sample_rate;

	struct {
		LV2_URID moony_code;
		LV2_URID moony_error;
		LV2_URID moony_trace;
		LV2_URID moony_panic;
		LV2_URID moony_state;
		LV2_URID moony_editorHidden;
		LV2_URID moony_logHidden;
		LV2_URID moony_logFollow;
		LV2_URID moony_logReset;
		LV2_URID moony_paramHidden;
		LV2_URID moony_paramCols;
		LV2_URID moony_paramRows;
		LV2_URID moony_color;
		LV2_URID moony_syntax;

		LV2_URID midi_event;

		patch_t patch;

		LV2_URID rdfs_label;
		LV2_URID rdfs_range;
		LV2_URID rdfs_comment;

		LV2_URID rdf_value;

		LV2_URID lv2_minimum;
		LV2_URID lv2_maximum;
		LV2_URID lv2_scale_point;
		LV2_URID lv2_minor_version;
		LV2_URID lv2_micro_version;

		LV2_URID units_unit;
		LV2_URID units_symbol;

		LV2_URID atom_beat_time;
		LV2_URID atom_frame_time;
		LV2_URID atom_child_type;

		LV2_URID xpress_Token;
		LV2_URID xpress_Alive;
		LV2_URID xpress_source;
		LV2_URID xpress_uuid;
		LV2_URID xpress_zone;
		LV2_URID xpress_body;
		LV2_URID xpress_pitch;
		LV2_URID xpress_pressure;
		LV2_URID xpress_timbre;
		LV2_URID xpress_dPitch;
		LV2_URID xpress_dPressure;
		LV2_URID xpress_dTimbre;

		LV2_URID param_sampleRate;
	} uris;

	LV2_OSC_URID osc_urid;
	LV2_OSC_Schedule *osc_sched;

	LV2_Worker_Schedule *sched;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	xpress_t xpress;

	LV2_Canvas_URID canvas_urid;

	moony_vm_t *vm;
	atomic_uintptr_t vm_new;

	bool once;
	bool error_out;

	// udata cache
	int itr [MOONY_UDATA_COUNT];
	int upc [MOONY_UPCLOSURE_COUNT];

	atomic_flag state_lock;

	LV2_Atom *state_atom;
	atomic_uintptr_t state_atom_new;

	LV2_Atom *stash_atom;
	uint32_t stash_size;

	varchunk_t *from_dsp;

	latom_driver_hash_t atom_driver_hash [DRIVER_HASH_MAX];

	size_t mem_size;
	bool testing;

	atomic_int editor_hidden;
	atomic_int log_hidden;
	atomic_int log_follow;
	atomic_int log_reset;
	atomic_int param_hidden;
	atomic_int param_cols;
	atomic_int param_rows;

	char error [MOONY_MAX_ERROR_LEN];
	atomic_uintptr_t err_new;

	char chunk [MOONY_MAX_CHUNK_LEN];
	atomic_uintptr_t chunk_new;
	char *chunk_nrt;
};

// in api.c
int moony_init(moony_t *moony, const char *subject, double sample_rate,
	const LV2_Feature *const *features, size_t mem_size, bool testing);
void moony_deinit(moony_t *moony);
void moony_open(moony_t *moony, moony_vm_t *vm, lua_State *L);
void moony_pre(moony_t *moony, LV2_Atom_Sequence *notify);
bool moony_in(moony_t *moony, const LV2_Atom_Sequence *control, LV2_Atom_Sequence *notify);
void moony_out(moony_t *moony, LV2_Atom_Sequence *notify, uint32_t frames);
const void* extension_data(const char* uri);
void *moony_newuserdata(lua_State *L, moony_t *moony, moony_udata_t type, bool cache);
LV2_Worker_Status moony_wake_worker(const LV2_Worker_Schedule *work_sched);

__realtime static inline void
moony_freeuserdata(moony_t *moony)
{
	for(unsigned i=0; i<MOONY_UDATA_COUNT; i++)
		moony->itr[i] = 1; // reset iterator
	for(unsigned i=0; i<MOONY_UPCLOSURE_COUNT; i++)
		moony->upc[i] = 1; // reset iterator
}

__realtime static inline bool
moony_bypass(moony_t *moony)
{
	return moony->error[0] != 0x0;
}

__realtime static inline const char *
_err_skip(const char *msg)
{
	const char *err = strstr(msg, "\"]:"); // search end mark of header [string ""]:
	err = err
		? err + 3 // skip header end mark
		: msg; // use whole error string alternatively

	return err;
}

__non_realtime static inline void
moony_err_async(moony_t *moony, const char *msg)
{
	const char *err = _err_skip(msg);

	if(moony->log)
		lv2_log_error(&moony->logger, "%s\n", err);

	char *err_new = strdup(err);
	if(err_new)
	{
		char *err_old = (char *)atomic_exchange_explicit(&moony->err_new, (uintptr_t)err_new, memory_order_relaxed);

		if(err_old)
			free(err_old);
	}
}

__realtime static inline void
moony_err(moony_t *moony, const char *msg)
{
	const char *err = _err_skip(msg);

	if(moony->log)
		lv2_log_trace(&moony->logger, "%s\n", err);

	if(moony->error[0] == 0x0) // don't overwrite any previous error message
		snprintf(moony->error, MOONY_MAX_ERROR_LEN, "%s", err);
	moony->error_out = true;
}

__realtime static inline void
moony_trace(moony_t *moony, const char *msg)
{
	const char *err = _err_skip(msg);

	if(moony->log)
		lv2_log_trace(&moony->logger, "%s\n", err);
}

__realtime static inline lua_State *
moony_current(moony_t *moony)
{
	return moony->vm->L;
}

__realtime static inline void
moony_error(moony_t *moony)
{
	lua_State *L = moony_current(moony);

	const char *msg = lua_tostring(L, -1);
	if(msg)
		moony_err(moony, msg);
	lua_pop(L, 1);
}

#define _spin_lock(FLAG) while(atomic_flag_test_and_set_explicit((FLAG), memory_order_acquire)) {}
#define _try_lock(FLAG) !atomic_flag_test_and_set_explicit((FLAG), memory_order_acquire)
#define _unlock(FLAG) atomic_flag_clear_explicit((FLAG), memory_order_release)

__realtime static inline LV2_Atom_Forge_Ref
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
moony_rt_alloc(moony_vm_t *vm, size_t nsize);

void *
moony_rt_realloc(moony_vm_t *vm, void *buf, size_t osize, size_t nsize);

void
moony_rt_free(moony_vm_t *vm, void *buf, size_t osize);

LV2_Atom_Forge_Ref
_sink_rt(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size);
LV2_Atom_Forge_Ref
_sink_non_rt(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size);

LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref);

#endif // _MOONY_H
