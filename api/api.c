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

#include <limits.h> // INT_MAX
#include <math.h> // INFINITY
#include <inttypes.h>
#include <stdatomic.h>

#include <osc.lv2/endian.h>

#include <api_atom.h>
#include <api_forge.h>
#include <api_stash.h>
#include <api_midi.h>
#include <api_osc.h>
#include <api_time.h>
#include <api_state.h>
#include <api_parameter.h>

#define RDF_PREFIX    "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDFS_PREFIX   "http://www.w3.org/2000/01/rdf-schema#"

#define RDF__value    RDF_PREFIX"value"
#define RDF__type     RDF_PREFIX"type"
#define RDFS__label   RDFS_PREFIX"label"
#define RDFS__range   RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

#ifndef LV2_PATCH__Copy
#	define LV2_PATCH__Copy LV2_PATCH_PREFIX "Copy"
#endif

#ifndef LV2_UNITS__midiController
#	define LV2_UNITS__midiController LV2_UNITS_PREFIX "midiController"
#endif

static const char *moony_ref [MOONY_UDATA_COUNT] = {
	[MOONY_UDATA_ATOM]	= "latom",
	[MOONY_UDATA_FORGE]	= "lforge",
	[MOONY_UDATA_STASH]	= "lstash"
};

static const size_t moony_sz [MOONY_UDATA_COUNT] = {
	[MOONY_UDATA_ATOM]	= sizeof(latom_t),
	[MOONY_UDATA_FORGE]	= sizeof(lforge_t),
	[MOONY_UDATA_STASH]	= sizeof(lstash_t)
};

static _Atomic xpress_uuid_t voice_uuid = ATOMIC_VAR_INIT(INT64_MAX / UINT16_MAX * 2LL);

__realtime static int64_t
_voice_map_new_uuid(void *handle)
{
	_Atomic xpress_uuid_t *uuid = handle;
	return atomic_fetch_add_explicit(uuid, 1, memory_order_relaxed);
}

static xpress_map_t voice_map_fallback = {
	.handle = &voice_uuid,
	.new_uuid = _voice_map_new_uuid
};

__non_realtime static int
_hash_sort(const void *itm1, const void *itm2)
{
	const latom_driver_hash_t *hash1 = itm1;
	const latom_driver_hash_t *hash2 = itm2;

	if(hash1->type < hash2->type)
		return -1;
	else if(hash1->type > hash2->type)
		return 1;
	return 0;
}

__realtime static int // map is not really realtime in most hosts
_lmap__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	const char *uri = luaL_checkstring(L, 2);
	LV2_URID urid = moony->map->map(moony->map->handle, uri); // non-rt
	if(urid)
	{
		lua_pushinteger(L, urid);

		// cache it
		lua_pushvalue(L, 2); // uri
		lua_pushvalue(L, -2); // urid
		lua_rawset(L, 1);  // self
	}
	else
		lua_pushnil(L);

	return 1;
}

static const luaL_Reg lmap_mt [] = {
	{"__index", _lmap__index},
	{"__call", _lmap__index},
	{NULL, NULL}
};

__realtime static int // unmap is not really realtime in most hosts
_lunmap__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	LV2_URID urid = luaL_checkinteger(L, 2);
	const char *uri = moony->unmap->unmap(moony->unmap->handle, urid); // non-rt
	if(uri)
	{
		lua_pushstring(L, uri);

		// cache it
		lua_pushvalue(L, 2); // urid
		lua_pushvalue(L, -2); // uri
		lua_rawset(L, 1);  // self
	}
	else
		lua_pushnil(L);

	return 1;
}

static const luaL_Reg lunmap_mt [] = {
	{"__index", _lunmap__index},
	{"__call", _lunmap__index},
	{NULL, NULL}
};

__realtime static int // map is not really realtime in most hosts
_lhash_map__index(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	if(lua_isstring(L, 2))
	{
		lua_getglobal(L, "Map");
		lua_pushvalue(L, lua_upvalueindex(2)); // uri.prefix
		lua_pushvalue(L, 2); // uri.postfix
		lua_concat(L, 2); // uri
		lua_gettable(L, -2); // Map[uri];
		if(lua_isinteger(L, -1))
		{
			// cache it
			lua_pushvalue(L, 2); // uri
			lua_pushvalue(L, -2); // urid
			lua_rawset(L, 1);  // self
		}
		else
			lua_pushnil(L);
	}
	else
		lua_pushnil(L);

	return 1;
}

static const luaL_Reg lhash_map_mt [] = {
	{"__index", _lhash_map__index},
	{"__call", _lhash_map__index},
	{NULL, NULL}
};

__realtime static int // map is not really realtime in most hosts
_lhash_map(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	if(lua_isstring(L, 1))
	{
		lua_newtable(L);
		lua_newtable(L);
		lua_pushlightuserdata(L, moony); // @ upvalueindex 1
		lua_pushvalue(L, 1); // uri.prefix @ upvalueindex 2
		luaL_setfuncs(L, lhash_map_mt, 2);
		//_protect_metatable(L, -1); //TODO
		lua_setmetatable(L, -2);
	}
	else
		lua_pushnil(L);

	return 1;
}

__realtime static int
_lvoice_map(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_pushinteger(L, moony->voice_map->new_uuid(moony->voice_map->handle));
	return 1;
}

__realtime static int
_lmidi2cps(lua_State *L)
{
	const lua_Number note = luaL_checknumber(L, 1);
	const lua_Number base = luaL_optnumber(L, 2, 69.0);
	const lua_Number noct = luaL_optnumber(L, 3, 12.0);
	const lua_Number fref = luaL_optnumber(L, 4, 440.0);

	const lua_Number cps = exp2( (note - base) / noct) * fref;

	lua_pushnumber(L, cps);
	return 1;
}

__realtime static int
_lcps2midi(lua_State *L)
{
	const lua_Number cps = luaL_checknumber(L, 1);
	const lua_Number base = luaL_optnumber(L, 2, 69.0);
	const lua_Number noct = luaL_optnumber(L, 3, 12.0);
	const lua_Number fref = luaL_optnumber(L, 4, 440.0);

	const lua_Number note = log2(cps / fref) * noct + base;

	lua_pushnumber(L, note);
	return 1;
}

static const char *note_keys [12] = {
	"C", "C#",
	"D", "D#",
	"E",
	"F", "F#",
	"G", "G#",
	"A", "A#",
	"B"
};

__realtime static int
_lnote__index(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // ignore superfluous arguments

	const int type = lua_type(L, 2);
	if(type == LUA_TNUMBER)
	{
		const int note = lua_tointeger(L, 2);

		if( (note >= 0) && (note < 0x80) )
		{
			char name [16];
			const int8_t octave = note / 12 - 1;
			const uint8_t key = note % 12;
			snprintf(name, 16, "%s%+"PRIi8, note_keys[key], octave);

			lua_pushstring(L, name);
			return 1;
		}
	}
	else if(type == LUA_TSTRING)
	{
		size_t str_len;
		const char *str = lua_tolstring(L, 2, &str_len);

		for(int i=0; i<12; i++)
		{
			const char *key = note_keys[i];
			const size_t key_len = strlen(key);

			if( (str_len - 2 == key_len) && !strncmp(str, key, key_len) )
			{
				const int octave = atoi(str + key_len);
				const int note = (octave + 1)*12 + i;
				if( (note >= 0) && (note < 0x80) )
				{
					lua_pushinteger(L, note);
					return 1;
				}
			}
		}
	}

	lua_pushnil(L);
	return 1;
}

__realtime static int
_lnote__call(lua_State *L)
{
	lua_settop(L, 2);
	lua_gettable(L, -2); // self[uri]

	return 1;
}

static const luaL_Reg lnote_mt [] = {
	{"__index", _lnote__index},
	{"__call", _lnote__index},
	{NULL, NULL}
};

__realtime static int
_log(lua_State *L)
{
	int n = lua_gettop(L);

	if(!n)
		return 0;

	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	moony_vm_t *vm = lua_touserdata(L, lua_upvalueindex(2));

	luaL_Buffer buf;
	luaL_buffinit(L, &buf);

  lua_getglobal(L, "tostring"); //TODO cache this
  for(int i=1; i<=n; i++)
	{
    lua_pushvalue(L, -1);  // function to be called
    lua_pushvalue(L, i);   // value to print
    lua_call(L, 1, 1);
    if(i>1)
			luaL_addlstring(&buf, "\t", 1);
		size_t len;
		const char *s = lua_tolstring(L, -1, &len);
		luaL_addlstring(&buf, s, len);
    lua_pop(L, 1);  // pop result
  }

	luaL_pushresult(&buf);

	size_t len;
	const char *res = lua_tolstring(L, -1, &len);

	if(moony->log)
	{
		if(vm->nrt) // we're running in worker thread
			lv2_log_note(&moony->logger, "%s\n", res);
		else // we're running in rt-thread
			lv2_log_trace(&moony->logger, "%s\n", res);
	}

	if(vm->nrt) // we're running in worker thread
	{
		//FIXME
	}
	else // we're running in rt-hread
	{
		// feedback to UI
		if(!moony->trace_overflow)
		{
			char *end = strrchr(moony->trace, '\0'); // search end of string
			if(end)
			{
				const size_t sz = end - moony->trace + 1;
				if(MOONY_MAX_TRACE_LEN - sz > len)
				{
					if(end != moony->trace)
					{
						*end++ = '\n'; // append to
						*end = '\0';
					}

					snprintf(end, len + 1, "%s", res);
					moony->trace_out = true; // set flag
				}
				else
					moony->trace_overflow = true;
			}
		}
	}

	return 0;
}

