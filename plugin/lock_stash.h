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

#ifndef _LOCK_STASH_H
#define _LOCK_STASH_H

#define LOCK_STASH_SIZE 0x2000 //TODO how big?

typedef struct _lock_stash_t lock_stash_t;

struct _lock_stash_t {
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge_Ref ref;
	union {
		LV2_Atom_Sequence seq;
		uint8_t buf [LOCK_STASH_SIZE];
	};
};

static inline void
_stash_init(lock_stash_t *stash, LV2_URID_Map *map)
{
	lv2_atom_forge_init(&stash->forge, map);
}

static inline void
_stash_reset(lock_stash_t *stash)
{
	lv2_atom_forge_set_buffer(&stash->forge, stash->buf, LOCK_STASH_SIZE);

	stash->ref = lv2_atom_forge_sequence_head(&stash->forge, &stash->frame, 0);
}

#endif
