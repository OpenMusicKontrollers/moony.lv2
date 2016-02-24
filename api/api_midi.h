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

#ifndef _MOONY_API_MIDI_H
#define _MOONY_API_MIDI_H

#include <moony.h>

typedef struct _midi_msg_t midi_msg_t;

struct _midi_msg_t {
	uint8_t type;
	const char *key;
};

int
_lmidiresponder(lua_State *L);

extern const luaL_Reg lmidiresponder_mt [];

extern const midi_msg_t midi_msgs [];
extern const midi_msg_t controllers [];

#endif
