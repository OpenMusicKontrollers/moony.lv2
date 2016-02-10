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

#ifndef _MOONY_COMMON_UI_H
#define _MOONY_COMMON_UI_H

typedef void (*encoder_begin_t)(void *data);
typedef void (*encoder_append_t)(const char *str, void *data);
typedef void (*encoder_end_t)(void *data);
typedef struct _moony_encoder_t moony_encoder_t;

struct _moony_encoder_t {
	encoder_begin_t begin;
	encoder_append_t append;
	encoder_end_t end;
	void *data;
};
extern moony_encoder_t *encoder;

void lua_to_markup(const char *utf8, FILE *f);

#endif