__realtime LV2_Atom_Forge_Ref
_sink_rt(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
{
	atom_ser_t *ser = handle;

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;

	const uint32_t new_offset = ser->offset + size;
	if(new_offset > ser->size)
	{
		uint32_t new_size = ser->size << 1;
		while(new_offset > new_size)
			new_size <<= 1;

		assert(ser->moony);
		if(!(ser->buf = moony_rt_realloc(ser->moony->vm, ser->buf, ser->size, new_size)))
			return 0; // realloc failed

		ser->size = new_size;
	}

	memcpy(ser->buf + ser->offset, buf, size);
	ser->offset = new_offset;

	return ref;
}

__non_realtime LV2_Atom_Forge_Ref
_sink_non_rt(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
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

__realtime LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

__realtime static int
_stash(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_getglobal(L, "stash");
	if(lua_isfunction(L, -1))
	{
		lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, true);
		lframe->depth = 0;
		lframe->last.frames = 0;
		lframe->forge = &moony->stash_forge;

		atom_ser_t ser = {
			.moony = moony,
			.size = 1024,
			.offset = 0
		};
		ser.buf = moony_rt_alloc(moony->vm, ser.size);

		if(ser.buf)
		{
			memset(ser.buf, 0x0, sizeof(LV2_Atom));

			lv2_atom_forge_set_sink(lframe->forge, _sink_rt, _deref, &ser);
			lua_call(L, 1, 0);

			if(moony->stash_atom)
				moony_rt_free(moony->vm, moony->stash_atom, moony->stash_size);
			LV2_Atom *atom = (LV2_Atom *)ser.buf;
			moony->stash_atom = atom;
			moony->stash_size = ser.size;
		}
	}
	else
		lua_pop(L, 1);

	return 0;
}

__realtime static int
_apply(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_getglobal(L, "apply");
	if(lua_isfunction(L, -1))
	{
		_latom_new(L, moony->stash_atom, true);
		lua_call(L, 1, 0);
	}
	else
		lua_pop(L, 1);

	return 0;
}

__non_realtime static int
_save(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	if(lua_getglobal(L, "save") == LUA_TFUNCTION)
	{
		lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, true);
		lframe->depth = 0;
		lframe->last.frames = 0;
		lframe->forge = &moony->state_forge;

		lua_call(L, 1, 0);
	}

	return 0;
}

