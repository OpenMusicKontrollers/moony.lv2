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

static inline void
_latom_body_new(lua_State *L, uint32_t size, LV2_URID type, const void *body)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	size_t atom_size = sizeof(LV2_Atom) + size;

	latom_t *latom = lua_newuserdata(L, sizeof(latom_t) + atom_size);
	latom->atom = (const LV2_Atom *)latom->body;
	latom->body->size = size;
	latom->body->type = type;
	memcpy(LV2_ATOM_BODY(latom->body), body, size);

	luaL_getmetatable(L, "latom");
	lua_setmetatable(L, -2);
}

int
_latom_clone(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	const LV2_Atom *atom = latom->atom;

	_latom_body_new(L, atom->size, atom->type, LV2_ATOM_BODY_CONST(atom));

	return 1;
}

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
	lua_pushfstring(L, "%i", latom->i32->body);
	return 1;
}

static inline int
_latom_int_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->i32->body);
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
	lua_pushfstring(L, "%li", latom->i64->body);
	return 1;
}

static inline int
_latom_long_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->i64->body);
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
	lua_pushfstring(L, "%f", latom->f32->body);
	return 1;
}

static inline int
_latom_float_value(lua_State *L, latom_t *latom)
{
	lua_pushnumber(L, latom->f32->body);
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
	lua_pushfstring(L, "%lf", latom->f64->body);
	return 1;
}

static inline int
_latom_double_value(lua_State *L, latom_t *latom)
{
	lua_pushnumber(L, latom->f64->body);
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
	lua_pushfstring(L, "%s", latom->i32->body ? "true" : "false");
	return 1;
}

static inline int
_latom_bool_value(lua_State *L, latom_t *latom)
{
	lua_pushboolean(L, latom->i32->body);
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
	lua_pushfstring(L, "%u", latom->u32->body);
	return 1;
}

static inline int
_latom_urid_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->u32->body);
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
	lua_pushlstring(L, LV2_ATOM_BODY_CONST(latom->atom), latom->atom->size - 1);
	return 1;
}

const latom_driver_t latom_string_driver = {
	.__len = _latom_string__len,
	.__tostring = _latom_string_value,
	.value = _latom_string_value
};

// Literal driver
static int
_latom_literal__indexk(lua_State *L, latom_t *latom)
{
	const char *key = lua_tostring(L, 2);
	if(!strcmp(key, "datatype"))
		lua_pushinteger(L, latom->lit->body.datatype);
	else if(!strcmp(key, "lang"))
		lua_pushinteger(L, latom->lit->body.lang);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_literal_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, latom->atom), latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	return 1;
}

int
_latom_literal_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	lua_pushlstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, latom->atom), latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	lua_pushinteger(L, latom->lit->body.datatype);
	lua_pushinteger(L, latom->lit->body.lang);
	return 1;
}

const latom_driver_t latom_literal_driver = {
	.__indexk = _latom_literal__indexk,
	.__len = _latom_string__len,
	.__tostring = _latom_literal_value,
	.value = _latom_literal_value,
	.unpack = UDATA_OFFSET + MOONY_UDATA_COUNT + MOONY_CCLOSURE_LIT_UNPACK
};

static int
_latom_tuple__indexi(lua_State *L, latom_t *latom)
{
	ltuple_t *ltuple = &latom->tuple;
	const int idx = lua_tointeger(L, 2);

	int count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
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
	ltuple_t *ltuple = &latom->tuple;

	int count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_tuple__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(tuple)");
	return 1;
}

