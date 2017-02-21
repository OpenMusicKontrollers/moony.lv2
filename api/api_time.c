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

#include <api_time.h>
#include <api_atom.h>
#include <api_forge.h>

#include <timely.h>

__realtime static void
_ltimeresponder_cb(timely_t *timely, int64_t frames, LV2_URID type,
	void *data)
{
	lua_State *L = data;

	if(lua_geti(L, 5, type) != LUA_TNIL) // uservalue[type]
	{
		lua_pushvalue(L, 5); // uservalue
		lua_pushinteger(L, frames); // frames
		lua_pushvalue(L, 4); // data

		if(type == TIMELY_URI_BAR_BEAT(timely))
		{
			lua_pushnumber(L, TIMELY_BAR_BEAT_RAW(timely));
		}
		else if(type == TIMELY_URI_BAR(timely))
		{
			lua_pushinteger(L, TIMELY_BAR(timely));
		}
		else if(type == TIMELY_URI_BEAT_UNIT(timely))
		{
			lua_pushinteger(L, TIMELY_BEAT_UNIT(timely));
		}
		else if(type == TIMELY_URI_BEATS_PER_BAR(timely))
		{
			lua_pushnumber(L, TIMELY_BEATS_PER_BAR(timely));
		}
		else if(type == TIMELY_URI_BEATS_PER_MINUTE(timely))
		{
			lua_pushnumber(L, TIMELY_BEATS_PER_MINUTE(timely));
		}
		else if(type == TIMELY_URI_FRAME(timely))
		{
			lua_pushinteger(L, TIMELY_FRAME(timely));
		}
		else if(type == TIMELY_URI_FRAMES_PER_SECOND(timely))
		{
			lua_pushnumber(L, TIMELY_FRAMES_PER_SECOND(timely));
		}
		else if(type == TIMELY_URI_SPEED(timely))
		{
			lua_pushnumber(L, TIMELY_SPEED(timely));
		}
		else
		{
			lua_pushnil(L);
		}

		lua_call(L, 4, 0);
	}
	else
		lua_pop(L, 1); // nil
}

__realtime static int
_ltimeresponder__call(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 5); // discard superfluous arguments
	// 1: self
	// 2: from
	// 3: to
	// 4: data aka forge
	// 5: atom || nil
	
	timely_t *timely = lua_touserdata(L, 1);
	int64_t from = luaL_checkinteger(L, 2);
	int64_t to = luaL_checkinteger(L, 3);
	latom_t *latom = NULL;
	if(luaL_testudata(L, 5, "latom"))
		latom = lua_touserdata(L, 5);
	lua_pop(L, 1); // atom

	lua_getuservalue(L, 1); // 5: uservalue
	const int handled = latom
		?  timely_advance_body(timely, latom->atom->size, latom->atom->type, latom->body.obj, from, to)
		:  timely_advance_body(timely, 0, 0, NULL, from, to);

	lua_pushboolean(L, handled); // handled vs not handled
	return 1;
}

__realtime int
_ltimeresponder_apply(lua_State *L)
{
	// 1: self
	// 2: atom
	
	lua_pushinteger(L, 0);
	lua_insert(L, 2); // from

	lua_pushinteger(L, 0);
	lua_insert(L, 3); // to

	lua_pushnil(L);
	lua_insert(L, 4); // data aka forge

	return _ltimeresponder__call(L);
}

__realtime int
_ltimeresponder_stash(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: lforge

	timely_t *timely = lua_touserdata(L, 1);
	lforge_t *lforge = luaL_checkudata(L, 2, "lforge");

	// serialize full time state to stash
	LV2_Atom_Forge_Frame frame;
	if(  !lv2_atom_forge_object(lforge->forge, &frame, 0, timely->urid.time_position)

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_barBeat)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_BAR_BEAT(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_bar)
		|| !lv2_atom_forge_long(lforge->forge, TIMELY_BAR(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_beatUnit)
		|| !lv2_atom_forge_int(lforge->forge, TIMELY_BEAT_UNIT(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_beatsPerBar)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_BEATS_PER_BAR(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_beatsPerMinute)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_BEATS_PER_MINUTE(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_frame)
		|| !lv2_atom_forge_long(lforge->forge, TIMELY_FRAME(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_framesPerSecond)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_FRAMES_PER_SECOND(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_speed)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_SPEED(timely)) )
		luaL_error(L, forge_buffer_overflow);
	lv2_atom_forge_pop(lforge->forge, &frame);

	return 1; // forge
}

__realtime static int
_ltimeresponder__index(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: type
	
	timely_t *timely = lua_touserdata(L, 1);

	int ltype = lua_type(L, 2);
	if(ltype != LUA_TNUMBER)
	{
		if(ltype == LUA_TSTRING)
		{
			const char *key = lua_tostring(L, 2);
			if(!strcmp(key, "stash"))
				lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TIME_STASH);
			else if(!strcmp(key, "apply"))
				lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TIME_APPLY);
			else
				lua_pushnil(L);
		}
		else
			lua_pushnil(L);

		return 1;
	}

	LV2_URID type = lua_tointeger(L, 2);

	if(type == TIMELY_URI_BAR_BEAT(timely))
	{
		lua_pushnumber(L, TIMELY_BAR_BEAT(timely));
	}
	else if(type == TIMELY_URI_BAR(timely))
	{
		lua_pushinteger(L, TIMELY_BAR(timely));
	}
	else if(type == TIMELY_URI_BEAT_UNIT(timely))
	{
		lua_pushinteger(L, TIMELY_BEAT_UNIT(timely));
	}
	else if(type == TIMELY_URI_BEATS_PER_BAR(timely))
	{
		lua_pushnumber(L, TIMELY_BEATS_PER_BAR(timely));
	}
	else if(type == TIMELY_URI_BEATS_PER_MINUTE(timely))
	{
		lua_pushnumber(L, TIMELY_BEATS_PER_MINUTE(timely));
	}
	else if(type == TIMELY_URI_FRAME(timely))
	{
		lua_pushinteger(L, TIMELY_FRAME(timely));
	}
	else if(type == TIMELY_URI_FRAMES_PER_SECOND(timely))
	{
		lua_pushnumber(L, TIMELY_FRAMES_PER_SECOND(timely));
	}
	else if(type == TIMELY_URI_SPEED(timely))
	{
		lua_pushnumber(L, TIMELY_SPEED(timely));
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

__realtime int
_ltimeresponder(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	timely_mask_t mask = TIMELY_MASK_BAR_BEAT
		| TIMELY_MASK_BAR
		| TIMELY_MASK_BEAT_UNIT
		| TIMELY_MASK_BEATS_PER_BAR
		| TIMELY_MASK_BEATS_PER_MINUTE
		| TIMELY_MASK_FRAME
		| TIMELY_MASK_FRAMES_PER_SECOND
		| TIMELY_MASK_SPEED
		| TIMELY_MASK_BAR_BEAT_WHOLE
		| TIMELY_MASK_BAR_WHOLE;

	// o = o or {}
	if(lua_isnil(L, 1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
	}

	// TODO do we want to cache/reuse this, too?
	timely_t *timely = lua_newuserdata(L, sizeof(timely_t)); // userdata
	timely_init(timely, moony->map, moony->opts.sample_rate, mask,
		_ltimeresponder_cb, L);

	// userdata.uservalue = o
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "ltimeresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

const luaL_Reg ltimeresponder_mt [] = {
	{"__index", _ltimeresponder__index},
	{"__call", _ltimeresponder__call},
	{NULL, NULL}
};
