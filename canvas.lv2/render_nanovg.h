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

#ifndef _LV2_CANVAS_RENDER_NANOVG_H
#define _LV2_CANVAS_RENDER_NANOVG_H

#include <assert.h>

#include <nanovg.h>

#if defined(__APPLE__)
#	include <OpenGL/gl.h>
#	include <OpenGL/glext.h>
#else
#	include <GL/glew.h>
#endif

#define NANOVG_GL2_IMPLEMENTATION
#include <nanovg_gl.h>

#if defined(NANOVG_GL2_IMPLEMENTATION)
#	define nvgCreate nvgCreateGL2
#	define nvgDelete nvgDeleteGL2
#elif defined(NANOVG_GL3_IMPLEMENTATION)
#	define nvgCreate nvgCreateGL3
#	define nvgDelete nvgDeleteGL3
#elif defined(NANOVG_GLES2_IMPLEMENTATION)
#	define nvgCreate nvgCreateGLES2
#	define nvgDelete nvgDeleteGLES2
#elif defined(NANOVG_GLES3_IMPLEMENTATION)
#	define nvgCreate nvgCreateGLES3
#	define nvgDelete nvgDeleteGLES3
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline void
_lv2_canvas_render_beginPath(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	nvgBeginPath(ctx);
}

static inline void
_lv2_canvas_render_closePath(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	nvgClosePath(ctx);
}

static inline void
_lv2_canvas_render_arc(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 5);

	if(v)
	{
		nvgArc(ctx, v[0], v[1], v[2], v[3], v[4], NVG_CCW);
	}
}

static inline void
_lv2_canvas_render_curveTo(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 6);

	if(v)
	{
		nvgBezierTo(ctx, v[0], v[1], v[2], v[3], v[4], v[5]);
	}
}

