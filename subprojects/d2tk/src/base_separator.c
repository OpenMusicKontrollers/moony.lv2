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

#include <string.h>

#include "base_internal.h"

D2TK_API d2tk_state_t
d2tk_base_separator(d2tk_base_t *base, const d2tk_rect_t *rect, d2tk_flag_t flag)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);

	const d2tk_hash_dict_t dict [] = {
		{ rect, sizeof(d2tk_rect_t) },
		{ style, sizeof(d2tk_style_t) },
		{ &flag, sizeof(d2tk_flag_t) },
		{ NULL, 0 }
	};
	const uint64_t hash = d2tk_hash_dict(dict);

	d2tk_core_t *core = base->core;

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		d2tk_coord_t x0, x1, y0, y1;

		if(flag & D2TK_FLAG_SEPARATOR_X)
		{
			x0 = x1 = rect->x + rect->w/2;
			y0 = rect->y;
			y1 = rect->y + rect->h;
		}
		else // D2TK_FLAG_SEPARATOR_Y
		{
			y0 = y1 = rect->y + rect->h/2;
			x0 = rect->x;
			x1 = rect->x + rect->w;
		}

		d2tk_core_begin_path(core);
		d2tk_core_move_to(core, x0, y0);
		d2tk_core_line_to(core, x1, y1);
		d2tk_core_color(core, style->stroke_color[triple]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_bbox_pop(core, ref);
	}

	return D2TK_STATE_NONE;
}
