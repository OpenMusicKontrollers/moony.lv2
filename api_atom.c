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

#include <api_atom.h>
#include <api_stash.h>

#include <inttypes.h>

#define LV2_ATOM_VECTOR_BODY_ITEM_CONST(body, i) \
	(LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector_Body, (body)) + (i)*(body)->child_size)

static const lua_CFunction upclosures [MOONY_UPCLOSURE_COUNT] = {
	[MOONY_UPCLOSURE_TUPLE_FOREACH] = _latom_tuple_foreach_itr,
	[MOONY_UPCLOSURE_VECTOR_FOREACH] = _latom_vec_foreach_itr,
	[MOONY_UPCLOSURE_OBJECT_FOREACH] = _latom_obj_foreach_itr,
	[MOONY_UPCLOSURE_SEQUENCE_FOREACH] = _latom_seq_foreach_itr
};

static inline void
_pushupclosure(lua_State *L, moony_t *moony, moony_upclosure_t type)
{
	assert( (type >= MOONY_UPCLOSURE_TUPLE_FOREACH) && (type < MOONY_UPCLOSURE_COUNT) );

	int *upc = &moony->upc[type];
	void *data = NULL;

	lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_COUNT + type); // ref
	if(lua_rawgeti(L, -1, *upc) == LUA_TNIL) // no cached udata, create one!
	{
#if 0
		if(moony->log)
			lv2_log_trace(&moony->logger, "_pushupclosure:\n");
#endif
		lua_pop(L, 1); // nil

		lua_pushlightuserdata(L, moony);
		_latom_new(L, NULL); // place-holder
		lua_pushcclosure(L, upclosures[type], 2);

		lua_pushvalue(L, -1);
		lua_rawseti(L, -3, *upc); // store in cache
	}
	lua_remove(L, -2); // ref
	*upc += 1;
}

int
_latom_clone(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	
	latom_t *litem = lua_newuserdata(L, sizeof(latom_t) + lv2_atom_total_size(latom->atom));
	litem->type = MOONY_UDATA_ATOM;
	litem->atom = (const LV2_Atom *)litem->payload;
	litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);

	litem->payload->size = latom->atom->size;
	litem->payload->type = latom->atom->type;
	memcpy(LV2_ATOM_BODY(litem->payload), latom->body.raw, latom->atom->size);

	luaL_getmetatable(L, "latom");
	lua_setmetatable(L, -2);

	return 1;
}

static int
_latom__gc(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	if(latom->type == MOONY_UDATA_STASH)
		return _lstash__gc(L);

	return 0;
}

// Nil driver
static int
_latom_nil__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_nil__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(nil: %p)", latom);
	return 1;
}

static inline int
_latom_nil_value(lua_State *L, latom_t *latom)
{
	lua_pushnil(L);
	return 1;
}

const latom_driver_t latom_nil_driver = {
	.__len = _latom_nil__len,
	.__tostring = _latom_nil__tostring,
	.value = _latom_nil_value
};

// Int driver
static int
_latom_int__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_int__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(int: %p, %"PRIi32")", latom, *latom->body.i32);
	return 1;
}

static inline int
_latom_int_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, *latom->body.i32);
	return 1;
}

const latom_driver_t latom_int_driver = {
	.__len = _latom_int__len,
	.__tostring = _latom_int__tostring,
	.value = _latom_int_value
};

// Long driver
static int
_latom_long__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_long__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(long: %p, %"PRIi64")", latom, *latom->body.i64);
	return 1;
}

static inline int
_latom_long_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, *latom->body.i64);
	return 1;
}

const latom_driver_t latom_long_driver = {
	.__len = _latom_long__len,
	.__tostring = _latom_long__tostring,
	.value = _latom_long_value
};

// Float driver
static int
_latom_float__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_float__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(float: %p, %f)", latom, *latom->body.f32);
	return 1;
}

static inline int
_latom_float_value(lua_State *L, latom_t *latom)
{
	lua_pushnumber(L, *latom->body.f32);
	return 1;
}

