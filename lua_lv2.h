/*
 * Copyright (c) 2014 Hanspeter Portner (dev@open-music-kontrollers.ch)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 */

#ifndef _LUA_LV2_H
#define _LUA_LV2_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <endian.h>

#define LUA_COMPAT_ALL
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <tlsf.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#define LUA_URI							"http://open-music-kontrollers.ch/lv2/lua"

#define LUA_COMMON_URI			LUA_URI"#common"
#define LUA_COMMON_UI_URI		LUA_COMMON_URI"/ui"

#define LUA_CONTROL_URI			LUA_URI"#control"
#define LUA_MIDI_URI				LUA_URI"#midi"
#define LUA_OSC_URI					LUA_URI"#osc"

typedef struct _Lua_VM Lua_VM;

struct _Lua_VM {
	lua_State *L;

	char *chunk; // Lua program

	void *area;
	tlsf_t tlsf;
	pool_t pool;
};

int lua_vm_init(Lua_VM *lvm);
int lua_vm_deinit(Lua_VM *lvm);
	
#endif // _LUA_LV2_H
