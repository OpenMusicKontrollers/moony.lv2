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

#include <tlsf.h>

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

#include <lv2_osc.h>

#include <lua.h>
#include <lauxlib.h>

#define MOONY_MAX_CHUNK_LEN		0x10000 // 64KB
#define MOONY_MAX_ERROR_LEN		0x400 // 1KB

#define MOONY_URI							"http://open-music-kontrollers.ch/lv2/moony"

#define MOONY_MESSAGE_URI			MOONY_URI"#message"
#define MOONY_CODE_URI				MOONY_URI"#code"
#define MOONY_SELECTION_URI		MOONY_URI"#selection"
#define MOONY_ERROR_URI				MOONY_URI"#error"
#define MOONY_TRACE_URI				MOONY_URI"#trace"
#define MOONY_STATE_URI				MOONY_URI"#state"

#define MOONY_COMMON_UI_URI		MOONY_URI"#ui5_common_ui"
#define MOONY_COMMON_KX_URI		MOONY_URI"#ui6_common_kx"
#define MOONY_COMMON_X11_URI	MOONY_URI"#ui7_common_x11"
#define MOONY_COMMON_EO_URI		MOONY_URI"#ui8_common_eo"

#define MOONY_SIMPLE_UI_URI		MOONY_URI"#ui3_simple_ui"
#define MOONY_SIMPLE_KX_URI		MOONY_URI"#ui4_simple_kx"

#define MOONY_WEB_UI_URI			MOONY_URI"#ui1_web_ui"
#define MOONY_WEB_KX_URI			MOONY_URI"#ui2_web_kx"

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
extern const LV2UI_Descriptor common_x11;
extern const LV2UI_Descriptor common_kx;

extern const LV2UI_Descriptor simple_ui;
extern const LV2UI_Descriptor simple_kx;

extern const LV2UI_Descriptor web_ui;
extern const LV2UI_Descriptor web_kx;

// from vm.c
#define MOONY_POOL_NUM 8

typedef struct _moony_vm_t moony_vm_t;
typedef struct _moony_mem_t moony_mem_t;

struct _moony_vm_t {
	tlsf_t tlsf;

	size_t size [MOONY_POOL_NUM];
	void *area [MOONY_POOL_NUM]; // 128K, 256K, 512K, 1M, 2M, 4M, 8M, 16M
	pool_t pool [MOONY_POOL_NUM];

	size_t space;
	size_t used;

	lua_State *L;
};

struct _moony_mem_t {
	int i;
	void *mem;
};

int moony_vm_init(moony_vm_t *vm);
int moony_vm_deinit(moony_vm_t *vm);
void *moony_vm_mem_alloc(size_t size);
void moony_vm_mem_free(void *area, size_t size);
int moony_vm_mem_extend(moony_vm_t *vm);

typedef enum _moony_udata_t {
	MOONY_UDATA_ATOM,
	MOONY_UDATA_FORGE,
	MOONY_UDATA_STASH,

	MOONY_UDATA_COUNT
} moony_udata_t;

// from encoder.l
typedef void (*encoder_begin_t)(void *data);
typedef void (*encoder_append_t)(const char *str, void *data);
typedef void (*encoder_end_t)(void *data);
typedef struct _moony_encoder_t moony_encoder_t;

struct _moony_encoder_t {
	encoder_begin_t begin;
	encoder_append_t append;
	encoder_end_t end;
	void *data;
};
extern moony_encoder_t *encoder;

void lua_to_markup(const char *utf8, FILE *f);

// from api.c
typedef struct _latom_t latom_t;
typedef struct _latom_driver_t latom_driver_t;
typedef struct _latom_driver_hash_t latom_driver_hash_t;

typedef int (*latom_driver_function_t)(lua_State *L, latom_t *latom);

struct _latom_driver_t {
	latom_driver_function_t __indexi;
	latom_driver_function_t __indexk;
	latom_driver_function_t __len;
	latom_driver_function_t __tostring;
	latom_driver_function_t __call;

	latom_driver_function_t value;
	lua_CFunction unpack;
	lua_CFunction foreach;
};

