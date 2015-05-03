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

#include <lua_lv2.h>

#include <lauxlib.h>

// there is a bug in LV2 <= 0.10
#if defined(LV2_ATOM_TUPLE_FOREACH)
#	undef LV2_ATOM_TUPLE_FOREACH
#	define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))
#endif

typedef struct _lobj_t lobj_t;
typedef struct _ltuple_t ltuple_t;
typedef struct _lframe_t lframe_t;
typedef struct _latom_t latom_t;

struct _lobj_t {
	const LV2_Atom_Object *obj;
	const LV2_Atom_Property_Body *itr;
};

struct _ltuple_t {
	const LV2_Atom_Tuple *tuple;
	const LV2_Atom *itr;
};

struct _lframe_t {
	LV2_Atom_Forge_Frame frame;
};

struct _latom_t {
	const LV2_Atom *atom;
};

static void
_latom_new(lua_State *L, const LV2_Atom *atom)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &lua_atom->forge;

	if(atom->type == forge->Object)
	{
		lobj_t *lobj = lua_newuserdata(L, sizeof(lobj_t));
		lobj->obj = (const LV2_Atom_Object *)atom;
		luaL_getmetatable(L, "lobj");
	}
	else if(atom->type == forge->Tuple)
	{
		ltuple_t *ltuple = lua_newuserdata(L, sizeof(ltuple_t));
		ltuple->tuple = (const LV2_Atom_Tuple *)atom;
		luaL_getmetatable(L, "ltuple");
	}
	else
	{
		latom_t *latom = lua_newuserdata(L, sizeof(latom_t));
		latom->atom = atom;
		luaL_getmetatable(L, "latom");
	}

	lua_setmetatable(L, -2);
}

static int
_lseq_foreach_itr(lua_State *L)
{
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	if(!lv2_atom_sequence_is_end(&lseq->seq->body, lseq->seq->atom.size, lseq->itr))
	{
		// push frame time
		lua_pushinteger(L, lseq->itr->time.frames);
		// push atom
		_latom_new(L, &lseq->itr->body);
	
		// advance iterator
		lseq->itr = lv2_atom_sequence_next(lseq->itr);

		return 2;
	}

	// end of sequence reached
	lua_pushnil(L);
	return 1;
}

static int
_lseq__index(lua_State *L)
{
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	if(lua_isinteger(L, 2))
	{
		int index = lua_tointeger(L, 2); // indexing start from 1
		int count = 0;
		LV2_ATOM_SEQUENCE_FOREACH(lseq->seq, ev)
		{
			if(++count == index) 
			{
				_latom_new(L, &ev->body);
				break;
			}
		}
		if(count != index) // index is out of bounds
			lua_pushnil(L);
	}
	else if(lua_isstring(L, 2))
	{
		const char *key = lua_tostring(L, 2);

		if(!strcmp(key, "type"))
		{
			lua_pushinteger(L, lseq->seq->atom.type);
		}
		else // look in metatable
		{
			lua_getmetatable(L, 1);
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);
		}
	}
	else
		lua_pushnil(L); // unsupported key

	return 1;
}

static int
_lseq__len(lua_State *L)
{
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	uint32_t count = 0;
	LV2_ATOM_SEQUENCE_FOREACH(lseq->seq, ev)
		count++;
	lua_pushinteger(L, count);

	return 1;
}

static int
_lseq__tostring(lua_State *L)
{
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	lua_pushstring(L, "Atom_Sequence");

	return 1;
}