const latom_driver_t latom_float_driver = {
	.__len = _latom_float__len,
	.__tostring = _latom_float__tostring,
	.value = _latom_float_value
};

// Double driver
static int
_latom_double__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_double__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(double: %p, %lf)", latom, *latom->body.f64);
	return 1;
}

static inline int
_latom_double_value(lua_State *L, latom_t *latom)
{
	lua_pushnumber(L, *latom->body.f64);
	return 1;
}

const latom_driver_t latom_double_driver = {
	.__len = _latom_double__len,
	.__tostring = _latom_double__tostring,
	.value = _latom_double_value
};

// Bool driver
static int
_latom_bool__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_bool__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(bool: %p, %s)", latom, *latom->body.i32 ? "true" : "false");
	return 1;
}

static inline int
_latom_bool_value(lua_State *L, latom_t *latom)
{
	lua_pushboolean(L, *latom->body.i32);
	return 1;
}

const latom_driver_t latom_bool_driver = {
	.__len = _latom_bool__len,
	.__tostring = _latom_bool__tostring,
	.value = _latom_bool_value
};

// URID driver
static int
_latom_urid__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_urid__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(URID: %p, %"PRIu32")", latom, *latom->body.u32);
	return 1;
}

static inline int
_latom_urid_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, *latom->body.u32);
	return 1;
}

const latom_driver_t latom_urid_driver = {
	.__len = _latom_urid__len,
	.__tostring = _latom_urid__tostring,
	.value = _latom_urid_value
};

// String driver
static int
_latom_string__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static inline int
_latom_string_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L, latom->body.str, latom->atom->size - 1);
	return 1;
}

static int
_latom_string__tostring(lua_State *L, latom_t *latom)
{
	//FIXME URI, Path
	lua_pushfstring(L, "(string: %p, %s)", latom, latom->body.str);
	return 1;
}

const latom_driver_t latom_string_driver = {
	.__len = _latom_string__len,
	.__tostring = _latom_string__tostring,
	.value = _latom_string_value
};

// Literal driver
static int
_latom_literal__indexk(lua_State *L, latom_t *latom)
{
	const char *key = lua_tostring(L, 2);
	if(!strcmp(key, "datatype"))
		lua_pushinteger(L, latom->body.lit->datatype);
	else if(!strcmp(key, "lang"))
		lua_pushinteger(L, latom->body.lit->lang);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_literal_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L,
		LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal_Body, latom->body.lit),
		latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	return 1;
}

static int
_latom_literal__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(literal: %p, %s)", latom, 
		LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal_Body, latom->body.lit));
	return 1;
}

int
_latom_literal_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	lua_pushlstring(L,
		LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal_Body, latom->body.lit),
		latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	lua_pushinteger(L, latom->body.lit->datatype);
	lua_pushinteger(L, latom->body.lit->lang);
	return 1;
}

const latom_driver_t latom_literal_driver = {
	.__indexk = _latom_literal__indexk,
	.__len = _latom_string__len,
	.__tostring = _latom_literal__tostring,
	.value = _latom_literal_value,
	.unpack = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_LIT_UNPACK
};

