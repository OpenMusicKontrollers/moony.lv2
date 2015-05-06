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

#define LV2_ATOM_VECTOR_ITEM_CONST(vec, i) \
	(LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, (vec)) + (i)*(vec)->body.child_size)

typedef struct _lobj_t lobj_t;
typedef struct _ltuple_t ltuple_t;
typedef struct _lvec_t lvec_t;
typedef struct _lframe_t lframe_t;
typedef struct _latom_t latom_t;

struct _lobj_t {
	const LV2_Atom_Object *obj;
	const LV2_Atom_Property_Body *itr;
	LV2_Atom body [0];
};

struct _ltuple_t {
	const LV2_Atom_Tuple *tuple;
	uint32_t pos;
	const LV2_Atom *itr;
	LV2_Atom body [0];
};

struct _lvec_t {
	const LV2_Atom_Vector *vec;
	uint32_t count;
	uint32_t pos;
	LV2_Atom body [0];
};

struct _latom_t {
	const LV2_Atom *atom;
	LV2_Atom body [0];
};

struct _lframe_t {
	LV2_Atom_Forge_Frame frame;
};

static void
_latom_new(lua_State *L, const LV2_Atom *atom)
{
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &lua_handle->forge;

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
	else if(atom->type == forge->Vector)
	{
		lvec_t *lvec = lua_newuserdata(L, sizeof(lvec_t));
		lvec->vec = (const LV2_Atom_Vector *)atom;
		lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
			/ lvec->vec->body.child_size;
		luaL_getmetatable(L, "lvec");
	}
	else
	{
		latom_t *latom = lua_newuserdata(L, sizeof(latom_t));
		latom->atom = atom;
		luaL_getmetatable(L, "latom");
	}

	lua_setmetatable(L, -2);
}