__non_realtime static LV2_State_Status
_state_save(LV2_Handle instance,
	LV2_State_Store_Function store, LV2_State_Handle state,
	uint32_t flags, const LV2_Feature *const *features)
{
	moony_t *moony = (moony_t *)instance;

	LV2_State_Status status = LV2_STATE_SUCCESS;

	if(moony->chunk_nrt)
	{
		status = store(
			state,
			moony->uris.moony_code,
			moony->chunk_nrt,
			strlen(moony->chunk_nrt) + 1,
			moony->forge.String,
			LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
		(void)status; //TODO check status
	}

	const int32_t minor_version = MOONY_MINOR_VERSION;
	status = store(
		state,
		moony->uris.core_minor_version,
		&minor_version,
		sizeof(int32_t),
		moony->forge.Int,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	(void)status; //TODO check status

	const int32_t micro_version = MOONY_MICRO_VERSION;
	status = store(
		state,
		moony->uris.core_micro_version,
		&micro_version,
		sizeof(int32_t),
		moony->forge.Int,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	(void)status; //TODO check status

	int32_t i32 = atomic_load_explicit(&moony->editor_hidden, memory_order_acquire);
	status = store(
		state,
		moony->uris.moony_editorHidden,
		&i32,
		sizeof(int32_t),
		moony->forge.Bool,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	(void)status; //TODO check status

	i32 = atomic_load_explicit(&moony->log_hidden, memory_order_acquire);
	status = store(
		state,
		moony->uris.moony_logHidden,
		&i32,
		sizeof(int32_t),
		moony->forge.Bool,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	(void)status; //TODO check status

	i32 = atomic_load_explicit(&moony->param_hidden, memory_order_acquire);
	status = store(
		state,
		moony->uris.moony_paramHidden,
		&i32,
		sizeof(int32_t),
		moony->forge.Bool,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	(void)status; //TODO check status

	i32 = atomic_load_explicit(&moony->param_cols, memory_order_acquire);
	status = store(
		state,
		moony->uris.moony_paramCols,
		&i32,
		sizeof(int32_t),
		moony->forge.Int,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	(void)status; //TODO check status

	i32 = atomic_load_explicit(&moony->param_rows, memory_order_acquire);
	status = store(
		state,
		moony->uris.moony_paramRows,
		&i32,
		sizeof(int32_t),
		moony->forge.Int,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
	(void)status; //TODO check status

	atom_ser_t ser = {
		.moony = NULL,
		.size = 1024,
		.offset = 0
	};
	ser.buf = malloc(ser.size);

	if(ser.buf)
	{
		memset(ser.buf, 0x0, sizeof(LV2_Atom));

		lv2_atom_forge_set_sink(&moony->state_forge, _sink_non_rt, _deref, &ser);

		// lock Lua state, so it cannot be accessed by realtime thread
		_spin_lock(&moony->state_lock);
		{
			// restore Lua defined properties
			lua_State *L = moony_current(moony);
			lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_SAVE);
			if(lua_pcall(L, 0, 0, 0))
			{
				moony_err_async(moony, lua_tostring(L, -1));
				lua_pop(L, 1);
			}
#ifdef USE_MANUAL_GC
			lua_gc(L, LUA_GCSTEP, 0);
#endif
		}
		_unlock(&moony->state_lock);

		LV2_Atom *state_atom_new = (LV2_Atom *)ser.buf;
		if( (state_atom_new->type) && (state_atom_new->size) )
		{
			status = store(
				state,
				moony->uris.moony_state,
				LV2_ATOM_BODY(state_atom_new),
				state_atom_new->size,
				state_atom_new->type,
				LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
		}

		LV2_Atom *state_atom_old = (LV2_Atom *)atomic_exchange_explicit(&moony->state_atom_new, (uintptr_t)state_atom_new, memory_order_relaxed);
		if(state_atom_old)
			free(state_atom_old);
	}

	return status;
}

__non_realtime static int
_restore(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	if(lua_getglobal(L, "restore") == LUA_TFUNCTION)
	{
		_latom_new(L, moony->state_atom, false);
		lua_call(L, 1, 0);
	}

	return 0;
}

__non_realtime static moony_vm_t *
_compile(moony_t *moony, const char *chunk)
{
	// save a copy for _state_save
	if(moony->chunk_nrt)
		free(moony->chunk_nrt);
	moony->chunk_nrt = strdup(chunk);
	if(!moony->chunk_nrt)
		return NULL;

	char *chunk_new = strdup(chunk);
	if(!chunk_new)
		return NULL;

	// chunk is always updated, even if compilation should fail
	char *chunk_old = (char *)atomic_exchange_explicit(&moony->chunk_new, (uintptr_t)chunk_new, memory_order_relaxed);
	if(chunk_old)
		free(chunk_old);

	moony_vm_t *vm = moony_vm_new(moony->mem_size, moony->testing, moony);
	if(!vm)
	{
		moony_err_async(moony, "moony_vm_new failed");
		return NULL;
	}

	moony_vm_nrt_enter(vm);
	moony_open(moony, vm, vm->L);
	if(luaL_dostring(vm->L, chunk))
	{
		moony_err_async(moony, lua_tostring(vm->L, -1));
		lua_pop(vm->L, 1);

		moony_vm_free(vm);
		return NULL;
	}
	moony_vm_nrt_leave(vm);

	return vm;
}

__non_realtime static LV2_State_Status
_state_restore(LV2_Handle instance,
	LV2_State_Retrieve_Function retrieve, LV2_State_Handle state,
	uint32_t flags, const LV2_Feature *const *features)
{
	moony_t *moony = (moony_t *)instance;
	const LV2_Worker_Schedule *work_sched = NULL;

	for(unsigned i = 0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_WORKER__schedule))
			work_sched = features[i]->data;
	}

	size_t size;
	uint32_t type;
	uint32_t flags2;

	// get minor version
	const int32_t *minor_version = retrieve(
		state,
		moony->uris.core_minor_version,
		&size,
		&type,
		&flags2);
	if(!minor_version || (size != sizeof(int32_t)) || (type != moony->forge.Int) )
		minor_version = NULL;

	// get micro version
	const int32_t *micro_version = retrieve(
		state,
		moony->uris.core_micro_version,
		&size,
		&type,
		&flags2);
	if(!micro_version || (size != sizeof(int32_t)) || (type != moony->forge.Int) )
		micro_version = NULL;

	//TODO check preset version with current plugin version for API compatibility

	// get moony:editorHidden
	const int32_t *i32 = retrieve(
		state,
		moony->uris.moony_editorHidden,
		&size,
		&type,
		&flags2);
	if(i32 && (size == sizeof(int32_t)) && (type == moony->forge.Bool) )
	{
		atomic_store_explicit(&moony->editor_hidden, *i32, memory_order_release);
	}

	// get moony:logHidden
	i32 = retrieve(
		state,
		moony->uris.moony_logHidden,
		&size,
		&type,
		&flags2);
	if(i32 && (size == sizeof(int32_t)) && (type == moony->forge.Bool) )
	{
		atomic_store_explicit(&moony->log_hidden, *i32, memory_order_release);
	}

	// get moony:paramHidden
	i32 = retrieve(
		state,
		moony->uris.moony_paramHidden,
		&size,
		&type,
		&flags2);
	if(i32 && (size == sizeof(int32_t)) && (type == moony->forge.Bool) )
	{
		atomic_store_explicit(&moony->param_hidden, *i32, memory_order_release);
	}

	// get moony:paramCols
	i32 = retrieve(
		state,
		moony->uris.moony_paramCols,
		&size,
		&type,
		&flags2);
	if(i32 && (size == sizeof(int32_t)) && (type == moony->forge.Int) )
	{
		atomic_store_explicit(&moony->param_cols, *i32, memory_order_release);
	}

	// get moony:paramRows
	i32 = retrieve(
		state,
		moony->uris.moony_paramRows,
		&size,
		&type,
		&flags2);
	if(i32 && (size == sizeof(int32_t)) && (type == moony->forge.Int) )
	{
		atomic_store_explicit(&moony->param_rows, *i32, memory_order_release);
	}

	// get state
	const uint8_t *body = retrieve(
		state,
		moony->uris.moony_state,
		&size,
		&type,
		&flags2
	);

	if(body && size && type)
	{
		// allocate new state_atom
		LV2_Atom *state_atom_new = malloc(sizeof(LV2_Atom) + size);
		if(state_atom_new)
		{
			state_atom_new->size = size;
			state_atom_new->type = type;
			memcpy(LV2_ATOM_BODY(state_atom_new), body, size);

			LV2_Atom *state_atom_old = (LV2_Atom *)atomic_exchange_explicit(&moony->state_atom_new, (uintptr_t)state_atom_new, memory_order_relaxed);
			if(state_atom_old)
				free(state_atom_old);
		}
	}

	// get code chunk
	const char *chunk = retrieve(
		state,
		moony->uris.moony_code,
		&size,
		&type,
		&flags2);

	if(chunk && size && (type == moony->forge.String) )
	{
		if(size <= MOONY_MAX_CHUNK_LEN)
		{
			moony_vm_t *vm_new = _compile(moony, chunk);
			if(vm_new)
			{
				moony_vm_t *vm_old = (moony_vm_t *)atomic_exchange_explicit(&moony->vm_new, (uintptr_t)vm_new, memory_order_relaxed);
				if(vm_old)
					moony_vm_free(vm_old);
			}
		}
		else
		{
			moony_err_async(moony, "restore: moony:code too long");
		}
	}
	else
	{
		moony_err_async(moony, "restore: moony:code property not found");
	}

	return LV2_STATE_SUCCESS;
}

static const LV2_State_Interface state_iface = {
	.save = _state_save,
	.restore = _state_restore
};

__non_realtime static LV2_Worker_Status
_work_job(moony_t *moony,
	LV2_Worker_Respond_Function respond,
	LV2_Worker_Respond_Handle target,
	const moony_job_t *job)
{
	//printf("_work: %i\n", job->type);

	switch(job->type)
	{
		case MOONY_JOB_MEM_ALLOC:
		{
			const int32_t i = job->mem.i;
			moony->vm->area[i] = moony_vm_mem_alloc(moony->vm->size[i]);

			const moony_job_t req = {
				.type = MOONY_JOB_MEM_ALLOC,
				.mem = {
					.i = i
				}
			};

			return respond(target, sizeof(moony_job_t), &req); // signal to _work_response
		} break;
		case MOONY_JOB_MEM_FREE:
		{
			const int32_t i =job->mem.i;
			void *ptr= job->mem.ptr;

			moony_vm_mem_free(ptr, moony->vm->size[i]);
		} break;
		case MOONY_JOB_VM_ALLOC:
		{
			moony_vm_t *vm_new = _compile(moony, job->chunk);
			if(vm_new)
			{
				moony_vm_t *vm_old = (moony_vm_t *)atomic_exchange_explicit(&moony->vm_new, (uintptr_t)vm_new, memory_order_relaxed);
				if(vm_old)
					moony_vm_free(vm_old);
			}
			else
			{
				return LV2_WORKER_ERR_UNKNOWN;
			}
		} break;
		case MOONY_JOB_VM_FREE:
		{
			moony_vm_free(job->vm);
		} break;
		case MOONY_JOB_PTR_FREE:
		{
			free(job->ptr);
		} break;
	}

	return LV2_WORKER_SUCCESS;
}

__non_realtime static LV2_Worker_Status
_work(LV2_Handle instance,
	LV2_Worker_Respond_Function respond,
	LV2_Worker_Respond_Handle target,
	uint32_t size,
	const void *body)
{
	moony_t *moony = instance;

	LV2_Worker_Status status = LV2_WORKER_SUCCESS;

	size_t sz;
	const moony_job_t *job;
	while((job = varchunk_read_request(moony->from_dsp, &sz)))
	{
		status = _work_job(moony, respond, target, job);

		varchunk_read_advance(moony->from_dsp);
	}

	return status;
}

__realtime static LV2_Worker_Status
_work_response(LV2_Handle instance, uint32_t size, const void *body)
{
	moony_t *moony = instance;
	const moony_job_t *job = body;

	//printf("_work_response: %i\n", job->type);
	switch(job->type)
	{
		case MOONY_JOB_MEM_ALLOC:
		{
			const int32_t i = job->mem.i;

			if(!moony->vm->area[i]) // allocation failed
				return LV2_WORKER_ERR_UNKNOWN;

			// tlsf add pool
			moony->vm->pool[i] = tlsf_add_pool(moony->vm->tlsf,
				moony->vm->area[i], moony->vm->size[i]); //FIXME stoat complains about printf

			if(!moony->vm->pool[i]) // pool addition failed
			{
				moony_job_t *req;
				if((req = varchunk_write_request(moony->from_dsp, sizeof(moony_job_t))))
				{
					req->type = MOONY_JOB_MEM_FREE;
					req->mem.i = i;
					req->mem.ptr = moony->vm->area[i];

					moony->vm->area[i] = NULL;

					varchunk_write_advance(moony->from_dsp, sizeof(moony_job_t));
					if(moony_wake_worker(moony->sched) != LV2_WORKER_SUCCESS)
						moony_trace(moony, "waking worker failed");
				}

				return LV2_WORKER_ERR_UNKNOWN;
			}

			moony->vm->space += moony->vm->size[i];
			//printf("mem extended to %zu KB\n", moony->vm->space / 1024);
		} break;

		case MOONY_JOB_VM_ALLOC:
		case MOONY_JOB_MEM_FREE:
		case MOONY_JOB_VM_FREE:
		case MOONY_JOB_PTR_FREE:
			break; // never reached
	}

	return LV2_WORKER_SUCCESS;
}

__realtime static LV2_Worker_Status
_end_run(LV2_Handle instance)
{
	moony_t *moony = instance;

	// do nothing
	moony->working = false;

	return LV2_WORKER_SUCCESS;
}

static const LV2_Worker_Interface work_iface = {
	.work = _work,
	.work_response = _work_response,
	.end_run = _end_run
};

__non_realtime const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_WORKER__interface))
		return &work_iface;
	else if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;
	else
		return NULL;
}

__non_realtime int
moony_init(moony_t *moony, const char *subject, double sample_rate,
	const LV2_Feature *const *features, size_t mem_size, bool testing)
{
	atomic_init(&moony->state_atom_new, 0);
	atomic_init(&moony->vm_new, 0);
	atomic_init(&moony->err_new, 0);
	atomic_init(&moony->chunk_new, 0);
	moony->state_lock = (atomic_flag)ATOMIC_FLAG_INIT;

	moony->from_dsp = varchunk_new(MOONY_MAX_CHUNK_LEN * 2, true);
	if(!moony->from_dsp)
	{
		fprintf(stderr, "varchunk_new failed\n");
		return -1;
	}

	moony->mem_size = mem_size;
	moony->testing = testing;
	moony->vm = moony_vm_new(moony->mem_size, testing, moony);
	if(!moony->vm)
	{
		fprintf(stderr, "Lua VM cannot be initialized\n");
		return -1;
	}

	LV2_Options_Option *opts = NULL;
	bool load_default_state = false;

	for(unsigned i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			moony->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			moony->unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_WORKER__schedule))
			moony->sched = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			moony->log = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_OPTIONS__options))
			opts = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_OSC__schedule))
			moony->osc_sched = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_STATE__loadDefaultState))
			load_default_state = true;
		else if(!strcmp(features[i]->URI, XPRESS_VOICE_MAP))
			moony->voice_map = features[i]->data;
	}

	if(!moony->map)
	{
		fprintf(stderr, "Host does not support urid:map\n");
		return -1;
	}
	if(!moony->unmap)
	{
		fprintf(stderr, "Host does not support urid:unmap\n");
		return -1;
	}
	if(!moony->sched)
	{
		fprintf(stderr, "Host does not support worker:schedule\n");
		return -1;
	}
	if(!load_default_state)
	{
		strcpy(moony->chunk,
			"-- host does not support state:loadDefaultState feature\n\n"
			"function run(n, control, notify, ...)\n"
			"end");
		moony->chunk_nrt = strdup(moony->chunk);
	}
	if(!moony->voice_map)
		moony->voice_map = &voice_map_fallback;

	moony->uris.moony_code = moony->map->map(moony->map->handle, MOONY_CODE_URI);
	moony->uris.moony_selection = moony->map->map(moony->map->handle, MOONY_SELECTION_URI);
	moony->uris.moony_error = moony->map->map(moony->map->handle, MOONY_ERROR_URI);
	moony->uris.moony_trace = moony->map->map(moony->map->handle, MOONY_TRACE_URI);
	moony->uris.moony_panic = moony->map->map(moony->map->handle, MOONY_PANIC_URI);
	moony->uris.moony_state = moony->map->map(moony->map->handle, MOONY_STATE_URI);
	moony->uris.moony_editorHidden = moony->map->map(moony->map->handle, MOONY_EDITOR_HIDDEN_URI);
	moony->uris.moony_logHidden = moony->map->map(moony->map->handle, MOONY_LOG_HIDDEN_URI);
	moony->uris.moony_paramHidden = moony->map->map(moony->map->handle, MOONY_PARAM_HIDDEN_URI);
	moony->uris.moony_paramCols = moony->map->map(moony->map->handle, MOONY_PARAM_COLS_URI);
	moony->uris.moony_paramRows = moony->map->map(moony->map->handle, MOONY_PARAM_ROWS_URI);

	moony->uris.midi_event = moony->map->map(moony->map->handle, LV2_MIDI__MidiEvent);

	moony->uris.bufsz_min_block_length = moony->map->map(moony->map->handle,
		LV2_BUF_SIZE__minBlockLength);
	moony->uris.bufsz_max_block_length = moony->map->map(moony->map->handle,
		LV2_BUF_SIZE__maxBlockLength);
	moony->uris.bufsz_sequence_size = moony->map->map(moony->map->handle,
		LV2_BUF_SIZE__sequenceSize);
	moony->uris.ui_update_rate= moony->map->map(moony->map->handle,
		LV2_UI__updateRate);

	moony->uris.patch.self = moony->map->map(moony->map->handle, subject);

	moony->uris.patch.get = moony->map->map(moony->map->handle, LV2_PATCH__Get);
	moony->uris.patch.set = moony->map->map(moony->map->handle, LV2_PATCH__Set);
	moony->uris.patch.put = moony->map->map(moony->map->handle, LV2_PATCH__Put);
	moony->uris.patch.patch = moony->map->map(moony->map->handle, LV2_PATCH__Patch);
	moony->uris.patch.body = moony->map->map(moony->map->handle, LV2_PATCH__body);
	moony->uris.patch.subject = moony->map->map(moony->map->handle, LV2_PATCH__subject);
	moony->uris.patch.property = moony->map->map(moony->map->handle, LV2_PATCH__property);
	moony->uris.patch.value = moony->map->map(moony->map->handle, LV2_PATCH__value);
	moony->uris.patch.add = moony->map->map(moony->map->handle, LV2_PATCH__add);
	moony->uris.patch.remove = moony->map->map(moony->map->handle, LV2_PATCH__remove);
	moony->uris.patch.wildcard = moony->map->map(moony->map->handle, LV2_PATCH__wildcard);
	moony->uris.patch.writable = moony->map->map(moony->map->handle, LV2_PATCH__writable);
	moony->uris.patch.readable = moony->map->map(moony->map->handle, LV2_PATCH__readable);
	moony->uris.patch.destination = moony->map->map(moony->map->handle, LV2_PATCH__destination);
	moony->uris.patch.sequence = moony->map->map(moony->map->handle, LV2_PATCH__sequenceNumber);
	moony->uris.patch.error = moony->map->map(moony->map->handle, LV2_PATCH__Error);
	moony->uris.patch.ack = moony->map->map(moony->map->handle, LV2_PATCH__Ack);

	moony->uris.rdfs_label = moony->map->map(moony->map->handle, RDFS__label);
	moony->uris.rdfs_range = moony->map->map(moony->map->handle, RDFS__range);
	moony->uris.rdfs_comment = moony->map->map(moony->map->handle, RDFS__comment);

	moony->uris.rdf_value = moony->map->map(moony->map->handle, RDF__value);

	moony->uris.core_minimum = moony->map->map(moony->map->handle, LV2_CORE__minimum);
	moony->uris.core_maximum = moony->map->map(moony->map->handle, LV2_CORE__maximum);
	moony->uris.core_scale_point = moony->map->map(moony->map->handle, LV2_CORE__scalePoint);
	moony->uris.core_minor_version= moony->map->map(moony->map->handle, LV2_CORE__minorVersion);
	moony->uris.core_micro_version= moony->map->map(moony->map->handle, LV2_CORE__microVersion);

	moony->uris.units_unit = moony->map->map(moony->map->handle, LV2_UNITS__unit);
	moony->uris.units_symbol = moony->map->map(moony->map->handle, LV2_UNITS__symbol);

	moony->uris.atom_frame_time = moony->map->map(moony->map->handle, LV2_ATOM__frameTime);
	moony->uris.atom_beat_time = moony->map->map(moony->map->handle, LV2_ATOM__beatTime);

	lv2_canvas_urid_init(&moony->canvas_urid, moony->map);

	lv2_osc_urid_init(&moony->osc_urid, moony->map);
	lv2_atom_forge_init(&moony->forge, moony->map);
	lv2_atom_forge_init(&moony->state_forge, moony->map);
	lv2_atom_forge_init(&moony->stash_forge, moony->map);
	lv2_atom_forge_init(&moony->notify_forge, moony->map);
	if(moony->log)
		lv2_log_logger_init(&moony->logger, moony->map, moony->log);

	latom_driver_hash_t *latom_driver_hash = moony->atom_driver_hash;
	unsigned pos = 0;

	latom_driver_hash[pos].type = 0;
	latom_driver_hash[pos++].driver = &latom_nil_driver;

	latom_driver_hash[pos].type = moony->forge.Int;
	latom_driver_hash[pos++].driver = &latom_int_driver;

	latom_driver_hash[pos].type = moony->forge.Long;
	latom_driver_hash[pos++].driver = &latom_long_driver;

	latom_driver_hash[pos].type = moony->forge.Float;
	latom_driver_hash[pos++].driver = &latom_float_driver;

	latom_driver_hash[pos].type = moony->forge.Double;
	latom_driver_hash[pos++].driver = &latom_double_driver;

	latom_driver_hash[pos].type = moony->forge.Bool;
	latom_driver_hash[pos++].driver = &latom_bool_driver;

	latom_driver_hash[pos].type = moony->forge.URID;
	latom_driver_hash[pos++].driver = &latom_urid_driver;

	latom_driver_hash[pos].type = moony->forge.String;
	latom_driver_hash[pos++].driver = &latom_string_driver;

	latom_driver_hash[pos].type = moony->forge.URI;
	latom_driver_hash[pos++].driver = &latom_string_driver;

	latom_driver_hash[pos].type = moony->forge.Path;
	latom_driver_hash[pos++].driver = &latom_string_driver;

	latom_driver_hash[pos].type = moony->forge.Literal;
	latom_driver_hash[pos++].driver = &latom_literal_driver;

	latom_driver_hash[pos].type = moony->forge.Tuple;
	latom_driver_hash[pos++].driver = &latom_tuple_driver;

	latom_driver_hash[pos].type = moony->forge.Object;
	latom_driver_hash[pos++].driver = &latom_object_driver;

	latom_driver_hash[pos].type = moony->forge.Vector;
	latom_driver_hash[pos++].driver = &latom_vector_driver;

	latom_driver_hash[pos].type = moony->forge.Sequence;
	latom_driver_hash[pos++].driver = &latom_sequence_driver;

	assert(pos == DRIVER_HASH_MAX);
	qsort(latom_driver_hash, DRIVER_HASH_MAX, sizeof(latom_driver_hash_t), _hash_sort);

	if(opts)
	{
		for(LV2_Options_Option *opt = opts;
			(opt->key != 0) && (opt->value != NULL);
			opt++)
		{
			if(opt->key == moony->uris.bufsz_min_block_length)
				moony->opts.min_block_length = *(int32_t *)opt->value;
			else if(opt->key == moony->uris.bufsz_max_block_length)
				moony->opts.max_block_length = *(int32_t *)opt->value;
			else if(opt->key == moony->uris.bufsz_sequence_size)
				moony->opts.sequence_size = *(int32_t *)opt->value;
			else if(opt->key == moony->uris.ui_update_rate)
				moony->opts.update_rate = *(float *)opt->value;
		}
	}
	moony->opts.sample_rate = sample_rate;

	moony->dirty_out = true; // trigger update of UI
	moony->props_out = true; // trigger update of UI

	moony_freeuserdata(moony);

	moony->editor_hidden = ATOMIC_VAR_INIT(0);
	moony->log_hidden = ATOMIC_VAR_INIT(1);
	moony->param_hidden = ATOMIC_VAR_INIT(1);
	moony->param_cols = ATOMIC_VAR_INIT(3);
	moony->param_rows = ATOMIC_VAR_INIT(4);

	return 0;
}

