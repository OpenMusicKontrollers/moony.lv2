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

#include <math.h>

#include <api_osc.h>
#include <api_atom.h>

#include <osc.lv2/util.h>

static inline bool
_osc_path_has_wildcards(const char *path)
{
	for(const char *ptr=path+1; *ptr; ptr++)
		if(strchr("?*[{", *ptr))
			return true;
	return false;
}

//TODO can we rewrite this directly in C?
const char *loscresponder_match =
	"function __expand(v)\n"
	"	return coroutine.wrap(function()\n"
	"		local item = string.match(v, '%b{}')\n"
	"		if item then\n"
	"			for key in string.gmatch(item, '{?([^,}]*)') do\n"
	"				local vv = string.gsub(v, item, key)\n"
	"				for w in __expand(vv) do\n"
	"					coroutine.yield(w)\n"
	"				end\n"
	"			end\n"
	"		else\n"
	"			coroutine.yield(v)\n"
	"		end\n"
	"	end)\n"
	"end\n"
	"\n"
	"function __match(v, o, ...)\n"
	"	local handled = false\n"
	"	v = string.gsub(v, '%?', '.')\n"
	"	v = string.gsub(v, '%*', '.*')\n"
	"	v = string.gsub(v, '%[%!', '[^')\n"
	"	v = '^' .. v .. '$'\n"
	"	for w in __expand(v) do\n"
	"		for k, x in pairs(o) do\n"
	"			if string.match(k, w) then\n"
	"				handled = x(o, ...) or handled\n"
	"			end\n"
	"		end\n"
	"	end\n"
	"	return handled\n"
	"end";

static inline void
_loscresponder_method(const char *path, const LV2_Atom_Tuple *arguments, void *data)
{
	moony_t *moony = data;
	lua_State *L = moony->vm.L;
	LV2_Atom_Forge *forge = &moony->forge;
	LV2_OSC_URID *osc_urid = &moony->osc_urid;
	LV2_URID_Unmap *unmap = moony->unmap;

	// 1: uservalue
	// 2: frames
	// 3: data

	int matching = 0;
	if(_osc_path_has_wildcards(path))
	{
		lua_getglobal(L, "__match"); // push pattern matching function
		lua_pushstring(L, path); // path
		matching = 1;
	}
	else if(lua_getfield(L, 1, path) == LUA_TNIL) // raw string match
	{
		lua_pop(L, 1); // nil
		return;
	}

	lua_pushvalue(L, 1); // self
	lua_pushvalue(L, 2); // frames
	lua_pushvalue(L, 3); // data

	int oldtop = lua_gettop(L);

	LV2_ATOM_TUPLE_FOREACH(arguments, atom)
	{
		const LV2_Atom_Object *obj= (const LV2_Atom_Object *)atom;

		switch(lv2_osc_argument_type(osc_urid, atom))
		{
			case LV2_OSC_INT32:
			{
				lua_pushinteger(L, ((const LV2_Atom_Int *)atom)->body);
				break;
			}
			case LV2_OSC_FLOAT:
			{
				lua_pushnumber(L, ((const LV2_Atom_Float *)atom)->body);
				break;
			}
			case LV2_OSC_STRING:
			{
				lua_pushstring(L, LV2_ATOM_BODY_CONST(atom));
				break;
			}
			case LV2_OSC_BLOB:
			{
				const uint8_t *b = LV2_ATOM_BODY_CONST(atom);
				lua_createtable(L, atom->size, 0);
				for(unsigned i=0; i<atom->size; i++)
				{
					lua_pushinteger(L, b[i]);
					lua_rawseti(L, -2, i+1);
				}
				break;
			}

			case LV2_OSC_INT64:
			{
				lua_pushinteger(L, ((const LV2_Atom_Long *)atom)->body);
				break;
			}
			case LV2_OSC_DOUBLE:
			{
				lua_pushnumber(L, ((const LV2_Atom_Double *)atom)->body);
				break;
			}
			case LV2_OSC_TIMETAG:
			{
				LV2_OSC_Timetag tt;
				lv2_osc_timetag_get(osc_urid, obj, &tt);
				lua_pushinteger(L, lv2_osc_timetag_parse(&tt));
				break;
			}

			case LV2_OSC_TRUE:
			{
				lua_pushboolean(L, 1);
				break;
			}
			case LV2_OSC_FALSE:
			{
				lua_pushboolean(L, 0);
				break;
			}
			case LV2_OSC_NIL:
			{
				lua_pushnil(L);
				break;
			}
			case LV2_OSC_IMPULSE:
			{
				lua_pushnumber(L, HUGE_VAL);
				break;
			}

			case LV2_OSC_SYMBOL:
			{
				lua_pushinteger(L, ((const LV2_Atom_URID *)atom)->body);
				break;
			}
			case LV2_OSC_MIDI:
			{
				const uint8_t *m = LV2_ATOM_BODY_CONST(atom);
				lua_createtable(L, atom->size, 0);
				for(unsigned i=0; i<atom->size; i++)
				{
					lua_pushinteger(L, m[i]);
					lua_rawseti(L, -2, i+1);
				}
				break;
			}
			case LV2_OSC_CHAR:
			{
				lua_pushinteger(L, *(const uint8_t *)LV2_ATOM_BODY_CONST(atom));
				break;
			}
			case LV2_OSC_RGBA:
			{
				const uint8_t *r = LV2_ATOM_BODY_CONST(atom);
				lua_createtable(L, atom->size, 0);
				for(unsigned i=0; i<atom->size; i++)
				{
					lua_pushinteger(L, r[i]);
					lua_rawseti(L, -2, i+1);
				}
				break;
			}
		}
	}

	lua_call(L, 3 + matching + lua_gettop(L) - oldtop, 0);
}

static int
_loscresponder__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 4); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: data
	// 4: atom
	
	latom_t *latom = NULL;
	if(luaL_testudata(L, 4, "latom"))
		latom = lua_touserdata(L, 4);
	lua_pop(L, 1); // atom

	// check for valid atom and event type
	const LV2_Atom_Object *obj = (const LV2_Atom_Object *)latom->atom;

	if(  !latom
		|| !lv2_atom_forge_is_object_type(&moony->forge, obj->atom.type)
		|| !(lv2_osc_is_message_or_bundle_type(&moony->osc_urid, obj->body.otype)))
	{
		lua_pushboolean(L, 0); // not handled
		return 1;
	}

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	lua_pushboolean(L, lv2_osc_body_unroll(&moony->osc_urid,
		latom->atom->size, latom->body.obj, _loscresponder_method, moony));
	return 1;
}

int
_loscresponder(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	// o = new 
	int32_t *dummy = lua_newuserdata(L, sizeof(int32_t));
	(void)dummy;

	// o.uservalue = uservalue
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "loscresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

const luaL_Reg loscresponder_mt [] = {
	{"__call", _loscresponder__call},
	{NULL, NULL}
};