static int
_lseq_foreach(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	// reset iterator to beginning of sequence
	lseq->itr = lv2_atom_sequence_begin(&lseq->seq->body);

	lua_pushlightuserdata(L, lua_atom);
	lua_pushcclosure(L, _lseq_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const luaL_Reg lseq_mt [] = {
	{"__index", _lseq__index},
	{"__len", _lseq__len},
	{"__tostring", _lseq__tostring},
	{"foreach", _lseq_foreach},
	{NULL, NULL}
};

static int
_ltuple_foreach_itr(lua_State *L)
{
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	if(!lv2_atom_tuple_is_end(LV2_ATOM_BODY(ltuple->tuple), ltuple->tuple->atom.size, ltuple->itr))
	{
		// push atom
		_latom_new(L, ltuple->itr);
	
		// advance iterator
		ltuple->itr = lv2_atom_tuple_next(ltuple->itr);

		return 1;
	}

	// end of sequence reached
	lua_pushnil(L);
	return 1;
}

static int
_ltuple__index(lua_State *L)
{
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	if(lua_isnumber(L, 2))
	{
		int index = lua_tointeger(L, 2); // indexing start from 1
		int count = 0;
		LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
		{
			if(++count == index) 
			{
				_latom_new(L, atom);
				break;
			}
		}
		if(count != index) // index is out of bounds
			lua_pushnil(L);
	}
	else if(lua_isstring(L, 2))
	{
		const char *key = lua_tostring(L, 2);

		if(!strcmp(key, "type"))
		{
			lua_pushinteger(L, ltuple->tuple->atom.type);
		}
		else // look in metatable
		{
			lua_getmetatable(L, 1);
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);
		}
	}
	else
		lua_pushnil(L); // unsupported key

	return 1;
}

static int
_ltuple__len(lua_State *L)
{
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	uint32_t count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
		count++;
	lua_pushinteger(L, count);

	return 1;
}

static int
_ltuple__tostring(lua_State *L)
{
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	lua_pushstring(L, "Atom_Tuple");

	return 1;
}

static int
_ltuple_foreach(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	// reset iterator to beginning of tuple
	ltuple->itr = lv2_atom_tuple_begin(ltuple->tuple);

	lua_pushlightuserdata(L, lua_atom);
	lua_pushcclosure(L, _ltuple_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static int
_ltuple_unpack(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	uint32_t count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
	{
		// push atom
		_latom_new(L, atom);

		// increase counter
		count++;
	}

	return count;
}

static const luaL_Reg ltuple_mt [] = {
	{"__index", _ltuple__index},
	{"__len", _ltuple__len},
	{"__tostring", _ltuple__tostring},
	{"foreach", _ltuple_foreach},
	{"unpack", _ltuple_unpack},
	{NULL, NULL}
};

static int
_lobj_foreach_itr(lua_State *L)
{
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	if(!lv2_atom_object_is_end(&lobj->obj->body, lobj->obj->atom.size, lobj->itr))
	{
		// push atom
		lua_pushinteger(L, lobj->itr->key);
		lua_pushinteger(L, lobj->itr->context);
		_latom_new(L, &lobj->itr->value);
	
		// advance iterator
		lobj->itr = lv2_atom_object_next(lobj->itr);

		return 3;
	}

	// end of sequence reached
	lua_pushnil(L);
	return 1;
}

static int
_lobj__index(lua_State *L)
{
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	if(lua_isinteger(L, 2))
	{
		LV2_URID urid = lua_tointeger(L, 2);

		// start a query for given URID
		const LV2_Atom *atom = NULL;
		LV2_Atom_Object_Query q [] = {
			{urid, &atom},
			LV2_ATOM_OBJECT_QUERY_END
		};
		lv2_atom_object_query(lobj->obj, q);

		if(atom) // query returned a matching atom
			_latom_new(L, atom);
		else // query returned no matching atom
			lua_pushnil(L);
	}
	else if(lua_isstring(L, 2))
	{
		const char *key = lua_tostring(L, 2);

		if(!strcmp(key, "type"))
		{
			lua_pushinteger(L, lobj->obj->atom.type);
		}
		else if(!strcmp(key, "id"))
		{
			lua_pushinteger(L, lobj->obj->body.id);
		}
		else if(!strcmp(key, "otype"))
		{
			lua_pushinteger(L, lobj->obj->body.otype);
		}
		else // look in metatable
		{
			lua_getmetatable(L, 1);
			lua_pushvalue(L, 2);
			lua_rawget(L, -2);
		}
	}
	else
		lua_pushnil(L); // unsupported key

	return 1;
}

static int
_lobj__len(lua_State *L)
{
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	uint32_t count = 0;
	LV2_ATOM_OBJECT_FOREACH(lobj->obj, prop)
		count++;
	lua_pushinteger(L, count);

	return 1;
}
static int
_lobj__tostring(lua_State *L)
{
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	lua_pushstring(L, "Atom_Object");

	return 1;
}

static int
_lobj_foreach(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	// reset iterator to beginning of tuple
	lobj->itr = lv2_atom_object_begin(&lobj->obj->body);

	lua_pushlightuserdata(L, lua_atom);
	lua_pushcclosure(L, _lobj_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const luaL_Reg lobj_mt [] = {
	{"__index", _lobj__index},
	{"__len", _lobj__len},
	{"__tostring", _lobj__tostring},
	{"foreach", _lobj_foreach},
	{NULL, NULL}
};

static void
_latom_value(lua_State *L, const LV2_Atom *atom)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &lua_atom->forge;

	if(atom->type == forge->Int)
		lua_pushinteger(L, ((const LV2_Atom_Int *)atom)->body);
	else if(atom->type == forge->Long)
		lua_pushinteger(L, ((const LV2_Atom_Long *)atom)->body);
	else if(atom->type == forge->Float)
		lua_pushnumber(L, ((const LV2_Atom_Float *)atom)->body);
	else if(atom->type == forge->Double)
		lua_pushnumber(L, ((const LV2_Atom_Double *)atom)->body);
	else if(atom->type == forge->Bool)
		lua_pushboolean(L, ((const LV2_Atom_Bool *)atom)->body);
	else if(atom->type == forge->URID)
		lua_pushinteger(L, ((const LV2_Atom_URID *)atom)->body);
	else if(atom->type == forge->String)
		lua_pushstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_String, atom));
	else if(atom->type == forge->URI)
		lua_pushstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_String, atom));
	else if(atom->type == forge->Path)
		lua_pushstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_String, atom));
	else if(atom->type == forge->Literal)
		lua_pushstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, atom));
	else if(atom->type == lua_atom->uris.midi_event)
	{
		const uint8_t *m = LV2_ATOM_BODY_CONST(atom);
		lua_createtable(L, atom->size, 0);
		for(int i=0; i<atom->size; i++)
		{
			lua_pushinteger(L, m[i]);
			lua_rawseti(L, -2, i+1);
		}
	}
	else // unknown type
		lua_pushnil(L);
}