__non_realtime void
moony_deinit(moony_t *moony)
{
	LV2_Atom *state_atom_old = (LV2_Atom *)atomic_load_explicit(&moony->state_atom_new, memory_order_relaxed);
	if(state_atom_old)
		free(state_atom_old);
	if(moony->state_atom)
		free(moony->state_atom);

	if(moony->stash_atom)
		moony_rt_free(moony->vm, moony->stash_atom, moony->stash_size);
	moony->stash_atom = NULL;
	moony->stash_size = 0;

	moony_vm_t *vm_old = (moony_vm_t *)atomic_load_explicit(&moony->vm_new, memory_order_relaxed);
	if(vm_old)
		moony_vm_free(vm_old);
	if(moony->vm)
		moony_vm_free(moony->vm);

	char *err_old = (char *)atomic_load_explicit(&moony->err_new, memory_order_relaxed);
	if(err_old)
		free(err_old);

	char *chunk_old = (char *)atomic_load_explicit(&moony->chunk_new, memory_order_relaxed);
	if(chunk_old)
		free(chunk_old);

	if(moony->chunk_nrt)
		free(moony->chunk_nrt);

	if(moony->from_dsp)
		varchunk_free(moony->from_dsp);
}

#define _protect_metatable(L, idx) \
	lua_pushboolean(L, 0); \
	lua_setfield(L, idx - 1, "__metatable");