struct _latom_driver_hash_t {
	LV2_URID type;
	const latom_driver_t *driver;
};

// from moony.c
typedef struct _moony_t moony_t;
typedef struct _lseq_t lseq_t;
typedef struct _lforge_t lforge_t;

#define DRIVER_HASH_MAX 16

struct _moony_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge state_forge;
	LV2_Atom_Forge stash_forge;

	struct {
		LV2_URID subject;

		LV2_URID moony_message;
		LV2_URID moony_code;
		LV2_URID moony_selection;
		LV2_URID moony_error;
		LV2_URID moony_trace;
		LV2_URID moony_state;

		LV2_URID midi_event;

		LV2_URID bufsz_max_block_length;
		LV2_URID bufsz_min_block_length;
		LV2_URID bufsz_sequence_size;

		LV2_URID patch_get;
		LV2_URID patch_set;
		LV2_URID patch_put;
		LV2_URID patch_patch;
		LV2_URID patch_body;
		LV2_URID patch_subject;
		LV2_URID patch_property;
		LV2_URID patch_value;
		LV2_URID patch_add;
		LV2_URID patch_remove;
		LV2_URID patch_wildcard;
		LV2_URID patch_writable;
		LV2_URID patch_readable;

		LV2_URID rdfs_label;
		LV2_URID rdfs_range;
		LV2_URID rdfs_comment;

		LV2_URID rdf_value;

		LV2_URID core_minimum;
		LV2_URID core_maximum;
		LV2_URID core_scale_point;

		LV2_URID units_unit;
	} uris;

	struct {
		uint32_t max_block_length;
		uint32_t min_block_length;
		uint32_t sequence_size;
		uint32_t sample_rate;
	} opts;

	osc_forge_t oforge;
	
	LV2_Worker_Schedule *sched;
	volatile int working;
	volatile int fully_extended;

	osc_schedule_t *osc_sched;
	
	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	moony_vm_t vm;

	char chunk [MOONY_MAX_CHUNK_LEN];
	volatile int dirty_out;
	volatile int error_out;
	volatile int trace_out;
	char error [MOONY_MAX_ERROR_LEN];
	char trace [MOONY_MAX_ERROR_LEN];

	// udata cache
	int itr [MOONY_UDATA_COUNT];

	struct {
		atomic_flag chunk;
		atomic_flag error;
		atomic_flag state;
	} lock;

	LV2_Atom *state_atom;
	LV2_Atom *stash_atom;

	latom_driver_hash_t atom_driver_hash [DRIVER_HASH_MAX];
};

struct _lseq_t {
	const LV2_Atom_Sequence *seq;
	const LV2_Atom_Event *itr;
	LV2_Atom body [0];
};

struct _lforge_t {
	LV2_Atom_Forge *forge;
	int depth;
	union {
		int64_t frames;	// Time in audio frames
		double  beats; // Time in beats
	} last;
	LV2_Atom_Forge_Frame frame [2];
};

int moony_init(moony_t *moony, const char *subject, double sample_rate,
	const LV2_Feature *const *features);
void moony_deinit(moony_t *moony);
void moony_open(moony_t *moony, lua_State *L, bool use_assert);
void moony_in(moony_t *moony, const LV2_Atom_Sequence *seq);
void moony_out(moony_t *moony, LV2_Atom_Sequence *seq, uint32_t frames);
const void* extension_data(const char* uri);

void *
moony_newuserdata(lua_State *L, moony_t *moony, moony_udata_t type);

static inline void
moony_freeuserdata(moony_t *moony)
{
	for(unsigned i=0; i<MOONY_UDATA_COUNT; i++)
		moony->itr[i] = 0; // reset iterator
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
_moony_message_forge(LV2_Atom_Forge *forge, LV2_URID otype, LV2_URID key,
	uint32_t size, const char *str)
{
	LV2_Atom_Forge_Frame frame;

	LV2_Atom_Forge_Ref ref = lv2_atom_forge_object(forge, &frame, 0, otype);
	if(str)
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, key);
		if(ref)
			ref = lv2_atom_forge_string(forge, str, size);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &frame);
	return ref;
}

#endif // _MOONY_H
