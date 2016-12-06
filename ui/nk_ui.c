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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <inttypes.h>

#include <moony.h>

#define NK_PUGL_IMPLEMENTATION
#include "nk_pugl/nk_pugl.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern int luaopen_lpeg(lua_State *L);

#ifdef Bool
#	undef Bool // Xlib.h interferes with LV2_Atom_Forge.Bool
#endif

#define RDFS_PREFIX   "http://www.w3.org/2000/01/rdf-schema#"

#define RDFS__label   RDFS_PREFIX"label"
#define RDFS__range   RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

typedef union _body_t body_t;
typedef struct _prop_t prop_t;
typedef struct _plughandle_t plughandle_t;

union _body_t {
	int32_t i;
	int64_t h;
	uint32_t u;
	float f;
	double d;
	struct nk_text_edit editor;
};

struct _prop_t {
	LV2_URID key;
	LV2_URID range;
	LV2_URID unit;
	char *label;
	char *comment;
	body_t value;
	body_t minimum;
	body_t maximum;
};

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	nk_pugl_window_t win;

	LV2UI_Controller *controller;
	LV2UI_Write_Function writer;

	uint32_t control;
	uint32_t notify;
	LV2_URID atom_eventTransfer;
	LV2_URID patch_self;
	LV2_URID patch_Get;
	LV2_URID patch_Set;
	LV2_URID patch_Patch;
	LV2_URID patch_writable;
	LV2_URID patch_readable;
	LV2_URID patch_add;
	LV2_URID patch_remove;
	LV2_URID patch_wildcard;
	LV2_URID patch_property;
	LV2_URID patch_value;
	LV2_URID patch_subject;
	LV2_URID moony_code;
	LV2_URID moony_selection;
	LV2_URID moony_error;
	LV2_URID moony_trace;
	LV2_URID rdfs_label;
	LV2_URID rdfs_range;
	LV2_URID rdfs_comment;
	LV2_URID lv2_minimum;
	LV2_URID lv2_maximum;
	LV2_URID units_unit;

	atom_ser_t ser;

	float dy;
	bool code_hidden;
	bool prop_hidden;

	char code [MOONY_MAX_CHUNK_LEN];
	struct nk_text_edit editor;
	char *clipboard;

	char error [MOONY_MAX_ERROR_LEN];
	int error_sz;

	int n_trace;
	char **traces;

	int n_writable;
	prop_t *writables;
	int n_readable;
	prop_t *readables;

	lua_State *L;
};

static prop_t *
_prop_get(prop_t **properties, int *n_properties, LV2_URID key)
{
	for(int p = 0; p < *n_properties; p++)
	{
		prop_t *prop = &(*properties)[p];

		if(prop->key == key)
			return prop;
	}

	return NULL;
}

static int
_cmp_r(const void *a, const void *b, void *data)
{
	LV2_URID_Unmap *unmap = data;
	const prop_t *A = a;
	const prop_t *B = b;
	const char *alpha = unmap->unmap(unmap->handle, A->key);
	const char *beta = unmap->unmap(unmap->handle, B->key);

	return strcmp(alpha, beta);
}

static prop_t *
_prop_get_or_add(plughandle_t *handle, prop_t **properties, int *n_properties, LV2_URID key)
{
	prop_t *prop = _prop_get(properties, n_properties, key);
	if(prop)
		return prop;

	*properties = realloc(*properties, (*n_properties + 1)*sizeof(prop_t));
	prop = &(*properties)[*n_properties];
	memset(prop, 0x0, sizeof(prop_t));
	prop->key = key;
	*n_properties += 1;

	// sort properties according to URI string comparison
	qsort_r(*properties, *n_properties, sizeof(prop_t), _cmp_r, handle->unmap);

	return _prop_get(properties, n_properties, key);
}

static void
_prop_free(plughandle_t *handle, prop_t *prop)
{
	if(prop->label)
		free(prop->label);

	if(prop->comment)
		free(prop->comment);

	if(  (prop->range == handle->forge.String)
		|| (prop->range == handle->forge.URID) )
	{
		nk_textedit_free(&prop->value.editor);
	}
}

LV2_Atom_Forge_Ref
_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
{
	atom_ser_t *ser = handle;

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;

	const uint32_t new_offset = ser->offset + size;
	if(new_offset > ser->size)
	{
		uint32_t new_size = ser->size << 1;
		while(new_offset > new_size)
			new_size <<= 1;

		if(!(ser->buf = realloc(ser->buf, new_size)))
			return 0; // realloc failed

		ser->size = new_size;
	}

	memcpy(ser->buf + ser->offset, buf, size);
	ser->offset = new_offset;

	return ref;
}

LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

static void
_patch_get(plughandle_t *handle, LV2_URID property)
{
	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Get);
	if(property)
	{
		lv2_atom_forge_key(forge, handle->patch_property);
		lv2_atom_forge_urid(forge, property);
	}
	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_patch_set(plughandle_t *handle, LV2_URID property, uint32_t size, LV2_URID type, const void *body)
{
	LV2_Atom_Forge *forge = &handle->forge;
	atom_ser_t *ser = &handle->ser;

	ser->offset = 0;
	lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_object(forge, &frame, 0, handle->patch_Set);

	lv2_atom_forge_key(forge, handle->patch_property);
	lv2_atom_forge_urid(forge, property);

	lv2_atom_forge_key(forge, handle->patch_value);
	lv2_atom_forge_atom(forge, size, type);
	lv2_atom_forge_write(forge, body, size);

	lv2_atom_forge_pop(forge, &frame);

	const uint32_t sz = lv2_atom_total_size(ser->atom);
	handle->writer(handle->controller, handle->control, sz, handle->atom_eventTransfer, ser->atom);
}

static void
_clear_error(plughandle_t *handle)
{
	handle->error[0] = '\0';
}

static void
_submit_selection(plughandle_t *handle, const char *sel, uint32_t sz)
{
	_clear_error(handle);

	_patch_set(handle, handle->moony_selection,
		sz, handle->forge.String, sel);
}

