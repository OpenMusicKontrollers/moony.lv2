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
#include <math.h>

#define LV2_ATOM_VECTOR_BODY_ITEM_CONST(body, i) \
	(LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector_Body, (body)) + (i)*(body)->child_size)

const lua_CFunction upclosures [MOONY_UPCLOSURE_COUNT] = {
	[MOONY_UPCLOSURE_TUPLE_FOREACH] = _latom_tuple_foreach_itr,
	[MOONY_UPCLOSURE_VECTOR_FOREACH] = _latom_vec_foreach_itr,
	[MOONY_UPCLOSURE_OBJECT_FOREACH] = _latom_obj_foreach_itr,
	[MOONY_UPCLOSURE_SEQUENCE_FOREACH] = _latom_seq_foreach_itr,
	[MOONY_UPCLOSURE_SEQUENCE_MULTIPLEX] = _latom_seq_multiplex_itr
};

__realtime static inline void
_pushupclosure(lua_State *L, moony_t *moony, moony_upclosure_t type, bool cache)
{
	int *upc = &moony->upc[type];

	lua_rawgetp(L, LUA_REGISTRYINDEX, &upclosures[type]); // ref
	if(lua_rawgeti(L, -1, *upc) == LUA_TNIL) // no cached udata, create one!
	{
#if 0
		if(moony->log)
			lv2_log_trace(&moony->logger, "_pushupclosure:\n");
#endif
		lua_pop(L, 1); // nil

		lua_pushlightuserdata(L, moony);
		_latom_new(L, NULL, false); // place-holder
		lua_pushcclosure(L, upclosures[type], 2);

		lua_pushvalue(L, -1);
		lua_rawseti(L, -3, *upc); // store in cache
	}
	lua_remove(L, -2); // ref
	*upc += 1;
}

__realtime int
_latom_clone(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	latom_t *litem = lua_newuserdata(L, sizeof(latom_t) + lv2_atom_total_size(latom->atom));
	litem->lheader.type = MOONY_UDATA_ATOM;
	litem->lheader.cache = false;
	litem->atom = (const LV2_Atom *)litem->payload;
	litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);

	litem->payload->size = latom->atom->size;
	litem->payload->type = latom->atom->type;
	memcpy(LV2_ATOM_BODY(litem->payload), latom->body.raw, latom->atom->size);

	luaL_getmetatable(L, "latom");
	lua_setmetatable(L, -2);

	return 1;
}

__realtime static int
_latom__gc(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	if(latom->lheader.type == MOONY_UDATA_STASH)
		return _lstash__gc(L);

	return 0;
}

// Nil driver
__realtime static int
_latom_nil__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom_nil__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(nil: %p)", latom);
	return 1;
}

__realtime static inline int
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
__realtime static int
_latom_int__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom_int__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(int: %p, %d)", latom, *latom->body.i32);
	return 1;
}

__realtime static inline int
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
__realtime static int
_latom_long__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
__realtime _latom_long__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(long: %p, %d)", latom, *latom->body.i64);
	return 1;
}

__realtime static inline int
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
__realtime static int
_latom_float__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom_float__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(float: %p, %f)", latom, *latom->body.f32);
	return 1;
}

__realtime static inline int
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
__realtime static int
_latom_double__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom_double__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(double: %p, %f)", latom, *latom->body.f64);
	return 1;
}

__realtime static inline int
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
__realtime static int
_latom_bool__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom_bool__tostring(lua_State *L, latom_t *latom)
{
	if(*latom->body.i32 == 0)
		lua_pushfstring(L, "(bool: %p, false)", latom);
	else
		lua_pushfstring(L, "(bool: %p, true)", latom);
	return 1;
}

__realtime static inline int
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
__realtime static int
_latom_urid__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom_urid__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(URID: %p, %d)", latom, *latom->body.u32);
	return 1;
}

__realtime static inline int
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
__realtime static int
_latom_string__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static inline int
_latom_string_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L, latom->body.str, latom->atom->size - 1);
	return 1;
}

__realtime static int
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
__realtime static int
_latom_literal__indexk(lua_State *L, latom_t *latom, const char *key)
{
	if(!strcmp(key, "datatype"))
		lua_pushinteger(L, latom->body.lit->datatype);
	else if(!strcmp(key, "lang"))
		lua_pushinteger(L, latom->body.lit->lang);
	else
		lua_pushnil(L);
	return 1;
}

