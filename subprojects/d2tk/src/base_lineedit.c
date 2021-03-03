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

#include <encodings/utf8.h>

#define FONT_CODE_REGULAR "FiraCode:regular"

typedef struct _d2tk_atom_body_lineedit_t d2tk_atom_body_lineedit_t;

struct _d2tk_atom_body_lineedit_t {
	atomic_uintptr_t filter;
	atomic_uintptr_t line;
	atomic_uintptr_t fill;
};

const size_t d2tk_atom_body_lineedit_sz = sizeof(d2tk_atom_body_lineedit_t);

static char *
_atomic_get(atomic_uintptr_t *ptr)
{
	uintptr_t desired = 0;
	char *old = (char *)atomic_exchange(ptr, desired);

	return old;
}

static void
_atomic_set(atomic_uintptr_t *ptr, const char *new)
{
	uintptr_t desired = new
		? (uintptr_t)strdup(new)
		: 0;
	char *old = (char *)atomic_exchange(ptr, desired);

	if(old)
	{
		free(old);
	}
}

static void
_atomic_clr(atomic_uintptr_t *ptr)
{
	_atomic_set(ptr, 0);
}

static int
_entry(void *data, int file_in, int file_out)
{
	static const char *prompt = "◆";
	d2tk_atom_body_lineedit_t *body = data;
	char entry [2048] = "";

	linenoiseApp *app = linenoiseAppNew(file_in, file_out);
	if(!app)
	{
		return 1;
	}

	linenoiseSetMultiLine(app, 0);
	linenoiseHistorySetMaxLen(app, 32);
	linenoiseSetFill(app, entry);

	d2tk_lineedit_filter_t *filter = (d2tk_lineedit_filter_t *)atomic_load(&body->filter);
	if(filter)
	{
		if(filter->completion_cb)
		{
			linenoiseSetCompletionCallback(app, filter->completion_cb);
		}

		if(filter->hints_cb)
		{
			linenoiseSetHintsCallback(app, filter->hints_cb);
		}

		if(filter->free_hints_cb)
		{
			linenoiseSetFreeHintsCallback(app, filter->free_hints_cb);
		}
	}

	linenoiseSetEncodingFunctions(app, linenoiseUtf8PrevCharLen,
		linenoiseUtf8NextCharLen, linenoiseUtf8ReadCode);

	char *fill = _atomic_get(&body->fill);
	if(fill)
	{
		snprintf(entry, sizeof(entry), "%s", fill);
		free(fill);
	}

	char *line;
	while( (line= linenoise(app, prompt)) )
	{
		_atomic_set(&body->line, line);
		snprintf(entry, sizeof(entry), "%s", line);

		linenoiseHistoryAdd(app, line);
		linenoiseFree(app, line);
		linenoiseClearScreen(app);
	}

	linenoiseAppFree(app);

	return 0;
}

static int
_lineedit_event(d2tk_atom_event_type_t event, void *data)
{
	d2tk_atom_body_lineedit_t *body = data;

	switch(event)
	{
		case D2TK_ATOM_EVENT_DEINIT:
		{
			_atomic_clr(&body->line);
			_atomic_clr(&body->fill);
		} break;

		case D2TK_ATOM_EVENT_FD:
			// fall-through
		case D2TK_ATOM_EVENT_NONE:
			// fall-through
		default:
		{
			// nothing to do
		} break;
	}

	return 0;
}

D2TK_API d2tk_state_t
d2tk_base_lineedit(d2tk_base_t *base, d2tk_id_t id, size_t line_len,
	char *line, const d2tk_lineedit_filter_t *filter,
	const d2tk_rect_t *rect, d2tk_flag_t flags)
{
	static const float mul = 0.5f;

	const d2tk_id_t subid = (1ULL << 32) | id;
	d2tk_atom_body_lineedit_t *body = _d2tk_base_get_atom(base, subid,
		D2TK_ATOM_LINEEDIT, _lineedit_event);

	atomic_store(&body->filter, (uintptr_t)filter);
	_atomic_set(&body->fill, line);
	
	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect, D2TK_FLAG_NONE);

	if(d2tk_state_is_focused(state))
	{
		const d2tk_coord_t dh = rect->h * mul / 2;

		d2tk_rect_t bnd;
		d2tk_rect_shrink_y(&bnd, rect, dh);

		d2tk_pty_t *pty = d2tk_pty_begin_state(base, id, state, _entry, body,
			bnd.h, &bnd, flags, alloca(d2tk_pty_sz));

		state = d2tk_pty_get_state(pty);

		char *old = _atomic_get(&body->line);
		if(old)
		{
			if(line)
			{
				snprintf(line, line_len, "%s", old);
				state |= D2TK_STATE_CHANGED;
			}

			free(old);
		}
	}
	else
	{
		const d2tk_style_t *old_style = d2tk_base_get_style(base);
		d2tk_style_t style = *old_style;
		style.font_face = FONT_CODE_REGULAR;
		d2tk_base_set_style(base, &style);

		char lbl [512];
		const size_t lbl_len = snprintf(lbl, sizeof(lbl), "◇%s", line);

		state = d2tk_base_label(base, lbl_len, lbl, mul, rect,
			D2TK_ALIGN_MIDDLE | D2TK_ALIGN_LEFT);

		d2tk_base_set_style(base, old_style);
	}

	return state;
}