static inline void
_lv2_canvas_render_lineTo(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		nvgLineTo(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_moveTo(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		nvgMoveTo(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_rectangle(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 4);

	if(v)
	{
		nvgRect(ctx, v[0], v[1], v[2], v[3]);
	}
}

static inline void
_lv2_canvas_render_polyline(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	uint32_t N;
	const float *v = _lv2_canvas_render_get_float_vecs(urid, body, &N);

	if(v)
	{
		for(uint32_t i = 0; i < 2; i += 2)
		{
			nvgMoveTo(ctx, v[i], v[i+1]);
		}

		for(uint32_t i = 2; i < N; i += 2)
		{
			nvgLineTo(ctx, v[i], v[i+1]);
		}
	}
}

static inline void
_lv2_canvas_render_style(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const int64_t *v = _lv2_canvas_render_get_type(body, urid->forge.Long);

	if(v)
	{
		const uint8_t r = (*v >> 24) & 0xff;
		const uint8_t g = (*v >> 16) & 0xff;
		const uint8_t b = (*v >>  8) & 0xff;
		const uint8_t a = (*v >>  0) & 0xff;

		nvgStrokeColor(ctx, nvgRGBA(r, g, b, a));
		nvgFillColor(ctx, nvgRGBA(r, g, b, a));
	}
}

static inline void
_lv2_canvas_render_lineWidth(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		nvgStrokeWidth(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_lineDash(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	(void)ctx; //FIXME
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		//const double d[2] = {v[0], v[1]};
		//FIXME cairo_set_dash(ctx, d, 2, 0);
	}
}

static inline void
_lv2_canvas_render_lineCap(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const LV2_URID *v = _lv2_canvas_render_get_type(body, urid->forge.URID);

	if(v)
	{
		int cap = NVG_BUTT;

		if(*v == urid->Canvas_lineCapButt)
			cap = NVG_BUTT;
		else if(*v == urid->Canvas_lineCapRound)
			cap = NVG_ROUND;
		else if(*v == urid->Canvas_lineCapSquare)
			cap = NVG_SQUARE;

		nvgLineCap(ctx, cap);
	}
}

static inline void
_lv2_canvas_render_lineJoin(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const LV2_URID *v = _lv2_canvas_render_get_type(body, urid->forge.URID);

	if(v)
	{
		int join = NVG_MITER;

		if(*v == urid->Canvas_lineJoinMiter)
			join = NVG_MITER;
		else if(*v == urid->Canvas_lineJoinRound)
			join = NVG_ROUND;
		else if(*v == urid->Canvas_lineJoinBevel)
			join = NVG_BEVEL;

		nvgLineJoin(ctx, join);
	}
}

static inline void
_lv2_canvas_render_miterLimit(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		nvgMiterLimit(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_stroke(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	nvgStroke(ctx);
}

static inline void
_lv2_canvas_render_fill(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	nvgFill(ctx);
}

static inline void
_lv2_canvas_render_clip(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	(void)ctx; //FIXME
	//FIXME cairo_clip(ctx);
}

static inline void
_lv2_canvas_render_save(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	nvgSave(ctx);
}

static inline void
_lv2_canvas_render_restore(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	nvgRestore(ctx);
}

static inline void
_lv2_canvas_render_translate(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		nvgTranslate(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_scale(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 2);

	if(v)
	{
		nvgScale(ctx, v[0], v[1]);
	}
}

static inline void
_lv2_canvas_render_rotate(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		nvgRotate(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_transform(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_float_vec(urid, body, 6);

	if(v)
	{
		nvgTransform(ctx, v[0], v[1], v[2], v[3], v[4], v[5]);
	}
}

static inline void
_lv2_canvas_render_reset(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	nvgReset(ctx);
}

static inline void
_lv2_canvas_render_fontSize(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	const float *v = _lv2_canvas_render_get_type(body, urid->forge.Float);

	if(v)
	{
		nvgFontSize(ctx, *v);
	}
}

static inline void
_lv2_canvas_render_fillText(void *data,
	LV2_Canvas_URID *urid, const LV2_Atom *body)
{
	NVGcontext *ctx = data;
	(void)ctx; //FIXME
	const char *v = _lv2_canvas_render_get_type(body, urid->forge.String);

	if(v)
	{
		/* FIXME
		NVGcontextext_extents_t extents;
		NVGcontextext_extents(ctx, v, &extents);
		const float dx = extents.width/2 + extents.x_bearing;
		const float dy = extents.height/2 + extents.y_bearing;
		cairo_rel_move_to(ctx, -dx, -dy);
		cairo_show_text(ctx, v);
		*/
		/*
		float bounds [4];
		const float adv_x = nvgTextBounds(ctx, 0.f, 0.f, v, NULL, bounds);
		nvgText(ctx, float x, float y, const char* string, const char* end);
		*/
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
lv2_canvas_render_body(LV2_Canvas *canvas, NVGcontext *ctx, uint32_t type,
	uint32_t size, const LV2_Atom *body)
{
	LV2_Canvas_URID *urid = &canvas->urid;

	if(!body || (type != urid->forge.Tuple) )
		return false;

	// save state
	nvgSave(ctx);

	// clear surface
	nvgBeginPath(ctx);
	nvgRect(ctx, 0, 0, 1.f, 1.f);
	nvgFillColor(ctx, nvgRGBA(0x1e, 0x1e, 0x1e, 0xff));
	nvgFill(ctx);

	nvgFontSize(ctx, 0.1);
	nvgStrokeWidth(ctx, 0.01);
	nvgStrokeColor(ctx, nvgRGBA(0xff, 0xff, 0xff, 0xff));
	nvgFillColor(ctx, nvgRGBA(0xff, 0xff, 0xff, 0xff));

	LV2_ATOM_TUPLE_BODY_FOREACH(body, size, itm)
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
	nvgRestore(ctx);

	return true;
}

static inline bool
lv2_canvas_render(LV2_Canvas *canvas, NVGcontext *ctx, const LV2_Atom_Tuple *tup)
{
	return lv2_canvas_render_body(canvas, ctx, tup->atom.type, tup->atom.size,
		LV2_ATOM_BODY_CONST(&tup->atom));
}

#ifdef __cplusplus
}
#endif

#endif // LV2_CANVAS_RENDER_NANOVG_H 
