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

#ifndef _LV2_CANVAS_IDISP_H
#define _LV2_CANVAS_IDISP_H

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_CANVAS_RENDER_CAIRO
#include <canvas.lv2/render.h>
#include <canvas.lv2/lv2_extensions.h>

typedef struct _LV2_Canvas_Idisp LV2_Canvas_Idisp;

struct _LV2_Canvas_Idisp {
	LV2_Inline_Display *queue_draw;
	LV2_Canvas canvas;
	LV2_Inline_Display_Image_Surface image_surface;
	struct {
		cairo_surface_t *surface;
		cairo_t *ctx;
	} cairo;
};

static inline LV2_Inline_Display_Image_Surface *
_lv2_canvas_idisp_surf_init(LV2_Canvas_Idisp *idisp, int w, int h)
{
	LV2_Inline_Display_Image_Surface *surf = &idisp->image_surface;

	surf->width = w;
	surf->height = h;
	surf->stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, surf->width);
	surf->data = realloc(surf->data, surf->stride * surf->height);
	if(!surf->data)
	{
		return NULL;
	}

	idisp->cairo.surface = cairo_image_surface_create_for_data(
		surf->data, CAIRO_FORMAT_ARGB32, surf->width, surf->height, surf->stride);

	if(idisp->cairo.surface)
	{
		cairo_surface_set_device_scale(idisp->cairo.surface, surf->width, surf->height);

		idisp->cairo.ctx = cairo_create(idisp->cairo.surface);
		if(idisp->cairo.ctx)
		{
			cairo_select_font_face(idisp->cairo.ctx, "cairo:monospace",
				CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		}
	}

	return surf;
}

static inline void
_lv2_canvas_idisp_surf_deinit(LV2_Canvas_Idisp *idisp)
{
	LV2_Inline_Display_Image_Surface *surf = &idisp->image_surface;

	if(idisp->cairo.ctx)
	{
		cairo_destroy(idisp->cairo.ctx);
		idisp->cairo.ctx = NULL;
	}

	if(idisp->cairo.surface)
	{
		cairo_surface_finish(idisp->cairo.surface);
		cairo_surface_destroy(idisp->cairo.surface);
		idisp->cairo.surface = NULL;
	}

	if(surf->data)
	{
		free(surf->data);
		surf->data = NULL;
	}
}

static inline LV2_Inline_Display_Image_Surface *
lv2_canvas_idisp_surf_configure(LV2_Canvas_Idisp *idisp, 
	uint32_t w, uint32_t h, float aspect_ratio)
{
	LV2_Inline_Display_Image_Surface *surf = &idisp->image_surface;
	int W;
	int H;

	if(aspect_ratio < 1.f)
	{
		W = h * aspect_ratio;
		H = h;
	}
	else if(aspect_ratio > 1.f)
	{
		W = w;
		H = w / aspect_ratio;
	}
	else // aspect_ratio == 1.f
	{
		W = w;
		H = h;
	}

	if( (surf->width != W) || (surf->height != H) || !surf->data)
	{
		_lv2_canvas_idisp_surf_deinit(idisp);
		surf = _lv2_canvas_idisp_surf_init(idisp, W, H);
	}

	return surf;
}

static inline void
lv2_canvas_idisp_init(LV2_Canvas_Idisp *idisp, LV2_Inline_Display *queue_draw,
	LV2_URID_Map *map)
{
	lv2_canvas_init(&idisp->canvas, map);
	idisp->queue_draw = queue_draw;
}

static inline void
lv2_canvas_idisp_deinit(LV2_Canvas_Idisp *idisp)
{
	_lv2_canvas_idisp_surf_deinit(idisp);
}

static inline void
lv2_canvas_idisp_queue_draw(LV2_Canvas_Idisp *idisp)
{
	if(idisp->queue_draw)
	{
		idisp->queue_draw->queue_draw(idisp->queue_draw->handle);
	}
}

static inline bool
lv2_canvas_idisp_render_body(LV2_Canvas_Idisp *idisp, uint32_t type,
	uint32_t size, const LV2_Atom *body)
{
	return lv2_canvas_render_body(&idisp->canvas, idisp->cairo.ctx,
		type, size, body);
}

#ifdef __cplusplus
}
#endif

#endif // _LV2_CANVAS_IDISP_H