#define _index_metatable(L, idx) \
	lua_pushvalue(L, idx); \
	lua_setfield(L, idx - 1, "__index");

__non_realtime void
moony_open(moony_t *moony, moony_vm_t *vm, lua_State *L)
{
	luaL_newmetatable(L, "latom");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, latom_mt, 1);
	_protect_metatable(L, -1);
	lua_pop(L, 1);

	luaL_newmetatable(L, "lforge");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lforge_mt, 1);
	_protect_metatable(L, -1);
	_index_metatable(L, -1);
	lua_pop(L, 1);

	luaL_newmetatable(L, "lstash");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lstash_mt, 1);
	_protect_metatable(L, -1);
	_index_metatable(L, -1);
	lua_pop(L, 1);

	// lv2.map
	lua_newtable(L);
	lua_newtable(L);
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs(L, lmap_mt, 1);
	_protect_metatable(L, -1);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Map");

	// lv2.unmap
	lua_newtable(L);
	lua_newtable(L);
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs(L, lunmap_mt, 1);
	_protect_metatable(L, -1);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Unmap");

	// lv2.hash
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lhash_map, 1);
	lua_setglobal(L, "HashMap");

	// lv2.voiceMap
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lvoice_map, 1);
	lua_setglobal(L, "VoiceMap");

	// lv2.midi2cps
	lua_pushcclosure(L, _lmidi2cps, 0);
	lua_setglobal(L, "midi2cps");

	// lv2cps2midi.
	lua_pushcclosure(L, _lcps2midi, 0);
	lua_setglobal(L, "cps2midi");

