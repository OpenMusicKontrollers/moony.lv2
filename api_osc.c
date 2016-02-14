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

#include <api_osc.h>
#include <api_atom.h>

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

static void
_loscresponder_msg(const char *path, const char *fmt, const LV2_Atom_Tuple *args,
	void *data)
{
	moony_t *moony = data;
	lua_State *L = moony->vm.L;
	LV2_Atom_Forge *forge = &moony->forge;
	osc_forge_t *oforge = &moony->oforge;

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
		return; // no match
	}

	lua_pushvalue(L, 1); // self
	lua_pushvalue(L, 2); // frames
	lua_pushvalue(L, 3); // data
	lua_pushstring(L, fmt);

	int oldtop = lua_gettop(L);
	const LV2_Atom *atom = lv2_atom_tuple_begin(args);

	for(const char *type = fmt; *type && atom; type++)
	{
		switch(*type)
		{
			case 'i':
			{
				int32_t i = 0;
				atom = osc_deforge_int32(oforge, forge, atom, &i);
				lua_pushinteger(L, i);
				break;
			}
			case 'f':
			{
				float f = 0.f;
				atom = osc_deforge_float(oforge, forge, atom, &f);
				lua_pushnumber(L, f);
				break;
			}
			case 's':
			{
				const char *s = NULL;
				atom = osc_deforge_string(oforge, forge, atom, &s);
				lua_pushstring(L, s);
				break;
			}
			case 'S':
			{
				const char *s = NULL;
				atom = osc_deforge_symbol(oforge, forge, atom, &s);
				lua_pushstring(L, s);
				break;
			}
			case 'b':
			{
				uint32_t size = 0;
				const uint8_t *b = NULL;
				atom = osc_deforge_blob(oforge, forge, atom, &size, &b);
				//TODO or return a AtomChunk?
				lua_createtable(L, size, 0);
				for(unsigned i=0; i<size; i++)
				{
					lua_pushinteger(L, b[i]);
					lua_rawseti(L, -2, i+1);
				}
				break;
			}
			
			case 'h':
			{
				int64_t h = 0;
				atom = osc_deforge_int64(oforge, forge, atom, &h);
				lua_pushinteger(L, h);
				break;
			}
			case 'd':
			{
				double d = 0.f;
				atom = osc_deforge_double(oforge, forge, atom, &d);
				lua_pushnumber(L, d);
				break;
			}
			case 't':
			{
				uint64_t t = 0;
				atom = osc_deforge_timestamp(oforge, forge, atom, &t);
				lua_pushinteger(L, t);
				break;
			}
			
			case 'c':
			{
				char c = '\0';
				atom = osc_deforge_char(oforge, forge, atom, &c);
				lua_pushinteger(L, c);
				break;
			}
			case 'm':
			{
				uint32_t size = 0;
				const uint8_t *m = NULL;
				atom = osc_deforge_midi(oforge, forge, atom, &size, &m);
				//TODO or return a MIDIEvent?
				lua_createtable(L, size, 0);
				for(unsigned i=0; i<size; i++)
				{
					lua_pushinteger(L, m[i]);
					lua_rawseti(L, -2, i+1);
				}
				break;
			}
			
			case 'T':
			case 'F':
			case 'N':
			case 'I':
			{
				break;
			}

			default: // unknown argument type
			{
				break;
			}
		}
	}

	lua_call(L, 4 + matching + lua_gettop(L) - oldtop, 0);
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
	if(!latom || !(osc_atom_is_bundle(&moony->oforge, (const LV2_Atom_Object *)latom->atom) //FIXME use body
		|| osc_atom_is_message(&moony->oforge, (const LV2_Atom_Object *)latom->atom))) //FIXME use body
	{
		lua_pushboolean(L, 0); // not handled
		return 1;
	}

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	osc_atom_event_unroll(&moony->oforge, (const LV2_Atom_Object *)latom->atom, NULL, NULL, _loscresponder_msg, moony); //FIXME use body

	lua_pushboolean(L, 1); // handled
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
