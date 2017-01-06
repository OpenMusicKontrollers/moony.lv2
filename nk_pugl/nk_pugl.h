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

#ifndef _NK_PUGL_H
#define _NK_PUGL_H

#include <stdatomic.h>
#include <ctype.h> // isalpha

#ifdef __cplusplus
extern C {
#endif

#include "pugl/pugl.h"
#include "pugl/gl.h"

#if defined(_WIN32)
#	include <windows.h>  // Broken Windows GL headers require this
#	include "GL/wglext.h"
#	include "GL/glext.h"
#endif

#if !defined(__APPLE__) && !defined(_WIN32)
#	include "GL/glx.h"
#	include "GL/glext.h"
#endif

#define NK_ZERO_COMMAND_MEMORY
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_SIN sinf
#define NK_COS cosf
#define NK_SQRT sqrtf

#include "nuklear/nuklear.h"
#include "nuklear/example/stb_image.h"

typedef struct _nk_pugl_config_t nk_pugl_config_t;
typedef struct _nk_pugl_window_t nk_pugl_window_t;
typedef void (*nkglGenerateMipmap)(GLenum target);
typedef void (*nk_pugl_expose_t)(struct nk_context *ctx,
	struct nk_rect wbounds, void *data);

struct _nk_pugl_config_t {
	unsigned width;
	unsigned height;

	bool resizable;
	bool ignore;
	const char *class;
	const char *title;

	struct {
		const char *face;
		int size;
	} font;

	intptr_t parent;

	void *data;
	nk_pugl_expose_t expose;
};

struct _nk_pugl_window_t {
	nk_pugl_config_t cfg;

	PuglView *view;
	int quit;

	struct nk_buffer cmds;
	struct nk_buffer vbuf;
	struct nk_buffer ebuf;
	struct nk_draw_null_texture null;
	struct nk_context ctx;
	struct nk_font_atlas atlas;
	struct nk_convert_config conv;
	struct {
		void *buffer;
		size_t size;
	} last;
	bool has_left;
	bool has_entered;

	GLuint font_tex;
	nkglGenerateMipmap glGenerateMipmap;

	intptr_t widget;
	PuglMod state;
#if !defined(__APPLE__) && !defined(_WIN32)
	atomic_flag async;
	Display *disp;
#endif
};

static inline intptr_t
nk_pugl_init(nk_pugl_window_t *win);

static inline void
nk_pugl_show(nk_pugl_window_t *win);

static inline void
nk_pugl_hide(nk_pugl_window_t *win);

static inline void
nk_pugl_shutdown(nk_pugl_window_t *win);

static inline void
nk_pugl_wait_for_event(nk_pugl_window_t *win);

static inline int
nk_pugl_process_events(nk_pugl_window_t *win);

static inline void
nk_pugl_post_redisplay(nk_pugl_window_t *win);

static inline void
nk_pugl_async_redisplay(nk_pugl_window_t *win);

static inline void
nk_pugl_quit(nk_pugl_window_t *win);

static struct nk_image
nk_pugl_icon_load(nk_pugl_window_t *win, const char *filename);

static void
nk_pugl_icon_unload(nk_pugl_window_t *win, struct nk_image img);

#ifdef __cplusplus
}
#endif

#endif // _NK_PUGL_H

#ifdef NK_PUGL_IMPLEMENTATION

