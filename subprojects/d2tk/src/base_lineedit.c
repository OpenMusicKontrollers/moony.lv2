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

#include <linenoise.h>
#include <encodings/utf8.h>

typedef struct _d2tk_atom_body_lineedit_t d2tk_atom_body_lineedit_t;
typedef struct _d2tk_lineedit_t d2tk_lineedit_t;

struct _d2tk_atom_body_lineedit_t {
	atomic_flag lock;
	char fill [512];
	char line [512];
};

struct _d2tk_lineedit_t {
	d2tk_state_t state;
	d2tk_atom_body_lineedit_t *body;
};

const size_t d2tk_atom_body_lineedit_sz = sizeof(d2tk_atom_body_lineedit_t);
const size_t d2tk_lineedit_sz = sizeof(d2tk_lineedit_t);

static void
_completion(const char *buf, linenoiseCompletions *lc)
{
	if(!strcasecmp(buf, ":"))
	{
		linenoiseAddCompletion(lc, ":author");
		linenoiseAddCompletion(lc, ":class");
	};
}

static char *
_hints(const char *buf, int *color, int *bold)
{
	if(!strcasecmp(buf, ":"))
	{
		*color = 35;
		*bold = 0;
		return " search-category (author|class)";
	}
	else if(!strcasecmp(buf, ":author"))
	{
		*color = 35;
		*bold = 0;
		return " author-name";
	}
	else if(!strcasecmp(buf, ":class"))
	{
		*color = 35;
		*bold = 0;
		return " class-name";
	}

	return NULL;
}

static void
_body_lock(d2tk_atom_body_lineedit_t *body)
{
	while(atomic_flag_test_and_set(&body->lock))
	{}
}

static void
_body_unlock(d2tk_atom_body_lineedit_t *body)
{
		atomic_flag_clear(&body->lock);
}

static int
_entry(void *data)
{
	static const char *prompt = "> ";
	d2tk_atom_body_lineedit_t *body = data;
	char fill [512] = "";

	linenoiseApp *app = linenoiseAppNew();
	if(!app)
	{
		return 1;
	}

	linenoiseSetMultiLine(app, 0);
	linenoiseHistorySetMaxLen(app, 32);
	linenoiseSetFill(app, fill);

	linenoiseSetCompletionCallback(app, _completion);
	linenoiseSetHintsCallback(app, _hints);
	linenoiseSetEncodingFunctions(app, linenoiseUtf8PrevCharLen,
		linenoiseUtf8NextCharLen, linenoiseUtf8ReadCode);

	_body_lock(body);
	snprintf(fill, sizeof(fill), "%s", body->fill);
	_body_unlock(body);

	char *line;
	while( (line= linenoise(app, prompt)) )
	{
		_body_lock(body);
		snprintf(body->line, sizeof(body->line), "%s", line);
		_body_unlock(body);

		_body_lock(body);
		snprintf(fill, sizeof(fill), "%s", body->fill);
		_body_unlock(body);

		linenoiseHistoryAdd(app, line);
		linenoiseFree(app, line);
		linenoiseClearScreen(app);
	}

	linenoiseAppFree(app);

	return 0;
}

D2TK_API d2tk_lineedit_t *
d2tk_lineedit_begin(d2tk_base_t *base, d2tk_id_t id, const char *fill,
	d2tk_coord_t height, const d2tk_rect_t *rect, d2tk_flag_t flags,
	d2tk_lineedit_t *lineedit)
{
	memset(lineedit, 0x0, sizeof(d2tk_lineedit_t));

	lineedit->body = _d2tk_base_get_atom(base, id, D2TK_ATOM_LINEEDIT,
		NULL);

	if(fill)
	{
		_body_lock(lineedit->body);
		snprintf(lineedit->body->fill, sizeof(lineedit->body->fill), "%s", fill);
		_body_unlock(lineedit->body);
	}

	const d2tk_id_t subid = (1ULL << 32) | id;

	d2tk_pty_t *pty = d2tk_pty_begin(base, subid, _entry, lineedit->body, height, rect,
		flags, alloca(d2tk_pty_sz));

	lineedit->state = d2tk_pty_get_state(pty);

	return lineedit;
}

D2TK_API bool
d2tk_lineedit_not_end(d2tk_lineedit_t *lineedit)
{
	return lineedit ? true : false;
}

D2TK_API d2tk_lineedit_t *
d2tk_lineedit_next(d2tk_lineedit_t *lineedit __attribute__((unused)))
{
	return NULL;
}

D2TK_API d2tk_state_t
d2tk_lineedit_get_state(d2tk_lineedit_t *lineedit)
{
	return lineedit->state;
}

D2TK_API const char *
d2tk_lineedit_acquire_line(d2tk_lineedit_t *lineedit)
{
	_body_lock(lineedit->body);

	return lineedit->body->line;
}

D2TK_API void
d2tk_lineedit_release_line(d2tk_lineedit_t *lineedit)
{
	_body_unlock(lineedit->body);
}

D2TK_API char *
d2tk_lineedit_acquire_fill(d2tk_lineedit_t *lineedit, size_t *fill_len)
{
	_body_lock(lineedit->body);

	if(fill_len)
	{
		*fill_len = sizeof(lineedit->body->fill);
	}

	return lineedit->body->fill;
}

D2TK_API void
d2tk_lineedit_release_fill(d2tk_lineedit_t *lineedit)
{
	_body_unlock(lineedit->body);
}