static void
_submit_all(plughandle_t *handle)
{
	_clear_error(handle);

	struct nk_str *str = &handle->editor.string;
	_patch_set(handle, handle->moony_code,
		nk_str_len_char(str), handle->forge.String, nk_str_get_const(str));
}

static void
_submit_line(plughandle_t *handle)
{
	uint32_t newlines = 0;
	uint32_t from = 0;
	for(int i = 0; i < handle->editor.cursor; i++) // count newlines
	{
		if(handle->code[i] == '\n')
		{
			newlines += 1;
			from = i + 1;
		}
	}

	// create selection with prefixed newlines
	struct nk_str *str = &handle->editor.string;
	const char *code = nk_str_get_const(str);
	const int code_sz  = nk_str_len_char(str);
	char *end = memchr(code + from, '\n', code_sz);
	const uint32_t to = end ? end - code : code_sz;
	const uint32_t len = to - from;
	if(len > 0)
	{
		const uint32_t sz = newlines + len + 1;
		char *sel = calloc(sz, sizeof(char));
		if(sel)
		{
			memset(sel, '\n', newlines);
			memcpy(sel + newlines, code + from, len);

			_submit_selection(handle, sel, sz);

			free(sel);
		}
	}
}

static void
_submit_sel(plughandle_t *handle)
{
	struct nk_str *str = &handle->editor.string;
	const char *code = nk_str_get_const(str);

	const uint32_t len = handle->editor.select_end - handle->editor.select_start;
	if(len > 0)
	{
		uint32_t newlines = 0;
		for(int i = 0; i < handle->editor.select_start; i++) // count newlines
		{
			if(code[i] == '\n')
				newlines += 1;
		}

		const uint32_t sz = newlines + len + 1;
		char *sel = calloc(sz, sizeof(char));
		if(sel)
		{
			memset(sel, '\n', newlines);
			memcpy(sel + newlines, code + handle->editor.select_start, len);

			_submit_selection(handle, sel, sz);

			free(sel);
		}
	}
}

static void
_submit_line_or_sel(plughandle_t *handle)
{
	if(handle->editor.select_end == handle->editor.select_start)
		_submit_line(handle);
	else
		_submit_sel(handle);
}

static void
_clear_log(plughandle_t *handle)
{
	for(int i = 0; i < handle->n_trace; i++)
		free(handle->traces[i]);
	free(handle->traces);

	handle->n_trace = 0;
	handle->traces = NULL;
}

static int
_dial_bool(struct nk_context *ctx, int32_t *val)
{
	const int32_t tmp = *val;
	struct nk_rect bounds = nk_layout_space_bounds(ctx);
	const enum nk_widget_layout_states layout_states = nk_widget(&bounds, ctx);

	if(layout_states != NK_WIDGET_INVALID)
	{
		enum nk_widget_states states = NK_WIDGET_STATE_INACTIVE;

		if(layout_states == NK_WIDGET_VALID)
		{
			struct nk_input *in = &ctx->input;

			const bool left_mouse_click_in_cursor = nk_input_has_mouse_click_down_in_rect(in,
				NK_BUTTON_LEFT, bounds, nk_true);
			bool mouse_has_scrolled = false;

			if(left_mouse_click_in_cursor)
			{
				states = NK_WIDGET_STATE_ACTIVED;
			}
			else if(nk_input_is_mouse_hovering_rect(in, bounds))
			{
				if(in->mouse.scroll_delta != 0.f) // has scrolling
				{
					mouse_has_scrolled = true;
					in->mouse.scroll_delta = 0.f;
				}

				states = NK_WIDGET_STATE_HOVER;
			}

			if(left_mouse_click_in_cursor || mouse_has_scrolled)
			{
				*val = !*val;
			}
		}

		const struct nk_style_item *fg = NULL;
		const struct nk_style_item *bg = NULL;

		switch(states)
		{
			case NK_WIDGET_STATE_HOVER:
			{
				bg = &ctx->style.progress.hover;
				fg = &ctx->style.progress.cursor_hover;
			}	break;
			case NK_WIDGET_STATE_ACTIVED:
			{
				bg = &ctx->style.progress.active;
				fg = &ctx->style.progress.cursor_active;
			}	break;
			default:
			{
				bg = &ctx->style.progress.normal;
				fg = &ctx->style.progress.cursor_normal;
			}	break;
		}

		struct nk_command_buffer *canv= nk_window_get_canvas(ctx);
		const float w2 = bounds.w/2;
		const float h2 = bounds.h/2;
		const float r1 = h2;
		const float r2 = r1 / 2;
		const float cx = bounds.x + w2;
		const float cy = bounds.y + h2;

		nk_fill_arc(canv, cx, cy, r2, 0.f, 2*M_PI, fg->data.color);
		nk_fill_arc(canv, cx, cy, r2 - 2, 0.f, 2*M_PI, ctx->style.window.background);
		nk_fill_arc(canv, cx, cy, r2 - 4, 0.f, 2*M_PI,
			*val ? fg->data.color : bg->data.color);
	}

	return tmp != *val;
}

static float
_dial_numeric_behavior(struct nk_context *ctx, struct nk_rect bounds,
	enum nk_widget_states *states, int *divider)
{
	struct nk_input *in = &ctx->input;

	const struct nk_mouse_button *btn = &in->mouse.buttons[NK_BUTTON_LEFT];;
	const bool left_mouse_down = btn->down;
	const bool left_mouse_click_in_cursor = nk_input_has_mouse_click_down_in_rect(in,
		NK_BUTTON_LEFT, bounds, nk_true);

	float dd = 0.f;
	if(left_mouse_down && left_mouse_click_in_cursor)
	{
		const float dx = in->mouse.delta.x;
		const float dy = in->mouse.delta.y;
		dd = fabs(dx) > fabs(dy) ? dx : -dy;

		*states = NK_WIDGET_STATE_ACTIVED;
	}
	else if(nk_input_is_mouse_hovering_rect(in, bounds))
	{
		if(in->mouse.scroll_delta != 0.f) // has scrolling
		{
			dd = in->mouse.scroll_delta;
			in->mouse.scroll_delta = 0.f;
		}

		*states = NK_WIDGET_STATE_HOVER;
	}