static int
_latom__index(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const char *key = lua_tostring(L, 2);

	if(!strcmp(key, "type"))
		lua_pushinteger(L, latom->atom->type);
	else if(!strcmp(key, "value"))
		_latom_value(L, latom->atom);
	else if(!strcmp(key, "datatype") && (latom->atom->type == lua_atom->forge.Literal) )
		lua_pushinteger(L, ((LV2_Atom_Literal *)latom->atom)->body.datatype);
	else if(!strcmp(key, "lang") && (latom->atom->type == lua_atom->forge.Literal) )
		lua_pushinteger(L, ((LV2_Atom_Literal *)latom->atom)->body.lang);
	else // look in metatable
	{
		lua_getmetatable(L, 1);
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
	}

	return 1;
}

static int
_latom__len(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");

	if(latom->atom->type == lua_atom->forge.Literal)
		lua_pushinteger(L, latom->atom->size - sizeof(LV2_Atom_Literal_Body));
	else
		lua_pushinteger(L, latom->atom->size);

	return 1;
}

static int
_latom__tostring(lua_State *L)
{
	latom_t *latom = luaL_checkudata(L, 1, "latom");

	_latom_value(L, latom->atom);
	if(!lua_isstring(L, -1))
		lua_tostring(L, -1);

	return 1;
}

static int
_latom__concat(lua_State *L)
{
	latom_t *latom;
	if((latom = luaL_testudata(L, 1, "latom")))
	{
		_latom_value(L, latom->atom);
		if(!lua_isstring(L, -1))
			lua_tostring(L, -1);
	}
	else
		lua_pushvalue(L, 1);
	
	if((latom = luaL_testudata(L, 2, "latom")))
	{
		_latom_value(L, latom->atom);
		if(!lua_isstring(L, -1))
			lua_tostring(L, -1);
	}
	else
		lua_pushvalue(L, 2);

	lua_concat(L, 2);

	return 1;
}

static const luaL_Reg latom_mt [] = {
	{"__index", _latom__index},
	{"__len", _latom__len},
	{"__tostring", _latom__tostring},
	{"__concat", _latom__concat},
	{NULL, NULL}
};

static int
_lforge_frame_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int64_t val = luaL_checkinteger(L, 2);

	lv2_atom_forge_frame_time(lforge->forge, val);

	return 0;
}

