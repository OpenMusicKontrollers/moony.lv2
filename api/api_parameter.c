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

#include <api_parameter.h>

static int
_lparameter__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: value or nil

	lua_geti(L, 1, moony->uris.rdf_value);

	if(!lua_isnil(L, 2))
	{
		lua_insert(L, 2);
		lua_seti(L, 1, moony->uris.rdf_value);
	}

	return 1;
}

int
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
	{"__call", _lparameter__call},
	{NULL, NULL}
};
