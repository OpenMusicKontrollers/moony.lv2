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

d2tk_state_t
_d2tk_base_tooltip_draw(d2tk_base_t *base, ssize_t lbl_len, const char *lbl,
	d2tk_coord_t h)
{
	const bool has_lbl = lbl_len && lbl;

	if(has_lbl && (lbl_len == -1) ) // zero terminated string
	{
		lbl_len = strlen(lbl);
	}

	const d2tk_style_t *style = d2tk_base_get_style(base);

	d2tk_core_t *core = base->core;

	const d2tk_coord_t w = d2tk_core_text_extent(core, lbl_len, lbl, h);

	d2tk_coord_t x, y, W, H;
	d2tk_base_get_mouse_pos(base, &x, &y);
	d2tk_base_get_dimensions(base, &W, &H);

	d2tk_clip_int32(w/2, &x, W - w/2);
	d2tk_clip_int32(h, &y, H);

	const d2tk_rect_t rect = D2TK_RECT(x - w/2, y - h, w, h);

	const d2tk_hash_dict_t dict [] = {
		{ &rect, sizeof(d2tk_rect_t) },
		{ style, sizeof(d2tk_style_t) },
		{ lbl, lbl_len },
		{ NULL, 0 }
	};
	const uint64_t hash = d2tk_hash_dict(dict);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_rect_t bnd;
		d2tk_rect_shrink(&bnd, &rect, style->padding);

		const d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		const size_t ref = d2tk_core_bbox_push(core, true, &rect);

		{
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &bnd);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);
		}

		{
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &bnd);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);
		}

		if(lbl_len > 0)
		{
			d2tk_core_save(core);
			d2tk_core_scissor(core, &bnd);
			d2tk_core_font_size(core, bnd.h);
			d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
			d2tk_core_color(core, style->text_stroke_color[triple]);
			d2tk_core_text(core, &bnd, lbl_len, lbl, D2TK_ALIGN_CENTERED);
			d2tk_core_restore(core);
		}

		d2tk_core_bbox_pop(core, ref);
	}

	return D2TK_STATE_NONE;
}

D2TK_API void
d2tk_base_set_tooltip(d2tk_base_t *base, ssize_t lbl_len, const char *lbl,
	d2tk_coord_t h)
{
	strncpy(base->tooltip.buf, lbl, lbl_len);
	base->tooltip.len = lbl_len;
	base->tooltip.h = h;
}

D2TK_API void
d2tk_base_clear_tooltip(d2tk_base_t *base)
{
	base->tooltip.len = 0;
}
