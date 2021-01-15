/*
 * Copyright (c) 2015-2021 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <api_parameter.h>

__realtime static int
_lparameter__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	// 1: self
	// 2: key

	if(lua_isinteger(L, 2) && (lua_tointeger(L, 2) == moony->uris.rdf_value))
	{
		if(lua_geti(L, 1, moony->uris.patch.get) == LUA_TFUNCTION)
		{
			lua_pushvalue(L, 1); // self
			lua_call(L, 1, 1);

			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

__realtime static int
_lparameter__newindex(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	// 1: self
	// 2: key
	// 3: value

	if(lua_isinteger(L, 2) && (lua_tointeger(L, 2) == moony->uris.rdf_value))
	{
		if(lua_geti(L, 1, moony->uris.patch.set) == LUA_TFUNCTION)
		{
			lua_pushvalue(L, 1); // self
			lua_pushvalue(L, 3); // value
			lua_call(L, 2, 0);
		}
	}

	return 0;
}

__realtime static int
_lparameter__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: value or nil

	// push old value on stack
	lua_geti(L, 1, moony->uris.rdf_value);

	if(!lua_isnil(L, 2)) // has value to set
	{
		lua_pushvalue(L, 2);
		lua_seti(L, 1, moony->uris.rdf_value); // param[RDF.value] = value
	}

	return 1;
}

__realtime int
_lparameter(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	// o = o or {}
	if(lua_isnil(L, 1))
	{
		lua_pop(L, 1); // nil
		lua_newtable(L);
	}

	// setmetatable(o, self)
	luaL_getmetatable(L, "lparameter");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

const luaL_Reg lparameter_mt [] = {
	{"__index", _lparameter__index},
	{"__newindex", _lparameter__newindex},
	{"__call", _lparameter__call},
	{NULL, NULL}
};
