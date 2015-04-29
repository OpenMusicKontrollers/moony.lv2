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

#ifndef _LUA_LV2_H
#define _LUA_LV2_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <tlsf.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include <lua.h>

#define LUA_URI							"http://open-music-kontrollers.ch/lv2/lua"

#define LUA_MESSAGE_URI			LUA_URI"#message"
#define LUA_CODE_URI				LUA_URI"#code"
#define LUA_ERROR_URI				LUA_URI"#error"

#define LUA_COMMON_EO_URI		LUA_URI"#common_eo"
#define LUA_COMMON_UI_URI		LUA_URI"#common_ui"
#define LUA_COMMON_X11_URI	LUA_URI"#common_x11"
#define LUA_COMMON_KX_URI		LUA_URI"#common_kx"

#define LUA_C1XC1_URI				LUA_URI"#c1xc1"
#define LUA_C2XC2_URI				LUA_URI"#c2xc2"
#define LUA_C4XC4_URI				LUA_URI"#c4xc4"

#define LUA_A1XA1_URI				LUA_URI"#a1xa1"
#define LUA_A2XA2_URI				LUA_URI"#a2xa2"
#define LUA_A4XA4_URI				LUA_URI"#a4xa4"

#define LUA_A1XC1_URI				LUA_URI"#a1xc1"
#define LUA_A1XC2_URI				LUA_URI"#a1xc2"
#define LUA_A1XC4_URI				LUA_URI"#a1xc4"

#define LUA_C1XA1_URI				LUA_URI"#c1xa1"
#define LUA_C2XA1_URI				LUA_URI"#c2xa1"
#define LUA_C4XA1_URI				LUA_URI"#c4xa1"

extern const LV2_Descriptor c1xc1;
extern const LV2_Descriptor c2xc2;
extern const LV2_Descriptor c4xc4;

extern const LV2_Descriptor a1xa1;
extern const LV2_Descriptor a2xa2;
extern const LV2_Descriptor a4xa4;

extern const LV2_Descriptor a1xc1;
extern const LV2_Descriptor a1xc2;
extern const LV2_Descriptor a1xc4;

extern const LV2_Descriptor c1xa1;
extern const LV2_Descriptor c2xa1;
extern const LV2_Descriptor c4xa1;

extern const LV2UI_Descriptor common_eo;
extern const LV2UI_Descriptor common_ui;
extern const LV2UI_Descriptor common_x11;
extern const LV2UI_Descriptor common_kx;

// from lua_vm.c
#define POOL_NUM 8

typedef struct _Lua_VM Lua_VM;

struct _Lua_VM {
	tlsf_t tlsf;

	size_t size [POOL_NUM];
	void *area [POOL_NUM]; // 128K, 256K, 512K, 1M, 2M, 4M, 8M, 16M
	pool_t pool [POOL_NUM];

	size_t space;
	size_t used;

	lua_State *L;
};

int lua_vm_init(Lua_VM *lvm);
int lua_vm_deinit(Lua_VM *lvm);
void *lua_vm_mem_alloc(size_t size);
void lua_vm_mem_free(void *area, size_t size);
int lua_vm_mem_half_full(Lua_VM *lvm);
int lua_vm_mem_extend(Lua_VM *lvm);

// from encoder.l
typedef void (*encoder_append_t)(const char *str, void *data);
typedef struct _encoder_t encoder_t;

struct _encoder_t {
	encoder_append_t append;
	void *data;
};
extern encoder_t *encoder;

void lua_to_markup(const char *utf8);

// from lua_atom.c
typedef struct _lua_atom_t lua_atom_t;
typedef struct _lseq_t lseq_t;
typedef struct _lforge_t lforge_t;

struct _lua_atom_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;

	struct {
		LV2_URID lua_message;
		LV2_URID lua_code;
		LV2_URID lua_error;
		LV2_URID midi_event;
	} uris;
};

struct _lseq_t {
	const LV2_Atom_Sequence *seq;
	const LV2_Atom_Event *itr;
};

struct _lforge_t {
	LV2_Atom_Forge *forge;
};

int lua_atom_init(lua_atom_t *lua_atom, const LV2_Feature *const *features);
void lua_atom_open(lua_atom_t *lua_atom, lua_State *L);

// strdup fallback for windows
#if defined(_WIN32)
static inline char *
strndup(const char *s, size_t n)
{
    char *result;
    size_t len = strlen (s);
    if (n < len) len = n;
    result = (char *) malloc (len + 1);
    if (!result) return 0;
    result[len] = '\0';
    return (char *) strncpy (result, s, len);
}
#endif
	
#endif // _LUA_LV2_H