__realtime static int
_latom_literal_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L,
		LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal_Body, latom->body.lit),
		latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	return 1;
}

__realtime static int
_latom_literal__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(literal: %p, %s)", latom,
		LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal_Body, latom->body.lit));
	return 1;
}

__realtime int
_latom_literal_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	lua_pushlstring(L,
		LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal_Body, latom->body.lit),
		latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	lua_pushinteger(L, latom->body.lit->datatype);
	lua_pushinteger(L, latom->body.lit->lang);
	return 3;
}

const latom_driver_t latom_literal_driver = {
	.__indexk = _latom_literal__indexk,
	.__len = _latom_string__len,
	.__tostring = _latom_literal__tostring,
	.value = _latom_literal_value,
	.unpack = _latom_literal_unpack
};

__realtime static int
_latom_tuple__indexi(lua_State *L, latom_t *latom)
{
	const int idx = lua_tointeger(L, 2);

	int count = 0;
	LV2_ATOM_TUPLE_BODY_FOREACH(latom->body.tuple, latom->atom->size, atom)
	{
		if(++count == idx)
		{
			_latom_new(L, atom, latom->lheader.cache);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

__realtime static int
_latom_tuple__len(lua_State *L, latom_t *latom)
{
	int count = 0;
	LV2_ATOM_TUPLE_BODY_FOREACH(latom->body.tuple, latom->atom->size, atom)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

__realtime static int
_latom_tuple__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(tuple: %p)", latom);
	return 1;
}

__realtime int
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
				_latom_new(L, atom, latom->lheader.cache);
				count += 1;
			}
			else
				break;
		}

		pos += 1;
	}

	return count;
}

static const LV2_Atom nil_atom = {
	.size = 0,
	.type = 0
};

__realtime static void
_latom_clear(latom_t *litem)
{
	litem->atom = &nil_atom;
	litem->body.raw = NULL;
}

__realtime int
_latom_tuple_foreach_itr(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));

	if(!lv2_atom_tuple_is_end(latom->body.tuple, latom->atom->size, latom->iter.tuple.item))
	{
		// push index
		lua_pushinteger(L, latom->iter.tuple.pos);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		litem->atom = latom->iter.tuple.item;
		litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);

		// advance iterator
		latom->iter.tuple.pos += 1;
		latom->iter.tuple.item = lv2_atom_tuple_next(latom->iter.tuple.item);

		return 2;
	}

	// end of tuple reached
	_latom_clear(litem);
	lua_pushnil(L);
	return 1;
}

__realtime int
_latom_tuple_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of tuple
	latom->iter.tuple.pos = 1;
	latom->iter.tuple.item = latom->body.tuple;

	_pushupclosure(L, moony, MOONY_UPCLOSURE_TUPLE_FOREACH, latom->lheader.cache);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_tuple_driver = {
	.__indexi = _latom_tuple__indexi,
	.__len = _latom_tuple__len,
	.__tostring = _latom_tuple__tostring,
	.unpack = _latom_tuple_unpack,
	.foreach = _latom_tuple_foreach
};

__realtime static int
_latom_obj__indexi(lua_State *L, latom_t *latom)
{
	const LV2_URID urid = lua_tointeger(L, 2);

	const LV2_Atom *atom = NULL;
	lv2_atom_object_body_get(latom->atom->size, latom->body.obj, urid, &atom, 0);

	if(atom) // query returned a matching atom
		_latom_new(L, atom, latom->lheader.cache);
	else // query returned no matching atom
		lua_pushnil(L);

	return 1;
}

__realtime static int
_latom_obj__indexk(lua_State *L, latom_t *latom, const char *key)
{
	if(!strcmp(key, "id"))
		lua_pushinteger(L, latom->body.obj->id);
	else if(!strcmp(key, "otype"))
		lua_pushinteger(L, latom->body.obj->otype);
	else
		lua_pushnil(L);
	return 1;
}

__realtime static int
_latom_obj__len(lua_State *L, latom_t *latom)
{
	int count = 0;
	LV2_ATOM_OBJECT_BODY_FOREACH(latom->body.obj, latom->atom->size, prop)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

__realtime static int
_latom_obj__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(object: %p)", latom);
	return 1;
}