static int
_latom_tuple__indexi(lua_State *L, latom_t *latom)
{
	const int idx = lua_tointeger(L, 2);

	int count = 0;
	LV2_ATOM_TUPLE_BODY_FOREACH(latom->body.tuple, latom->atom->size, atom)
	{
		if(++count == idx)
		{
			_latom_new(L, atom);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

static int
_latom_tuple__len(lua_State *L, latom_t *latom)
{
	int count = 0;
	LV2_ATOM_TUPLE_BODY_FOREACH(latom->body.tuple, latom->atom->size, atom)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_tuple__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(tuple: %p)", latom);
	return 1;
}

int
_latom_tuple_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	int n = lua_gettop(L);
	int min = n > 1
		? luaL_checkinteger(L, 2)
		: 1;
	int max = n > 2
		? luaL_checkinteger(L, 3)
		: INT_MAX;

	int pos = 1;
	int count = 0;
	LV2_ATOM_TUPLE_BODY_FOREACH(latom->body.tuple, latom->atom->size, atom)
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

int
_latom_tuple_foreach_itr(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	if(!lv2_atom_tuple_is_end(latom->body.tuple, latom->atom->size, latom->iter.tuple.item))
	{
		// push index
		lua_pushinteger(L, latom->iter.tuple.pos);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));
		litem->atom = latom->iter.tuple.item;
		litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);
	
		// advance iterator
		latom->iter.tuple.pos += 1;
		latom->iter.tuple.item = lv2_atom_tuple_next(latom->iter.tuple.item);

		return 2;
	}

	// end of tuple reached
	lua_pushnil(L);
	return 1;
}

int
_latom_tuple_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of tuple
	latom->iter.tuple.pos = 1;
	latom->iter.tuple.item = latom->body.tuple;

	_pushupclosure(L, moony, MOONY_UPCLOSURE_TUPLE_FOREACH);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_tuple_driver = {
	.__indexi = _latom_tuple__indexi,
	.__len = _latom_tuple__len,
	.__tostring = _latom_tuple__tostring,
	.unpack = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TUPLE_UNPACK,
	.foreach = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_TUPLE_FOREACH
};

static int
_latom_obj__indexi(lua_State *L, latom_t *latom)
{
	const LV2_URID urid = lua_tointeger(L, 2);

	const LV2_Atom *atom = NULL;
	lv2_atom_object_body_get(latom->atom->size, latom->body.obj, urid, &atom, 0);

	if(atom) // query returned a matching atom
		_latom_new(L, atom);
	else // query returned no matching atom
		lua_pushnil(L);

	return 1;
}

static int
_latom_obj__indexk(lua_State *L, latom_t *latom)
{
	const char *key = lua_tostring(L, 2);
	if(!strcmp(key, "id"))
		lua_pushinteger(L, latom->body.obj->id);
	else if(!strcmp(key, "otype"))
		lua_pushinteger(L, latom->body.obj->otype);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_obj__len(lua_State *L, latom_t *latom)
{
	int count = 0;
	LV2_ATOM_OBJECT_BODY_FOREACH(latom->body.obj, latom->atom->size, prop)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_obj__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(object: %p)", latom);
	return 1;
}

int
_latom_obj_foreach_itr(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	if(!lv2_atom_object_is_end(latom->body.obj, latom->atom->size, latom->iter.obj.prop))
	{
		// push key/context
		lua_pushinteger(L, latom->iter.obj.prop->key);
		lua_pushinteger(L, latom->iter.obj.prop->context);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));
		litem->atom = &latom->iter.obj.prop->value;
		litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);
	
		// advance iterator
		latom->iter.obj.prop = lv2_atom_object_next(latom->iter.obj.prop);

		return 3;
	}

	// end of object reached
	lua_pushnil(L);
	return 1;
}

int
_latom_obj_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of object
	latom->iter.obj.prop = lv2_atom_object_begin(latom->body.obj);

	_pushupclosure(L, moony, MOONY_UPCLOSURE_OBJECT_FOREACH);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_object_driver = {
	.__indexi = _latom_obj__indexi,
	.__indexk = _latom_obj__indexk,
	.__len = _latom_obj__len,
	.__tostring = _latom_obj__tostring,
	.foreach = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_OBJECT_FOREACH
};