#define SET_MAP(L, PREFIX, PROPERTY) \
({ \
	lua_getglobal(L, "Map"); \
	lua_getfield(L, -1, PREFIX ## PROPERTY); \
	LV2_URID urid = luaL_checkinteger(L, -1); \
	lua_remove(L, -2); /* Map */ \
	lua_setfield(L, -2, #PROPERTY); \
	urid; \
})

	LV2_URID core_sample_rate;

	lua_newtable(L);
	{
		SET_MAP(L, LV2_CORE__, ControlPort);
	}
	lua_setglobal(L, "State");

	lua_newtable(L);
	{
		//SET_MAP(L, LV2_ATOM__, Blank); // is depracated
		SET_MAP(L, LV2_ATOM__, Bool);
		SET_MAP(L, LV2_ATOM__, Chunk);
		SET_MAP(L, LV2_ATOM__, Double);
		SET_MAP(L, LV2_ATOM__, Float);
		SET_MAP(L, LV2_ATOM__, Int);
		SET_MAP(L, LV2_ATOM__, Long);
		SET_MAP(L, LV2_ATOM__, Literal);
		SET_MAP(L, LV2_ATOM__, Object);
		SET_MAP(L, LV2_ATOM__, Path);
		SET_MAP(L, LV2_ATOM__, Property);
		//SET_MAP(L, LV2_ATOM__, Resource); // is deprecated
		SET_MAP(L, LV2_ATOM__, Sequence);
		SET_MAP(L, LV2_ATOM__, String);
		SET_MAP(L, LV2_ATOM__, Tuple);
		SET_MAP(L, LV2_ATOM__, URI);
		SET_MAP(L, LV2_ATOM__, URID);
		SET_MAP(L, LV2_ATOM__, Vector);
		SET_MAP(L, LV2_ATOM__, beatTime);
		SET_MAP(L, LV2_ATOM__, frameTime);
	}
	lua_setglobal(L, "Atom");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_MIDI__, MidiEvent);

		for(const midi_msg_t *msg=midi_msgs; msg->key; msg++)
		{
			lua_pushinteger(L, msg->type);
			lua_setfield(L, -2, msg->key);
		}
		for(const midi_msg_t *msg=controllers; msg->key; msg++)
		{
			lua_pushinteger(L, msg->type);
			lua_setfield(L, -2, msg->key);
		}
	}
	lua_setglobal(L, "MIDI");

	// Note
	lua_newtable(L);
	lua_newtable(L);
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs(L, lnote_mt, 1);
	_protect_metatable(L, -1);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Note");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_TIME__, Position);
		SET_MAP(L, LV2_TIME__, barBeat);
		SET_MAP(L, LV2_TIME__, bar);
		SET_MAP(L, LV2_TIME__, beat);
		SET_MAP(L, LV2_TIME__, beatUnit);
		SET_MAP(L, LV2_TIME__, beatsPerBar);
		SET_MAP(L, LV2_TIME__, beatsPerMinute);
		SET_MAP(L, LV2_TIME__, frame);
		SET_MAP(L, LV2_TIME__, framesPerSecond);
		SET_MAP(L, LV2_TIME__, speed);
	}
	lua_setglobal(L, "Time");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_OSC__, Event);
		SET_MAP(L, LV2_OSC__, Packet);
		SET_MAP(L, LV2_OSC__, Bundle);
		SET_MAP(L, LV2_OSC__, bundleTimetag);
		SET_MAP(L, LV2_OSC__, bundleItems);
		SET_MAP(L, LV2_OSC__, Message);
		SET_MAP(L, LV2_OSC__, messagePath);
		SET_MAP(L, LV2_OSC__, messageArguments);
		SET_MAP(L, LV2_OSC__, Timetag);
		SET_MAP(L, LV2_OSC__, timetagIntegral);
		SET_MAP(L, LV2_OSC__, timetagFraction);
		SET_MAP(L, LV2_OSC__, Nil);
		SET_MAP(L, LV2_OSC__, Impulse);
		SET_MAP(L, LV2_OSC__, Char);
		SET_MAP(L, LV2_OSC__, RGBA);
	}
	lua_setglobal(L, "OSC");

	lua_newtable(L);
	{
		core_sample_rate = SET_MAP(L, LV2_CORE__, sampleRate);
		SET_MAP(L, LV2_CORE__, minimum);
		SET_MAP(L, LV2_CORE__, maximum);
		SET_MAP(L, LV2_CORE__, scalePoint);
	}
	lua_setglobal(L, "Core");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_BUF_SIZE__, minBlockLength);
		SET_MAP(L, LV2_BUF_SIZE__, maxBlockLength);
		SET_MAP(L, LV2_BUF_SIZE__, sequenceSize);
	}
	lua_setglobal(L, "Buf_Size");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_PATCH__, Ack);
		SET_MAP(L, LV2_PATCH__, Delete);
		SET_MAP(L, LV2_PATCH__, Copy);
		SET_MAP(L, LV2_PATCH__, Error);
		SET_MAP(L, LV2_PATCH__, Get);
		SET_MAP(L, LV2_PATCH__, Message);
		SET_MAP(L, LV2_PATCH__, Move);
		SET_MAP(L, LV2_PATCH__, Patch);
		SET_MAP(L, LV2_PATCH__, Post);
		SET_MAP(L, LV2_PATCH__, Put);
		SET_MAP(L, LV2_PATCH__, Request);
		SET_MAP(L, LV2_PATCH__, Response);
		SET_MAP(L, LV2_PATCH__, Set);
		SET_MAP(L, LV2_PATCH__, add);
		SET_MAP(L, LV2_PATCH__, body);
		SET_MAP(L, LV2_PATCH__, destination);
		SET_MAP(L, LV2_PATCH__, property);
		SET_MAP(L, LV2_PATCH__, readable);
		SET_MAP(L, LV2_PATCH__, remove);
		SET_MAP(L, LV2_PATCH__, request);
		SET_MAP(L, LV2_PATCH__, subject);
		SET_MAP(L, LV2_PATCH__, sequenceNumber);
		SET_MAP(L, LV2_PATCH__, value);
		SET_MAP(L, LV2_PATCH__, wildcard);
		SET_MAP(L, LV2_PATCH__, writable);
	}
	lua_setglobal(L, "Patch");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_UI__, updateRate);
		//TODO add more and document
	}
	lua_setglobal(L, "Ui");

	lua_newtable(L);
	{
		SET_MAP(L, RDF__, value);
		SET_MAP(L, RDF__, type);
	}
	lua_setglobal(L, "RDF");

	lua_newtable(L);
	{
		SET_MAP(L, RDFS__, label);
		SET_MAP(L, RDFS__, range);
		SET_MAP(L, RDFS__, comment);
	}
	lua_setglobal(L, "RDFS");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_UNITS__, Conversion);
		SET_MAP(L, LV2_UNITS__, Unit);
		SET_MAP(L, LV2_UNITS__, bar);
		SET_MAP(L, LV2_UNITS__, beat);
		SET_MAP(L, LV2_UNITS__, bpm);
		SET_MAP(L, LV2_UNITS__, cent);
		SET_MAP(L, LV2_UNITS__, cm);
		SET_MAP(L, LV2_UNITS__, coef);
		SET_MAP(L, LV2_UNITS__, conversion);
		SET_MAP(L, LV2_UNITS__, db);
		SET_MAP(L, LV2_UNITS__, degree);
		SET_MAP(L, LV2_UNITS__, frame);
		SET_MAP(L, LV2_UNITS__, hz);
		SET_MAP(L, LV2_UNITS__, inch);
		SET_MAP(L, LV2_UNITS__, khz);
		SET_MAP(L, LV2_UNITS__, km);
		SET_MAP(L, LV2_UNITS__, m);
		SET_MAP(L, LV2_UNITS__, mhz);
		SET_MAP(L, LV2_UNITS__, midiNote);
		SET_MAP(L, LV2_UNITS__, midiController);
		SET_MAP(L, LV2_UNITS__, mile);
		SET_MAP(L, LV2_UNITS__, min);
		SET_MAP(L, LV2_UNITS__, mm);
		SET_MAP(L, LV2_UNITS__, ms);
		SET_MAP(L, LV2_UNITS__, name);
		SET_MAP(L, LV2_UNITS__, oct);
		SET_MAP(L, LV2_UNITS__, pc);
		SET_MAP(L, LV2_UNITS__, prefixConversion);
		SET_MAP(L, LV2_UNITS__, render);
		SET_MAP(L, LV2_UNITS__, s);
		SET_MAP(L, LV2_UNITS__, semitone12TET);
		SET_MAP(L, LV2_UNITS__, symbol);
		SET_MAP(L, LV2_UNITS__, unit);
	}
	lua_setglobal(L, "Units");

	lua_newtable(L);
	{
		SET_MAP(L, CANVAS__, graph);
		SET_MAP(L, CANVAS__, body);
		SET_MAP(L, CANVAS__, BeginPath);
		SET_MAP(L, CANVAS__, ClosePath);
		SET_MAP(L, CANVAS__, Arc);
		SET_MAP(L, CANVAS__, CurveTo);
		SET_MAP(L, CANVAS__, LineTo);
		SET_MAP(L, CANVAS__, MoveTo);
		SET_MAP(L, CANVAS__, Rectangle);
		SET_MAP(L, CANVAS__, Style);
		SET_MAP(L, CANVAS__, LineWidth);
		SET_MAP(L, CANVAS__, LineDash);
		SET_MAP(L, CANVAS__, LineCap);
		SET_MAP(L, CANVAS__, LineJoin);
		SET_MAP(L, CANVAS__, MiterLimit);
		SET_MAP(L, CANVAS__, Stroke);
		SET_MAP(L, CANVAS__, Fill);
		SET_MAP(L, CANVAS__, Clip);
		SET_MAP(L, CANVAS__, Save);
		SET_MAP(L, CANVAS__, Restore);
		SET_MAP(L, CANVAS__, Translate);
		SET_MAP(L, CANVAS__, Scale);
		SET_MAP(L, CANVAS__, Rotate);
		SET_MAP(L, CANVAS__, Reset);
		SET_MAP(L, CANVAS__, FontSize);
		SET_MAP(L, CANVAS__, FillText);
		SET_MAP(L, CANVAS__, lineCapButt);
		SET_MAP(L, CANVAS__, lineCapRound);
		SET_MAP(L, CANVAS__, lineCapSquare);
		SET_MAP(L, CANVAS__, lineJoinMiter);
		SET_MAP(L, CANVAS__, lineJoinRound);
		SET_MAP(L, CANVAS__, lineJoinBevel);
		SET_MAP(L, CANVAS__, mouseButtonLeft);
		SET_MAP(L, CANVAS__, mouseButtonMiddle);
		SET_MAP(L, CANVAS__, mouseButtonRight);
		SET_MAP(L, CANVAS__, mouseWheelX);
		SET_MAP(L, CANVAS__, mouseWheelY);
		SET_MAP(L, CANVAS__, mousePositionX);
		SET_MAP(L, CANVAS__, mousePositionY);
		SET_MAP(L, CANVAS__, mouseFocus);
	}
	lua_setglobal(L, "Canvas");

	lua_newtable(L);
	{
		if(moony->opts.min_block_length)
		{
			lua_pushinteger(L, moony->uris.bufsz_min_block_length);
			lua_pushinteger(L, moony->opts.min_block_length);
			lua_rawset(L, -3);
		}

		if(moony->opts.max_block_length)
		{
			lua_pushinteger(L, moony->uris.bufsz_max_block_length);
			lua_pushinteger(L, moony->opts.max_block_length);
			lua_rawset(L, -3);
		}

		if(moony->opts.sequence_size)
		{
			lua_pushinteger(L, moony->uris.bufsz_sequence_size);
			lua_pushinteger(L, moony->opts.sequence_size);
			lua_rawset(L, -3);
		}

		if(moony->opts.sample_rate)
		{
			lua_pushinteger(L, core_sample_rate);
			lua_pushnumber(L, moony->opts.sample_rate);
			lua_rawset(L, -3);
		}

		if(moony->opts.update_rate)
		{
			lua_pushinteger(L, moony->uris.ui_update_rate);
			lua_pushnumber(L, moony->opts.update_rate);
			lua_rawset(L, -3);
		}
	}
	lua_setglobal(L, "Options");

	// create userdata caches
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_ATOM);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_FORGE);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_STASH);

	// overwrite print function with LV2 log
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushlightuserdata(L, vm); // @ upvalueindex 2
	lua_pushcclosure(L, _log, 2);
	lua_setglobal(L, "print");

	// MIDIResponder metatable
	luaL_newmetatable(L, "lmidiresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lmidiresponder_mt, 1);
	_protect_metatable(L, -1);
	_index_metatable(L, -1);
	lua_pop(L, 1);

	// MIDIResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lmidiresponder, 1);
	lua_setglobal(L, "MIDIResponder");

	// OSCResponder metatable
	luaL_newmetatable(L, "loscresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, loscresponder_mt, 1);
	_protect_metatable(L, -1);
	_index_metatable(L, -1);
	lua_pop(L, 1);

	// OSCResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _loscresponder, 1);
	lua_setglobal(L, "OSCResponder");

	// OSCResponder pattern matcher
	luaL_dostring(L, loscresponder_match); // cannot fail

	// TimeResponder metatable
	luaL_newmetatable(L, "ltimeresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, ltimeresponder_mt, 1);
	_protect_metatable(L, -1);
	// we have a __index function, thus no __index table here
	lua_pop(L, 1);

	// TimeResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _ltimeresponder, 1);
	lua_setglobal(L, "TimeResponder");

	// StateResponder metatable
	luaL_newmetatable(L, "lstateresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lstateresponder_mt, 1);
	_protect_metatable(L, -1);
	_index_metatable(L, -1);
	lua_pop(L, 1);

	// StateResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lstateresponder, 1);
	lua_setglobal(L, "StateResponder");

	// Parameter metatable
	luaL_newmetatable(L, "lparameter");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lparameter_mt, 1);
	_protect_metatable(L, -1);
	//_index_metatable(L, -1);
	lua_pop(L, 1);

	// Parameter factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lparameter, 1);
	lua_setglobal(L, "Parameter");

	// Stash factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lstash, 1);
	lua_setglobal(L, "Stash");

	// create cclosure caches
	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _save, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_SAVE);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _restore, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_RESTORE);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _stash, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_STASH);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _apply, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_APPLY);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _ltimeresponder_stash, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TIME_STASH);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _ltimeresponder_apply, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TIME_APPLY);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_clone, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_CLONE);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _lstash_write, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_WRITE);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _lstash_read, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_READ);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_literal_unpack, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_LIT_UNPACK);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_tuple_unpack, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TUPLE_UNPACK);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_vec_unpack, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_VECTOR_UNPACK);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_chunk_unpack, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_CHUNK_UNPACK);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_tuple_foreach, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TUPLE_FOREACH);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_vec_foreach, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_VECTOR_FOREACH);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_obj_foreach, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_OBJECT_FOREACH);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_seq_foreach, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_SEQUENCE_FOREACH);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _lforge_autopop_itr, 1);
	lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_FORGE_AUTOPOP_ITR);

	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_COUNT + MOONY_UPCLOSURE_TUPLE_FOREACH);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_COUNT + MOONY_UPCLOSURE_VECTOR_FOREACH);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_COUNT + MOONY_UPCLOSURE_OBJECT_FOREACH);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_COUNT + MOONY_UPCLOSURE_SEQUENCE_FOREACH);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_COUNT + MOONY_UPCLOSURE_SEQUENCE_MULTIPLEX);