	if(nk_input_is_key_down(in, NK_KEY_CTRL))
		*divider *= 4;
	if(nk_input_is_key_down(in, NK_KEY_SHIFT))
		*divider *= 4;

	return dd;
}

static void
_dial_numeric_draw(struct nk_context *ctx, struct nk_rect bounds,
	enum nk_widget_states states, float perc)
{
	struct nk_command_buffer *canv= nk_window_get_canvas(ctx);
	const struct nk_style_item *bg = NULL;
	const struct nk_style_item *fg = NULL;

	switch(states)
	{
		case NK_WIDGET_STATE_HOVER:
		{
			bg = &ctx->style.progress.hover;
			fg = &ctx->style.progress.cursor_hover;
		}	break;
		case NK_WIDGET_STATE_ACTIVED:
		{
			bg = &ctx->style.progress.active;
			fg = &ctx->style.progress.cursor_active;
		}	break;
		default:
		{
			bg = &ctx->style.progress.normal;
			fg = &ctx->style.progress.cursor_normal;
		}	break;
	}

	const float w2 = bounds.w/2;
	const float h2 = bounds.h/2;
	const float r1 = h2;
	const float r2 = r1 / 2;
	const float cx = bounds.x + w2;
	const float cy = bounds.y + h2;
	const float aa = M_PI/6;
	const float a1 = M_PI/2 + aa;
	const float a2 = 2*M_PI + M_PI/2 - aa;
	const float a3 = a1 + (a2 - a1)*perc;

	nk_fill_arc(canv, cx, cy, r1, a1, a2, bg->data.color);
	nk_fill_arc(canv, cx, cy, r1, a1, a3, fg->data.color);
	nk_fill_arc(canv, cx, cy, r2, 0.f, 2*M_PI, ctx->style.window.background);
}

static int
_dial_double(struct nk_context *ctx, double min, double *val, double max, float mul)
{
	const double tmp = *val;
	struct nk_rect bounds = nk_layout_space_bounds(ctx);
	const enum nk_widget_layout_states layout_states = nk_widget(&bounds, ctx);

	if(layout_states != NK_WIDGET_INVALID)
	{
		enum nk_widget_states states = NK_WIDGET_STATE_INACTIVE;
		const double range = max - min;

		if(layout_states == NK_WIDGET_VALID)
		{
			int divider = 1;
			const float dd = _dial_numeric_behavior(ctx, bounds, &states, &divider);

			if(dd != 0.f) // update value
			{
				const double per_pixel_inc = mul * range / bounds.w / divider;

				*val += dd * per_pixel_inc;
				*val = NK_CLAMP(min, *val, max);
			}
		}

		const float perc = (*val - min) / range;
		_dial_numeric_draw(ctx, bounds, states, perc);
	}

	return tmp != *val;
}

static int
_dial_long(struct nk_context *ctx, int64_t min, int64_t *val, int64_t max, float mul)
{
	const int64_t tmp = *val;
	struct nk_rect bounds = nk_layout_space_bounds(ctx);
	const enum nk_widget_layout_states layout_states = nk_widget(&bounds, ctx);

	if(layout_states != NK_WIDGET_INVALID)
	{
		enum nk_widget_states states = NK_WIDGET_STATE_INACTIVE;
		const int64_t range = max - min;

		if(layout_states == NK_WIDGET_VALID)
		{
			int divider = 1;
			const float dd = _dial_numeric_behavior(ctx, bounds, &states, &divider);

			if(dd != 0.f) // update value
			{
				const double per_pixel_inc = mul * range / bounds.w / divider;

				const double diff = dd * per_pixel_inc;
				*val += diff < 0.0 ? floor(diff) : ceil(diff);
				*val = NK_CLAMP(min, *val, max);
			}
		}

		const float perc = (float)(*val - min) / range;
		_dial_numeric_draw(ctx, bounds, states, perc);
	}

	return tmp != *val;
}

static int
_dial_float(struct nk_context *ctx, float min, float *val, float max, float mul)
{
	double tmp = *val;
	const int res = _dial_double(ctx, min, &tmp, max, mul);
	*val = tmp;

	return res;
}

static int
_dial_int(struct nk_context *ctx, int32_t min, int32_t *val, int32_t max, float mul)
{
	int64_t tmp = *val;
	const int res = _dial_long(ctx, min, &tmp, max, mul);
	*val = tmp;

	return res;
}