static int
_latom_seq__indexi(lua_State *L, latom_t *latom)
{
	int index = lua_tointeger(L, 2); // indexing start from 1

	int count = 0;
	LV2_ATOM_SEQUENCE_BODY_FOREACH(latom->body.seq, latom->atom->size, ev)
	{
		if(++count == index) 
		{
			_latom_new(L, &ev->body);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

static int
_latom_seq__len(lua_State *L, latom_t *latom)
{
	int count = 0;
	LV2_ATOM_SEQUENCE_BODY_FOREACH(latom->body.seq, latom->atom->size, ev)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_seq__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(sequence: %p)", latom);
	return 1;
}

int
_latom_seq_foreach_itr(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	if(!lv2_atom_sequence_is_end(latom->body.seq, latom->atom->size, latom->iter.seq.ev))
	{
		// push frame time
		lua_pushinteger(L, latom->iter.seq.ev->time.frames);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));
		litem->atom = &latom->iter.seq.ev->body;
		litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);
	
		// advance iterator
		latom->iter.seq.ev = lv2_atom_sequence_next(latom->iter.seq.ev);

		return 2;
	}

	// end of sequence reached
	lua_pushnil(L);
	return 1;
}

int
_latom_seq_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of sequence
	latom->iter.seq.ev = lv2_atom_sequence_begin(latom->body.seq);

	_pushupclosure(L, moony, MOONY_UPCLOSURE_SEQUENCE_FOREACH);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_sequence_driver = {
	.__indexi = _latom_seq__indexi,
	.__len = _latom_seq__len,
	.__tostring = _latom_seq__tostring,
	.foreach = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_SEQUENCE_FOREACH
};

static int
_latom_vec__indexi(lua_State *L, latom_t *latom)
{
	int index = lua_tointeger(L, 2); // indexing start from 1

	const int count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;

	if( (index > 0) && (index <= count) )
	{
		latom_t *litem = _latom_new(L, NULL);
		litem->atom = (const LV2_Atom *)latom->body.vec;
		litem->body.raw = LV2_ATOM_VECTOR_BODY_ITEM_CONST(latom->body.vec, index - 1);
	}
	else // index is out of bounds
		lua_pushnil(L);

	return 1;
}

static int
_latom_vec__indexk(lua_State *L, latom_t *latom)
{
	const char *key = lua_tostring(L, 2);
	if(!strcmp(key, "child_type"))
		lua_pushinteger(L, latom->body.vec->child_type);
	else if(!strcmp(key, "child_size"))
		lua_pushinteger(L, latom->body.vec->child_size);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_vec__len(lua_State *L, latom_t *latom)
{
	const int count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_vec__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(vector: %p)", latom);
	return 1;
}

int
_latom_vec_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	const int count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;

	int n = lua_gettop(L);
	int min = 1;
	int max = count;

	if(n > 1) // check provided index ranges
	{
		min = luaL_checkinteger(L, 2);
		min = min < 1
			? 1
			: (min > count
				? count
				: min);

		if(n > 2)
		{
			max = luaL_checkinteger(L, 3);
			max = max < 1
				? 1
				: (max > count
					? count
					: max);
		}
	}

	for(int i=min; i<=max; i++)
	{
		latom_t *litem = _latom_new(L, NULL);
		litem->atom = (const LV2_Atom *)latom->body.vec;
		litem->body.raw = LV2_ATOM_VECTOR_BODY_ITEM_CONST(latom->body.vec, i - 1);
	}

	return max - min + 1;
}

int
_latom_vec_foreach_itr(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	if(latom->iter.vec.pos < latom->iter.vec.count)
	{
		// push index
		lua_pushinteger(L, latom->iter.vec.pos + 1);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));
		litem->atom = (const LV2_Atom *)latom->body.vec;
		litem->body.raw = LV2_ATOM_VECTOR_BODY_ITEM_CONST(latom->body.vec, latom->iter.vec.pos);

		// advance iterator
		latom->iter.vec.pos += 1;

		return 2;
	}

	// end of vector reached
	lua_pushnil(L);
	return 1;
}

int
_latom_vec_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of vector
	latom->iter.vec.count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;
	latom->iter.vec.pos = 0;

	_pushupclosure(L, moony, MOONY_UPCLOSURE_VECTOR_FOREACH);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_vector_driver = {
	.__indexi = _latom_vec__indexi,
	.__indexk = _latom_vec__indexk,
	.__len = _latom_vec__len,
	.__tostring = _latom_vec__tostring,
	.unpack = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_VECTOR_UNPACK,
	.foreach = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_VECTOR_FOREACH
};

