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
typedef struct _lforge_t lforge_t;
typedef struct _latom_t latom_t;
typedef struct _lobj_t lobj_t;
typedef struct _ltuple_t ltuple_t;
typedef struct _lvec_t lvec_t;

typedef int (*latom_driver_function_t)(lua_State *L, latom_t *latom);

struct _latom_driver_t {
	latom_driver_function_t __indexi;
	latom_driver_function_t __indexk;
	latom_driver_function_t __len;
	latom_driver_function_t __tostring;
	latom_driver_function_t __call;

	latom_driver_function_t value;
	int unpack;
	int foreach;
};

struct _lseq_t {
	const LV2_Atom_Sequence *seq;
	const LV2_Atom_Event *itr;
	LV2_Atom body [0];
};

struct _lforge_t {
	LV2_Atom_Forge *forge;
	int depth;
	union {
		int64_t frames;	// Time in audio frames
		double  beats; // Time in beats
	} last;
	LV2_Atom_Forge_Frame frame [2];
};

struct _lobj_t {
	const LV2_Atom_Object *obj;
	const LV2_Atom_Property_Body *itr;
};

struct _ltuple_t {
	const LV2_Atom_Tuple *tuple;
	int pos;
	const LV2_Atom *itr;
};

struct _lvec_t {
	const LV2_Atom_Vector *vec;
	int count;
	int pos;
};

struct _latom_t {
	union {
		const LV2_Atom *atom;

		const LV2_Atom_Int *i32;
		const LV2_Atom_Long *i64;
		const LV2_Atom_Float *f32;
		const LV2_Atom_Double *f64;
		const LV2_Atom_URID *u32;
		const LV2_Atom_Literal *lit;

		lseq_t seq;
		lobj_t obj;
		ltuple_t tuple;
		lvec_t vec;
	};

	LV2_Atom body [0];
};

// in api_atom.c
extern const latom_driver_t latom_int_driver;
extern const latom_driver_t latom_long_driver;
extern const latom_driver_t latom_float_driver;
extern const latom_driver_t latom_double_driver;
extern const latom_driver_t latom_bool_driver;
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

static inline void
_latom_new(lua_State *L, const LV2_Atom *atom)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	latom_t *latom = moony_newuserdata(L, moony, MOONY_UDATA_ATOM);
	latom->atom = atom;
}

static inline const latom_driver_t *
_latom_driver(moony_t *moony, LV2_URID type)
{
	const latom_driver_hash_t *base = moony->atom_driver_hash;
	const latom_driver_hash_t *p;

	for(size_t lim = DRIVER_HASH_MAX; lim != 0; lim >>= 1)
	{
		p = base + (lim >> 1);

		if(type == p->type)
			return p->driver;

		if(type > p->type)
		{
			base = p + 1;
			lim--;
		}
	}

	return NULL;
}

static void
_latom_value(lua_State *L, const LV2_Atom *atom)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	const latom_driver_t *driver = _latom_driver(moony, atom->type);

	// dummy wrapping
	latom_t latom = {
		.atom = atom
	};

	if(driver && driver->value)
		driver->value(L, &latom);
	else
		lua_pushnil(L); // unknown type
}

#endif