static struct nk_token *
_lex(void *data, const char *code, int code_sz)
{
	lua_State *L = data;
	struct nk_token *tokens = NULL;
	const int top = lua_gettop(L);

	//printf("_lex: %i\n", code_sz);

	lua_getglobal(L, "lexer");
	lua_getfield(L, -1, "lex");
	lua_getglobal(L, "moony");
	lua_pushlstring(L, code, code_sz);
	if(lua_pcall(L, 2, 1, 0))
	{
		fprintf(stderr, "err: %s\n", lua_tostring(L, -1));
	}
	else
	{
		const int len = luaL_len(L, -1);

		tokens = calloc(len/2 + 1, sizeof(struct nk_token)); //FIXME use realloc
		if(tokens)
		{
			for(int i = 0; i < len; i += 2)
			{
				struct nk_token *token = &tokens[i/2];

				lua_rawgeti(L, -1, i + 1);
				const char *token_str = luaL_checkstring(L, -1);
				lua_pop(L, 1);

				//TODO cache colors
				if(!strcmp(token_str, "whitespace"))
					token->color = nk_rgb(0xdd, 0xdd, 0xdd);
				else if(!strcmp(token_str, "keyword"))
					token->color = nk_rgb(0x00, 0x69, 0x8f);
				else if(!strcmp(token_str, "function"))
					token->color = nk_rgb(0x00, 0xae, 0xef);
				else if(!strcmp(token_str, "constant"))
					token->color = nk_rgb(0xff, 0x66, 0x00);
				else if(!strcmp(token_str, "library"))
					token->color = nk_rgb(0x8d, 0xff, 0x0a);
				else if(!strcmp(token_str, "table"))
					token->color = nk_rgb(0x8d, 0xff, 0x0a);
				else if(!strcmp(token_str, "identifier"))
					token->color = nk_rgb(0xdd, 0xdd, 0xdd);
				else if(!strcmp(token_str, "string"))
					token->color = nk_rgb(0x58, 0xc5, 0x54);
				else if(!strcmp(token_str, "comment"))
					token->color = nk_rgb(0x55, 0x55, 0x55);
				else if(!strcmp(token_str, "number"))
					token->color = nk_rgb(0xfb, 0xfb, 0x00);
				else if(!strcmp(token_str, "label"))
					token->color = nk_rgb(0xfd, 0xc2, 0x51);
				else if(!strcmp(token_str, "binop"))
					token->color = nk_rgb(0xcc, 0x00, 0x00);
				else if(!strcmp(token_str, "operator"))
					token->color = nk_rgb(0xcc, 0x00, 0x00);
				else if(!strcmp(token_str, "brace"))
					token->color = nk_rgb(0xff, 0xff, 0xff);
				else
					token->color = nk_rgb(0xdd, 0xdd, 0xdd);

				lua_rawgeti(L, -1, i + 2);
				const int token_offset = luaL_checkinteger(L, -1);
				lua_pop(L, 1);

				token->offset = token_offset - 1;
			}

			struct nk_token *token = &tokens[len/2];
			token->offset = INT32_MAX;
		}
	}

	lua_settop(L, top);
	return tokens;
}

static void
_paste(nk_handle userdata, struct nk_text_edit* editor)
{
	plughandle_t *handle = userdata.ptr;

	if(handle->clipboard)
		nk_textedit_paste(editor, handle->clipboard, strlen(handle->clipboard));
}

static void
_copy(nk_handle userdata, const char *buf, int len)
{
	plughandle_t *handle = userdata.ptr;

	if(handle->clipboard)
		free(handle->clipboard);

	handle->clipboard = strndup(buf, len);
}

static bool
_tooltip_visible(struct nk_context *ctx)
{
	return nk_widget_has_mouse_click_down(ctx, NK_BUTTON_RIGHT, nk_true)
		|| (nk_widget_is_hovered(ctx) && nk_input_is_key_down(&ctx->input, NK_KEY_CTRL));
}