static int
_lforge_beat_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	double val = luaL_checknumber(L, 2);

	lv2_atom_forge_beat_time(lforge->forge, val);

	return 0;
}

static int
_lforge_int(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int32_t val = luaL_checkinteger(L, 2);

	lv2_atom_forge_int(lforge->forge, val);

	return 0;
}

static int
_lforge_long(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int64_t val = luaL_checkinteger(L, 2);

	lv2_atom_forge_long(lforge->forge, val);

	return 0;
}

static int
_lforge_float(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	float val = luaL_checknumber(L, 2);

	lv2_atom_forge_float(lforge->forge, val);

	return 0;
}

static int
_lforge_double(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	double val = luaL_checknumber(L, 2);

	lv2_atom_forge_double(lforge->forge, val);

	return 0;
}

static int
_lforge_bool(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	uint32_t val = lua_toboolean(L, 2);

	lv2_atom_forge_bool(lforge->forge, val);

	return 0;
}

static int
_lforge_urid(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID val = luaL_checkinteger(L, 2);

	lv2_atom_forge_urid(lforge->forge, val);

	return 0;
}

static int
_lforge_string(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	lv2_atom_forge_string(lforge->forge, val, size);

	return 0;
}

static int
_lforge_literal(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);
	LV2_URID datatype = luaL_checkinteger(L, 3);
	LV2_URID lang = luaL_checkinteger(L, 4);

	lv2_atom_forge_literal(lforge->forge, val, size, datatype, lang);

	return 0;
}

static int
_lforge_uri(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	lv2_atom_forge_uri(lforge->forge, val, size);

	return 0;
}

static int
_lforge_path(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	lv2_atom_forge_path(lforge->forge, val, size);

	return 0;
}

static int
_lforge_midi(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	if(lua_istable(L, 2))
	{
		uint32_t size = lua_rawlen(L, 2);
		lv2_atom_forge_atom(lforge->forge, size, lua_atom->uris.midi_event);
		for(int i=1; i<=size; i++)
		{
			lua_rawgeti(L, -1, i);
			uint8_t m = luaL_checkinteger(L, -1);
			lua_pop(L, 1);
			lv2_atom_forge_raw(lforge->forge, &m, 1);
		}
		lv2_atom_forge_pad(lforge->forge, size);
	}

	return 0;
}

static int
_lforge_tuple(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lframe_t *lframe = lua_newuserdata(L, sizeof(lframe_t));
	luaL_getmetatable(L, "lframe");
	lua_setmetatable(L, -2);

	lv2_atom_forge_tuple(lforge->forge, &lframe->frame);

	return 1;
}

static int
_lforge_object(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID id = luaL_checkinteger(L, 2);
	LV2_URID otype = luaL_checkinteger(L, 3);
	lframe_t *lframe = lua_newuserdata(L, sizeof(lframe_t));
	luaL_getmetatable(L, "lframe");
	lua_setmetatable(L, -2);

	lv2_atom_forge_object(lforge->forge, &lframe->frame, 0, otype);

	return 1;
}

static int
_lforge_key(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);

	lv2_atom_forge_key(lforge->forge, key);

	return 1;
}

static int
_lforge_property(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);
	LV2_URID context = luaL_checkinteger(L, 3);

	lv2_atom_forge_property_head(lforge->forge, key, context);

	return 1;
}

static int
_lforge_pop(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lframe_t *lframe = luaL_checkudata(L, 2, "lframe");

	lv2_atom_forge_pop(lforge->forge, &lframe->frame);

	return 0;
}

static const luaL_Reg lframe_mt [] = {
	{NULL, NULL}
};

static const luaL_Reg lforge_mt [] = {
	{"frame_time", _lforge_frame_time},
	{"beat_time", _lforge_beat_time},

	{"int", _lforge_int},
	{"long", _lforge_long},
	{"float", _lforge_float},
	{"double", _lforge_double},
	{"bool", _lforge_bool},
	{"urid", _lforge_urid},
	{"string", _lforge_string},
	{"literal", _lforge_literal},
	{"uri", _lforge_uri},
	{"path", _lforge_path},

	//TODO vector
	//TODO chunk
	{"midi", _lforge_midi},
	{"tuple", _lforge_tuple},
	{"object", _lforge_object},
	{"key", _lforge_key},
	{"property", _lforge_property},

	{"pop", _lforge_pop},

	{NULL, NULL}
};