#undef SET_MAP
}

__realtime void *
moony_newuserdata(lua_State *L, moony_t *moony, moony_udata_t type, bool cache)
{
	assert( (type >= MOONY_UDATA_ATOM) && (type < MOONY_UDATA_COUNT) );

	int *itr = &moony->itr[type];
	void *data = NULL;

	if(cache) // do cash this!
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + type); // ref
		if(lua_rawgeti(L, -1, *itr) == LUA_TNIL) // no cached udata, create one!
		{
#if 0
			if(moony->log)
				lv2_log_trace(&moony->logger, "moony_newuserdata: %s\n", moony_ref[type]);
#endif
			lua_pop(L, 1); // nil

			data = lua_newuserdata(L, moony_sz[type]);
			lheader_t *lheader = data;
			lheader->type = type;
			lheader->cache = cache;
			luaL_getmetatable(L, moony_ref[type]);
			lua_setmetatable(L, -2);
			lua_pushvalue(L, -1);
			lua_rawseti(L, -3, *itr); // store in cache
		}
		else // there is a cached udata, use it!
		{
			data = lua_touserdata(L, -1);
			//printf("moony_newuserdata: %s %"PRIi32" %p\n", moony_ref[type], *itr, data);
		}
		lua_remove(L, -2); // ref
		*itr += 1;
	}
	else // do not cash this!
	{
		data = lua_newuserdata(L, moony_sz[type]);
		lheader_t *lheader = data;
		lheader->type = type;
		lheader->cache = cache;
		luaL_getmetatable(L, moony_ref[type]);
		lua_setmetatable(L, -2);
	}

	return data;
}

__realtime void
moony_pre(moony_t *moony, LV2_Atom_Sequence *notify)
{
	// initialize notify forge
	const uint32_t capacity = notify->atom.size;
	lv2_atom_forge_set_buffer(&moony->notify_forge, (uint8_t *)notify, capacity);
	moony->notify_ref = lv2_atom_forge_sequence_head(&moony->notify_forge, &moony->notify_frame, 0);
}

__realtime static LV2_Atom_Forge_Ref
_patch_set(patch_t *patch, LV2_Atom_Forge *forge, LV2_URID property, uint32_t size, LV2_URID type, const void *body)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge_Ref ref = lv2_atom_forge_object(forge, &frame, 0, patch->set);

	if(ref)
		ref = lv2_atom_forge_key(forge, patch->property);
	if(ref)
		ref = lv2_atom_forge_urid(forge, property);

	if(ref)
		ref = lv2_atom_forge_key(forge, patch->value);
	if(ref)
		ref = lv2_atom_forge_atom(forge, size, type);
	if(ref)
		ref = lv2_atom_forge_write(forge, body, size);

	if(ref)
		lv2_atom_forge_pop(forge, &frame);

	return ref;
}

__realtime LV2_Worker_Status
moony_wake_worker(const LV2_Worker_Schedule *work_sched)
{
	int32_t dummy;
	return work_sched->schedule_work(work_sched->handle, sizeof(int32_t), &dummy);
}

__realtime bool
moony_in(moony_t *moony, const LV2_Atom_Sequence *control, LV2_Atom_Sequence *notify)
{
	LV2_Atom_Forge *forge = &moony->notify_forge;
	LV2_Atom_Forge_Ref ref = moony->notify_ref;

	char *chunk_new = (char *)atomic_exchange_explicit(&moony->chunk_new, 0, memory_order_relaxed);
	if(chunk_new)
	{
		snprintf(moony->chunk, MOONY_MAX_CHUNK_LEN, "%s", chunk_new);
		moony->dirty_out = true;

		moony->error[0] = 0x0; // clear error message
		moony->error_out = true;

		moony_job_t *req;
		if((req = varchunk_write_request(moony->from_dsp, sizeof(moony_job_t))))
		{
			req->type = MOONY_JOB_PTR_FREE;
			req->ptr = chunk_new;

			varchunk_write_advance(moony->from_dsp, sizeof(moony_job_t));
			if(moony_wake_worker(moony->sched) != LV2_WORKER_SUCCESS)
				moony_trace(moony, "waking worker failed");
		}
	}

	char *err_new = (char *)atomic_exchange_explicit(&moony->err_new, 0, memory_order_relaxed);
	if(err_new)
	{
		if(moony->error[0] == 0x0) // don't overwrite any previous error message
			snprintf(moony->error, MOONY_MAX_ERROR_LEN, "%s", err_new);
		moony->error_out = true;

		moony_job_t *req;
		if((req = varchunk_write_request(moony->from_dsp, sizeof(moony_job_t))))
		{
			req->type = MOONY_JOB_PTR_FREE;
			req->ptr = err_new;

			varchunk_write_advance(moony->from_dsp, sizeof(moony_job_t));
			if(moony_wake_worker(moony->sched) != LV2_WORKER_SUCCESS)
				moony_trace(moony, "waking worker failed");
		}
	}

	LV2_Atom *state_atom_new = (LV2_Atom *)atomic_exchange_explicit(&moony->state_atom_new, 0, memory_order_relaxed);
	if(state_atom_new)
	{
		LV2_Atom *state_atom_old = moony->state_atom;
		moony->state_atom = state_atom_new;

		if(state_atom_old)
		{
			moony_job_t *req;
			if((req = varchunk_write_request(moony->from_dsp, sizeof(moony_job_t))))
			{
				req->type = MOONY_JOB_PTR_FREE;
				req->ptr = state_atom_old;

				varchunk_write_advance(moony->from_dsp, sizeof(moony_job_t));
				if(moony_wake_worker(moony->sched) != LV2_WORKER_SUCCESS)
					moony_trace(moony, "waking worker failed");
			}
		}
	}

	moony_vm_t *vm_new = (moony_vm_t *)atomic_exchange_explicit(&moony->vm_new, 0, memory_order_relaxed);
	if(vm_new)
	{
		moony_vm_t *vm_old = moony->vm;
		moony->vm = vm_new;

		lua_State *L = moony_current(moony);

		if(moony->state_atom)
		{
			// restore Lua defined properties
			lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_RESTORE);
			if(lua_pcall(L, 0, 0, 0))
				moony_error(moony);
#ifdef USE_MANUAL_GC
			lua_gc(L, LUA_GCSTEP, 0);
#endif
		}

		// apply stash
		if(moony->stash_atom) // something has been stashed previously
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_APPLY);
			if(lua_pcall(L, 0, 0, 0))
				moony_error(moony);
#ifdef USE_MANUAL_GC
			lua_gc(L, LUA_GCSTEP, 0);
