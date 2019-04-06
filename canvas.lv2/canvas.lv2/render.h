/*
 * Copyright (c) 2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _LV2_CANVAS_RENDER_H
#define _LV2_CANVAS_RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <canvas.lv2/canvas.h>

#define LV2_CANVAS_NUM_METHODS 26

typedef struct _LV2_Canvas_Meth LV2_Canvas_Meth;
typedef struct _LV2_Canvas LV2_Canvas;
typedef void (*LV2_Canvas_Func)(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body);

struct _LV2_Canvas_Meth {
	LV2_URID command;
	LV2_Canvas_Func func;
};

struct _LV2_Canvas {
	LV2_Canvas_URID urid;
	LV2_Canvas_Meth methods [LV2_CANVAS_NUM_METHODS];
};

static inline const float *
_lv2_canvas_render_get_float_vecs(LV2_Canvas_URID *urid, const LV2_Atom *body,
	uint32_t *n)
{
	const LV2_Atom_Vector *vec = (const LV2_Atom_Vector *)body;
	const float *flt = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, vec);
	*n = (vec->atom.type == urid->forge.Vector)
		&& (vec->body.child_type == urid->forge.Float)
		&& (vec->body.child_size == sizeof(float))
		? (vec->atom.size - sizeof(LV2_Atom_Vector_Body)) / vec->body.child_size
		: 0;

	return flt;
}

static inline const float *
_lv2_canvas_render_get_float_vec(LV2_Canvas_URID *urid, const LV2_Atom *body,
	uint32_t n)
{
	uint32_t N;
	const float *flt = _lv2_canvas_render_get_float_vecs(urid, body, &N);

	return n == N ? flt : NULL;
}

static inline const void *
_lv2_canvas_render_get_type(const LV2_Atom *body, LV2_URID type)
{
	return body->type == type
		? LV2_ATOM_BODY_CONST(body)
		: NULL;
}

#ifdef __cplusplus
}
#endif

#if defined(LV2_CANVAS_RENDER_NANOVG)
#	include <canvas.lv2/render_nanovg.h>
#else
#	include <canvas.lv2/render_cairo.h>
#endif

#endif // _LV2_CANVAS_RENDER_H