int
_latom_tuple_unpack(lua_State *L)
{
	ltuple_t *ltuple = lua_touserdata(L, 1);

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

int
_latom_tuple_foreach_itr(lua_State *L)
{
	ltuple_t *ltuple = lua_touserdata(L, 1);

	if(!lv2_atom_tuple_is_end(LV2_ATOM_BODY(ltuple->tuple), ltuple->tuple->atom.size, ltuple->itr))
	{
		// push index
		lua_pushinteger(L, ltuple->pos);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		latom_t *latom = lua_touserdata(L, lua_upvalueindex(2));
		latom->atom = ltuple->itr;
	
		// advance iterator
		ltuple->pos += 1;
		ltuple->itr = lv2_atom_tuple_next(ltuple->itr);

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
	ltuple_t *ltuple = lua_touserdata(L, 1);

	// reset iterator to beginning of tuple
	ltuple->pos = 1;
	ltuple->itr = lv2_atom_tuple_begin(ltuple->tuple);

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
	lobj_t *lobj = &latom->obj;
	const LV2_URID urid = lua_tointeger(L, 2);

	// start a query for given URID
	const LV2_Atom *atom = NULL;
	LV2_Atom_Object_Query q [] = {
		{ urid, &atom },
		{	0, NULL }
	};
	lv2_atom_object_query(lobj->obj, q);

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
		lua_pushinteger(L, latom->obj.obj->body.id);
	else if(!strcmp(key, "otype"))
		lua_pushinteger(L, latom->obj.obj->body.otype);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_obj__len(lua_State *L, latom_t *latom)
{
	lobj_t *lobj = &latom->obj;

	int count = 0;
	LV2_ATOM_OBJECT_FOREACH(lobj->obj, prop)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_obj__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(object)");
	return 1;
}

int
_latom_obj_foreach_itr(lua_State *L)
{
	lobj_t *lobj = lua_touserdata(L, 1);

	if(!lv2_atom_object_is_end(&lobj->obj->body, lobj->obj->atom.size, lobj->itr))
	{
		// push key/context
		lua_pushinteger(L, lobj->itr->key);
		lua_pushinteger(L, lobj->itr->context);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		latom_t *latom = lua_touserdata(L, lua_upvalueindex(2));
		latom->atom = &lobj->itr->value;
	
		// advance iterator
		lobj->itr = lv2_atom_object_next(lobj->itr);

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
	lobj_t *lobj = lua_touserdata(L, 1);

	// reset iterator to beginning of object
	lobj->itr = lv2_atom_object_begin(&lobj->obj->body);

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
	lseq_t *lseq = lua_touserdata(L, 1);
	int index = lua_tointeger(L, 2); // indexing start from 1

	int count = 0;
	LV2_ATOM_SEQUENCE_FOREACH(lseq->seq, ev)
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
	lseq_t *lseq = lua_touserdata(L, 1);

	int count = 0;
	LV2_ATOM_SEQUENCE_FOREACH(lseq->seq, ev)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_seq__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(sequence)");
	return 1;
}

int
_latom_seq_foreach_itr(lua_State *L)
{
	lseq_t *lseq = lua_touserdata(L, 1);

	if(!lv2_atom_sequence_is_end(&lseq->seq->body, lseq->seq->atom.size, lseq->itr))
	{
		// push frame time
		lua_pushinteger(L, lseq->itr->time.frames);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		latom_t *latom = lua_touserdata(L, lua_upvalueindex(2));
		latom->atom = &lseq->itr->body;
	
		// advance iterator
		lseq->itr = lv2_atom_sequence_next(lseq->itr);

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
	lseq_t *lseq = lua_touserdata(L, 1);

	// reset iterator to beginning of sequence
	lseq->itr = lv2_atom_sequence_begin(&lseq->seq->body);

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
	lvec_t *lvec = lua_touserdata(L, 1);
	int index = lua_tointeger(L, 2); // indexing start from 1

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

	if( (index > 0) && (index <= lvec->count) )
	{
		_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
			LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, index - 1));
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
		lua_pushinteger(L, latom->vec.vec->body.child_type);
	else if(!strcmp(key, "child_size"))
		lua_pushinteger(L, latom->vec.vec->body.child_size);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_vec__len(lua_State *L, latom_t *latom)
{
	lvec_t *lvec = lua_touserdata(L, 1);

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

	lua_pushinteger(L, lvec->count);
	return 1;
}

static int
_latom_vec__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(vector)");
	return 1;
}

int
_latom_vec_unpack(lua_State *L)
{
	lvec_t *lvec = lua_touserdata(L, 1);

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

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

int
_latom_vec_foreach_itr(lua_State *L)
{
	lvec_t *lvec = lua_touserdata(L, 1);

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

	if(lvec->pos < lvec->count)
	{
		// push index
		lua_pushinteger(L, lvec->pos + 1);
		// push atom TODO try to recycle
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

int
_latom_vec_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lvec_t *lvec = lua_touserdata(L, 1);

	// reset iterator to beginning of vector
	lvec->pos = 0;

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
	const uint8_t *payload = LV2_ATOM_BODY_CONST(latom->atom);
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
	lua_pushstring(L, "(chunk)");
	return 1;
}

static int
_latom_chunk_value(lua_State *L, latom_t *latom)
{
	const uint8_t *payload = LV2_ATOM_BODY_CONST(latom->atom);

	lua_newtable(L);
	for(unsigned i=0; i<latom->atom->size; i++)
	{
		lua_pushinteger(L, payload[i]);
		lua_rawseti(L, -2, i+1);
	}

	return 1;
}

int
_latom_chunk_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	const uint8_t *payload = LV2_ATOM_BODY_CONST(latom->atom);

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

	lua_pushnil(L);
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

const luaL_Reg latom_mt [] = {
	{"__index", _latom__index},
	{"__len", _latom__len},
	{"__tostring", _latom__tostring},

	{NULL, NULL}
};
