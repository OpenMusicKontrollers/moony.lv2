/*
 * Copyright (c) 2018-2019 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _D2TK_UTIL_H
#define _D2TK_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include <d2tk/d2tk.h>

D2TK_API int
d2tk_util_spawn(char **argv);

D2TK_API int
d2tk_util_kill(int *kid);

D2TK_API int
d2tk_util_wait(int *kid);

#ifdef __cplusplus
}
#endif

#endif // _D2TK_UTIL_H
