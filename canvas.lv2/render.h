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

#include <assert.h>

#include <canvas.lv2/canvas.h>

#include <cairo/cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_CANVAS_NUM_METHODS 26

typedef struct _LV2_Canvas_Meth LV2_Canvas_Meth;
typedef struct _LV2_Canvas LV2_Canvas;
typedef void (*LV2_Canvas_Func)(cairo_t *ctx,
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

static inline void
_lv2_canvas_render_beginPath(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	cairo_new_sub_path(ctx);
}

static inline void
_lv2_canvas_render_closePath(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	cairo_close_path(ctx);
}

static inline void
_lv2_canvas_render_arc(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 5);

	if(v)
	{
		cairo_arc(ctx, v[0], v[1], v[2], v[3], v[4]);
	}
}

static inline void
_lv2_canvas_render_curveTo(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 6);

	if(v)
	{
		cairo_curve_to(ctx, v[0], v[1], v[2], v[3], v[4], v[5]);
	}
}

static inline void
_lv2_canvas_render_lineTo(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		cairo_line_to(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_moveTo(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		cairo_move_to(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_rectangle(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 4);

	if(v)
	{
		cairo_rectangle(ctx, v[0], v[1], v[2], v[3]);
	}
}

static inline void
_lv2_canvas_render_polyline(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	uint32_t N;
	const float *v = _lv2_canvas_render_get_float_vecs(urid, body, &N);

	if(v)
	{
		for(uint32_t i = 0; i < 2; i += 2)
		{
			cairo_move_to(ctx, v[i], v[i+1]);
		}

		for(uint32_t i = 2; i < N; i += 2)
		{
			cairo_line_to(ctx, v[i], v[i+1]);
		}
	}
}

static inline void
_lv2_canvas_render_style(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const int64_t *v = _lv2_canvas_render_get_type(body, urid->forge.Long);

	if(v)
	{
		const float r = (float)((*v >> 24) & 0xff) / 0xff;
		const float g = (float)((*v >> 16) & 0xff) / 0xff;
		const float b = (float)((*v >>  8) & 0xff) / 0xff;
		const float a = (float)((*v >>  0) & 0xff) / 0xff;

		cairo_set_source_rgba(ctx, r, g, b, a);
	}
}

static inline void
_lv2_canvas_render_lineWidth(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		cairo_set_line_width(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_lineDash(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		const double d[2] = {v[0], v[1]};
		cairo_set_dash(ctx, d, 2, 0);
	}
}

static inline void
_lv2_canvas_render_lineCap(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const LV2_URID *v = _lv2_canvas_render_get_type(body, urid->forge.URID);

	if(v)
	{
		cairo_line_cap_t cap = CAIRO_LINE_CAP_BUTT;

		if(*v == urid->Canvas_lineCapButt)
			cap = CAIRO_LINE_CAP_BUTT;
		else if(*v == urid->Canvas_lineCapRound)
			cap = CAIRO_LINE_CAP_ROUND;
		else if(*v == urid->Canvas_lineCapSquare)
			cap = CAIRO_LINE_CAP_SQUARE;

		cairo_set_line_cap(ctx, cap);
	}
}

static inline void
_lv2_canvas_render_lineJoin(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const LV2_URID *v = _lv2_canvas_render_get_type(body, urid->forge.URID);

	if(v)
	{
		cairo_line_join_t join = CAIRO_LINE_JOIN_MITER;

		if(*v == urid->Canvas_lineJoinMiter)
			join = CAIRO_LINE_JOIN_MITER;
		else if(*v == urid->Canvas_lineJoinRound)
			join = CAIRO_LINE_JOIN_ROUND;
		else if(*v == urid->Canvas_lineJoinBevel)
			join = CAIRO_LINE_JOIN_BEVEL;

		cairo_set_line_join(ctx, join);
	}
}

static inline void
_lv2_canvas_render_miterLimit(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		cairo_set_miter_limit(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_stroke(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	cairo_stroke(ctx);
}

static inline void
_lv2_canvas_render_fill(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	cairo_fill(ctx);
}

static inline void
_lv2_canvas_render_clip(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	cairo_clip(ctx);
}

static inline void
_lv2_canvas_render_save(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	cairo_save(ctx);
}

static inline void
_lv2_canvas_render_restore(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	cairo_restore(ctx);
}

static inline void
_lv2_canvas_render_translate(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		cairo_translate(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_scale(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		cairo_scale(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_rotate(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		cairo_rotate(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_transform(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 6);

	if(v)
	{
		const cairo_matrix_t matrix = {
			.xx = v[0],
			.xy = v[1],
			.x0 = v[2],
			.yy = v[3],
			.yx = v[4],
			.y0 = v[5]
		};

		cairo_transform(ctx, &matrix);
	}
}

static inline void
_lv2_canvas_render_reset(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
		cairo_identity_matrix(ctx);
}

static inline void
_lv2_canvas_render_fontSize(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		cairo_set_font_size(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_fillText(cairo_t *ctx,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	const char *v = _lv2_canvas_render_get_type(body, urid->forge.String);

	if(v)
	{
		cairo_text_extents_t extents;
		cairo_text_extents(ctx, v, &extents);
		const float dx = extents.width/2 + extents.x_bearing;
		const float dy = extents.height/2 + extents.y_bearing;
		cairo_rel_move_to(ctx, -dx, -dy);
		cairo_show_text(ctx, v);
	}
}

static inline void
_lv2_canvas_qsort(LV2_Canvas_Meth *A, int n)
{
	if(n < 2)
		return;

	LV2_Canvas_Meth *p = A;

	int i = -1;
	int j = n;

	while(true)
	{
		do {
			i += 1;
		} while(A[i].command < p->command);

		do {
			j -= 1;
		} while(A[j].command > p->command);

		if(i >= j)
			break;

		const LV2_Canvas_Meth tmp = A[i];
		A[i] = A[j];
		A[j] = tmp;
	}

	_lv2_canvas_qsort(A, j + 1);
	_lv2_canvas_qsort(A + j + 1, n - j - 1);
}

static inline LV2_Canvas_Meth *
_lv2_canvas_bsearch(LV2_URID p, LV2_Canvas_Meth *a, int n)
{
	LV2_Canvas_Meth *base = a;

	for(int N = n, half; N > 1; N -= half)
	{
		half = N/2;
		LV2_Canvas_Meth *dst = &base[half];
		base = (dst->command > p) ? base : dst;
	}

	return (base->command == p) ? base : NULL;
}

static inline void
lv2_canvas_init(LV2_Canvas *canvas, LV2_URID_Map *map)
{
	lv2_canvas_urid_init(&canvas->urid, map);

	unsigned ptr = 0;

	canvas->methods[ptr].command = canvas->urid.Canvas_BeginPath;
	canvas->methods[ptr++].func = _lv2_canvas_render_beginPath;

	canvas->methods[ptr].command = canvas->urid.Canvas_ClosePath;
	canvas->methods[ptr++].func = _lv2_canvas_render_closePath;

	canvas->methods[ptr].command = canvas->urid.Canvas_Arc;
	canvas->methods[ptr++].func = _lv2_canvas_render_arc;

	canvas->methods[ptr].command = canvas->urid.Canvas_CurveTo;
	canvas->methods[ptr++].func = _lv2_canvas_render_curveTo;

	canvas->methods[ptr].command = canvas->urid.Canvas_LineTo;
	canvas->methods[ptr++].func = _lv2_canvas_render_lineTo;

	canvas->methods[ptr].command = canvas->urid.Canvas_MoveTo;
	canvas->methods[ptr++].func = _lv2_canvas_render_moveTo;

	canvas->methods[ptr].command = canvas->urid.Canvas_Rectangle;
	canvas->methods[ptr++].func = _lv2_canvas_render_rectangle;

	canvas->methods[ptr].command = canvas->urid.Canvas_PolyLine;
	canvas->methods[ptr++].func = _lv2_canvas_render_polyline;

	canvas->methods[ptr].command = canvas->urid.Canvas_Style;
	canvas->methods[ptr++].func = _lv2_canvas_render_style;

	canvas->methods[ptr].command = canvas->urid.Canvas_LineWidth;
	canvas->methods[ptr++].func = _lv2_canvas_render_lineWidth;

	canvas->methods[ptr].command = canvas->urid.Canvas_LineDash;
	canvas->methods[ptr++].func = _lv2_canvas_render_lineDash;

	canvas->methods[ptr].command = canvas->urid.Canvas_LineCap;
	canvas->methods[ptr++].func = _lv2_canvas_render_lineCap;

	canvas->methods[ptr].command = canvas->urid.Canvas_LineJoin;
	canvas->methods[ptr++].func = _lv2_canvas_render_lineJoin;

	canvas->methods[ptr].command = canvas->urid.Canvas_MiterLimit;
	canvas->methods[ptr++].func = _lv2_canvas_render_miterLimit;

	canvas->methods[ptr].command = canvas->urid.Canvas_Stroke;
	canvas->methods[ptr++].func = _lv2_canvas_render_stroke;

	canvas->methods[ptr].command = canvas->urid.Canvas_Fill;
	canvas->methods[ptr++].func = _lv2_canvas_render_fill;

	canvas->methods[ptr].command = canvas->urid.Canvas_Clip;
	canvas->methods[ptr++].func = _lv2_canvas_render_clip;

	canvas->methods[ptr].command = canvas->urid.Canvas_Save;
	canvas->methods[ptr++].func = _lv2_canvas_render_save;

	canvas->methods[ptr].command = canvas->urid.Canvas_Restore;
	canvas->methods[ptr++].func = _lv2_canvas_render_restore;

	canvas->methods[ptr].command = canvas->urid.Canvas_Translate;
	canvas->methods[ptr++].func = _lv2_canvas_render_translate;

	canvas->methods[ptr].command = canvas->urid.Canvas_Scale;
	canvas->methods[ptr++].func = _lv2_canvas_render_scale;

	canvas->methods[ptr].command = canvas->urid.Canvas_Rotate;
	canvas->methods[ptr++].func = _lv2_canvas_render_rotate;

	canvas->methods[ptr].command = canvas->urid.Canvas_Transform;
	canvas->methods[ptr++].func = _lv2_canvas_render_transform;

	canvas->methods[ptr].command = canvas->urid.Canvas_Reset;
	canvas->methods[ptr++].func = _lv2_canvas_render_reset;

	canvas->methods[ptr].command = canvas->urid.Canvas_FontSize;
	canvas->methods[ptr++].func = _lv2_canvas_render_fontSize;

	canvas->methods[ptr].command = canvas->urid.Canvas_FillText;
	canvas->methods[ptr++].func = _lv2_canvas_render_fillText;

	assert(ptr == LV2_CANVAS_NUM_METHODS);

	_lv2_canvas_qsort(canvas->methods, LV2_CANVAS_NUM_METHODS);
}

static inline bool
lv2_canvas_render(LV2_Canvas *canvas, cairo_t *ctx, const LV2_Atom_Tuple *tup)
{
	LV2_Canvas_URID *urid = &canvas->urid;

	if(!tup || (tup->atom.type != urid->forge.Tuple) )
		return false;

	// save state
	cairo_save(ctx);

	// clear surface
	cairo_set_operator(ctx, CAIRO_OPERATOR_CLEAR);
	cairo_paint(ctx);

	// default attributes
	cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
	cairo_set_font_size(ctx, 0.1);
	cairo_set_line_width(ctx, 0.01);
	cairo_set_source_rgba(ctx, 1.0, 1.0, 1.0, 1.0);

	LV2_ATOM_TUPLE_FOREACH(tup, itm)
	{
		if(lv2_atom_forge_is_object_type(&urid->forge, itm->type))
		{
			const LV2_Atom_Object *obj = (const LV2_Atom_Object *)itm;
			const LV2_Atom *body = NULL;

			lv2_atom_object_get(obj, urid->Canvas_body, &body, 0);

			LV2_Canvas_Meth *meth = _lv2_canvas_bsearch(obj->body.otype,
				canvas->methods, LV2_CANVAS_NUM_METHODS);

			if(meth)
			{
				meth->func(ctx, urid, body);
			}
		}
	}

	// save state
	cairo_restore(ctx);

	// flush
	cairo_surface_t *surface = cairo_get_target(ctx);
	cairo_surface_flush(surface);

	return true;
}

#ifdef __cplusplus
}
#endif

#endif // CANVAS_IMPLEMENTATION
