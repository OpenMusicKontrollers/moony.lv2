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

#ifndef _MOONY_API_FORGE_H
#define _MOONY_API_FORGE_H

#include <moony.h>

typedef struct _lforge_t lforge_t;

struct _lforge_t {
	lheader_t lheader;

	LV2_Atom_Forge *forge;
	int depth;
	union {
		int64_t frames;	// Time in audio frames
		double  beats; // Time in beats
	} last;
	union {
		struct {
			LV2_URID otype; // Object type
			LV2_URID id; // Object ID
		};
		LV2_URID unit; // Sequence time unit
	} pack;
	LV2_Atom_Forge_Frame frame [2];
};

LV2_Atom_Forge_Ref
_lforge_basic(lua_State *L, int pos, LV2_Atom_Forge *forge, LV2_URID range);

int
_lforge_autopop_itr(lua_State *L);

extern const char *forge_buffer_overflow;

extern const luaL_Reg lforge_mt [];

#endif
