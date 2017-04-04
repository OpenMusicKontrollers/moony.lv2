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

#ifndef _MOONY_API_ATOM_H
#define _MOONY_API_ATOM_H

#include <moony.h>

typedef struct _lseq_t lseq_t;
typedef struct _latom_t latom_t;
typedef struct _lobj_t lobj_t;
typedef struct _ltuple_t ltuple_t;
typedef struct _lvec_t lvec_t;

typedef int (*latom_driver_function_t)(lua_State *L, latom_t *latom);
typedef int (*latom_driver_function_indexk_t)(lua_State *L, latom_t *latom, const char *key);

struct _latom_driver_t {
	latom_driver_function_t __indexi;
	latom_driver_function_indexk_t __indexk;
	latom_driver_function_t __len;
	latom_driver_function_t __tostring;
	latom_driver_function_t __call;

	latom_driver_function_t value;
	int unpack;
	int foreach;
};

struct _latom_t {
	lheader_t lheader;

	const LV2_Atom *atom;

	union {
		const void *raw;

		const int32_t *i32;
		const int64_t *i64;
		const float *f32;
		const double *f64;
		const uint32_t *u32;
		const char *str;
		const LV2_Atom_Literal_Body *lit;

		const LV2_Atom_Sequence_Body *seq;
		const LV2_Atom_Object_Body *obj;
		const LV2_Atom *tuple;
		const LV2_Atom_Vector_Body *vec;
	} body;

	union {
		struct _lseq_t {
			const LV2_Atom_Event *ev;
		} seq;

		struct _lobj_t {
			const LV2_Atom_Property_Body *prop;
		} obj;

		struct _ltuple_t {
			int pos;
			const LV2_Atom *item;
		} tuple;

		struct _lvec_t {
			int count;
			int pos;
		} vec;
	} iter;

	LV2_Atom payload [0];
};

// in api_atom.c
extern const latom_driver_t latom_nil_driver;
extern const latom_driver_t latom_bool_driver;
extern const latom_driver_t latom_int_driver;
extern const latom_driver_t latom_long_driver;
extern const latom_driver_t latom_float_driver;
extern const latom_driver_t latom_double_driver;
extern const latom_driver_t latom_urid_driver;
extern const latom_driver_t latom_string_driver;
extern const latom_driver_t latom_literal_driver;
extern const latom_driver_t latom_tuple_driver;
extern const latom_driver_t latom_object_driver;
extern const latom_driver_t latom_vector_driver;
extern const latom_driver_t latom_sequence_driver;
extern const latom_driver_t latom_chunk_driver;

extern const luaL_Reg latom_mt [];

int
_latom_clone(lua_State *L);

int
_latom_literal_unpack(lua_State *L);
int
_latom_tuple_unpack(lua_State *L);
int
_latom_vec_unpack(lua_State *L);
int
_latom_chunk_unpack(lua_State *L);

int
_latom_tuple_foreach(lua_State *L);
int
_latom_obj_foreach(lua_State *L);
int
_latom_seq_foreach(lua_State *L);
int
_latom_vec_foreach(lua_State *L);

int
_latom_tuple_foreach_itr(lua_State *L);
int
_latom_obj_foreach_itr(lua_State *L);
int
_latom_seq_foreach_itr(lua_State *L);
int
_latom_vec_foreach_itr(lua_State *L);

int
_latom_seq_multiplex_itr(lua_State *L);

__realtime static inline latom_t *
_latom_body_new(lua_State *L, const LV2_Atom *atom, const void *body, bool cache)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	latom_t *latom = moony_newuserdata(L, moony, MOONY_UDATA_ATOM, cache);
	if(atom)
	{
		latom->atom = atom;
		latom->body.raw = body;
	}

	return latom;
}

__realtime static inline latom_t *
_latom_new(lua_State *L, const LV2_Atom *atom, bool cache)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	latom_t *latom = moony_newuserdata(L, moony, MOONY_UDATA_ATOM, cache);
	if(atom)
	{
		latom->atom = atom;
		latom->body.raw = LV2_ATOM_BODY_CONST(atom);
	}

	return latom;
}

__realtime static inline const latom_driver_t *
_latom_driver(moony_t *moony, LV2_URID type)
{
	const latom_driver_hash_t *base = moony->atom_driver_hash;

	for(unsigned N = DRIVER_HASH_MAX, half; N > 1; N -= half)
	{
		half = N/2;
		const latom_driver_hash_t *dst = &base[half];
		base = (dst->type > type) ? base : dst;
	}

	return (base->type == type) ? base->driver : &latom_chunk_driver;
}

__realtime static void
_latom_value(lua_State *L, const LV2_Atom *atom)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	const latom_driver_t *driver = _latom_driver(moony, atom->type);

	// dummy wrapping
	latom_t latom = {
		.atom = atom,
		.body.raw = LV2_ATOM_BODY_CONST(atom),
	};

	if(driver && driver->value)
		driver->value(L, &latom);
	else
		lua_pushnil(L); // unknown type
}

#endif
