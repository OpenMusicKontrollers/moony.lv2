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
#include <endian.h>

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

#define LUA_CONTROL_URI			LUA_URI"#control"
#define LUA_ATOM_URI				LUA_URI"#atom"

extern const LV2_Descriptor lv2_lua_control;
extern const LV2_Descriptor lv2_lua_atom;

extern const LV2UI_Descriptor lv2_lua_common_eo;
extern const LV2UI_Descriptor lv2_lua_common_ui;
extern const LV2UI_Descriptor lv2_lua_common_x11;
extern const LV2UI_Descriptor lv2_lua_common_kx;

typedef struct _Lua_VM Lua_VM;

#define POOL_NUM 8

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

typedef void (*encoder_append_t)(const char *str, void *data);
typedef struct _encoder_t encoder_t;

struct _encoder_t {
	encoder_append_t append;
	void *data;
};
extern encoder_t *encoder;

void lua_to_markup(const char *utf8);
	
#endif // _LUA_LV2_H