__realtime int
_latom_obj_foreach_itr(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));

	if(!lv2_atom_object_is_end(latom->body.obj, latom->atom->size, latom->iter.obj.prop))
	{
		// push key
		lua_pushinteger(L, latom->iter.obj.prop->key);
		// push atom

		lua_pushvalue(L, lua_upvalueindex(2));
		litem->atom = &latom->iter.obj.prop->value;
		litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);

		// push context
		lua_pushinteger(L, latom->iter.obj.prop->context);

		// advance iterator
		latom->iter.obj.prop = lv2_atom_object_next(latom->iter.obj.prop);

		return 3;
	}

	// end of object reached
	_latom_clear(litem);
	lua_pushnil(L);
	return 1;
}

__realtime int
_latom_obj_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of object
	latom->iter.obj.prop = lv2_atom_object_begin(latom->body.obj);

	_pushupclosure(L, moony, MOONY_UPCLOSURE_OBJECT_FOREACH, latom->lheader.cache);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_object_driver = {
	.__indexi = _latom_obj__indexi,
	.__indexk = _latom_obj__indexk,
	.__len = _latom_obj__len,
	.__tostring = _latom_obj__tostring,
	.foreach = _latom_obj_foreach
};

__realtime static int
_latom_seq__indexk(lua_State *L, latom_t *latom, const char *key)
{
	if(!strcmp(key, "unit"))
		lua_pushinteger(L, latom->body.seq->unit);
	else
		lua_pushnil(L);
	return 1;
}

__realtime static int
_latom_seq__indexi(lua_State *L, latom_t *latom)
{
	int index = lua_tointeger(L, 2); // indexing start from 1

	int count = 0;
	LV2_ATOM_SEQUENCE_BODY_FOREACH(latom->body.seq, latom->atom->size, ev)
	{
		if(++count == index)
		{
			_latom_new(L, &ev->body, latom->lheader.cache);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

__realtime static int
_latom_seq__len(lua_State *L, latom_t *latom)
{
	int count = 0;
	LV2_ATOM_SEQUENCE_BODY_FOREACH(latom->body.seq, latom->atom->size, ev)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

__realtime static int
_latom_seq__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(sequence: %p)", latom);
	return 1;
}

__realtime int
_latom_seq_multiplex_itr(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	const unsigned n = lua_rawlen(L, 1);
	latom_t *latom [n];
	latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));

	// fill latom* array
	for(unsigned i=0; i<n; i++)
	{
		lua_rawgeti(L, 1, 1+i);
		latom[i] = lua_touserdata(L, -1);
	}
	lua_pop(L, n);

	double huge = HUGE_VAL;
	int nxt = -1;
	for(unsigned i=0; i<n; i++)
	{
		if(lv2_atom_sequence_is_end(latom[i]->body.seq, latom[i]->atom->size, latom[i]->iter.seq.ev))
			continue;

		if(latom[i]->body.seq->unit == moony->uris.atom_beat_time)
		{
			if(latom[i]->iter.seq.ev->time.beats < huge)
			{
				huge = latom[i]->iter.seq.ev->time.beats;
				nxt = i;
			}
		}
		else
		{
			if(latom[i]->iter.seq.ev->time.frames < huge)
			{
				huge = latom[i]->iter.seq.ev->time.frames;
				nxt = i;
			}
		}
	}

	if(nxt >= 0) // is there a valid next event?
	{
		if(latom[nxt]->body.seq->unit == moony->uris.atom_beat_time)
			lua_pushnumber(L, latom[nxt]->iter.seq.ev->time.beats);
		else
			lua_pushinteger(L, latom[nxt]->iter.seq.ev->time.frames);

		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		litem->atom = &latom[nxt]->iter.seq.ev->body;
		litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);
		lua_rawgeti(L, 1, 1+nxt);

		// advance iterator
		latom[nxt]->iter.seq.ev = lv2_atom_sequence_next(latom[nxt]->iter.seq.ev);

		return 3;
	}

	// end of sequence reached
	_latom_clear(litem);
	lua_pushnil(L);
	return 1;
}

__realtime static int
_latom_seq_multiplex(lua_State *L, unsigned n)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	for(unsigned i=1; i<=n; i++)
	{
		latom_t *lmux = lua_touserdata(L, i);

		// reset iterator to beginning of sequence
		lmux->iter.seq.ev = lv2_atom_sequence_begin(lmux->body.seq);
	}

	_pushupclosure(L, moony, MOONY_UPCLOSURE_SEQUENCE_MULTIPLEX, latom->lheader.cache);

	lua_createtable(L, n, 0); //FIXME cache this?
	for(unsigned i=1; i<=n; i++)
	{
		lua_pushvalue(L, i);
		lua_rawseti(L, -2, i);
	}

	return 2;
}

