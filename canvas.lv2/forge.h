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

#ifndef _LV2_CANVAS_FORGE_H
#define _LV2_CANVAS_FORGE_H

#include <canvas.lv2/canvas.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline LV2_Atom_Forge_Ref
_lv2_canvas_forge_simple(LV2_Atom_Forge *forge, LV2_URID otype)
{
	LV2_Atom_Forge_Ref ref;
	LV2_Atom_Forge_Frame frame;

	ref = lv2_atom_forge_object(forge, &frame, 0, otype);
	if(ref)
		lv2_atom_forge_pop(forge, &frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
_lv2_canvas_forge_vec(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	LV2_URID otype, uint32_t n, const float *vec)
{
	LV2_Atom_Forge_Ref ref;
	LV2_Atom_Forge_Frame frame;

	ref = lv2_atom_forge_object(forge, &frame, 0, otype);
	if(ref)
		ref = lv2_atom_forge_key(forge, urid->Canvas_body);
	if(ref)
		ref = lv2_atom_forge_vector(forge, sizeof(float), forge->Float, n, vec);
	if(ref)
		lv2_atom_forge_pop(forge, &frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
_lv2_canvas_forge_flt(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	LV2_URID otype, float flt)
{
	LV2_Atom_Forge_Ref ref;
	LV2_Atom_Forge_Frame frame;

	ref = lv2_atom_forge_object(forge, &frame, 0, otype);
	if(ref)
		ref = lv2_atom_forge_key(forge, urid->Canvas_body);
	if(ref)
		ref = lv2_atom_forge_float(forge, flt);
	if(ref)
		lv2_atom_forge_pop(forge, &frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
_lv2_canvas_forge_lng(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	LV2_URID otype, int64_t lng)
{
	LV2_Atom_Forge_Ref ref;
	LV2_Atom_Forge_Frame frame;

	ref = lv2_atom_forge_object(forge, &frame, 0, otype);
	if(ref)
		ref = lv2_atom_forge_key(forge, urid->Canvas_body);
	if(ref)
		ref = lv2_atom_forge_long(forge, lng);
	if(ref)
		lv2_atom_forge_pop(forge, &frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
_lv2_canvas_forge_prp(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	LV2_URID otype, LV2_URID prop)
{
	LV2_Atom_Forge_Ref ref;
	LV2_Atom_Forge_Frame frame;

	ref = lv2_atom_forge_object(forge, &frame, 0, otype);
	if(ref)
		ref = lv2_atom_forge_key(forge, urid->Canvas_body);
	if(ref)
		ref = lv2_atom_forge_urid(forge, prop);
	if(ref)
		lv2_atom_forge_pop(forge, &frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
_lv2_canvas_forge_str(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	LV2_URID otype, const char *text)
{
	LV2_Atom_Forge_Ref ref;
	LV2_Atom_Forge_Frame frame;

	ref = lv2_atom_forge_object(forge, &frame, 0, otype);
	if(ref)
		ref = lv2_atom_forge_key(forge, urid->Canvas_body);
	if(ref)
		ref = lv2_atom_forge_string(forge, text, strlen(text));
	if(ref)
		lv2_atom_forge_pop(forge, &frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_beginPath(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_BeginPath);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_closePath(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_ClosePath);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_arc(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float x, float y, float r, float a1, float a2)
{
	const float vec [5] = {x, y, r, a1, a2};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_Arc, 5, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_curveTo(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float x1, float y1, float x2, float y2, float x3, float y3)
{
	const float vec [6] = {x1, y1, x2, y2, x3, y3};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_CurveTo, 6, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_lineTo(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float x, float y)
{
	const float vec [2] = {x, y};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_LineTo, 2, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_moveTo(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float x, float y)
{
	const float vec [2] = {x, y};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_MoveTo, 2, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_rectangle(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float x, float y, float w, float h)
{
	const float vec [4] = {x, y, w, h};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_Rectangle, 4, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_style(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	uint32_t style)
{
	return _lv2_canvas_forge_lng(forge, urid, urid->Canvas_Style, style);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_lineWidth(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float line_width)
{
	return _lv2_canvas_forge_flt(forge, urid, urid->Canvas_LineWidth, line_width);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_lineDash(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float dash_length, float separator_length)
{
	const float vec [2] = {dash_length, separator_length};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_LineDash, 2, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_lineCap(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	LV2_URID line_cap)
{
	return _lv2_canvas_forge_prp(forge, urid, urid->Canvas_LineCap, line_cap);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_lineJoin(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	LV2_URID line_join)
{
	return _lv2_canvas_forge_prp(forge, urid, urid->Canvas_LineJoin, line_join);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_miterLimit(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float miter_limit)
{
	return _lv2_canvas_forge_flt(forge, urid, urid->Canvas_MiterLimit, miter_limit);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_stroke(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_Stroke);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_fill(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_Fill);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_clip(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_Clip);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_save(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_Save);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_restore(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_Restore);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_translate(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float x, float y)
{
	const float vec [2] = {x, y};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_Translate, 2, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_scale(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float w, float h)
{
	const float vec [2] = {w, h};

	return _lv2_canvas_forge_vec(forge, urid, urid->Canvas_Scale, 2, vec);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_rotate(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float a)
{
	return _lv2_canvas_forge_flt(forge, urid, urid->Canvas_Rotate, a);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_reset(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid)
{
	return _lv2_canvas_forge_simple(forge, urid->Canvas_Reset);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_fontSize(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	float size)
{
	return _lv2_canvas_forge_flt(forge, urid, urid->Canvas_FontSize, size);
}

static inline LV2_Atom_Forge_Ref
lv2_canvas_forge_fillText(LV2_Atom_Forge *forge, LV2_Canvas_URID *urid,
	const char *text)
{
	return _lv2_canvas_forge_str(forge, urid, urid->Canvas_FillText, text);
}

#ifdef __cplusplus
}
#endif

#endif // _LV2_CANVAS_FORGE_H