static void
_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	ctx->clip.paste = _paste;
	ctx->clip.copy = _copy;
	ctx->clip.userdata.ptr = handle;

	struct nk_input *in = &ctx->input;
	const float dy = handle->dy;
	const struct nk_vec2 window_padding = ctx->style.window.padding;
	const struct nk_vec2 group_padding = ctx->style.window.group_padding;
	const float header_h = 1*dy + 2*window_padding.y;
	const float footer_h = 1*dy + 2*window_padding.y;
	const float body_h = wbounds.h - header_h - footer_h;
	const float header_height = ctx->style.font->height + 2*ctx->style.window.header.padding.y
		+ 2*ctx->style.window.header.label_padding.y;
	const float prop_h = header_height + dy*4 + group_padding.y*3 + 2*ctx->style.window.border;

	if(nk_begin(ctx, "Moony", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);

		bool has_enter = false;
		if(  nk_input_is_key_pressed(in, NK_KEY_ENTER)
			&& nk_input_is_key_down(in, NK_KEY_SHIFT) )
		{
			_submit_all(handle);
			has_enter = true;
		}

		nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
		{
			nk_layout_row_push(ctx, 0.6);
			handle->code_hidden = !nk_select_label(ctx, "Editor", NK_TEXT_LEFT, !handle->code_hidden);

			nk_layout_row_push(ctx, 0.4);
			handle->prop_hidden = !nk_select_label(ctx, "Parameters", NK_TEXT_LEFT, !handle->prop_hidden);
		}

		nk_layout_row_begin(ctx, NK_DYNAMIC, body_h, !handle->code_hidden + !handle->prop_hidden);
		{
			if(!handle->code_hidden)
			{
				nk_layout_row_push(ctx, handle->prop_hidden ? 1.0 : 0.6);
				if(nk_group_begin(ctx, "Logic", NK_WINDOW_NO_SCROLLBAR))
				{
					const float editor_h = body_h - 1*dy - 4*group_padding.y;
					struct nk_str *str = &handle->editor.string;
					const int old_len = nk_str_len_char(str);

					nk_layout_row_dynamic(ctx, editor_h*0.9, 1);
					nk_flags flags = NK_EDIT_BOX;
					if(has_enter)
						flags |= NK_EDIT_SIG_ENTER;
					const nk_flags state = nk_edit_buffer(ctx, flags,
						&handle->editor, nk_filter_default);

					if(old_len != nk_str_len_char(str))
					{
						handle->editor.lexer.needs_refresh = 1;
					}

					nk_layout_row_dynamic(ctx, editor_h*0.1, 1);
					struct nk_list_view lview;
					if(nk_list_view_begin(ctx, &lview, "Traces", NK_WINDOW_BORDER, dy, handle->n_trace))
					{
						nk_layout_row_dynamic(ctx, dy, 1);
						for(int l = lview.begin; (l < lview.end) && (l < handle->n_trace); l++)
						{
							nk_labelf(ctx, NK_TEXT_LEFT, "%i: %s", l + 1, handle->traces[l]);
						}

						nk_list_view_end(&lview);
					}

					nk_layout_row_dynamic(ctx, dy, 3);
					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Shift-Enter");
					if(nk_button_label(ctx, "Run all"))
						_submit_all(handle);

					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Ctrl-Shift-Enter");
					if(nk_button_label(ctx, "Run line/sel."))
						_submit_line_or_sel(handle);

					if(_tooltip_visible(ctx))
						nk_tooltip(ctx, "Ctrl-L");
					if(nk_button_label(ctx, "Clear log"))
						_clear_log(handle);

					nk_group_end(ctx);
				}
			}

			if(!handle->prop_hidden)
			{
				nk_layout_row_push(ctx, handle->code_hidden ? 1.0 : 0.4);
				if(nk_group_begin(ctx, "Properties", 0))
				{
					const int ncol = handle->code_hidden ? 6 : 3;

					nk_layout_row_dynamic(ctx, prop_h, ncol);
					for(int p = 0; p < handle->n_writable; p++)
					{
						prop_t *prop = &handle->writables[p];
						if(!prop->key || !prop->range || !prop->label) // marked for removal
							continue;

						const char *lab = "#";

						if(prop->comment && _tooltip_visible(ctx))
							nk_tooltip(ctx, prop->comment);
						if(nk_group_begin(ctx, prop->label, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
						{
							if(prop->range == handle->forge.Int)
							{
								nk_layout_row_dynamic(ctx, dy*3, 1);
								if(_dial_int(ctx, prop->minimum.i, &prop->value.i, prop->maximum.i, 1.f))
								{
									_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &prop->value.i);
								}

								nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
								nk_layout_row_push(ctx, 0.9);
								const int32_t val = nk_propertyi(ctx, lab,
									prop->minimum.i, prop->value.i, prop->maximum.i, 1.f, 0.f);
								if(val != prop->value.i)
								{
									prop->value.i = val;
									_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &val);
								}
								nk_layout_row_push(ctx, 0.1);
								nk_label(ctx, "dB", NK_TEXT_RIGHT);
							}
							else if(prop->range == handle->forge.Long)
							{
								nk_layout_row_dynamic(ctx, dy*3, 1);
								if(_dial_long(ctx, prop->minimum.h, &prop->value.h, prop->maximum.h, 1.f))
								{
									_patch_set(handle, prop->key, sizeof(int64_t), prop->range, &prop->value.h);
								}

								nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
								nk_layout_row_push(ctx, 0.9);
								const int64_t val = nk_propertyi(ctx, lab,
									prop->minimum.h, prop->value.h, prop->maximum.h, 1.f, 0.f);
								if(val != prop->value.h)
								{
									prop->value.h = val;
									_patch_set(handle, prop->key, sizeof(int64_t), prop->range, &val);
								}
								nk_layout_row_push(ctx, 0.1);
								nk_label(ctx, "dB", NK_TEXT_RIGHT);
							}
							else if(prop->range == handle->forge.Float)
							{
								nk_layout_row_dynamic(ctx, dy*3, 1);
								if(_dial_float(ctx, prop->minimum.f, &prop->value.f, prop->maximum.f, 1.f))
								{
									_patch_set(handle, prop->key, sizeof(float), prop->range, &prop->value.f);
								}

								nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
								nk_layout_row_push(ctx, 0.9);
								const float val = nk_propertyf(ctx, lab,
									prop->minimum.f, prop->value.f, prop->maximum.f, 0.05f, 0.f);
								if(val != prop->value.f)
								{
									prop->value.f = val;
									_patch_set(handle, prop->key, sizeof(float), prop->range, &val);
								}
								nk_layout_row_push(ctx, 0.1);
								nk_label(ctx, "dB", NK_TEXT_RIGHT);
							}
							else if(prop->range == handle->forge.Double)
							{
								nk_layout_row_dynamic(ctx, dy*3, 1);
								if(_dial_double(ctx, prop->minimum.d, &prop->value.d, prop->maximum.d, 1.f))
								{
									_patch_set(handle, prop->key, sizeof(double), prop->range, &prop->value.d);
								}

								nk_layout_row_begin(ctx, NK_DYNAMIC, dy, 2);
								nk_layout_row_push(ctx, 0.9);
								const double val = nk_propertyd(ctx, lab,
									prop->minimum.d, prop->value.d, prop->maximum.d, 0.05f, 0.f);
								if(val != prop->value.d)
								{
									prop->value.d = val;
									_patch_set(handle, prop->key, sizeof(double), prop->range, &val);
								}
								nk_layout_row_push(ctx, 0.1);
								nk_label(ctx, "dB", NK_TEXT_RIGHT);
							}
							else if(prop->range == handle->forge.Bool)
							{
								nk_layout_row_dynamic(ctx, dy*3, 1);
								if(_dial_bool(ctx, &prop->value.i))
								{
									_patch_set(handle, prop->key, sizeof(int32_t), prop->range, &prop->value.i);
								}
							}
							else if(prop->range == handle->forge.URID)
							{
								nk_layout_row_dynamic(ctx, dy*4, 1);
								const nk_flags state = nk_edit_buffer(ctx,
									NK_EDIT_BOX | NK_EDIT_SIG_ENTER, &prop->value.editor, nk_filter_default);
								if(state & NK_EDIT_COMMITED)
								{
									struct nk_str *str = &prop->value.editor.string;
									LV2_URID urid = 0;
									char *uri = strndup(nk_str_get_const(str), nk_str_len_char(str));
									if(uri)
									{
										urid = handle->map->map(handle->map->handle, uri);
										free(uri);
									}
									if(urid)
										_patch_set(handle, prop->key, sizeof(uint32_t), prop->range, &urid);
								}
							}
							else if(prop->range == handle->forge.String)
							{
								nk_layout_row_dynamic(ctx, dy*4, 1);
								const nk_flags state = nk_edit_buffer(ctx,
									NK_EDIT_BOX | NK_EDIT_SIG_ENTER, &prop->value.editor, nk_filter_default);
								if(state & NK_EDIT_COMMITED)
								{
									struct nk_str *str = &prop->value.editor.string;
									_patch_set(handle, prop->key, nk_str_len_char(str), prop->range, nk_str_get_const(str));
								}
							}
							else if(prop->range == handle->forge.Chunk)
							{
								nk_layout_row_dynamic(ctx, dy*3, 1);
								if(nk_button_label(ctx, "Load"))
								{
									//FIXME load file selector
								}
								nk_layout_row_dynamic(ctx, dy, 1);
								nk_labelf(ctx, NK_TEXT_RIGHT, "%"PRIu32" bytes", prop->value.u);
							}
							else
							{
								nk_layout_row_dynamic(ctx, dy, 1);
								nk_label(ctx, prop->label, NK_TEXT_LEFT);
							}

							nk_group_end(ctx);
						}
					}
					for(int p = 0; p < handle->n_readable; p++)
					{
						prop_t *prop = &handle->readables[p];
						if(!prop->key || !prop->range || !prop->label) // marked for removal
							continue;

						nk_button_label(ctx, prop->label);
						//FIXME
					}

					nk_group_end(ctx);
				}
			}
		}
		nk_layout_row_end(ctx);

		nk_layout_row_dynamic(ctx, dy, 2);
		{
			if(handle->error[0] == 0)
				nk_spacing(ctx, 1);
			else
				nk_labelf_colored(ctx, NK_TEXT_LEFT, nk_yellow, "Stopped: %s", handle->error);
			nk_label(ctx, "Moony.lv2: "MOONY_VERSION, NK_TEXT_RIGHT);
		}
	}
	nk_end(ctx);
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	void *parent = NULL;
	LV2UI_Resize *host_resize = NULL;
	LV2UI_Port_Map *port_map = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
			host_resize = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			port_map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			handle->unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			handle->log = features[i]->data;
	}

	if(!parent)
	{
		fprintf(stderr,
			"%s: Host does not support ui:parent\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!port_map)
	{
		fprintf(stderr,
			"%s: Host does not support ui:portMap\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!handle->map || !handle->unmap)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map/unmap\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	if(handle->log)
		lv2_log_logger_init(&handle->logger, handle->map, handle->log);

	lv2_atom_forge_init(&handle->forge, handle->map);

	handle->controller = controller;
	handle->writer = write_function;

	handle->control = port_map->port_index(port_map->handle, "control");
	handle->notify = port_map->port_index(port_map->handle, "notify");

	handle->atom_eventTransfer = handle->map->map(handle->map->handle, LV2_ATOM__eventTransfer);

	handle->patch_self = handle->map->map(handle->map->handle, plugin_uri);
	handle->patch_Get = handle->map->map(handle->map->handle, LV2_PATCH__Get);
	handle->patch_Set = handle->map->map(handle->map->handle, LV2_PATCH__Set);
	handle->patch_Patch = handle->map->map(handle->map->handle, LV2_PATCH__Patch);
	handle->patch_writable = handle->map->map(handle->map->handle, LV2_PATCH__writable);
	handle->patch_readable = handle->map->map(handle->map->handle, LV2_PATCH__readable);
	handle->patch_add = handle->map->map(handle->map->handle, LV2_PATCH__add);
	handle->patch_remove = handle->map->map(handle->map->handle, LV2_PATCH__remove);
	handle->patch_wildcard = handle->map->map(handle->map->handle, LV2_PATCH__wildcard);
	handle->patch_property = handle->map->map(handle->map->handle, LV2_PATCH__property);
	handle->patch_value = handle->map->map(handle->map->handle, LV2_PATCH__value);
	handle->patch_subject = handle->map->map(handle->map->handle, LV2_PATCH__subject);
	handle->moony_code = handle->map->map(handle->map->handle, MOONY_CODE_URI);
	handle->moony_selection = handle->map->map(handle->map->handle, MOONY_SELECTION_URI);
	handle->moony_error = handle->map->map(handle->map->handle, MOONY_ERROR_URI);
	handle->moony_trace = handle->map->map(handle->map->handle, MOONY_TRACE_URI);
	handle->rdfs_label = handle->map->map(handle->map->handle, RDFS__label);
	handle->rdfs_range= handle->map->map(handle->map->handle, RDFS__range);
	handle->rdfs_comment = handle->map->map(handle->map->handle, RDFS__comment);
	handle->lv2_minimum = handle->map->map(handle->map->handle, LV2_CORE__minimum);
	handle->lv2_maximum = handle->map->map(handle->map->handle, LV2_CORE__maximum);
	handle->units_unit = handle->map->map(handle->map->handle, LV2_UNITS__unit);

	const char *NK_SCALE = getenv("NK_SCALE");
	const float scale = NK_SCALE ? atof(NK_SCALE) : 1.f;
	handle->dy = 20.f * scale;

	nk_pugl_config_t *cfg = &handle->win.cfg;
	cfg->width = 960 * scale;
	cfg->height = 960 * scale;
	cfg->resizable = true;
	cfg->ignore = false;
	cfg->class = "tracker";
	cfg->title = "Tracker";
	cfg->parent = (intptr_t)parent;
	cfg->data = handle;
	cfg->expose = _expose;

	char *path;
	if(asprintf(&path, "%sCousine-Regular.ttf", bundle_path) == -1)
		path = NULL;

	cfg->font.face = path;
	cfg->font.size = 13 * scale;
	
	*(intptr_t *)widget = nk_pugl_init(&handle->win);
	nk_pugl_show(&handle->win);

	if(path)
		free(path);

	if(host_resize)
		host_resize->ui_resize(host_resize->handle, cfg->width, cfg->height);

	handle->ser.moony = NULL;
	handle->ser.size = 1024;
	handle->ser.buf = malloc(1024);
	handle->ser.offset = 0;

	nk_textedit_init_fixed(&handle->editor, handle->code, MOONY_MAX_CHUNK_LEN);

	_patch_get(handle, handle->moony_code);
	_patch_get(handle, handle->moony_error);
	_patch_get(handle, 0);

	handle->L = luaL_newstate();
	if(handle->L)
	{
		luaL_requiref(handle->L, "base", luaopen_base, 0);
		luaL_requiref(handle->L, "coroutine", luaopen_coroutine, 1);
		luaL_requiref(handle->L, "table", luaopen_table, 1);
		luaL_requiref(handle->L, "string", luaopen_string, 1);
		luaL_requiref(handle->L, "math", luaopen_math, 1);
		luaL_requiref(handle->L, "utf8", luaopen_utf8, 1);
		luaL_requiref(handle->L, "debug", luaopen_debug, 1);
		luaL_requiref(handle->L, "lpeg", luaopen_lpeg, 1);
		lua_pop(handle->L, 8);

		//luaL_requiref(handle->L, "io", luaopen_io, 1);
		//luaL_requiref(handle->L, "os", luaopen_os, 1);
		//luaL_requiref(handle->L, "bit32", luaopen_bit32, 1);
		luaL_requiref(handle->L, "package", luaopen_package, 1);

		// automatic garbage collector
		lua_gc(handle->L, LUA_GCRESTART, 0); // enable automatic garbage collection
		lua_gc(handle->L, LUA_GCSETPAUSE, 105); // next step when memory increased by 5%
		lua_gc(handle->L, LUA_GCSETSTEPMUL, 105); // run 5% faster than memory allocation

		// set package.path
		lua_getglobal(handle->L, "package");
		lua_pushfstring(handle->L, "%s?.lua", bundle_path);
		lua_setfield(handle->L, -2, "path");
		lua_pop(handle->L, 1); // package

		path = NULL;
		if(asprintf(&path, "%s%s", bundle_path, "lexer.lua") != -1)
		{
			if(luaL_dofile(handle->L, path))
			{
				fprintf(stderr, "err: %s\n", lua_tostring(handle->L, -1));
				lua_pop(handle->L, 1); // err
			}
			else
			{
				lua_setglobal(handle->L, "lexer");
			}
			free(path);
		}

		lua_getglobal(handle->L, "lexer");
		lua_getfield(handle->L, -1, "load");
		lua_pushstring(handle->L, "moony");
		if(lua_pcall(handle->L, 1, 1, 0))
		{
			fprintf(stderr, "err: %s\n", lua_tostring(handle->L, -1));
			lua_pop(handle->L, 1);
		}
		else
		{
			lua_setglobal(handle->L, "moony");
		}
		lua_pop(handle->L, 1); // lexer
	}

	handle->editor.lexer.lex = _lex;
	handle->editor.lexer.data = handle->L;

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	if(handle->clipboard)
		free(handle->clipboard);

	if(handle->L)
		lua_close(handle->L);

	nk_textedit_free(&handle->editor);

	if(handle->editor.lexer.tokens)
		free(handle->editor.lexer.tokens);

	_clear_log(handle);

	for(int p = 0; p < handle->n_writable; p++)
		_prop_free(handle, &handle->writables[p]);
	if(handle->writables)
		free(handle->writables);

	for(int p = 0; p < handle->n_readable; p++)
		_prop_free(handle, &handle->readables[p]);
	if(handle->readables)
		free(handle->readables);

	if(handle->ser.buf)
		free(handle->ser.buf);

	nk_pugl_hide(&handle->win);
	nk_pugl_shutdown(&handle->win);

	free(handle);
}

static void
port_event(LV2UI_Handle instance, uint32_t index, uint32_t size,
	uint32_t protocol, const void *buf)
{
	plughandle_t *handle = instance;

	if( (index == handle->notify) && (protocol == handle->atom_eventTransfer) )
	{
		const LV2_Atom_Object *obj = buf;

		if(lv2_atom_forge_is_object_type(&handle->forge, obj->atom.type))
		{
			if(obj->body.otype == handle->patch_Set)
			{
				const LV2_Atom_URID *property = NULL;
				const LV2_Atom *value = NULL;

				lv2_atom_object_get(obj,
					handle->patch_property, &property,
					handle->patch_value, &value,
					0);

				if(  property && (property->atom.type == handle->forge.URID)
					&& value)
				{
					const char *body = LV2_ATOM_BODY_CONST(value);

					if(property->body == handle->moony_code)
					{
						if(value->size <= MOONY_MAX_CHUNK_LEN)
						{
							struct nk_str *str = &handle->editor.string;
							nk_str_clear(str);

							// replace tab with 2 spaces
							const char *end = body + value->size - 1;
							const char *from = body;
							for(const char *to = strchr(from, '\t');
								to;
								from = to + 1, to = strchr(from, '\t'))
							{
								nk_str_append_text_utf8(str, from, to-from);
								nk_str_append_text_utf8(str, "  ", 2);
							}
							nk_str_append_text_utf8(str, from, end-from);

							handle->editor.lexer.needs_refresh = 1;

							nk_pugl_post_redisplay(&handle->win);
						}
					}
					else if(property->body == handle->moony_trace)
					{
						if(value->size <= MOONY_MAX_CHUNK_LEN)
						{
							handle->traces = realloc(handle->traces, (handle->n_trace + 1)*sizeof(char *));
							handle->traces[handle->n_trace++] = strdup(body);

							nk_pugl_post_redisplay(&handle->win);
						}
					}
					else if(property->body == handle->moony_error)
					{
						if(value->size <= MOONY_MAX_CHUNK_LEN)
						{
							strncpy(handle->error, body, value->size);
							handle->error_sz = value->size - 1;

							nk_pugl_post_redisplay(&handle->win);
						}
					}
					else
					{
						prop_t *prop = _prop_get(&handle->readables, &handle->n_readable, property->body);
						if(!prop)
							prop = _prop_get(&handle->writables, &handle->n_writable, property->body);
						if(prop && (prop->range == value->type) )
						{
							if(prop->range == handle->forge.Int)
							{
								prop->value.i = ((const LV2_Atom_Int *)value)->body;
							}
							else if(prop->range == handle->forge.Long)
							{
								prop->value.h = ((const LV2_Atom_Long *)value)->body;
							}
							else if(prop->range == handle->forge.Float)
							{
								prop->value.f = ((const LV2_Atom_Float *)value)->body;
							}
							else if(prop->range == handle->forge.Double)
							{
								prop->value.d = ((const LV2_Atom_Double *)value)->body;
							}
							else if(prop->range == handle->forge.Bool)
							{
								prop->value.i = ((const LV2_Atom_Bool *)value)->body;
							}
							else if(prop->range == handle->forge.URID)
							{
								const LV2_URID urid = ((const LV2_Atom_URID *)value)->body;
								const char *uri = handle->unmap->unmap(handle->unmap->handle, urid);

								struct nk_str *str = &prop->value.editor.string;
								nk_str_clear(str);
								nk_str_append_text_utf8(str, uri, strlen(uri));
							}
							else if(prop->range == handle->forge.String)
							{
								struct nk_str *str = &prop->value.editor.string;
								nk_str_clear(str);
								nk_str_append_text_utf8(str, LV2_ATOM_BODY_CONST(value), value->size - 1);
							}
							else if(prop->range == handle->forge.Chunk)
							{
								prop->value.u = value->size; // store only chunk size
							}
							else
							{
								//FIXME
							}

							nk_pugl_post_redisplay(&handle->win);
						}
					}
				}
			}
			else if(obj->body.otype == handle->patch_Patch)
			{
				const LV2_Atom_URID *subj = NULL;
				const LV2_Atom_Object *add = NULL;
				const LV2_Atom_Object *rem = NULL;

				lv2_atom_object_get(obj,
					handle->patch_subject, &subj,
					handle->patch_add, &add,
					handle->patch_remove, &rem,
					0);

				if(  add && lv2_atom_forge_is_object_type(&handle->forge, add->atom.type)
					&& rem && lv2_atom_forge_is_object_type(&handle->forge, rem->atom.type) )
				{
					LV2_ATOM_OBJECT_FOREACH(rem, pro)
					{
						if(!subj || (subj->body == handle->patch_self))
						{
							const LV2_Atom_URID *property = (const LV2_Atom_URID *)&pro->value;

							if(  (pro->key == handle->patch_writable)
								&& (property->body == handle->patch_wildcard) )
							{
								//printf("clearing patch:writable\n");
								for(int p = 0; p < handle->n_writable; p++)
									_prop_free(handle, &handle->writables[p]);

								if(handle->writables)
									free(handle->writables);

								handle->writables = NULL;
								handle->n_writable = 0;
							}
							else if( (pro->key == handle->patch_readable)
								&& (property->body == handle->patch_wildcard) )
							{
								//printf("clearing patch:readable\n");
								for(int p = 0; p < handle->n_readable; p++)
									_prop_free(handle, &handle->readables[p]);

								if(handle->readables)
									free(handle->readables);

								handle->readables = NULL;
								handle->n_readable = 0;
							}
							else
							{
								//FIXME
							}
						}
						else
						{
							//FIXME
						}
					}

					LV2_ATOM_OBJECT_FOREACH(add, pro)
					{
						if(!subj || (subj->body == handle->patch_self))
						{
							const LV2_Atom_URID *property = (const LV2_Atom_URID *)&pro->value;

							if(pro->key == handle->patch_writable)
							{
								prop_t *prop = _prop_get_or_add(handle, &handle->writables, &handle->n_writable, property->body);
								(void)prop;
							}
							else if(pro->key == handle->patch_readable)
							{
								prop_t *prop = _prop_get_or_add(handle, &handle->readables, &handle->n_readable, property->body);
								(void)prop;
							}
						}
						else
						{
							prop_t *prop = _prop_get(&handle->readables, &handle->n_readable, subj->body);
							if(!prop)
								prop = _prop_get(&handle->writables, &handle->n_writable, subj->body);
							if(prop)
							{
								LV2_Atom_URID *range = NULL;
								LV2_Atom *label = NULL;
								LV2_Atom *comment = NULL;
								LV2_Atom *minimum = NULL;
								LV2_Atom *maximum = NULL;
								LV2_Atom_URID *unit = NULL;

								lv2_atom_object_get(add, 
									handle->rdfs_range, &range,
									handle->rdfs_label, &label,
									handle->rdfs_comment, &comment,
									handle->lv2_minimum, &minimum,
									handle->lv2_maximum, &maximum,
									handle->units_unit, &unit,
									0);

								if(range && (range->atom.type == handle->forge.URID))
								{
									prop->range = range->body;

									if( (prop->range == handle->forge.String)
										|| (prop->range == handle->forge.URID) )
									{
										nk_textedit_init_default(&prop->value.editor);
									}
								}

								if(label && (label->type == handle->forge.String))
									prop->label = strdup(LV2_ATOM_BODY_CONST(label));

								if(comment && (comment->type == handle->forge.String))
									prop->comment = strdup(LV2_ATOM_BODY_CONST(comment));

								if(unit && (unit->atom.type == handle->forge.URID))
									prop->unit = unit->body;

								if(minimum)
								{
									if(minimum->type == handle->forge.Int)
										prop->minimum.i = ((const LV2_Atom_Int *)(minimum))->body;
									else if(minimum->type == handle->forge.Long)
										prop->minimum.h = ((const LV2_Atom_Long *)(minimum))->body;
									else if(minimum->type == handle->forge.Float)
										prop->minimum.f = ((const LV2_Atom_Float *)(minimum))->body;
									else if(minimum->type == handle->forge.Double)
										prop->minimum.d = ((const LV2_Atom_Double *)(minimum))->body;
									//FIXME handle more types?
								}
								if(maximum)
								{
									if(maximum->type == handle->forge.Int)
										prop->maximum.i = ((const LV2_Atom_Int *)(maximum))->body;
									else if(maximum->type == handle->forge.Long)
										prop->maximum.h = ((const LV2_Atom_Long *)(maximum))->body;
									else if(maximum->type == handle->forge.Float)
										prop->maximum.f = ((const LV2_Atom_Float *)(maximum))->body;
									else if(maximum->type == handle->forge.Double)
										prop->maximum.d = ((const LV2_Atom_Double *)(maximum))->body;
									//FIXME handle more types?
								}
								//FIXME handle lv2:scalePoint
							}
						}
					}

					nk_pugl_post_redisplay(&handle->win);
				}
			}
		}
	}
}

static int
_idle(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	return nk_pugl_process_events(&handle->win);
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle
};

static const void *
ext_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
		
	return NULL;
}

const LV2UI_Descriptor nk_ui= {
	.URI            = MOONY_NK_URI,
	.instantiate    = instantiate,
	.cleanup        = cleanup,
	.port_event     = port_event,
	.extension_data = ext_data
};
