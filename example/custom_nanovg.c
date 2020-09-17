#include <stdint.h>
#include <math.h>

#include <d2tk/core.h>

#include <nanovg.h>

static void
_draw_custom(void *_ctx, const d2tk_rect_t *rect, const void *data)
{
	NVGcontext *ctx = _ctx;
	(void)data;

	d2tk_rect_t bnd = *rect;
	bnd.x += bnd.w/4;
	bnd.y += bnd.h/4;
	bnd.w /= 2;
	bnd.h /= 2;

	const NVGcolor col = nvgRGBA(0xff, 0xff, 0xff, 0x7f);

	nvgBeginPath(ctx);
	nvgRect(ctx, bnd.x, bnd.y, bnd.w, bnd.h);
	nvgFillColor(ctx, col);
	nvgFill(ctx);
}

d2tk_core_custom_t draw_custom = _draw_custom;