__realtime int
_latom_seq_foreach_itr(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);
	latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));

	if(!lv2_atom_sequence_is_end(latom->body.seq, latom->atom->size, latom->iter.seq.ev))
	{
		if(latom->body.seq->unit == moony->uris.atom_beat_time)
			lua_pushnumber(L, latom->iter.seq.ev->time.beats);
		else
			lua_pushinteger(L, latom->iter.seq.ev->time.frames);

		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		litem->atom = &latom->iter.seq.ev->body;
		litem->body.raw = LV2_ATOM_BODY_CONST(litem->atom);

		// advance iterator
		latom->iter.seq.ev = lv2_atom_sequence_next(latom->iter.seq.ev);

		return 2;
	}

	// end of sequence reached
	_latom_clear(litem);
	lua_pushnil(L);
	return 1;
}

__realtime int
_latom_seq_foreach(lua_State *L)
{
	const unsigned n = lua_gettop(L);
	if(n > 1) // multiplex if given any function arguments
		return _latom_seq_multiplex(L, n);

	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of sequence
	latom->iter.seq.ev = lv2_atom_sequence_begin(latom->body.seq);

	_pushupclosure(L, moony, MOONY_UPCLOSURE_SEQUENCE_FOREACH, latom->lheader.cache);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_sequence_driver = {
	.__indexk = _latom_seq__indexk,
	.__indexi = _latom_seq__indexi,
	.__len = _latom_seq__len,
	.__tostring = _latom_seq__tostring,
	.foreach = _latom_seq_foreach
};

__realtime static int
_latom_vec__indexi(lua_State *L, latom_t *latom)
{
	int index = lua_tointeger(L, 2); // indexing start from 1

	const int count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;

	if( (index > 0) && (index <= count) )
	{
		latom_t *litem = _latom_new(L, NULL, latom->lheader.cache);
		litem->atom = (const LV2_Atom *)latom->body.vec;
		litem->body.raw = LV2_ATOM_VECTOR_BODY_ITEM_CONST(latom->body.vec, index - 1);
	}
	else // index is out of bounds
		lua_pushnil(L);

	return 1;
}

__realtime static int
_latom_vec__indexk(lua_State *L, latom_t *latom, const char *key)
{
	if(!strcmp(key, "childType"))
		lua_pushinteger(L, latom->body.vec->child_type);
	else if(!strcmp(key, "childSize"))
		lua_pushinteger(L, latom->body.vec->child_size);
	else
		lua_pushnil(L);
	return 1;
}

__realtime static int
_latom_vec__len(lua_State *L, latom_t *latom)
{
	const int count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;

	lua_pushinteger(L, count);
	return 1;
}

__realtime static int
_latom_vec__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(vector: %p)", latom);
	return 1;
}

__realtime static int
_latom_vec_value(lua_State *L, latom_t *latom)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	const int n = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;
	const void *ptr = LV2_ATOM_VECTOR_BODY_ITEM_CONST(latom->body.vec, 0);

	lua_createtable(L, n, 0);

	if(latom->body.vec->child_type == moony->forge.Bool)
	{
		const int32_t *i32 = ptr;
		for(int i=0; i<n; i++)
		{
			lua_pushboolean(L, i32[i]);
			lua_rawseti(L, -2, i+1);
		}
	}
	else if(latom->body.vec->child_type == moony->forge.Int)
	{
		const int32_t *i32 = ptr;
		for(int i=0; i<n; i++)
		{
			lua_pushinteger(L, i32[i]);
			lua_rawseti(L, -2, i+1);
		}
	}
	else if(latom->body.vec->child_type == moony->forge.URID)
	{
		const uint32_t *u32 = ptr;
		for(int i=0; i<n; i++)
		{
			lua_pushinteger(L, u32[i]);
			lua_rawseti(L, -2, i+1);
		}
	}
	else if(latom->body.vec->child_type == moony->forge.Long)
	{
		const int64_t *i64 = ptr;
		for(int i=0; i<n; i++)
		{
			lua_pushinteger(L, i64[i]);
			lua_rawseti(L, -2, i+1);
		}
	}
	else if(latom->body.vec->child_type == moony->forge.Float)
	{
		const float *f32 = ptr;
		for(int i=0; i<n; i++)
		{
			lua_pushnumber(L, f32[i]);
			lua_rawseti(L, -2, i+1);
		}
	}
	else if(latom->body.vec->child_type == moony->forge.Double)
	{
		const double *f64 = ptr;
		for(int i=0; i<n; i++)
		{
			lua_pushnumber(L, f64[i]);
			lua_rawseti(L, -2, i+1);
		}
	}

	return 1;
}

