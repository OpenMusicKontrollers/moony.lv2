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

#include <moony.h>

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

static const char *moony_ref [MOONY_UDATA_COUNT] = {
	[MOONY_UDATA_SEQ]		= "lseq",
	[MOONY_UDATA_OBJ]		= "lobj",
	[MOONY_UDATA_TUPLE]	= "ltuple",
	[MOONY_UDATA_VEC]		= "lvec",
	[MOONY_UDATA_CHUNK]	= "lchunk",
	[MOONY_UDATA_ATOM]	= "latom",
	[MOONY_UDATA_FORGE]	= "lforge"
};

static const size_t moony_sz [MOONY_UDATA_COUNT] = {
	[MOONY_UDATA_SEQ]		= sizeof(lseq_t),
	[MOONY_UDATA_OBJ]		= sizeof(lobj_t),
	[MOONY_UDATA_TUPLE]	= sizeof(ltuple_t),
	[MOONY_UDATA_VEC]		= sizeof(lvec_t),
	[MOONY_UDATA_CHUNK]	= sizeof(latom_t),
	[MOONY_UDATA_ATOM]	= sizeof(latom_t),
	[MOONY_UDATA_FORGE]	= sizeof(lforge_t)
};

static const char *forge_buffer_overflow = "forge buffer overflow";

static void
_latom_new(lua_State *L, const LV2_Atom *atom)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &moony->forge;

	if(atom->type == forge->Object)
	{
		lobj_t *lobj = moony_newuserdata(L, moony, MOONY_UDATA_OBJ);
		lobj->obj = (const LV2_Atom_Object *)atom;
	}
	else if(atom->type == forge->Tuple)
	{
		ltuple_t *ltuple = moony_newuserdata(L, moony, MOONY_UDATA_TUPLE);
		ltuple->tuple = (const LV2_Atom_Tuple *)atom;
	}
	else if(atom->type == forge->Vector)
	{
		lvec_t *lvec = moony_newuserdata(L, moony, MOONY_UDATA_VEC);
		lvec->vec = (const LV2_Atom_Vector *)atom;
		lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
			/ lvec->vec->body.child_size;
	}
	else if(atom->type == forge->Sequence)
	{
		lseq_t *lseq = moony_newuserdata(L, moony, MOONY_UDATA_SEQ);
		lseq->seq = (const LV2_Atom_Sequence *)atom;
		lseq->itr = NULL;
	}
	else if( (atom->type == forge->Chunk) || (atom->type == moony->uris.midi_event) )
	{
		latom_t *latom = moony_newuserdata(L, moony, MOONY_UDATA_CHUNK);
		latom->atom = atom;
	}
	else
	{
		latom_t *latom = moony_newuserdata(L, moony, MOONY_UDATA_ATOM);
		latom->atom = atom;
	}
}

