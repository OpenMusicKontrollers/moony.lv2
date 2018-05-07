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

#ifndef _LV2_CANVAS_H
#define _LV2_CANVAS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#define CANVAS_URI                "http://open-music-kontrollers.ch/lv2/canvas"
#define CANVAS_PREFIX             CANVAS_URI"#"

#define CANVAS__graph             CANVAS_PREFIX"graph"
#define CANVAS__body              CANVAS_PREFIX"body"
#define CANVAS__aspectRatio       CANVAS_PREFIX"aspectRatio"

// Graph properties and attributes
#define CANVAS__BeginPath         CANVAS_PREFIX"BeginPath"
#define CANVAS__ClosePath         CANVAS_PREFIX"ClosePath"
#define CANVAS__Arc               CANVAS_PREFIX"Arc"
#define CANVAS__CurveTo           CANVAS_PREFIX"CurveTo"
#define CANVAS__LineTo            CANVAS_PREFIX"LineTo"
#define CANVAS__MoveTo            CANVAS_PREFIX"MoveTo"
#define CANVAS__Rectangle         CANVAS_PREFIX"Rectangle"
#define CANVAS__PolyLine          CANVAS_PREFIX"PolyLine"
#define CANVAS__Style             CANVAS_PREFIX"Style"
#define CANVAS__LineWidth         CANVAS_PREFIX"LineWidth"
#define CANVAS__LineDash          CANVAS_PREFIX"LineDash"
#define CANVAS__LineCap           CANVAS_PREFIX"LineCap"
#define CANVAS__LineJoin          CANVAS_PREFIX"LineJoin"
#define CANVAS__MiterLimit        CANVAS_PREFIX"MiterLimig"
#define CANVAS__Stroke            CANVAS_PREFIX"Stroke"
#define CANVAS__Fill              CANVAS_PREFIX"Fill"
#define CANVAS__Clip              CANVAS_PREFIX"Clip"
#define CANVAS__Save              CANVAS_PREFIX"Save"
#define CANVAS__Restore           CANVAS_PREFIX"Restore"
#define CANVAS__Translate         CANVAS_PREFIX"Translate"
#define CANVAS__Scale             CANVAS_PREFIX"Scale"
#define CANVAS__Rotate            CANVAS_PREFIX"Rotate"
#define CANVAS__Transform         CANVAS_PREFIX"Transform"
#define CANVAS__Reset             CANVAS_PREFIX"Reset"
#define CANVAS__FontSize          CANVAS_PREFIX"FontSize"
#define CANVAS__FillText          CANVAS_PREFIX"FillText"

#define CANVAS__lineCapButt       CANVAS_PREFIX"lineCapButt"
#define CANVAS__lineCapRound      CANVAS_PREFIX"lineCapRound"
#define CANVAS__lineCapSquare     CANVAS_PREFIX"lineCapSquare"

#define CANVAS__lineJoinMiter     CANVAS_PREFIX"lineJoinMiter"
#define CANVAS__lineJoinRound     CANVAS_PREFIX"lineJoinRound"
#define CANVAS__lineJoinBevel     CANVAS_PREFIX"lineJoinBevel"

// Input properties and attributes

#define CANVAS__mouseButtonLeft   CANVAS_PREFIX"mouseButtonLeft"
#define CANVAS__mouseButtonMiddle CANVAS_PREFIX"mouseButtonMiddle"
#define CANVAS__mouseButtonRight  CANVAS_PREFIX"mouseButtonRight"
#define CANVAS__mouseWheelX       CANVAS_PREFIX"mouseWheelX"
#define CANVAS__mouseWheelY       CANVAS_PREFIX"mouseWheelY"
#define CANVAS__mousePositionX    CANVAS_PREFIX"mousePositionX"
#define CANVAS__mousePositionY    CANVAS_PREFIX"mousePositionY"
#define CANVAS__mouseFocus        CANVAS_PREFIX"mouseFocus"

typedef struct _LV2_Canvas_URID LV2_Canvas_URID;

struct _LV2_Canvas_URID  {
	LV2_URID Canvas_graph;
	LV2_URID Canvas_body;
	LV2_URID Canvas_aspectRatio;

	LV2_URID Canvas_BeginPath;
	LV2_URID Canvas_ClosePath;
	LV2_URID Canvas_Arc;
	LV2_URID Canvas_CurveTo;
	LV2_URID Canvas_LineTo;
	LV2_URID Canvas_MoveTo;
	LV2_URID Canvas_Rectangle;
	LV2_URID Canvas_PolyLine;
	LV2_URID Canvas_Style;
	LV2_URID Canvas_LineWidth;
	LV2_URID Canvas_LineDash;
	LV2_URID Canvas_LineCap;
	LV2_URID Canvas_LineJoin;
	LV2_URID Canvas_MiterLimit;
	LV2_URID Canvas_Stroke;
	LV2_URID Canvas_Fill;
	LV2_URID Canvas_Clip;
	LV2_URID Canvas_Save;
	LV2_URID Canvas_Restore;
	LV2_URID Canvas_Translate;
	LV2_URID Canvas_Scale;
	LV2_URID Canvas_Rotate;
	LV2_URID Canvas_Transform;
	LV2_URID Canvas_Reset;
	LV2_URID Canvas_FontSize;
	LV2_URID Canvas_FillText;

