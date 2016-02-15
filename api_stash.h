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

#ifndef _MOONY_API_STASH_H
#define _MOONY_API_STASH_H

#include <moony.h>

#include <api_atom.h>
#include <api_forge.h>

typedef struct _atom_ser_t atom_ser_t;
typedef struct _lstash_t lstash_t;
typedef enum _lstash_mode_t lstash_mode_t;

enum _lstash_mode_t {
	LSTASH_MODE_READ,
	LSTASH_MODE_WRITE
};

struct _atom_ser_t {
	tlsf_t tlsf;
	uint32_t size;
	uint8_t *buf;
	uint32_t offset;
};

struct _lstash_t {
	lstash_mode_t mode;
	atom_ser_t ser;
	LV2_Atom_Forge forge;
};

int
_lstash(lua_State *L);

extern const luaL_Reg lstash_mt [];

#endif
