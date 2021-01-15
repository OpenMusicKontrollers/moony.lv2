/*
 * Copyright (c) 2015-2021 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _MOONY_API_STASH_H
#define _MOONY_API_STASH_H

#include <moony.h>

#include <api_atom.h>
#include <api_forge.h>

typedef struct _lstash_t lstash_t;

struct _lstash_t {
	union {
		latom_t latom;
		lforge_t lforge;
	};

	atom_ser_t ser;
	LV2_Atom_Forge forge;
};

int
_lstash(lua_State *L);
int
_lstash__gc(lua_State *L);
int
_lstash_write(lua_State *L);
int
_lstash_read(lua_State *L);

extern const luaL_Reg lstash_mt [];

#endif