#ifdef __cplusplus
extern C {
#endif

#define NK_ZERO_COMMAND_MEMORY
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_SIN sinf
#define NK_COS cosf
#define NK_SQRT sqrtf

#	define NK_IMPLEMENTATION
#	include "nuklear/nuklear.h"

#	define STB_IMAGE_IMPLEMENTATION
#	include "nuklear/example/stb_image.h"

typedef struct _nk_pugl_vertex_t nk_pugl_vertex_t;

struct _nk_pugl_vertex_t {
	float position [2];
	float uv [2];
	nk_byte col [4];
};

static const struct nk_draw_vertex_layout_element vertex_layout [] = {
	{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(nk_pugl_vertex_t, position)},
	{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(nk_pugl_vertex_t, uv)},
	{NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(nk_pugl_vertex_t, col)},
	{NK_VERTEX_LAYOUT_END}
};

#if defined(__APPLE__)
#	define GL_EXT(name) name
#endif

#if defined(_WIN32)
static void *
_nk_pugl_gl_ext(const char *name)
{
  void *p = wglGetProcAddress(name);

	if(  (p == 0) || (p == (void *)-1)
		|| (p == (void *)0x1) || (p == (void *)0x2) || (p == (void *)0x3) )
  {
    HMODULE module = LoadLibraryA("opengl32.dll");
    p = (void *)GetProcAddress(module, name);
  }

	if(!p)
		fprintf(stderr, "[GL]: failed to load extension: %s", name);

  return p;
}
#	define GL_EXT(name) (nk##name)_nk_pugl_gl_ext(#name)
#endif

#if !defined(__APPLE__) && !defined(_WIN32)
static void *
_nk_pugl_gl_ext(const char *name)
{
	void *p = glXGetProcAddress((const GLubyte*)name);

	if(!p)
		fprintf(stderr, "[GL]: failed to load extension: %s", name);

	return p;
}
#	define GL_EXT(name) (nk##name)_nk_pugl_gl_ext(#name)
#endif

static inline void
_nk_pugl_device_upload_atlas(nk_pugl_window_t *win, const void *image,
	int width, int height)
{
	glGenTextures(1, &win->font_tex);
	glBindTexture(GL_TEXTURE_2D, win->font_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image);
}

static inline void
_nk_pugl_render_gl2_push(unsigned width, unsigned height)
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.f, width, height, 0.f, -1.f, 1.f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
}

static inline void
_nk_pugl_render_gl2_pop(void)
{
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glPopAttrib();
}

static inline void
_nk_pugl_render_gl2(nk_pugl_window_t *win)
{
	nk_pugl_config_t *cfg = &win->cfg;

#if !defined(_WIN32)
	// compare current command buffer with last one to defer any changes
	bool has_changes = false;
	const size_t size = win->ctx.memory.allocated;
	const void *commands = nk_buffer_memory_const(&win->ctx.memory);

	if( (size != win->last.size) || memcmp(commands, win->last.buffer, size) )
	{
		// swap last buffer with current one for next comparison
		win->last.buffer = realloc(win->last.buffer, size);
		if(win->last.buffer)
		{
			win->last.size = size;
			memcpy(win->last.buffer, commands, size);
		}
		else
		{
			win->last.size = 0;
		}
		has_changes = true;
	}

	// only render if there were actually any changes
	if(has_changes || win->has_left || win->has_entered)
#endif
	{
		// convert shapes into vertexes if there were changes
		nk_convert(&win->ctx, &win->cmds, &win->vbuf, &win->ebuf, &win->conv);

		_nk_pugl_render_gl2_push(cfg->width, cfg->height);

		// setup vertex buffer pointers
		const GLsizei vs = sizeof(nk_pugl_vertex_t);
		const size_t vp = offsetof(nk_pugl_vertex_t, position);
		const size_t vt = offsetof(nk_pugl_vertex_t, uv);
		const size_t vc = offsetof(nk_pugl_vertex_t, col);
		const nk_byte *vertices = nk_buffer_memory_const(&win->vbuf);
		glVertexPointer(2, GL_FLOAT, vs, &vertices[vp]);
		glTexCoordPointer(2, GL_FLOAT, vs, &vertices[vt]);
		glColorPointer(4, GL_UNSIGNED_BYTE, vs, &vertices[vc]);

		// iterate over and execute each draw command
		const nk_draw_index *offset = nk_buffer_memory_const(&win->ebuf);
		const struct nk_draw_command *cmd;
		nk_draw_foreach(cmd, &win->ctx, &win->cmds)
		{
			if(!cmd->elem_count)
				continue;

			glBindTexture(GL_TEXTURE_2D, cmd->texture.id);
			glScissor(
				cmd->clip_rect.x,
				cfg->height - (cmd->clip_rect.y + cmd->clip_rect.h),
				cmd->clip_rect.w,
				cmd->clip_rect.h);
			glDrawElements(GL_TRIANGLES, cmd->elem_count, GL_UNSIGNED_SHORT, offset);

			offset += cmd->elem_count;
		}

		_nk_pugl_render_gl2_pop();

		nk_buffer_clear(&win->vbuf);
		nk_buffer_clear(&win->ebuf);

		win->has_entered = false;
	}

	nk_clear(&win->ctx);
}

