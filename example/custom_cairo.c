#include <stdint.h>

#include <d2tk/core.h>

#include <cairo.h>

static void
_draw_custom(void *_ctx, const d2tk_rect_t *rect, const void *data)
{
	cairo_t *ctx = _ctx;
	(void)data;

	d2tk_rect_t bnd = *rect;
	bnd.x += bnd.w/4;
	bnd.y += bnd.h/4;
	bnd.w /= 2;
	bnd.h /= 2;

	cairo_new_sub_path(ctx);
	cairo_rectangle(ctx, bnd.x, bnd.y, bnd.w, bnd.h);
	cairo_set_source_rgba(ctx, 1.f, 1.f, 1.f, 0.5f);
	cairo_fill(ctx);
}

d2tk_core_custom_t draw_custom = _draw_custom;
