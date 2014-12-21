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

#define LUA_CONTROL_URI			LUA_URI"#control"
#define LUA_CONTROL_UI_URI	LUA_CONTROL_URI"/ui"

#define LUA_MIDI_URI				LUA_URI"#midi"
#define LUA_MIDI_UI_URI			LUA_MIDI_URI"/ui"

#define LUA_OSC_URI					LUA_URI"#osc"
#define LUA_OSC_UI_URI			LUA_OSC_URI"/ui"

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