static int
_llv2_map(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	const char *uri = luaL_checkstring(L, 1);
	LV2_URID urid = lua_atom->map->map(lua_atom->map->handle, uri);

	if(urid)
		lua_pushinteger(L, urid);
	else
		lua_pushnil(L);

	return 1;
}

static int
_llv2_unmap(lua_State *L)
{
	lua_atom_t *lua_atom = lua_touserdata(L, lua_upvalueindex(1));
	LV2_URID urid = luaL_checkinteger(L, 1);
	const char *uri = lua_atom->unmap->unmap(lua_atom->unmap->handle, urid);

	if(uri)
		lua_pushstring(L, uri);
	else
		lua_pushnil(L);

	return 1;
}

static const luaL_Reg llv2 [] = {
	{"map", _llv2_map},
	{"unmap", _llv2_unmap},

	{NULL, NULL}
};

int
lua_atom_init(lua_atom_t *lua_atom, const LV2_Feature *const *features)
{
	for(int i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			lua_atom->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			lua_atom->unmap = (LV2_URID_Unmap *)features[i]->data;

	if(!lua_atom->map)
	{
		fprintf(stderr, "Host does not support urid:map\n");
		return -1;
	}
	if(!lua_atom->unmap)
	{
		fprintf(stderr, "Host does not support urid:unmap\n");
		return -1;
	}

	lua_atom->uris.lua_message = lua_atom->map->map(lua_atom->map->handle, LUA_MESSAGE_URI);
	lua_atom->uris.lua_code = lua_atom->map->map(lua_atom->map->handle, LUA_CODE_URI);
	lua_atom->uris.lua_error = lua_atom->map->map(lua_atom->map->handle, LUA_ERROR_URI);
	lua_atom->uris.midi_event = lua_atom->map->map(lua_atom->map->handle, LV2_MIDI__MidiEvent);

	lv2_atom_forge_init(&lua_atom->forge, lua_atom->map);

	return 0;
}

void
lua_atom_open(lua_atom_t *lua_atom, lua_State *L)
{
	luaL_newmetatable(L, "lseq");
	lua_pushlightuserdata(L, lua_atom); // @ upvalueindex 1
	luaL_setfuncs (L, lseq_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lobj");
	lua_pushlightuserdata(L, lua_atom); // @ upvalueindex 1
	luaL_setfuncs (L, lobj_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "ltuple");
	lua_pushlightuserdata(L, lua_atom); // @ upvalueindex 1
	luaL_setfuncs (L, ltuple_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "latom");
	lua_pushlightuserdata(L, lua_atom); // @ upvalueindex 1
	luaL_setfuncs (L, latom_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lforge");
	lua_pushlightuserdata(L, lua_atom); // @ upvalueindex 1
	luaL_setfuncs (L, lforge_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lframe");
	lua_pushlightuserdata(L, lua_atom); // @ upvalueindex 1
	luaL_setfuncs (L, lframe_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

#define SET_CONST(L, ID) \
	lua_pushinteger(L, lua_atom->forge.ID); \
	lua_setfield(L, -2, #ID);

	lua_newtable(L);
	lua_pushlightuserdata(L, lua_atom); // @ upvalueindex 1
	luaL_setfuncs(L, llv2, 1);
	//SET_CONST(L, Blank); // is depracated
	SET_CONST(L, Bool);
	SET_CONST(L, Chunk);
	SET_CONST(L, Double);
	SET_CONST(L, Float);
	SET_CONST(L, Int);
	SET_CONST(L, Long);
	SET_CONST(L, Literal);
	SET_CONST(L, Object);
	SET_CONST(L, Path);
	SET_CONST(L, Property);
	//SET_CONST(L, Resource); // is deprecated
	SET_CONST(L, Sequence);
	SET_CONST(L, String);
	SET_CONST(L, Tuple);
	SET_CONST(L, URI);
	SET_CONST(L, URID);
	SET_CONST(L, Vector);
	lua_pushinteger(L, lua_atom->uris.midi_event);
		lua_setfield(L, -2, "Midi");
	lua_setglobal(L, "lv2");

#undef SET_CONST
}
