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

#include <tlsf.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include <lv2_osc.h>

#include <lua.h>

#define MOONY_MAX_CHUNK_LEN		0x10000 // 64KB
#define MOONY_MAX_ERROR_LEN		0x400 // 1KB

#define MOONY_URI							"http://open-music-kontrollers.ch/lv2/moony"

#define MOONY_MESSAGE_URI			MOONY_URI"#message"
#define MOONY_CODE_URI				MOONY_URI"#code"
#define MOONY_ERROR_URI				MOONY_URI"#error"

#define MOONY_COMMON_EO_URI		MOONY_URI"#common_eo"
#define MOONY_COMMON_UI_URI		MOONY_URI"#common_ui"
#define MOONY_COMMON_X11_URI	MOONY_URI"#common_x11"
#define MOONY_COMMON_KX_URI		MOONY_URI"#common_kx"

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

typedef enum _moony_udata_t {
	MOONY_UDATA_SEQ		= 0,
	MOONY_UDATA_OBJ,
	MOONY_UDATA_TUPLE,
	MOONY_UDATA_VEC,
	MOONY_UDATA_CHUNK,
	MOONY_UDATA_ATOM,
	MOONY_UDATA_FORGE,

	MOONY_UDATA_COUNT
} moony_udata_t;

int moony_vm_init(moony_vm_t *vm);
int moony_vm_deinit(moony_vm_t *vm);
void *moony_vm_mem_alloc(size_t size);
void moony_vm_mem_free(void *area, size_t size);
int moony_vm_mem_extend(moony_vm_t *vm);

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

// from moony.c
typedef struct _moony_t moony_t;
typedef struct _lseq_t lseq_t;
typedef struct _lforge_t lforge_t;

struct _moony_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;

	struct {
		LV2_URID moony_message;
		LV2_URID moony_code;
		LV2_URID moony_error;

		LV2_URID log_trace;
		
		LV2_URID midi_event;

		LV2_URID time_position;
		LV2_URID time_barBeat;
		LV2_URID time_bar;
		LV2_URID time_beat;
		LV2_URID time_beatUnit;
		LV2_URID time_beatsPerBar;
		LV2_URID time_beatsPerMinute;
		LV2_URID time_frame;
		LV2_URID time_framesPerSecond;
		LV2_URID time_speed;
	} uris;

	osc_forge_t oforge;
	
	LV2_Worker_Schedule *sched;
	volatile int working;
	volatile int fully_extended;
	
	LV2_Log_Log *log;

	moony_vm_t vm;
	char chunk [MOONY_MAX_CHUNK_LEN];
	volatile int dirty_in;
	volatile int dirty_out;
	volatile int error_out;
	char error [MOONY_MAX_ERROR_LEN];

	// udata cache
	int itr [MOONY_UDATA_COUNT];
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

int moony_init(moony_t *moony, const LV2_Feature *const *features);
void moony_deinit(moony_t *moony);
void moony_open(moony_t *moony, lua_State *L);
void moony_activate(moony_t *moony, const char *chunk);
void moony_in(moony_t *moony, const LV2_Atom_Sequence *seq);
void moony_out(moony_t *moony, LV2_Atom_Sequence *seq, uint32_t frames);
const void* extension_data(const char* uri);

void *
moony_newuserdata(lua_State *L, moony_t *moony, moony_udata_t type);

static inline void
moony_freeuserdata(moony_t *moony)
{
	for(int i=0; i<MOONY_UDATA_COUNT; i++)
		moony->itr[i] = 0; // reset iterator
}

static inline int
moony_bypass(moony_t *moony)
{
	return moony->error[0] != '\0';
}

static inline void
moony_error(moony_t *moony)
{
	lua_State *L = moony->vm.L;

	strcpy(moony->error, lua_tostring(L, -1));
	lua_pop(L, 1);

	moony->error_out = 1;
}

// strdup fallback for windows
#if defined(_WIN32)
static inline char *
strndup(const char *s, size_t n)
{
	char *result;
	size_t len = strlen(s);
	if(n < len)
		len = n;
	result = (char *)malloc(len + 1);
	if(!result)
		return 0;
	result[len] = '\0';

	return (char *)strncpy(result, s, len);
}
#endif

#define _ATOM_ALIGNED __attribute__((aligned(8)))

typedef struct _moony_message_t moony_message_t;

struct _moony_message_t {
	LV2_Atom_Object obj _ATOM_ALIGNED;
	LV2_Atom_Property_Body prop _ATOM_ALIGNED;
	char body [0] _ATOM_ALIGNED;
} _ATOM_ALIGNED;
	
#endif // _MOONY_H