static void
_latom_body_new(lua_State *L, uint32_t size, LV2_URID type, const void *body)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &moony->forge;
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
	else if(type == forge->Sequence)
	{
		lseq_t *lseq = lua_newuserdata(L, sizeof(lseq_t) + atom_size);
		lseq->seq = (const LV2_Atom_Sequence *)lseq->body;
		lseq->body->size = size;
		lseq->body->type = type;
		memcpy(LV2_ATOM_BODY(lseq->body), body, size);
		lseq->itr = NULL;
		luaL_getmetatable(L, "lseq");
	}
	else if( (type == forge->Chunk) || (type == moony->uris.midi_event) )
	{
		latom_t *latom = lua_newuserdata(L, sizeof(latom_t) + atom_size);
		latom->atom = (const LV2_Atom *)latom->body;
		latom->body->size = size;
		latom->body->type = type;
		memcpy(LV2_ATOM_BODY(latom->body), body, size);
		luaL_getmetatable(L, "lchunk");
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
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	// reset iterator to beginning of sequence
	lseq->itr = lv2_atom_sequence_begin(&lseq->seq->body);

	lua_pushlightuserdata(L, moony);
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
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	// reset iterator to beginning of tuple
	ltuple->pos = 0;
	ltuple->itr = lv2_atom_tuple_begin(ltuple->tuple);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _ltuple_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static int
_ltuple_unpack(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	int n = lua_gettop(L);
	int min = n > 1
		? luaL_checkinteger(L, 2)
		: 1;
	int max = n > 2
		? luaL_checkinteger(L, 3)
		: INT_MAX;

	int pos = 1;
	int count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
	{
		if(pos >= min)
		{
			if(pos <= max)
			{
				_latom_new(L, atom);
				count += 1;
			}
			else
				break;
		}

		pos += 1;
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
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	// reset iterator to beginning of tuple
	lvec->pos = 0;

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _lvec_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static int
_lvec_unpack(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lvec_t *lvec = luaL_checkudata(L, 1, "lvec");

	int n = lua_gettop(L);
	int min = 1;
	int max = lvec->count;

	if(n > 1) // check provided index ranges
	{
		min = luaL_checkinteger(L, 2);
		min = min < 1
			? 1
			: (min > lvec->count
				? lvec->count
				: min);

		if(n > 2)
		{
			max = luaL_checkinteger(L, 3);
			max = max < 1
				? 1
				: (max > lvec->count
					? lvec->count
					: max);
		}
	}

	for(int i=min; i<=max; i++)
	{
		_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
			LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, i-1));
	}

	return max - min + 1;
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
_lchunk__index(lua_State *L)
{
	latom_t *lchunk = luaL_checkudata(L, 1, "lchunk");
	const uint8_t *payload = LV2_ATOM_BODY_CONST(lchunk->atom);

	if(lua_isnumber(L, 2))
	{
		int index = lua_tointeger(L, 2); // indexing start from 1
		if( (index > 0) && (index <= lchunk->atom->size) )
			lua_pushinteger(L, payload[index-1]);
		else // index is out of bounds
			lua_pushnil(L);
	}
	else if(lua_isstring(L, 2))
	{
		const char *key = lua_tostring(L, 2);

		if(!strcmp(key, "type"))
		{
			lua_pushinteger(L, lchunk->atom->type);
		}
		else if(!strcmp(key, "value"))
		{
			lua_createtable(L, lchunk->atom->size, 0);
			for(int i=0; i<lchunk->atom->size; i++)
			{
				lua_pushinteger(L, payload[i]);
				lua_rawseti(L, -2, i+1);
			}
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
_lchunk__len(lua_State *L)
{
	latom_t *lchunk = luaL_checkudata(L, 1, "lchunk");

	lua_pushinteger(L, lchunk->atom->size);

	return 1;
}

static int
_lchunk__tostring(lua_State *L)
{
	latom_t *lchunk = luaL_checkudata(L, 1, "lchunk");

	lua_pushstring(L, "Atom_Chunk");

	return 1;
}

static int
_lchunk_unpack(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *lchunk = luaL_checkudata(L, 1, "lchunk");
	const uint8_t *payload = LV2_ATOM_BODY_CONST(lchunk->atom);

	int n = lua_gettop(L);
	int min = 1;
	int max = lchunk->atom->size;

	if(n > 1) // check provided index ranges
	{
		min = luaL_checkinteger(L, 2);
		min = min < 1
			? 1
			: (min > lchunk->atom->size
				? lchunk->atom->size
				: min);

		if(n > 2)
		{
			max = luaL_checkinteger(L, 3);
			max = max < 1
				? 1
				: (max > lchunk->atom->size
					? lchunk->atom->size
					: max);
		}
	}

	for(int i=min; i<=max; i++)
		lua_pushinteger(L, payload[i-1]);

	return max - min + 1;
}

static const luaL_Reg lchunk_mt [] = {
	{"__index", _lchunk__index},
	{"__len", _lchunk__len},
	{"__tostring", _lchunk__tostring},
	{"unpack", _lchunk_unpack},
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
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	// reset iterator to beginning of tuple
	lobj->itr = lv2_atom_object_begin(&lobj->obj->body);

	lua_pushlightuserdata(L, moony);
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
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &moony->forge;

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
	else // unknown type
		lua_pushnil(L);
}

static int
_latom__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const char *key = lua_tostring(L, 2);

	if(!strcmp(key, "type"))
	{
		if(latom->atom->type)
			lua_pushinteger(L, latom->atom->type);
		else
			lua_pushnil(L); // null atom
	}
	else if(!strcmp(key, "value"))
		_latom_value(L, latom->atom);
	else if(!strcmp(key, "datatype") && (latom->atom->type == moony->forge.Literal) )
		lua_pushinteger(L, ((LV2_Atom_Literal *)latom->atom)->body.datatype);
	else if(!strcmp(key, "lang") && (latom->atom->type == moony->forge.Literal) )
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
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");

	if(latom->atom->type == moony->forge.Literal)
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

	if(val >= lforge->last.frames)
	{
		if(!lv2_atom_forge_frame_time(lforge->forge, val))
			luaL_error(L, forge_buffer_overflow);
		lforge->last.frames = val;

		lua_settop(L, 1);
		return 1;
	}

	return luaL_error(L, "invalid frame time, must not decrease");
}

static int
_lforge_beat_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	double val = luaL_checknumber(L, 2);

	if(val >= lforge->last.beats)
	{
		if(!lv2_atom_forge_beat_time(lforge->forge, val))
			luaL_error(L, forge_buffer_overflow);
		lforge->last.beats = val;

		lua_settop(L, 1);
		return 1;
	}

	return luaL_error(L, "invalid beat time, must not decrease");
}

static int
_lforge_atom(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	if(  luaL_testudata(L, 2, "lseq")
		|| luaL_testudata(L, 2, "lobj")
		|| luaL_testudata(L, 2, "ltuple")
		|| luaL_testudata(L, 2, "lvec")
		|| luaL_testudata(L, 2, "lchunk")
		|| luaL_testudata(L, 2, "latom") )
	{
		const LV2_Atom **atom_ptr = lua_touserdata(L, 2);
		const LV2_Atom *atom = *atom_ptr;

		if(!lv2_atom_forge_raw(lforge->forge, atom, sizeof(LV2_Atom) + atom->size))
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pad(lforge->forge, atom->size);

		lua_settop(L, 1);
		return 1;
	}

	return luaL_error(L, "Atom expected at position #2");
}

static int
_lforge_int(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int32_t val = luaL_checkinteger(L, 2);

	if(!lv2_atom_forge_int(lforge->forge, val))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_long(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int64_t val = luaL_checkinteger(L, 2);

	if(!lv2_atom_forge_long(lforge->forge, val))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_float(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	float val = luaL_checknumber(L, 2);

	if(!lv2_atom_forge_float(lforge->forge, val))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_double(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	double val = luaL_checknumber(L, 2);

	if(!lv2_atom_forge_double(lforge->forge, val))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_bool(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	uint32_t val = lua_toboolean(L, 2);

	if(!lv2_atom_forge_bool(lforge->forge, val))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_urid(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID val = luaL_checkinteger(L, 2);

	if(!lv2_atom_forge_urid(lforge->forge, val))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_string(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	if(!lv2_atom_forge_string(lforge->forge, val, size))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_literal(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);
	LV2_URID datatype = luaL_checkinteger(L, 3);
	LV2_URID lang = luaL_checkinteger(L, 4);

	if(!lv2_atom_forge_literal(lforge->forge, val, size, datatype, lang))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_uri(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	if(!lv2_atom_forge_uri(lforge->forge, val, size))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_path(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	if(!lv2_atom_forge_path(lforge->forge, val, size))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_bytes(lua_State *L, moony_t *moony, LV2_URID type)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	if(lua_istable(L, 2))
	{
		uint32_t size = lua_rawlen(L, 2);
		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		for(int i=1; i<=size; i++)
		{
			lua_rawgeti(L, -1, i);
			uint8_t byte = luaL_checkinteger(L, -1);
			lua_pop(L, 1);
			if(!lv2_atom_forge_raw(lforge->forge, &byte, 1))
				luaL_error(L, forge_buffer_overflow);
		}
		lv2_atom_forge_pad(lforge->forge, size);
	}
	else if(luaL_testudata(L, 2, "lchunk")) //to convert between chunk <-> midi
	{
		latom_t *lchunk = lua_touserdata(L, 2);
		uint32_t size = lchunk->atom->size;
		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_raw(lforge->forge, LV2_ATOM_BODY_CONST(lchunk->atom), size))
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pad(lforge->forge, size);
	}
	else // bytes as individual function arguments
	{
		uint32_t size = lua_gettop(L) - 1;

		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		for(int i=0; i<size; i++)
		{
			uint8_t byte = luaL_checkinteger(L, i+2);
			if(!lv2_atom_forge_raw(lforge->forge, &byte, 1))
				luaL_error(L, forge_buffer_overflow);
		}
		lv2_atom_forge_pad(lforge->forge, size);
	}

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_chunk(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	return _lforge_bytes(L, moony, moony->forge.Chunk);
}

static int
_lforge_midi(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	return _lforge_bytes(L, moony, moony->uris.midi_event);
}

static int
_lforge_osc_bundle(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	osc_forge_t *oforge = &moony->oforge;
	LV2_Atom_Forge *forge = lforge->forge;

	uint64_t timestamp = luaL_checkinteger(L, 2);

	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 2;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!osc_forge_bundle_push(oforge, forge, lframe->frame, timestamp))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_osc_message(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	osc_forge_t *oforge = &moony->oforge;
	LV2_Atom_Forge *forge = lforge->forge;

	const char *path = luaL_checkstring(L, 2);
	const char *fmt = luaL_optstring(L, 3, "");

	LV2_Atom_Forge_Frame frame [2];

	if(!osc_forge_message_push(oforge, forge, frame, path, fmt))
		luaL_error(L, forge_buffer_overflow);

	int pos = 4;
	for(const char *type = fmt; *type; type++)
	{
		switch(*type)
		{
			case 'i':
			{
				int32_t i = luaL_checkinteger(L, pos++);
				if(!osc_forge_int32(oforge, forge, i))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'f':
			{
				float f = luaL_checknumber(L, pos++);
				if(!osc_forge_float(oforge, forge, f))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 's':
			case 'S':
			{
				const char *s = luaL_checkstring(L, pos++);
				if(!osc_forge_string(oforge, forge, s))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'b':
			{
				if(lua_istable(L, pos))
				{
					int n = lua_rawlen(L, pos);

					if(!lv2_atom_forge_atom(forge, n, forge->Chunk))
						luaL_error(L, forge_buffer_overflow);
					for(int i=1; i<=n; i++)
					{
						lua_rawgeti(L, pos, i);
						uint8_t b = luaL_checkinteger(L, -1);
						lua_pop(L, 1);
						if(!lv2_atom_forge_raw(forge, &b, 1))
							luaL_error(L, forge_buffer_overflow);
					}
					lv2_atom_forge_pad(forge, n);
				}
				pos += 1;

				break;
			}

			case 'T':
			{
				if(!osc_forge_true(oforge, forge))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'F':
			{
				if(!osc_forge_false(oforge, forge))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'N':
			{
				if(!osc_forge_nil(oforge, forge))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'I':
			{
				if(!osc_forge_bang(oforge, forge))
					luaL_error(L, forge_buffer_overflow);
				break;
			}

			case 'h':
			{
				int64_t h = luaL_checkinteger(L, pos++);
				if(!osc_forge_int64(oforge, forge, h))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'd':
			{
				double d = luaL_checknumber(L, pos++);
				if(!osc_forge_double(oforge, forge, d))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 't':
			{
				uint64_t t = luaL_checkinteger(L, pos++);
				if(!osc_forge_timestamp(oforge, forge, t))
					luaL_error(L, forge_buffer_overflow);
				break;
			}

			case 'c':
			{
				char c = luaL_checkinteger(L, pos++);
				if(!osc_forge_char(oforge, forge, c))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'm':
			{
				if(lua_istable(L, pos))
				{
					int n = lua_rawlen(L, pos);

					if(!lv2_atom_forge_atom(forge, n, moony->oforge.MIDI_MidiEvent))
						luaL_error(L, forge_buffer_overflow);
					for(int i=1; i<=n; i++)
					{
						lua_rawgeti(L, pos, i);
						uint8_t b = luaL_checkinteger(L, -1);
						lua_pop(L, 1);
						if(!lv2_atom_forge_raw(forge, &b, 1))
							luaL_error(L, forge_buffer_overflow);
					}
					lv2_atom_forge_pad(forge, n);
				}
				pos += 1;

				break;
			}
		}
	}

	osc_forge_message_pop(oforge, forge, frame);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_tuple(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_tuple(lforge->forge, &lframe->frame[0]))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_object(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID id = luaL_checkinteger(L, 2);
	LV2_URID otype = luaL_checkinteger(L, 3);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, &lframe->frame[0], id, otype))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_key(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);

	if(!lv2_atom_forge_key(lforge->forge, key))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_property(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);
	LV2_URID context = luaL_checkinteger(L, 3);

	if(!lv2_atom_forge_property_head(lforge->forge, key, context))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_sequence(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID unit = luaL_optinteger(L, 2, 0); //TODO use proper unit
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = 0;
	lframe->forge = lforge->forge;
	
	if(!lv2_atom_forge_sequence_head(lforge->forge, &lframe->frame[0], unit))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_pop(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	for(int i=lforge->depth; i>0; i--)
		lv2_atom_forge_pop(lforge->forge, &lforge->frame[i-1]);
	lforge->depth = 0; // reset depth

	return 0;
}

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
	{"chunk", _lforge_chunk},
	{"midi", _lforge_midi},
	{"bundle", _lforge_osc_bundle},
	{"message", _lforge_osc_message},
	{"tuple", _lforge_tuple},
	{"object", _lforge_object},
	{"key", _lforge_key},
	{"property", _lforge_property},
	{"sequence", _lforge_sequence},

	{"pop", _lforge_pop},

	{NULL, NULL}
};

static int
_lmap__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_pushvalue(L, 2);
	lua_rawget(L, 1); // rawget(self, uri)

	if(lua_isnil(L, -1)) // no yet cached
	{
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
	}

	return 1;
}

static int
_lmap__call(lua_State *L)
{
	lua_settop(L, 2);
	lua_gettable(L, -2); // self[uri]

	return 1;
}

static const luaL_Reg lmap_mt [] = {
	{"__index", _lmap__index},
	{"__call", _lmap__index},
	{NULL, NULL}
};

static int
_lunmap__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_pushvalue(L, 2);
	lua_rawget(L, 1); // rawget(self, urid)

	if(lua_isnil(L, -1)) // no yet cached
	{
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
	}

	return 1;
}

static int
_lunmap__call(lua_State *L)
{
	lua_settop(L, 2);
	lua_gettable(L, -2); // self[uri]

	return 1;
}

static const luaL_Reg lunmap_mt [] = {
	{"__index", _lunmap__index},
	{"__call", _lunmap__index},
	{NULL, NULL}
};

static int
_log(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	int n = lua_gettop(L);

	if(!moony->log || !n)
		return 0;

	luaL_Buffer buf;
	luaL_buffinit(L, &buf);

	for(int i=1; i<=n; i++)
	{
		size_t len;
		const char *str = NULL;
		switch(lua_type(L, i))
		{
			case LUA_TNIL:
				str = "(nil)";
				break;
			case LUA_TBOOLEAN:
				str = lua_toboolean(L, i) ? "true" : "false";
				break;
			case LUA_TLIGHTUSERDATA:
				str = lua_tostring(L, i);
				if(!str)
					str = "(lightuserdata)";
				break;
			case LUA_TNUMBER:
				str = lua_tostring(L, i);
				break;
			case LUA_TSTRING:
				str = lua_tostring(L, i);
				break;
			case LUA_TTABLE:
				str = lua_tostring(L, i);
				if(!str)
					str = "(table)";
				break;
			case LUA_TFUNCTION:
				str = "(function)";
				break;
			case LUA_TUSERDATA:
				str = lua_tostring(L, i);
				if(!str)
					str = "(userdata)";
				break;
			case LUA_TTHREAD:
				str = "(thread)";
				break;
		};
		if(!str)
			continue;
		luaL_addstring(&buf, str);
		if(i < n)
			luaL_addchar(&buf, '\t');
	}

	luaL_pushresult(&buf);

	const char *res = lua_tostring(L, -1);
	moony->log->printf(moony->log->handle, moony->uris.log_trace, "%s", res);

	return 0;
}

int
moony_init(moony_t *moony, const LV2_Feature *const *features)
{
	if(moony_vm_init(&moony->vm))
	{
		fprintf(stderr, "Lua VM cannot be initialized\n");
		return -1;
	}

	for(int i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			moony->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			moony->unmap = (LV2_URID_Unmap *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_WORKER__schedule))
			moony->sched = (LV2_Worker_Schedule *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			moony->log = (LV2_Log_Log *)features[i]->data;

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

	moony->uris.moony_message = moony->map->map(moony->map->handle, MOONY_MESSAGE_URI);
	moony->uris.moony_code = moony->map->map(moony->map->handle, MOONY_CODE_URI);
	moony->uris.moony_error = moony->map->map(moony->map->handle, MOONY_ERROR_URI);

	moony->uris.log_trace = moony->map->map(moony->map->handle, LV2_LOG__Trace);
	
	moony->uris.midi_event = moony->map->map(moony->map->handle, LV2_MIDI__MidiEvent);

	moony->uris.time_position = moony->map->map(moony->map->handle, LV2_TIME__Position);
	moony->uris.time_barBeat = moony->map->map(moony->map->handle, LV2_TIME__barBeat);
	moony->uris.time_bar = moony->map->map(moony->map->handle, LV2_TIME__bar);
	moony->uris.time_beat = moony->map->map(moony->map->handle, LV2_TIME__beat);
	moony->uris.time_beatUnit = moony->map->map(moony->map->handle, LV2_TIME__beatUnit);
	moony->uris.time_beatsPerBar = moony->map->map(moony->map->handle, LV2_TIME__beatsPerBar);
	moony->uris.time_beatsPerMinute = moony->map->map(moony->map->handle, LV2_TIME__beatsPerMinute);
	moony->uris.time_frame = moony->map->map(moony->map->handle, LV2_TIME__frame);
	moony->uris.time_framesPerSecond = moony->map->map(moony->map->handle, LV2_TIME__framesPerSecond);
	moony->uris.time_speed = moony->map->map(moony->map->handle, LV2_TIME__speed);

	osc_forge_init(&moony->oforge, moony->map);
	lv2_atom_forge_init(&moony->forge, moony->map);

	return 0;
}

void
moony_deinit(moony_t *moony)
{
	moony_vm_deinit(&moony->vm);
}

void
moony_open(moony_t *moony, lua_State *L)
{
	luaL_newmetatable(L, "lseq");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lseq_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lobj");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lobj_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "ltuple");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, ltuple_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lvec");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lvec_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lchunk");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lchunk_mt, 1);
	lua_pop(L, 1);

	luaL_newmetatable(L, "latom");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, latom_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lforge");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lforge_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

#define SET_CONST(L, ID) \
	lua_pushinteger(L, moony->forge.ID); \
	lua_setfield(L, -2, #ID);

	lua_newtable(L);
	{
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
	}
	lua_setglobal(L, "Atom");

	lua_newtable(L);
	{
		lua_pushinteger(L, moony->uris.midi_event);
			lua_setfield(L, -2, "Event");
	}
	lua_setglobal(L, "MIDI");

	lua_newtable(L);
	{
		lua_pushinteger(L, moony->uris.time_position);
			lua_setfield(L, -2, "Position");
		lua_pushinteger(L, moony->uris.time_barBeat);
			lua_setfield(L, -2, "barBeat");
		lua_pushinteger(L, moony->uris.time_bar);
			lua_setfield(L, -2, "bar");
		lua_pushinteger(L, moony->uris.time_beat);
			lua_setfield(L, -2, "beat");
		lua_pushinteger(L, moony->uris.time_beatUnit);
			lua_setfield(L, -2, "beatUnit");
		lua_pushinteger(L, moony->uris.time_beatsPerBar);
			lua_setfield(L, -2, "beatsPerBar");
		lua_pushinteger(L, moony->uris.time_beatsPerMinute);
			lua_setfield(L, -2, "beatsPerMinute");
		lua_pushinteger(L, moony->uris.time_frame);
			lua_setfield(L, -2, "frame");
		lua_pushinteger(L, moony->uris.time_framesPerSecond);
			lua_setfield(L, -2, "framesPerSecond");
		lua_pushinteger(L, moony->uris.time_speed);
			lua_setfield(L, -2, "speed");
	}
	lua_setglobal(L, "Time");

	lua_newtable(L);
	{
		lua_pushinteger(L, moony->oforge.OSC_Event);
			lua_setfield(L, -2, "Event");

		lua_pushinteger(L, moony->oforge.OSC_Bundle);
			lua_setfield(L, -2, "Bundle");
		lua_pushinteger(L, moony->oforge.OSC_Message);
			lua_setfield(L, -2, "Message");
		
		lua_pushinteger(L, moony->oforge.OSC_bundleTimestamp);
			lua_setfield(L, -2, "bundleTimestamp");
		lua_pushinteger(L, moony->oforge.OSC_bundleItems);
			lua_setfield(L, -2, "bundleItems");
		
		lua_pushinteger(L, moony->oforge.OSC_messagePath);
			lua_setfield(L, -2, "messagePath");
		lua_pushinteger(L, moony->oforge.OSC_messageFormat);
			lua_setfield(L, -2, "messageFormat");
		lua_pushinteger(L, moony->oforge.OSC_messageArguments);
			lua_setfield(L, -2, "messageArguments");
	}
	lua_setglobal(L, "OSC");

	// lv2.map
	lua_newtable(L);
	lua_newtable(L);
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs(L, lmap_mt, 1);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Map");

	// lv2.unmap
	lua_newtable(L);
	lua_newtable(L);
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs(L, lunmap_mt, 1);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Unmap");

#define UDATA_OFFSET (LUA_RIDX_LAST + 1)
	// create userdata caches
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_SEQ);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_OBJ);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_TUPLE);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_VEC);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_CHUNK);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_ATOM);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_FORGE);

	// overwrite print function with LV2 log
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _log, 1);
	lua_setglobal(L, "print");

#undef SET_CONST
}

void *
moony_newuserdata(lua_State *L, moony_t *moony, moony_udata_t type)
{
	assert( (type >= MOONY_UDATA_SEQ) && (type < MOONY_UDATA_COUNT) );

	int *itr = &moony->itr[type];
	void *data = NULL;

	lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + type); // ref
	lua_rawgeti(L, -1, *itr); // udata?
	if(lua_isnil(L, -1)) // no cached udata, create one!
	{
		lua_pop(L, 1); // nil

		data = lua_newuserdata(L, moony_sz[type]);
		luaL_getmetatable(L, moony_ref[type]);
		lua_setmetatable(L, -2);
		lua_pushvalue(L, -1);
		lua_rawseti(L, -3, *itr); // store in cache
	}
	else // there is a cached udata, use it!
	{
		data = lua_touserdata(L, -1);
		//printf("moony_newuserdata: %s %i %p\n", moony_ref[type], *itr, data);
	}
	lua_remove(L, -2); // ref
	*itr += 1;

	return data;

#undef UDATA_OFFSET
}

void
moony_in(moony_t *moony, const LV2_Atom_Sequence *seq)
{
	lua_State *L = moony->vm.L;

	if(moony->dirty_in)
	{
		// load chunk
		if(luaL_dostring(L, moony->chunk))
		{
			strcpy(moony->error, lua_tostring(L, -1));
			lua_pop(L, 1);

			moony->error_out = 1;
		}
		else
			moony->error[0] = 0x0; // reset error flag

		moony->dirty_in = 0;
	}

	LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
	{
		const moony_message_t *msg = (const moony_message_t *)&ev->body;

		if(  (msg->obj.atom.type == moony->forge.Object)
			&& (msg->obj.body.otype == moony->uris.moony_message)
			&& (msg->prop.key == moony->uris.moony_code) )
		{
			if(msg->prop.value.size)
			{
				strncpy(moony->chunk, msg->body, msg->prop.value.size);
				moony->dirty_in = 1;
			}
			else
				moony->dirty_out = 1;
		}
	}
}

static LV2_State_Status
_state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	moony_t *moony = (moony_t *)instance;

	return store(
		state,
		moony->uris.moony_code,
		moony->chunk,
		strlen(moony->chunk)+1,
		moony->forge.String,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}

static LV2_State_Status
_state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	moony_t *moony = (moony_t *)instance;

	size_t size;
	uint32_t type;
	uint32_t flags2;
	const char *chunk = retrieve(
		state,
		moony->uris.moony_code,
		&size,
		&type,
		&flags2
	);

	//TODO check type, flags2

	if(chunk && size && type)
	{
		strncpy(moony->chunk, chunk, size);
		moony->dirty_in = 1;
		moony->dirty_out = 1;
	}

	return LV2_STATE_SUCCESS;
}

static const LV2_State_Interface state_iface = {
	.save = _state_save,
	.restore = _state_restore
};

// non-rt thread
static LV2_Worker_Status
_work(LV2_Handle instance,
	LV2_Worker_Respond_Function respond,
	LV2_Worker_Respond_Handle target,
	uint32_t size,
	const void *body)
{
	moony_t *moony = instance;

	assert(size == sizeof(moony_mem_t));
	const moony_mem_t *request = body;
	uint32_t i = request->i;

	if(request->mem) // request to free memory from _work_response
	{
		moony_vm_mem_free(request->mem, moony->vm.size[i]);
	}
	else // request to allocate memory from moony_vm_mem_extend
	{
		moony->vm.area[i] = moony_vm_mem_alloc(moony->vm.size[i]);
	
		respond(target, size, body); // signal to _work_response
	}
	
	return LV2_WORKER_SUCCESS;
}

// rt-thread
static LV2_Worker_Status
_work_response(LV2_Handle instance, uint32_t size, const void *body)
{
	moony_t *moony = instance;

	assert(size == sizeof(moony_mem_t));
	const moony_mem_t *request = body;
	uint32_t i = request->i;
	
	if(!moony->vm.area[i]) // allocation failed
		return LV2_WORKER_ERR_UNKNOWN;

	// tlsf add pool
	moony->vm.pool[i] = tlsf_add_pool(moony->vm.tlsf,
		moony->vm.area[i], moony->vm.size[i]);

	if(!moony->vm.pool[i]) // pool addition failed
	{
		const moony_mem_t req = {
			.i = i,
			.mem = moony->vm.area[i]
		};
		moony->vm.area[i] = NULL;

		// schedule freeing of memory to _work
		LV2_Worker_Status status = moony->sched->schedule_work(
			moony->sched->handle, sizeof(moony_mem_t), &req);

		//TODO check status

		return LV2_WORKER_ERR_UNKNOWN;
	}
		
	moony->vm.space += moony->vm.size[i];
	//printf("mem extended to %zu KB\n", moony->vm.space / 1024);

	return LV2_WORKER_SUCCESS;
}

// rt-thread
static LV2_Worker_Status
_end_run(LV2_Handle instance)
{
	moony_t *moony = instance;

	// do nothing
	moony->working = 0;

	return LV2_WORKER_SUCCESS;
}

static const LV2_Worker_Interface work_iface = {
	.work = _work,
	.work_response = _work_response,
	.end_run = _end_run
};

const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_WORKER__interface))
		return &work_iface;
	else if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;
	else
		return NULL;
}

void
moony_activate(moony_t *moony, const char *chunk)
{
	lua_State *L = moony->vm.L;

	// load chunk
	strcpy(moony->chunk, chunk);
	luaL_dostring(L, moony->chunk); // cannot fail

	moony->dirty_out = 1; // trigger update of UI
}

void
moony_out(moony_t *moony, LV2_Atom_Sequence *seq, uint32_t frames)
{
	// prepare notify atom forge
	LV2_Atom_Forge *forge = &moony->forge;
	LV2_Atom_Forge_Frame notify_frame;
	
	uint32_t capacity = seq->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)seq, capacity);
	lv2_atom_forge_sequence_head(forge, &notify_frame, 0);

	if(moony->dirty_out)
	{
		uint32_t len = strlen(moony->chunk) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, frames);
		lv2_atom_forge_object(forge, &obj_frame, 0, moony->uris.moony_message);
		lv2_atom_forge_key(forge, moony->uris.moony_code);
		lv2_atom_forge_string(forge, moony->chunk, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		moony->dirty_out = 0; // reset flag
	}

	if(moony->error_out)
	{
		uint32_t len = strlen(moony->error) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, frames);
		lv2_atom_forge_object(forge, &obj_frame, 0, moony->uris.moony_message);
		lv2_atom_forge_key(forge, moony->uris.moony_error);
		lv2_atom_forge_string(forge, moony->error, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		moony->error_out = 0; // reset flag
	}
	
	lv2_atom_forge_pop(forge, &notify_frame);
}