	LV2_URID Canvas_lineCapButt;
	LV2_URID Canvas_lineCapRound;
	LV2_URID Canvas_lineCapSquare;

	LV2_URID Canvas_lineJoinMiter;
	LV2_URID Canvas_lineJoinRound;
	LV2_URID Canvas_lineJoinBevel;

	LV2_URID Canvas_mouseButtonLeft;
	LV2_URID Canvas_mouseButtonMiddle;
	LV2_URID Canvas_mouseButtonRight;
	LV2_URID Canvas_mouseWheelX;
	LV2_URID Canvas_mouseWheelY;
	LV2_URID Canvas_mousePositionX;
	LV2_URID Canvas_mousePositionY;
	LV2_URID Canvas_mouseFocus;

	LV2_Atom_Forge forge;
};

static inline void
lv2_canvas_urid_init(LV2_Canvas_URID *urid, LV2_URID_Map *map)
{
	urid->Canvas_graph = map->map(map->handle, CANVAS__graph);
	urid->Canvas_body = map->map(map->handle, CANVAS__body);
	urid->Canvas_aspectRatio = map->map(map->handle, CANVAS__aspectRatio);

	urid->Canvas_BeginPath = map->map(map->handle, CANVAS__BeginPath);
	urid->Canvas_ClosePath = map->map(map->handle, CANVAS__ClosePath);
	urid->Canvas_Arc = map->map(map->handle, CANVAS__Arc);
	urid->Canvas_CurveTo = map->map(map->handle, CANVAS__CurveTo);
	urid->Canvas_LineTo = map->map(map->handle, CANVAS__LineTo);
	urid->Canvas_MoveTo = map->map(map->handle, CANVAS__MoveTo);
	urid->Canvas_Rectangle = map->map(map->handle, CANVAS__Rectangle);
	urid->Canvas_PolyLine = map->map(map->handle, CANVAS__PolyLine);
	urid->Canvas_Style = map->map(map->handle, CANVAS__Style);
	urid->Canvas_LineWidth = map->map(map->handle, CANVAS__LineWidth);
	urid->Canvas_LineDash = map->map(map->handle, CANVAS__LineDash);
	urid->Canvas_LineCap = map->map(map->handle, CANVAS__LineCap);
	urid->Canvas_LineJoin = map->map(map->handle, CANVAS__LineJoin);
	urid->Canvas_MiterLimit = map->map(map->handle, CANVAS__MiterLimit);
	urid->Canvas_Stroke = map->map(map->handle, CANVAS__Stroke);
	urid->Canvas_Fill = map->map(map->handle, CANVAS__Fill);
	urid->Canvas_Clip = map->map(map->handle, CANVAS__Clip);
	urid->Canvas_Save = map->map(map->handle, CANVAS__Save);
	urid->Canvas_Restore = map->map(map->handle, CANVAS__Restore);
	urid->Canvas_Translate = map->map(map->handle, CANVAS__Translate);
	urid->Canvas_Scale = map->map(map->handle, CANVAS__Scale);
	urid->Canvas_Rotate = map->map(map->handle, CANVAS__Rotate);
	urid->Canvas_Transform = map->map(map->handle, CANVAS__Transform);
	urid->Canvas_Reset = map->map(map->handle, CANVAS__Reset);
	urid->Canvas_FontSize = map->map(map->handle, CANVAS__FontSize);
	urid->Canvas_FillText = map->map(map->handle, CANVAS__FillText);

	urid->Canvas_lineCapButt = map->map(map->handle, CANVAS__lineCapButt);
	urid->Canvas_lineCapRound = map->map(map->handle, CANVAS__lineCapRound);
	urid->Canvas_lineCapSquare = map->map(map->handle, CANVAS__lineCapSquare);

	urid->Canvas_lineJoinMiter = map->map(map->handle, CANVAS__lineJoinMiter);
	urid->Canvas_lineJoinRound = map->map(map->handle, CANVAS__lineJoinRound);
	urid->Canvas_lineJoinBevel = map->map(map->handle, CANVAS__lineJoinBevel);

	urid->Canvas_mouseButtonLeft = map->map(map->handle, CANVAS__mouseButtonLeft);
	urid->Canvas_mouseButtonMiddle = map->map(map->handle, CANVAS__mouseButtonMiddle);
	urid->Canvas_mouseButtonRight = map->map(map->handle, CANVAS__mouseButtonRight);
	urid->Canvas_mouseWheelX = map->map(map->handle, CANVAS__mouseWheelX);
	urid->Canvas_mouseWheelY = map->map(map->handle, CANVAS__mouseWheelY);
	urid->Canvas_mousePositionX = map->map(map->handle, CANVAS__mousePositionX);
	urid->Canvas_mousePositionY = map->map(map->handle, CANVAS__mousePositionY);
	urid->Canvas_mouseFocus = map->map(map->handle, CANVAS__mouseFocus);

	lv2_atom_forge_init(&urid->forge, map);
}

#ifdef __cplusplus
}
#endif

#endif // _LV2_CANVAS_H