static void
_latom_body_new(lua_State *L, uint32_t size, LV2_URID type, const void *body)
{
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &lua_handle->forge;
	size_t atom_size = sizeof(LV2_Atom) + size;

	if(type == forge->Object)
	{
		lobj_t *lobj = lua_newuserdata(L, sizeof(lobj_t) + atom_size);
		lobj->obj = (const LV2_Atom_Object *)lobj->body;
		lobj->body->size = size;
		lobj->body->type = type;
		memcpy(LV2_ATOM_BODY(lobj->body), body, size);
		luaL_getmetatable(L, "lobj");
	}
	else if(type == forge->Tuple)
	{
		ltuple_t *ltuple = lua_newuserdata(L, sizeof(ltuple_t) + atom_size);
		ltuple->tuple = (const LV2_Atom_Tuple *)ltuple->body;
		ltuple->body->size = size;
		ltuple->body->type = type;
		memcpy(LV2_ATOM_BODY(ltuple->body), body, size);
		luaL_getmetatable(L, "ltuple");
	}
	else if(type == forge->Vector)
	{
		lvec_t *lvec = lua_newuserdata(L, sizeof(lvec_t) + atom_size);
		lvec->vec = (const LV2_Atom_Vector *)lvec->body;
		lvec->body->size = size;
		lvec->body->type = type;
		memcpy(LV2_ATOM_BODY(lvec->body), body, size);
		lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
			/ lvec->vec->body.child_size;
		luaL_getmetatable(L, "lvec");
	}
	else
	{
		latom_t *latom = lua_newuserdata(L, sizeof(latom_t) + atom_size);
		latom->atom = (const LV2_Atom *)latom->body;
		latom->body->size = size;
		latom->body->type = type;
		memcpy(LV2_ATOM_BODY(latom->body), body, size);
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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	// reset iterator to beginning of sequence
	lseq->itr = lv2_atom_sequence_begin(&lseq->seq->body);

	lua_pushlightuserdata(L, lua_handle);
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
		// push index
		lua_pushinteger(L, ltuple->pos + 1);

		// push atom
		_latom_new(L, ltuple->itr);
	
		// advance iterator
		ltuple->pos += 1;
		ltuple->itr = lv2_atom_tuple_next(ltuple->itr);

		return 2;
	}

	// end of tuple reached
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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	// reset iterator to beginning of tuple
	ltuple->pos = 0;
	ltuple->itr = lv2_atom_tuple_begin(ltuple->tuple);

	lua_pushlightuserdata(L, lua_handle);
	lua_pushcclosure(L, _ltuple_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static int
_ltuple_unpack(lua_State *L)
{
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
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
_lvec_foreach_itr(lua_State *L)
{
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	if(lvec->pos < lvec->count)
	{
		// push index
		lua_pushinteger(L, lvec->pos + 1);

		// push atom
		_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
			LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, lvec->pos));

		// advance iterator
		lvec->pos += 1;

		return 2;
	}

	// end of vector reached
	lua_pushnil(L);
	return 1;
}

static int
_lvec__index(lua_State *L)
{
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	if(lua_isnumber(L, 2))
	{
		int index = lua_tointeger(L, 2); // indexing start from 1
		if( (index > 0) && (index <= lvec->count) )
		{
			_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
				LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, index - 1));
		}
		else // index is out of bounds
			lua_pushnil(L);
	}
	else if(lua_isstring(L, 2))
	{
		const char *key = lua_tostring(L, 2);

		if(!strcmp(key, "type"))
		{
			lua_pushinteger(L, lvec->vec->atom.type);
		}
		else if(!strcmp(key, "child_type"))
		{
			lua_pushinteger(L, lvec->vec->body.child_type);
		}
		else if(!strcmp(key, "child_size"))
		{
			lua_pushinteger(L, lvec->vec->body.child_size);
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
_lvec__len(lua_State *L)
{
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	lua_pushinteger(L, lvec->count);

	return 1;
}

static int
_lvec__tostring(lua_State *L)
{
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	lua_pushstring(L, "Atom_Vector");

	return 1;
}

static int
_lvec_foreach(lua_State *L)
{
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	// reset iterator to beginning of tuple
	lvec->pos = 0;

	lua_pushlightuserdata(L, lua_handle);
	lua_pushcclosure(L, _lvec_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static int
_lvec_unpack(lua_State *L)
{
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	for(int i=0; i<lvec->count; i++)
	{
		_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
			LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, i));
	}

	return lvec->count;
}

static const luaL_Reg lvec_mt [] = {
	{"__index", _lvec__index},
	{"__len", _lvec__len},
	{"__tostring", _lvec__tostring},
	{"foreach", _lvec_foreach},
	{"unpack", _lvec_unpack},
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

	// end of object reached
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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	// reset iterator to beginning of tuple
	lobj->itr = lv2_atom_object_begin(&lobj->obj->body);

	lua_pushlightuserdata(L, lua_handle);
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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &lua_handle->forge;

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
	else if(atom->type == lua_handle->uris.midi_event)
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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const char *key = lua_tostring(L, 2);

	if(!strcmp(key, "type"))
		lua_pushinteger(L, latom->atom->type);
	else if(!strcmp(key, "value"))
		_latom_value(L, latom->atom);
	else if(!strcmp(key, "datatype") && (latom->atom->type == lua_handle->forge.Literal) )
		lua_pushinteger(L, ((LV2_Atom_Literal *)latom->atom)->body.datatype);
	else if(!strcmp(key, "lang") && (latom->atom->type == lua_handle->forge.Literal) )
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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");

	if(latom->atom->type == lua_handle->forge.Literal)
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
_lforge_atom(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	const LV2_Atom **atom_ptr = lua_touserdata(L, 2); // TODO check for latom, lobj, ltuple, lvec
	const LV2_Atom *atom = *atom_ptr;

	lv2_atom_forge_raw(lforge->forge, atom, sizeof(LV2_Atom) + atom->size);
	lv2_atom_forge_pad(lforge->forge, atom->size);

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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	if(lua_istable(L, 2))
	{
		uint32_t size = lua_rawlen(L, 2);
		lv2_atom_forge_atom(lforge->forge, size, lua_handle->uris.midi_event);
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

	{"atom", _lforge_atom},
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
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	const char *uri = luaL_checkstring(L, 1);
	LV2_URID urid = lua_handle->map->map(lua_handle->map->handle, uri);

	if(urid)
		lua_pushinteger(L, urid);
	else
		lua_pushnil(L);

	return 1;
}

static int
_llv2_unmap(lua_State *L)
{
	lua_handle_t *lua_handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_URID urid = luaL_checkinteger(L, 1);
	const char *uri = lua_handle->unmap->unmap(lua_handle->unmap->handle, urid);

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
lua_handle_init(lua_handle_t *lua_handle, const LV2_Feature *const *features)
{
	if(lua_vm_init(&lua_handle->lvm))
	{
		fprintf(stderr, "Lua VM cannot be initialized\n");
		return -1;
	}

	for(int i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			lua_handle->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			lua_handle->unmap = (LV2_URID_Unmap *)features[i]->data;

	if(!lua_handle->map)
	{
		fprintf(stderr, "Host does not support urid:map\n");
		return -1;
	}
	if(!lua_handle->unmap)
	{
		fprintf(stderr, "Host does not support urid:unmap\n");
		return -1;
	}

	lua_handle->uris.lua_message = lua_handle->map->map(lua_handle->map->handle, LUA_MESSAGE_URI);
	lua_handle->uris.lua_code = lua_handle->map->map(lua_handle->map->handle, LUA_CODE_URI);
	lua_handle->uris.lua_error = lua_handle->map->map(lua_handle->map->handle, LUA_ERROR_URI);
	lua_handle->uris.midi_event = lua_handle->map->map(lua_handle->map->handle, LV2_MIDI__MidiEvent);

	lv2_atom_forge_init(&lua_handle->forge, lua_handle->map);

	return 0;
}

void
lua_handle_deinit(lua_handle_t *lua_handle)
{
	lua_vm_deinit(&lua_handle->lvm);
}

void
lua_handle_open(lua_handle_t *lua_handle, lua_State *L)
{
	luaL_newmetatable(L, "lseq");
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
	luaL_setfuncs (L, lseq_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lobj");
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
	luaL_setfuncs (L, lobj_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "ltuple");
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
	luaL_setfuncs (L, ltuple_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lvec");
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
	luaL_setfuncs (L, lvec_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "latom");
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
	luaL_setfuncs (L, latom_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lforge");
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
	luaL_setfuncs (L, lforge_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lframe");
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
	luaL_setfuncs (L, lframe_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

#define SET_CONST(L, ID) \
	lua_pushinteger(L, lua_handle->forge.ID); \
	lua_setfield(L, -2, #ID);

	lua_newtable(L);
	lua_pushlightuserdata(L, lua_handle); // @ upvalueindex 1
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
	lua_pushinteger(L, lua_handle->uris.midi_event);
		lua_setfield(L, -2, "Midi");
	lua_setglobal(L, "lv2");

	// delete print function as it is not realtime-safe
	lua_pushnil(L);
	lua_setglobal(L, "print");

	//TODO support for LV2_Log extension

#undef SET_CONST
}

void
lua_handle_in(lua_handle_t *lua_handle, const LV2_Atom_Sequence *seq)
{
	lua_State *L = lua_handle->lvm.L;

	if(lua_handle->dirty_in)
	{
		// load chunk
		if(luaL_dostring(L, lua_handle->chunk))
		{
			strcpy(lua_handle->error, lua_tostring(L, -1));
			lua_pop(L, 1);

			lua_handle->error_out = 1;
		}
		else
			lua_handle->error[0] = 0x0; // reset error flag

		lua_handle->dirty_in = 0;
	}

	LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
	{
		const LV2_Atom *atom = &ev->body;

		if(atom->type == lua_handle->forge.String)
		{
			if(atom->size)
			{
				strncpy(lua_handle->chunk, LV2_ATOM_BODY_CONST(atom), atom->size);
				lua_handle->dirty_in = 1;
			}
			else
				lua_handle->dirty_out = 1;
		}
	}
}

static LV2_State_Status
_state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	lua_handle_t *lua_handle = (lua_handle_t *)instance;

	return store(
		state,
		lua_handle->uris.lua_code,
		lua_handle->chunk,
		strlen(lua_handle->chunk)+1,
		lua_handle->forge.String,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}

static LV2_State_Status
_state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	lua_handle_t *lua_handle = (lua_handle_t *)instance;

	size_t size;
	uint32_t type;
	uint32_t flags2;
	const char *chunk = retrieve(
		state,
		lua_handle->uris.lua_code,
		&size,
		&type,
		&flags2
	);

	//TODO check type, flags2

	if(chunk && size && type)
	{
		strncpy(lua_handle->chunk, chunk, size);
		lua_handle->dirty_in = 1;
	}

	return LV2_STATE_SUCCESS;
}

const LV2_State_Interface lua_handle_state_iface = {
	.save = _state_save,
	.restore = _state_restore
};

void
lua_handle_activate(lua_handle_t *lua_handle, const char *chunk)
{
	lua_State *L = lua_handle->lvm.L;

	// load chunk
	strcpy(lua_handle->chunk, chunk);
	luaL_dostring(L, lua_handle->chunk); // cannot fail

	lua_handle->dirty_out = 1; // trigger update of UI
}

void
lua_handle_out(lua_handle_t *lua_handle, LV2_Atom_Sequence *seq, uint32_t frames)
{
	// prepare notify atom forge
	LV2_Atom_Forge *forge = &lua_handle->forge;
	LV2_Atom_Forge_Frame notify_frame;
	
	uint32_t capacity = seq->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)seq, capacity);
	lv2_atom_forge_sequence_head(forge, &notify_frame, 0);

	if(lua_handle->dirty_out)
	{
		uint32_t len = strlen(lua_handle->chunk) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, frames);
		lv2_atom_forge_object(forge, &obj_frame, 0, lua_handle->uris.lua_message);
		lv2_atom_forge_key(forge, lua_handle->uris.lua_code);
		lv2_atom_forge_string(forge, lua_handle->chunk, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		lua_handle->dirty_out = 0; // reset flag
	}

	if(lua_handle->error_out)
	{
		uint32_t len = strlen(lua_handle->error) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, frames);
		lv2_atom_forge_object(forge, &obj_frame, 0, lua_handle->uris.lua_message);
		lv2_atom_forge_key(forge, lua_handle->uris.lua_error);
		lv2_atom_forge_string(forge, lua_handle->error, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		lua_handle->error_out = 0; // reset flag
	}
	
	lv2_atom_forge_pop(forge, &notify_frame);
}