#endif

			if(vm_old)
				moony_rt_free(vm_old, moony->stash_atom, moony->stash_size);
			moony->stash_atom = NULL;
			moony->stash_size = 0;
		}

		if(vm_old)
		{
			moony_job_t *req;
			if((req = varchunk_write_request(moony->from_dsp, sizeof(moony_job_t))))
			{
				req->type = MOONY_JOB_VM_FREE;
				req->vm = vm_old;

				varchunk_write_advance(moony->from_dsp, sizeof(moony_job_t));
				if(moony_wake_worker(moony->sched) != LV2_WORKER_SUCCESS)
					moony_trace(moony, "waking worker failed");
			}
		}

		moony->once = true;
		moony->props_out = true; // trigger update of UI
	}

	lua_State *L = moony_current(moony);

	// read control sequence
	LV2_ATOM_SEQUENCE_FOREACH(control, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		if(!lv2_atom_forge_is_object_type(&moony->forge, obj->atom.type))
			continue;

		if(obj->body.otype == moony->uris.patch.get)
		{
			const LV2_Atom_URID *subject = NULL;
			const LV2_Atom_URID *property = NULL;
			const LV2_Atom_Int *sequence= NULL;

			LV2_Atom_Object_Query q[] = {
				{ moony->uris.patch.subject, (const LV2_Atom **)&subject },
				{ moony->uris.patch.property, (const LV2_Atom **)&property },
				{ moony->uris.patch.sequence, (const LV2_Atom **)&sequence},
				{ 0, NULL }
			};
			lv2_atom_object_query(obj, q);

			int32_t sequence_num = 0;
			if(sequence && (sequence->atom.type == moony->forge.Int))
				sequence_num = sequence->body;
			(void)sequence_num; //FIXME use

			if(  subject
				&& (subject->atom.type == moony->forge.URID)
				&& (subject->body != moony->uris.patch.self) )
				continue; // subject does not match

			if(property && (property->atom.type == moony->forge.URID) )
			{
				if(property->body == moony->uris.moony_code)
				{
					moony->dirty_out = true;
				}
				else if(property->body == moony->uris.moony_error)
				{
					if(moony->error[0] != 0x0)
						moony->error_out = true;
				}
				else if(property->body == moony->uris.moony_editorHidden)
				{
					const int32_t i32 = atomic_load_explicit(&moony->editor_hidden, memory_order_acquire);
					if(ref)
						ref = lv2_atom_forge_frame_time(forge, 0); //FIXME
					if(ref)
						ref = _patch_set(&moony->uris.patch, forge, property->body, sizeof(int32_t), forge->Bool, &i32);
				}
				else if(property->body == moony->uris.moony_logHidden)
				{
					const int32_t i32 = atomic_load_explicit(&moony->log_hidden, memory_order_acquire);
					if(ref)
						ref = lv2_atom_forge_frame_time(forge, 0); //FIXME
					if(ref)
						ref = _patch_set(&moony->uris.patch, forge, property->body, sizeof(int32_t), forge->Bool, &i32);
				}
				else if(property->body == moony->uris.moony_paramHidden)
				{
					const int32_t i32 = atomic_load_explicit(&moony->param_hidden, memory_order_acquire);
					if(ref)
						ref = lv2_atom_forge_frame_time(forge, 0); //FIXME
					if(ref)
						ref = _patch_set(&moony->uris.patch, forge, property->body, sizeof(int32_t), forge->Bool, &i32);
				}
				else if(property->body == moony->uris.moony_paramCols)
				{
					const int32_t i32 = atomic_load_explicit(&moony->param_cols, memory_order_acquire);
					if(ref)
						ref = lv2_atom_forge_frame_time(forge, 0); //FIXME
					if(ref)
						ref = _patch_set(&moony->uris.patch, forge, property->body, sizeof(int32_t), forge->Int, &i32);
				}
				else if(property->body == moony->uris.moony_paramRows)
				{
					const int32_t i32 = atomic_load_explicit(&moony->param_rows, memory_order_acquire);
					if(ref)
						ref = lv2_atom_forge_frame_time(forge, 0); //FIXME
					if(ref)
						ref = _patch_set(&moony->uris.patch, forge, property->body, sizeof(int32_t), forge->Int, &i32);
				}
			}
			else // !property
			{
				moony->props_out = true; // trigger update of UI
			}
		}
		else if(obj->body.otype == moony->uris.patch.set)
		{
			const LV2_Atom_URID *subject = NULL;
			const LV2_Atom_URID *property = NULL;
			const LV2_Atom_Int *sequence = NULL;
			const LV2_Atom *value = NULL;

			LV2_Atom_Object_Query q[] = {
				{ moony->uris.patch.subject, (const LV2_Atom **)&subject },
				{ moony->uris.patch.property, (const LV2_Atom **)&property },
				{ moony->uris.patch.sequence, (const LV2_Atom **)&sequence },
				{ moony->uris.patch.value, &value },
				{ 0, NULL }
			};
			lv2_atom_object_query(obj, q);

			int32_t sequence_num = 0;
			if(sequence && (sequence->atom.type == moony->forge.Int))
				sequence_num = sequence->body;
			(void)sequence_num; //FIXME use

			if(  subject
				&& (subject->atom.type == moony->forge.URID)
				&& (subject->body != moony->uris.patch.self) )
				continue; // subject does not match

			if(  property && value
				&& (property->atom.type == moony->forge.URID) )
			{
				if( (property->body == moony->uris.moony_code) && (value->type == forge->String) )
				{
					// stash
					lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_STASH);
					if(lua_pcall(L, 0, 0, 0))
						moony_error(moony);
#ifdef USE_MANUAL_GC
					lua_gc(L, LUA_GCSTEP, 0);
#endif

					// send code to worker thread
					const size_t sz = sizeof(moony_job_t) + value->size;
					moony_job_t *req;
					if((req = varchunk_write_request(moony->from_dsp, sz)))
					{
						req->type = MOONY_JOB_VM_ALLOC;
						memcpy(req->chunk, LV2_ATOM_BODY_CONST(value), value->size);

						varchunk_write_advance(moony->from_dsp, sz);
						if(moony_wake_worker(moony->sched) != LV2_WORKER_SUCCESS)
							moony_trace(moony, "waking worker failed");
					}
				}
				else if( (property->body == moony->uris.moony_selection) && (value->type == forge->String) )
				{
					// we do not do any stash, apply and restore for selections

					// load chunk
					const char *str = LV2_ATOM_BODY_CONST(value);
					if(luaL_dostring(L, str)) // failed loading chunk
					{
						moony_error(moony);
					}
					else // succeeded loading chunk
					{
						moony->error[0] = 0x0; // reset error flag
						moony->error_out = true;
					}
				}
				else if( (property->body == moony->uris.moony_editorHidden) && (value->type == forge->Bool) )
				{
					atomic_store_explicit(&moony->editor_hidden, ((const LV2_Atom_Bool *)value)->body, memory_order_release);
				}
				else if( (property->body == moony->uris.moony_logHidden) && (value->type == forge->Bool) )
				{
					atomic_store_explicit(&moony->log_hidden, ((const LV2_Atom_Bool *)value)->body, memory_order_release);
				}
				else if( (property->body == moony->uris.moony_paramHidden) && (value->type == forge->Bool) )
				{
					atomic_store_explicit(&moony->param_hidden, ((const LV2_Atom_Bool *)value)->body, memory_order_release);
				}
				else if( (property->body == moony->uris.moony_paramCols) && (value->type == forge->Int) )
				{
					atomic_store_explicit(&moony->param_cols, ((const LV2_Atom_Int *)value)->body, memory_order_release);
				}
				else if( (property->body == moony->uris.moony_paramRows) && (value->type == forge->Int) )
				{
					atomic_store_explicit(&moony->param_rows, ((const LV2_Atom_Int *)value)->body, memory_order_release);
				}
				else if( (property->body == moony->uris.moony_panic) && (value->type == forge->Bool) )
				{
					const LV2_Atom_Bool *i32 = (const LV2_Atom_Bool *)value;
					if(i32->body)
						moony_err(moony, "user called panic");
				}
			}
		}
	}

	// clear all previously defined properties in UI, plugin may send its new properties afterwards
	if(moony->props_out)
	{
		// clear all properties in UI
		LV2_Atom_Forge_Frame obj_frame, add_frame, rem_frame;
		if(ref)
			ref = lv2_atom_forge_frame_time(forge, 0);
		if(ref)
			ref = lv2_atom_forge_object(forge, &obj_frame, 0, moony->uris.patch.patch);
		if(ref)
			ref = lv2_atom_forge_key(forge, moony->uris.patch.subject);
		if(ref)
			ref = lv2_atom_forge_urid(forge, moony->uris.patch.self);
		if(ref)
			ref = lv2_atom_forge_key(forge, moony->uris.patch.sequence);
		if(ref)
			ref = lv2_atom_forge_int(forge, 0); // we don't expect a reply
		if(ref)
			ref = lv2_atom_forge_key(forge, moony->uris.patch.remove);
		if(ref)
			ref = lv2_atom_forge_object(forge, &rem_frame, 0, 0);
		if(ref)
			ref = lv2_atom_forge_key(forge, moony->uris.patch.writable);
		if(ref)
			ref = lv2_atom_forge_urid(forge, moony->uris.patch.wildcard);
		if(ref)
			ref = lv2_atom_forge_key(forge, moony->uris.patch.readable);
		if(ref)
			ref = lv2_atom_forge_urid(forge, moony->uris.patch.wildcard);
		if(ref)
			lv2_atom_forge_pop(forge, &rem_frame);
		if(ref)
			ref = lv2_atom_forge_key(forge, moony->uris.patch.add);
		if(ref)
			lv2_atom_forge_object(forge, &add_frame, 0, 0);
		if(ref)
			lv2_atom_forge_pop(forge, &add_frame);
		if(ref)
			lv2_atom_forge_pop(forge, &obj_frame);

		moony->props_out = false;
	}

	if(moony->dirty_out)
	{
		const uint32_t len = strlen(moony->chunk);
		if(ref)
			ref = lv2_atom_forge_frame_time(forge, 0);
		if(ref)
			ref = _moony_patch(&moony->uris.patch, forge, moony->uris.moony_code, moony->chunk, len);

		moony->dirty_out = false; // reset flag
	}

	if(moony->error_out)
	{
		const uint32_t len = strlen(moony->error);
		if(ref)
			ref = lv2_atom_forge_frame_time(forge, 0);
		if(ref)
			ref = _moony_patch(&moony->uris.patch, forge, moony->uris.moony_error, moony->error, len);

		moony->error_out = false; // reset flag
	}

	moony->notify_ref = ref;

	return moony->once;
}

__realtime void
moony_out(moony_t *moony, LV2_Atom_Sequence *notify, uint32_t frames)
{
	LV2_Atom_Forge *forge = &moony->notify_forge;
	LV2_Atom_Forge_Ref ref = moony->notify_ref;

	if(moony->trace_out)
	{
		char *pch = strtok(moony->trace, "\n");
		while(pch)
		{
			uint32_t len = strlen(pch);
			if(ref)
				ref = lv2_atom_forge_frame_time(forge, frames);
			if(ref)
				ref = _moony_patch(&moony->uris.patch, forge, moony->uris.moony_trace, pch, len);

			pch = strtok(NULL, "\n");
		}

		moony->trace[0] = '\0';
		moony->trace_out = false; // reset flag
	}

	if(moony->trace_overflow)
	{
		if(moony->log)
			lv2_log_trace(&moony->logger, "trace buffer overflow\n");
		moony->trace_overflow = false; // reset flag
	}

	if(ref)
		lv2_atom_forge_pop(forge, &moony->notify_frame);
	else
		lv2_atom_sequence_clear(notify);

	moony->once = false;
}