static int
_latom_chunk__indexi(lua_State *L, latom_t *latom)
{
	const uint8_t *payload = latom->body.raw;
	int index = lua_tointeger(L, 2); // indexing start from 1

	if( (index > 0) && (index <= (int)latom->atom->size) )
		lua_pushinteger(L, payload[index-1]);
	else // index is out of bounds
		lua_pushnil(L);

	return 1;
}

static int
_latom_chunk__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_chunk__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(chunk: %p)", latom);
	return 1;
}

static int
_latom_chunk_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L, latom->body.raw, latom->atom->size);

	return 1;
}

int
_latom_chunk_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	const uint8_t *payload = latom->body.raw;

	int n = lua_gettop(L);
	int min = 1;
	int max = latom->atom->size;

	if(n > 1) // check provided index ranges
	{
		min = luaL_checkinteger(L, 2);
		min = min < 1
			? 1
			: (min > (int)latom->atom->size
				? (int)latom->atom->size
				: min);

		if(n > 2)
		{
			max = luaL_checkinteger(L, 3);
			max = max < 1
				? 1
				: (max > (int)latom->atom->size
					? (int)latom->atom->size
					: max);
		}
	}

	for(int i=min; i<=max; i++)
		lua_pushinteger(L, payload[i-1]);

	return max - min + 1;
}

const latom_driver_t latom_chunk_driver = {
	.__indexi = _latom_chunk__indexi,
	.__len = _latom_chunk__len,
	.__tostring = _latom_chunk__tostring,
	.value = _latom_chunk_value,
	.unpack = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_CHUNK_UNPACK,
};

static int
_latom__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver)
	{
		const int type = lua_type(L, 2);
		if(type == LUA_TSTRING)
		{
			const char *key = lua_tostring(L, 2);
			if(!strcmp(key, "type"))
			{
				lua_pushinteger(L, latom->atom->type);
				return 1;
			}
			else if(driver->value && !strcmp(key, "value"))
			{
				return driver->value(L, latom);
			}
			else if(driver->foreach && !strcmp(key, "foreach"))
			{
				lua_rawgeti(L, LUA_REGISTRYINDEX, driver->foreach);
				return 1;
			}
			else if(driver->unpack && !strcmp(key, "unpack"))
			{
				lua_rawgeti(L, LUA_REGISTRYINDEX, driver->unpack);
				return 1;
			}
			else if(!strcmp(key, "clone"))
			{
				lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_CLONE);
				return 1;
			}
			else if(!strcmp(key, "write") && (latom->type == MOONY_UDATA_STASH) )
			{
				lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_WRITE);
				return 1;
			}
			else if(driver->__indexk)
			{
				return driver->__indexk(L, latom);
			}
		}
		else if(driver->__indexi && (type == LUA_TNUMBER) )
		{
			return driver->__indexi(L, latom);
		}
	}

	lua_pushnil(L);
	return 1;
}

static int
_latom__len(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver && driver->__len)
		return driver->__len(L, latom);

	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom__tostring(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver && driver->__tostring)
		return driver->__tostring(L, latom);

	lua_pushnil(L);
	return 1;
}

static int
_latom__eq(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom1 = luaL_checkudata(L, 1, "latom");
	latom_t *latom2 = luaL_checkudata(L, 2, "latom");

	lua_pushboolean(L,
		(latom1->atom->type == latom2->atom->type)
		&& (latom1->atom->size == latom2->atom->size)
		&& (memcmp(latom1->body.raw, latom2->body.raw, latom1->atom->size) == 0) );

	return 1;
}

const luaL_Reg latom_mt [] = {
	{"__index", _latom__index},
	{"__len", _latom__len},
	{"__tostring", _latom__tostring},
	{"__eq", _latom__eq},
	{"__gc", _latom__gc},

	{NULL, NULL}
};