__realtime int
_latom_vec_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);

	const int count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;

	const int n = lua_gettop(L);
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
		latom_t *litem = _latom_new(L, NULL, latom->lheader.cache);
		litem->atom = (const LV2_Atom *)latom->body.vec;
		litem->body.raw = LV2_ATOM_VECTOR_BODY_ITEM_CONST(latom->body.vec, i - 1);
	}

	return max - min + 1;
}

__realtime int
_latom_vec_foreach_itr(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	latom_t *litem = lua_touserdata(L, lua_upvalueindex(2));

	if(latom->iter.vec.pos < latom->iter.vec.count)
	{
		// push index
		lua_pushinteger(L, latom->iter.vec.pos + 1);
		// push atom
		lua_pushvalue(L, lua_upvalueindex(2));
		litem->atom = (const LV2_Atom *)latom->body.vec;
		litem->body.raw = LV2_ATOM_VECTOR_BODY_ITEM_CONST(latom->body.vec, latom->iter.vec.pos);

		// advance iterator
		latom->iter.vec.pos += 1;

		return 2;
	}

	// end of vector reached
	_latom_clear(litem);
	lua_pushnil(L);
	return 1;
}

__realtime int
_latom_vec_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);

	// reset iterator to beginning of vector
	latom->iter.vec.count = (latom->atom->size - sizeof(LV2_Atom_Vector_Body))
		/ latom->body.vec->child_size;
	latom->iter.vec.pos = 0;

	_pushupclosure(L, moony, MOONY_UPCLOSURE_VECTOR_FOREACH, latom->lheader.cache);
	lua_pushvalue(L, 1);

	return 2;
}

const latom_driver_t latom_vector_driver = {
	.__indexi = _latom_vec__indexi,
	.__indexk = _latom_vec__indexk,
	.__len = _latom_vec__len,
	.__tostring = _latom_vec__tostring,
	.value = _latom_vec_value,
	.unpack = _latom_vec_unpack,
	.foreach = _latom_vec_foreach
};

__realtime static int
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

__realtime static int
_latom_chunk__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom_chunk__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "(chunk: %p)", latom);
	return 1;
}

__realtime static int
_latom_chunk_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L, latom->body.raw, latom->atom->size);

	return 1;
}

__realtime int
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
	.unpack = _latom_chunk_unpack
};

__realtime static int
_latom__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);
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
			else if(driver->value && !strcmp(key, "body"))
			{
				return driver->value(L, latom);
			}
			else if(driver->foreach && !strcmp(key, "foreach"))
			{
				lua_rawgetp(L, LUA_REGISTRYINDEX, driver->foreach);
				return 1;
			}
			else if(driver->unpack && !strcmp(key, "unpack"))
			{
				lua_rawgetp(L, LUA_REGISTRYINDEX, driver->unpack);
				return 1;
			}
			else if(!strcmp(key, "clone"))
			{
				lua_rawgetp(L, LUA_REGISTRYINDEX, _latom_clone);
				return 1;
			}
			else if(!strcmp(key, "raw"))
			{
				lua_pushlstring(L, latom->body.raw, latom->atom->size);
				return 1;
			}
			else if( (latom->lheader.type == MOONY_UDATA_STASH) && !strcmp(key, "write") )
			{
				lua_rawgetp(L, LUA_REGISTRYINDEX, _lstash_write);
				return 1;
			}
			else if( (latom->lheader.type == MOONY_UDATA_STASH) && !strcmp(key, "read") )
			{
				lua_rawgetp(L, LUA_REGISTRYINDEX, _lstash_read);
				return 1;
			}
			else if(driver->__indexk)
			{
				return driver->__indexk(L, latom, key);
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

__realtime static int
_latom__len(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver && driver->__len)
		return driver->__len(L, latom);

	lua_pushinteger(L, latom->atom->size);
	return 1;
}

__realtime static int
_latom__tostring(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = lua_touserdata(L, 1);
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver && driver->__tostring)
		return driver->__tostring(L, latom);

	lua_pushnil(L);
	return 1;
}

__realtime static int
_latom__eq(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom1 = lua_touserdata(L, 1);
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
