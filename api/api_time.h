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

#ifndef _MOONY_API_TIME_H
#define _MOONY_API_TIME_H

#include <moony.h>

int
_ltimeresponder(lua_State *L);

int
_ltimeresponder_apply(lua_State *L);

int
_ltimeresponder_stash(lua_State *L);

extern const luaL_Reg ltimeresponder_mt [];

#endif