static inline void
_nk_pugl_font_stash_begin(nk_pugl_window_t *win)
{
	struct nk_font_atlas *atlas = &win->atlas;

	nk_font_atlas_init_default(atlas);
	nk_font_atlas_begin(atlas);
}

static inline void
_nk_pugl_font_stash_end(nk_pugl_window_t *win)
{
	struct nk_font_atlas *atlas = &win->atlas;
	int w = 0;
	int h = 0;

	const void *image = nk_font_atlas_bake(atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	_nk_pugl_device_upload_atlas(win, image, w, h);
	nk_font_atlas_end(atlas, nk_handle_id(win->font_tex), &win->null);

	if(atlas->default_font)
		nk_style_set_font(&win->ctx, &atlas->default_font->handle);
}

static void
_nk_pugl_key_press(struct nk_context *ctx, enum nk_keys key)
{
	nk_input_key(ctx, key, nk_true);
	nk_input_key(ctx, key, nk_false);
}

static bool
_nk_pugl_special_key_down(nk_pugl_window_t *win, const PuglEventKey *ev)
{
	struct nk_context *ctx = &win->ctx;
	const bool control = ev->state & PUGL_MOD_CTRL;

	switch(ev->special)
	{
		case PUGL_KEY_F1:
		case PUGL_KEY_F2:
		case PUGL_KEY_F3:
		case PUGL_KEY_F4:
		case PUGL_KEY_F5:
		case PUGL_KEY_F6:
		case PUGL_KEY_F7:
		case PUGL_KEY_F8:
		case PUGL_KEY_F9:
		case PUGL_KEY_F10:
		case PUGL_KEY_F11:
		case PUGL_KEY_F12:
		{
			//TODO
		}	break;
		case PUGL_KEY_LEFT:
		{
			if(control)
				_nk_pugl_key_press(ctx, NK_KEY_TEXT_WORD_LEFT);
			else
				_nk_pugl_key_press(ctx, NK_KEY_LEFT);
		}	break;
		case PUGL_KEY_RIGHT:
		{
			if(control)
				_nk_pugl_key_press(ctx, NK_KEY_TEXT_WORD_RIGHT);
			else
				_nk_pugl_key_press(ctx, NK_KEY_RIGHT);
		}	break;
		case PUGL_KEY_UP:
		{
			_nk_pugl_key_press(ctx, NK_KEY_UP);
		}	break;
		case PUGL_KEY_DOWN:
		{
			_nk_pugl_key_press(ctx, NK_KEY_DOWN);
		}	break;
		case PUGL_KEY_PAGE_UP:
		{
			_nk_pugl_key_press(ctx, NK_KEY_SCROLL_UP);
		}	break;
		case PUGL_KEY_PAGE_DOWN:
		{
			_nk_pugl_key_press(ctx, NK_KEY_SCROLL_DOWN);
		}	break;
		case PUGL_KEY_HOME:
		{
			if(control)
			{
				_nk_pugl_key_press(ctx, NK_KEY_TEXT_START);
				_nk_pugl_key_press(ctx, NK_KEY_SCROLL_START);
			}
			else
			{
				_nk_pugl_key_press(ctx, NK_KEY_TEXT_LINE_START);
			}
		}	break;
		case PUGL_KEY_END:
		{
			if(control)
			{
				_nk_pugl_key_press(ctx, NK_KEY_TEXT_END);
				_nk_pugl_key_press(ctx, NK_KEY_SCROLL_END);
			}
			else
			{
				_nk_pugl_key_press(ctx, NK_KEY_TEXT_LINE_END);
			}
		}	break;
		case PUGL_KEY_INSERT:
		{
			//TODO
		}	break;
		case PUGL_KEY_SHIFT:
		{
			win->state |= PUGL_MOD_SHIFT;
			nk_input_key(ctx, NK_KEY_SHIFT, nk_true);
		}	return true;
		case PUGL_KEY_CTRL:
		{
			win->state |= PUGL_MOD_CTRL;
			nk_input_key(ctx, NK_KEY_CTRL, nk_true);
		}	return true;
		case PUGL_KEY_ALT:
		{
			// TODO
		}	break;
		case PUGL_KEY_SUPER:
		{
			// TODO
		}	break;
	}

	return false;
}

static bool
_nk_pugl_special_key_up(nk_pugl_window_t *win, const PuglEventKey *ev)
{
	struct nk_context *ctx = &win->ctx;

	switch(ev->special)
	{
		case PUGL_KEY_F1:
		case PUGL_KEY_F2:
		case PUGL_KEY_F3:
		case PUGL_KEY_F4:
		case PUGL_KEY_F5:
		case PUGL_KEY_F6:
		case PUGL_KEY_F7:
		case PUGL_KEY_F8:
		case PUGL_KEY_F9:
		case PUGL_KEY_F10:
		case PUGL_KEY_F11:
		case PUGL_KEY_F12:
		case PUGL_KEY_LEFT:
		case PUGL_KEY_RIGHT:
		case PUGL_KEY_UP:
		case PUGL_KEY_DOWN:
		case PUGL_KEY_PAGE_UP:
		case PUGL_KEY_PAGE_DOWN:
		case PUGL_KEY_HOME:
		case PUGL_KEY_END:
		case PUGL_KEY_INSERT:
		{
			// ignore
		}	break;
		case PUGL_KEY_SHIFT:
		{
			nk_input_key(ctx, NK_KEY_SHIFT, nk_false);
			win->state &= ~PUGL_MOD_SHIFT;
		}	return true;
		case PUGL_KEY_CTRL:
		{
			nk_input_key(ctx, NK_KEY_CTRL, nk_false);
			win->state &= ~PUGL_MOD_CTRL;
		}	return true;
		case PUGL_KEY_ALT:
		{
			// TODO
		}	break;
		case PUGL_KEY_SUPER:
		{
			// TODO
		}	break;
	}

	return false;
}

static bool
_nk_pugl_other_key_down(nk_pugl_window_t *win, const PuglEventKey *ev)
{
	struct nk_context *ctx = &win->ctx;
	const bool control = ev->state & PUGL_MOD_CTRL;

	switch(ev->character)
	{
		case '\r':
		{
			_nk_pugl_key_press(ctx, NK_KEY_ENTER);
		}	break;
		case '\t':
		{
			_nk_pugl_key_press(ctx, NK_KEY_TAB);
		}	break;
		case PUGL_CHAR_DELETE:
		{
			_nk_pugl_key_press(ctx, NK_KEY_DEL);
		}	break;
		case PUGL_CHAR_BACKSPACE:
		{
			_nk_pugl_key_press(ctx, NK_KEY_BACKSPACE);
		}	break;
		case PUGL_CHAR_ESCAPE:
		{
			_nk_pugl_key_press(ctx, NK_KEY_TEXT_RESET_MODE);
		} break;

		default:
		{
			if(control)
			{
				const uint32_t character = ev->character + 96; //FIXME why +96?
				switch(character)
				{
					case 'c':
					{
						_nk_pugl_key_press(ctx, NK_KEY_COPY);
					}	break;
					case 'v':
					{
						_nk_pugl_key_press(ctx, NK_KEY_PASTE);
					}	break;
					case 'x':
					{
						_nk_pugl_key_press(ctx, NK_KEY_CUT);
					}	break;
					case 'z':
					{
						_nk_pugl_key_press(ctx, NK_KEY_TEXT_UNDO);
					}	break;
					case 'r':
					{
						_nk_pugl_key_press(ctx, NK_KEY_TEXT_REDO);
					}	break;

					default:
					{
						if(isprint(character))
							nk_input_char(ctx, character);
					} break;
				}
			}
			else // !control
			{
				switch(ev->character)
				{
					case 'i':
					{
						_nk_pugl_key_press(ctx, NK_KEY_TEXT_INSERT_MODE);
					} break;
					case 'r':
					{
						_nk_pugl_key_press(ctx, NK_KEY_TEXT_REPLACE_MODE);
					} break;
				}

				nk_input_glyph(ctx, (const char *)ev->utf8);
			}
		} break;
	}

	return false;
}

static void
_nk_pugl_modifiers(nk_pugl_window_t *win, PuglMod state)
{
	struct nk_context *ctx = &win->ctx;

	if(win->state != state) // modifiers changed
	{
		if( (win->state & PUGL_MOD_SHIFT) != (state & PUGL_MOD_SHIFT))
			nk_input_key(ctx, NK_KEY_SHIFT, (state & PUGL_MOD_SHIFT) == PUGL_MOD_SHIFT);

		if( (win->state & PUGL_MOD_CTRL) != (state & PUGL_MOD_CTRL))
			nk_input_key(ctx, NK_KEY_CTRL, (state & PUGL_MOD_CTRL) == PUGL_MOD_CTRL);

		if( (win->state & PUGL_MOD_ALT) != (state & PUGL_MOD_ALT))
			; // not yet supported in nuklear

		if( (win->state & PUGL_MOD_SUPER) != (state & PUGL_MOD_SUPER))
			; // not yet supported in nuklear

		win->state = state; // switch old and new modifier states
	}
}

static bool
_nk_pugl_key_down(nk_pugl_window_t *win, const PuglEventKey *ev)
{
	if(ev->special)
		return _nk_pugl_special_key_down(win, ev);
	else if(ev->character && !ev->filter)
		return _nk_pugl_other_key_down(win, ev);

	return false;
}

static bool
_nk_pugl_key_up(nk_pugl_window_t *win, const PuglEventKey *ev)
{
	if(ev->special)
		return _nk_pugl_special_key_up(win, ev);

	return false;
}

static inline void
_nk_pugl_expose(PuglView *view)
{
	nk_pugl_window_t *win = puglGetHandle(view);
	nk_pugl_config_t *cfg = &win->cfg;
	struct nk_context *ctx = &win->ctx;

	const struct nk_rect wbounds = nk_rect(0, 0, cfg->width, cfg->height);

	if(nk_begin(ctx, "__bg__", wbounds, 0))
	{
		const struct nk_rect obounds = nk_window_get_bounds(ctx);

		if(  (obounds.x != wbounds.x) || (obounds.y != wbounds.y)
			|| (obounds.w != wbounds.w) || (obounds.h != wbounds.h) )
		{
			// size has changed
			nk_window_set_bounds(ctx, wbounds);
			puglPostRedisplay(view);
		}

		// clears window with widget background color
	}
	nk_end(ctx);

	if(cfg->expose)
		cfg->expose(ctx, wbounds, cfg->data);

	_nk_pugl_render_gl2(win);
}

static void
_nk_pugl_dummy_func(PuglView *view, const PuglEvent *e)
{
	// do nothing
}

static void
_nk_pugl_event_func(PuglView *view, const PuglEvent *e)
{
	nk_pugl_window_t *win = puglGetHandle(view);
	nk_pugl_config_t *cfg = &win->cfg;
	struct nk_context *ctx = &win->ctx;

	switch(e->type)
	{
		case PUGL_NOTHING:
		{
			break;
		}
		case PUGL_BUTTON_PRESS:
		{
			const PuglEventButton *ev = (const PuglEventButton *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_button(ctx, ev->button - 1, ev->x, ev->y, 1);

			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_BUTTON_RELEASE:
		{
			const PuglEventButton *ev = (const PuglEventButton *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_button(ctx, ev->button - 1, ev->x, ev->y, 0);

			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_CONFIGURE:
		{
			const PuglEventConfigure *ev = (const PuglEventConfigure *)e;

			cfg->width = ev->width;
			cfg->height = ev->height;

			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_EXPOSE:
		{
			nk_input_end(ctx);
			_nk_pugl_expose(win->view);
			nk_input_begin(ctx);

			break;
		}
		case PUGL_CLOSE:
		{
			nk_pugl_quit(win);

			break;
		}
		case PUGL_KEY_PRESS:
		{
			const PuglEventKey *ev = (const PuglEventKey *)e;

			if(!_nk_pugl_key_down(win, ev)) // no modifier change
				_nk_pugl_modifiers(win, ev->state);

			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_KEY_RELEASE:
		{
			const PuglEventKey *ev = (const PuglEventKey *)e;

			if(!_nk_pugl_key_up(win, ev)) // no modifier change
				_nk_pugl_modifiers(win, ev->state);

			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_MOTION_NOTIFY:
		{
			const PuglEventMotion *ev = (const PuglEventMotion *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_motion(ctx, ev->x, ev->y);

			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_SCROLL:
		{
			const PuglEventScroll *ev = (const PuglEventScroll *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_scroll(ctx, ev->dy);

			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_LEAVE_NOTIFY:
		{
			const PuglEventCrossing *ev = (const PuglEventCrossing *)e;

			_nk_pugl_modifiers(win, ev->state);
			// fall-through
		}
		case PUGL_FOCUS_OUT:
		{
			win->has_left = true;
			puglPostRedisplay(win->view);
			break;
		}
		case PUGL_ENTER_NOTIFY:
		{
			const PuglEventCrossing *ev = (const PuglEventCrossing *)e;

			_nk_pugl_modifiers(win, ev->state);
			// fall-through
		}
		case PUGL_FOCUS_IN:
		{
			win->has_left = false;
			win->has_entered = true;
			puglPostRedisplay(win->view);
			break;
		}
	}
}

static inline intptr_t
nk_pugl_init(nk_pugl_window_t *win)
{
	nk_pugl_config_t *cfg = &win->cfg;
	struct nk_convert_config *conv = &win->conv;

#if !defined(__APPLE__) && !defined(_WIN32)
	win->async = (atomic_flag)ATOMIC_FLAG_INIT;
	win->disp = XOpenDisplay(0);
#endif
	win->has_left = true;

	// init pugl
	win->view = puglInit(NULL, NULL);
	puglInitWindowClass(win->view, cfg->class ? cfg->class : "nuklear");
	puglInitWindowSize(win->view, cfg->width, cfg->height);
	puglInitWindowMinSize(win->view, cfg->width, cfg->height);
	puglInitResizable(win->view, cfg->resizable);
	puglInitWindowParent(win->view, cfg->parent);
	puglInitTransientFor(win->view, cfg->parent);
	puglSetHandle(win->view, win);
	puglSetEventFunc(win->view, _nk_pugl_dummy_func);
	puglIgnoreKeyRepeat(win->view, cfg->ignore);
	puglInitContextType(win->view, PUGL_GL);
	const int stat = puglCreateWindow(win->view, cfg->title ? cfg->title : "Nuklear");
	assert(stat == 0);

	puglEnterContext(win->view);
	{
		// init nuklear
		nk_buffer_init_default(&win->cmds);
		nk_buffer_init_default(&win->vbuf);
		nk_buffer_init_default(&win->ebuf);
		nk_init_default(&win->ctx, 0);

		// init nuklear font
		struct nk_font *ttf = NULL;
		struct nk_font_config fcfg = nk_font_config(cfg->font.size);
		static const nk_rune range [] = {
			0x0020, 0x007F, // Basic Latin
			0x00A0, 0x00FF, // Latin-1 Supplement
			0x0100, 0x017F, // Latin Extended-A
			0x0180, 0x024F, // Latin Extended-B
			0x0300, 0x036F, // Combining Diacritical Marks
			0x0370, 0x03FF, // Greek and Coptic
			0x0400, 0x04FF, // Cyrillic
			0x0500, 0x052F, // Cyrillic Supplementary
			0
		};
    fcfg.range = range;

		_nk_pugl_font_stash_begin(win);
		if(cfg->font.face && cfg->font.size)
			ttf = nk_font_atlas_add_from_file(&win->atlas, cfg->font.face, cfg->font.size, &fcfg);
		_nk_pugl_font_stash_end(win);
		if(ttf)
			nk_style_set_font(&win->ctx, &ttf->handle);

		win->glGenerateMipmap = GL_EXT(glGenerateMipmap);
	}
	puglLeaveContext(win->view, false);

	// fill convert configuration
	conv->vertex_layout = vertex_layout;
	conv->vertex_size = sizeof(nk_pugl_vertex_t);
	conv->vertex_alignment = NK_ALIGNOF(nk_pugl_vertex_t);
	conv->null = win->null;
	conv->circle_segment_count = 22;
	conv->curve_segment_count = 22;
	conv->arc_segment_count = 22;
	conv->global_alpha = 1.0f;
	conv->shape_AA = NK_ANTI_ALIASING_ON;
	conv->line_AA = NK_ANTI_ALIASING_ON;

	puglSetEventFunc(win->view, _nk_pugl_event_func);
	nk_input_begin(&win->ctx);

	win->widget = puglGetNativeWindow(win->view);
	return win->widget;
}

static inline void
nk_pugl_show(nk_pugl_window_t *win)
{
	if(!win->view)
		return;

	puglShowWindow(win->view);
}

static inline void
nk_pugl_hide(nk_pugl_window_t *win)
{
	if(!win->view)
		return;

	puglHideWindow(win->view);
}

static inline void
nk_pugl_shutdown(nk_pugl_window_t *win)
{
	if(!win->view)
		return;

	nk_input_end(&win->ctx);

	if(win->last.buffer)
		free(win->last.buffer);

	puglEnterContext(win->view);
	{
		// shutdown nuklear font
		nk_font_atlas_clear(&win->atlas);
		if(win->font_tex)
			glDeleteTextures(1, &win->font_tex);

		// shutdown nuklear
		nk_free(&win->ctx);
		nk_buffer_free(&win->cmds);
		nk_buffer_free(&win->vbuf);
		nk_buffer_free(&win->ebuf);
	}
	puglLeaveContext(win->view, false);

	// shutdown pugl
	puglDestroy(win->view);

#if !defined(__APPLE__) && !defined(_WIN32)
	if(win->disp)
		XCloseDisplay(win->disp);
#endif
}

static inline void
nk_pugl_wait_for_event(nk_pugl_window_t *win)
{
	if(!win->view)
		return;

	puglWaitForEvent(win->view);
}

static inline int
nk_pugl_process_events(nk_pugl_window_t *win)
{
	if(!win->view)
		return 1; // quit

	struct nk_context *ctx = &win->ctx;

	PuglStatus stat = puglProcessEvents(win->view);
	(void)stat;

	return win->quit;
}

static inline void
nk_pugl_post_redisplay(nk_pugl_window_t *win)
{
	if(!win->view)
		return;

	puglPostRedisplay(win->view);
}

static inline void
nk_pugl_async_redisplay(nk_pugl_window_t *win)
{
	if(!win->view)
		return;

#if defined(__APPLE__)
// TODO
#endif

#if defined(_WIN32)
	const HWND widget = (HWND)win->widget;
	const int status = SendNotifyMessage(widget, WM_PAINT, 0, 0);
	(void)status;
#endif

#if !defined(__APPLE__) && !defined(_WIN32)
	const Window widget = (Window)win->widget;
	XExposeEvent xevent = {
		.type = Expose,
		.display = win->disp,
		.window = widget
	};

	while(atomic_flag_test_and_set_explicit(&win->async, memory_order_acquire))
	{
		// spin
	}

	const Status status = XSendEvent(win->disp, widget, false, ExposureMask,
		(XEvent *)&xevent);
	(void)status;
	XFlush(win->disp);

	atomic_flag_clear_explicit(&win->async, memory_order_release);
#endif
}

static inline void
nk_pugl_quit(nk_pugl_window_t *win)
{
	win->quit = 1;
}

static struct nk_image
nk_pugl_icon_load(nk_pugl_window_t *win, const char *filename)
{
	GLuint tex = 0;

	if(!win->view)
		return nk_image_id(tex);

	int w, h, n;
	uint8_t *data = stbi_load(filename, &w, &h, &n, 0);
	if(data)
	{
		puglEnterContext(win->view);
		{
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			if(!win->glGenerateMipmap) // for GL >= 1.4 && < 3.1
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			if(win->glGenerateMipmap) // for GL >= 3.1
				win->glGenerateMipmap(GL_TEXTURE_2D);
		}
		puglLeaveContext(win->view, false);

		stbi_image_free(data);
	}

	return nk_image_id(tex);
}

static void
nk_pugl_icon_unload(nk_pugl_window_t *win, struct nk_image img)
{
	if(!win->view)
		return;

	if(img.handle.id)
	{
		puglEnterContext(win->view);
		{
			glDeleteTextures(1, (const GLuint *)&img.handle.id);
		}
		puglLeaveContext(win->view, false);
	}
}

#ifdef __cplusplus
}
#endif

#endif // NK_PUGL_IMPLEMENTATION
