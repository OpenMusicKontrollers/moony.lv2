/*
 * Copyright (c) 2016-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)
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
#include <math.h> // isalpha

#ifdef __cplusplus
extern C {
#endif

#include "pugl/pugl.h"
#include "pugl/pugl_gl.h"

#include <lv2/ui/ui.h>

#if defined(__APPLE__)
#	include <OpenGL/gl.h>
#	include <OpenGL/glext.h>
#else
#	include <GL/glew.h>
#	ifdef _WIN32
#		include <windows.h>
#	else
#		include <X11/Xresource.h>
#	endif
#endif

#define KEY_TAB '\t'
#define KEY_NEWLINE '\n'
#define KEY_RETURN '\r'
#define KEY_PLUS '+'
#define KEY_MINUS '-'
#define KEY_C 'c'
#define KEY_V 'v'
#define KEY_X 'x'
#define KEY_Z 'z'

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

#ifndef NK_PUGL_API
#	define NK_PUGL_API static inline
#endif

typedef struct _nk_pugl_config_t nk_pugl_config_t;
typedef struct _nk_pugl_window_t nk_pugl_window_t;
typedef void (*nkglGenerateMipmap)(GLenum target);
typedef void (*nk_pugl_expose_t)(struct nk_context *ctx,
	struct nk_rect wbounds, void *data);

struct _nk_pugl_config_t {
	unsigned width;
	unsigned height;
	unsigned min_width;
	unsigned min_height;

	bool resizable;
	bool fixed_aspect;
	bool ignore;
	const char *class;
	const char *title;

	struct {
		char *face;
		int size;
	} font;

	intptr_t parent;
	bool threads;

	LV2UI_Resize *host_resize;

	void *data;
	nk_pugl_expose_t expose;
};

struct _nk_pugl_window_t {
	nk_pugl_config_t cfg;
	char urn [46];
	float scale;

	PuglWorld *world;
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

NK_PUGL_API intptr_t
nk_pugl_init(nk_pugl_window_t *win);

NK_PUGL_API void
nk_pugl_show(nk_pugl_window_t *win);

NK_PUGL_API void
nk_pugl_hide(nk_pugl_window_t *win);

NK_PUGL_API void
nk_pugl_shutdown(nk_pugl_window_t *win);

NK_PUGL_API void
nk_pugl_wait_for_event(nk_pugl_window_t *win);

NK_PUGL_API int
nk_pugl_process_events(nk_pugl_window_t *win);

NK_PUGL_API int
nk_pugl_resize(nk_pugl_window_t *win, int width, int height);

NK_PUGL_API void
nk_pugl_post_redisplay(nk_pugl_window_t *win);

NK_PUGL_API void
nk_pugl_async_redisplay(nk_pugl_window_t *win);

NK_PUGL_API void
nk_pugl_quit(nk_pugl_window_t *win);

NK_PUGL_API struct nk_image
nk_pugl_icon_load(nk_pugl_window_t *win, const char *filename);

NK_PUGL_API void
nk_pugl_icon_unload(nk_pugl_window_t *win, struct nk_image img);

NK_PUGL_API bool
nk_pugl_is_shortcut_pressed(struct nk_input *in, char letter, bool clear);

NK_PUGL_API void
nk_pugl_copy_to_clipboard(nk_pugl_window_t *win, const char *selection, size_t len);

NK_PUGL_API const char *
nk_pugl_paste_from_clipboard(nk_pugl_window_t *win, size_t *len);

NK_PUGL_API float
nk_pugl_get_scale(nk_pugl_window_t *win);

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

#define NK_IMPLEMENTATION
#include "nuklear/nuklear.h"

#define STB_IMAGE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#pragma GCC diagnostic ignored "-Wshift-negative-value"
#include "nuklear/example/stb_image.h"
#pragma GCC diagnostic pop

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

#elif defined(_WIN32)
static void *
_nk_pugl_gl_ext(const char *name)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
  void *p = wglGetProcAddress(name);
#pragma GCC diagnostic pop

	if(  (p == 0) || (p == (void *)-1)
		|| (p == (void *)0x1) || (p == (void *)0x2) || (p == (void *)0x3) )
  {
    HMODULE module = LoadLibraryA("opengl32.dll");
    p = (void *)GetProcAddress(module, name);
  }

	if(!p)
	{
		fprintf(stderr, "[GL]: failed to load extension: %s", name);
	}

  return p;
}
#	define GL_EXT(name) (nk##name)_nk_pugl_gl_ext(#name)

#else
static void *
_nk_pugl_gl_ext(const char *name)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	void *p = puglGetProcAddress(name);
#pragma GCC diagnostic pop

	if(!p)
	{
		fprintf(stderr, "[GL]: failed to load extension: %s", name);
	}

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
	bool has_changes = win->has_left || win->has_entered;

	// compare current command buffer with last one to defer any changes
	if(!has_changes)
	{
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
	}

	if(has_changes)
	{
		// clear command/vertex buffers of last stable view
		nk_buffer_clear(&win->cmds);
		nk_buffer_clear(&win->vbuf);
		nk_buffer_clear(&win->ebuf);
		nk_draw_list_clear(&win->ctx.draw_list);

		// convert shapes into vertexes if there were changes
		nk_convert(&win->ctx, &win->cmds, &win->vbuf, &win->ebuf, &win->conv);
	}

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
		{
			continue;
		}

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

	win->has_entered = false;

	nk_clear(&win->ctx);
}

static void
_nk_pugl_glew_init()
{
#if defined(__APPLE__)
//FIXME
#else
	glewExperimental = GL_TRUE;
	const GLenum err = glewInit();
	if(err != GLEW_OK)
	{
		fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
		return;
	}
#endif
}

static void
_nk_pugl_font_init(nk_pugl_window_t *win)
{
	nk_pugl_config_t *cfg = &win->cfg;

	const int font_size = cfg->font.size * win->scale;

	// init nuklear font
	struct nk_font *ttf = NULL;
	struct nk_font_config fcfg = nk_font_config(font_size);
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
	fcfg.oversample_h = 8;
	fcfg.oversample_v = 8;

	struct nk_font_atlas *atlas = &win->atlas;
	nk_font_atlas_init_default(atlas);
	nk_font_atlas_begin(atlas);

	if(cfg->font.face && font_size)
	{
		ttf = nk_font_atlas_add_from_file(&win->atlas, cfg->font.face, font_size, &fcfg);
	}

	int w = 0;
	int h = 0;
	struct nk_draw_null_texture null;
	const void *image = nk_font_atlas_bake(atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	_nk_pugl_device_upload_atlas(win, image, w, h);
	nk_font_atlas_end(atlas, nk_handle_id(win->font_tex), &null);

	if(atlas->default_font)
	{
		nk_style_set_font(&win->ctx, &atlas->default_font->handle);
	}

	if(ttf)
	{
		nk_style_set_font(&win->ctx, &ttf->handle);
	}

	// to please compiler
	(void)nk_cos;
	(void)nk_sin;
	(void)nk_sqrt;
}

static void
_nk_pugl_font_deinit(nk_pugl_window_t *win)
{
	nk_font_atlas_clear(&win->atlas);

	if(win->font_tex)
	{
		glDeleteTextures(1, (const GLuint *)&win->font_tex);
	}
}

static void
_nk_pugl_host_resize(nk_pugl_window_t *win)
{
	nk_pugl_config_t *cfg = &win->cfg;

	if(cfg->host_resize)
	{
		cfg->host_resize->ui_resize(cfg->host_resize->handle,
			cfg->width, cfg->height);
	}
}

static void
_nk_pugl_key_press(struct nk_context *ctx, enum nk_keys key)
{
	nk_input_key(ctx, key, nk_true);
	nk_input_key(ctx, key, nk_false);
}

static void
_nk_pugl_modifiers(nk_pugl_window_t *win, PuglMod state)
{
	struct nk_context *ctx = &win->ctx;

	if(win->state != state) // modifiers changed
	{
		if( (win->state & PUGL_MOD_SHIFT) != (state & PUGL_MOD_SHIFT))
		{
			nk_input_key(ctx, NK_KEY_SHIFT, (state & PUGL_MOD_SHIFT) == PUGL_MOD_SHIFT);
		}

		if( (win->state & PUGL_MOD_CTRL) != (state & PUGL_MOD_CTRL))
		{
			nk_input_key(ctx, NK_KEY_CTRL, (state & PUGL_MOD_CTRL) == PUGL_MOD_CTRL);
		}

		if( (win->state & PUGL_MOD_ALT) != (state & PUGL_MOD_ALT))
		{
			// not yet supported in nuklear
		}

		if( (win->state & PUGL_MOD_SUPER) != (state & PUGL_MOD_SUPER))
		{
			// not yet supported in nuklear
		}

		win->state = state; // switch old and new modifier states
	}
}

static bool
_nk_pugl_key_down(nk_pugl_window_t *win, const PuglEventKey *ev)
{
	struct nk_context *ctx = &win->ctx;
#if defined(__APPLE__)
	const bool control = ev->state & PUGL_MOD_SUPER;
#else
	const bool control = ev->state & PUGL_MOD_CTRL;
#endif
	const bool shift = ev->state & PUGL_MOD_SHIFT;

	switch(ev->key)
	{
		case PUGL_KEY_LEFT:
		{
			_nk_pugl_key_press(ctx, control ? NK_KEY_TEXT_WORD_LEFT : NK_KEY_LEFT);
		}	break;
		case PUGL_KEY_RIGHT:
		{
			_nk_pugl_key_press(ctx, control ? NK_KEY_TEXT_WORD_RIGHT : NK_KEY_RIGHT);
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
			_nk_pugl_key_press(ctx, NK_KEY_TEXT_INSERT_MODE);
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
		case KEY_NEWLINE:
			// fall-through
		case KEY_RETURN:
		{
			_nk_pugl_key_press(ctx, NK_KEY_ENTER);
		}	break;
		case KEY_TAB:
		{
			_nk_pugl_key_press(ctx, NK_KEY_TAB);
		}	break;
		case PUGL_KEY_DELETE:
		{
#if defined(__APPLE__) // quirk around Apple's Delete key strangeness
			_nk_pugl_key_press(ctx, NK_KEY_BACKSPACE);
#else
			_nk_pugl_key_press(ctx, NK_KEY_DEL);
#endif
		}	break;
		case PUGL_KEY_BACKSPACE:
		{
#if defined(__APPLE__) // quirk around Apple's Delete key strangeness
			_nk_pugl_key_press(ctx, NK_KEY_DEL);
#else
			_nk_pugl_key_press(ctx, NK_KEY_BACKSPACE);
#endif
		}	break;
		case PUGL_KEY_ESCAPE:
		{
			_nk_pugl_key_press(ctx, NK_KEY_TEXT_RESET_MODE);
		} break;

		default:
		{
			if(control)
			{
				switch(ev->key)
				{
					case KEY_C:
					{
						_nk_pugl_key_press(ctx, NK_KEY_COPY);
					}	break;
					case KEY_V:
					{
						_nk_pugl_key_press(ctx, NK_KEY_PASTE);
					}	break;
					case KEY_X:
					{
						_nk_pugl_key_press(ctx, NK_KEY_CUT);
					}	break;
					case KEY_Z:
					{
						_nk_pugl_key_press(ctx, shift ? NK_KEY_TEXT_REDO : NK_KEY_TEXT_UNDO);
					}	break;
				}
			}
		}
	}

	return false;
}

static bool
_nk_pugl_key_up(nk_pugl_window_t *win, const PuglEventKey *ev)
{
	struct nk_context *ctx = &win->ctx;

	switch(ev->key)
	{
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
	}

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
			puglPostRedisplay(view);
		}

		// clears window with widget background color
	}
	nk_end(ctx);

	if(cfg->expose)
	{
		cfg->expose(ctx, wbounds, cfg->data);
	}

	_nk_pugl_render_gl2(win);
}

static PuglStatus
_nk_pugl_event_func(PuglView *view, const PuglEvent *e)
{
	nk_pugl_window_t *win = puglGetHandle(view);
	nk_pugl_config_t *cfg = &win->cfg;
	struct nk_context *ctx = &win->ctx;

	switch(e->type)
	{
		case PUGL_BUTTON_PRESS:
		{
			const PuglEventButton *ev = (const PuglEventButton *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_button(ctx, ev->button - 1, ev->x, ev->y, 1);

			puglPostRedisplay(win->view);
		} break;
		case PUGL_BUTTON_RELEASE:
		{
			const PuglEventButton *ev = (const PuglEventButton *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_button(ctx, ev->button - 1, ev->x, ev->y, 0);

			puglPostRedisplay(win->view);
		} break;
		case PUGL_CONFIGURE:
		{
			const PuglEventConfigure *ev = (const PuglEventConfigure *)e;

			// only redisplay if size has changed
			if( (cfg->width == ev->width) && (cfg->height == ev->height) )
			{
				break;
			}

			cfg->width = ev->width;
			cfg->height = ev->height;

			puglPostRedisplay(win->view);
		} break;
		case PUGL_EXPOSE:
		{
			nk_input_end(ctx);
			_nk_pugl_expose(win->view);
			nk_input_begin(ctx);
		} break;
		case PUGL_CLOSE:
		{
			nk_pugl_quit(win);
		} break;
		case PUGL_KEY_PRESS:
		{
			const PuglEventKey *ev = (const PuglEventKey *)e;

			if(!_nk_pugl_key_down(win, ev)) // no modifier change
			{
				_nk_pugl_modifiers(win, ev->state);
			}

			puglPostRedisplay(win->view);
		} break;
		case PUGL_KEY_RELEASE:
		{
			const PuglEventKey *ev = (const PuglEventKey *)e;

			if(!_nk_pugl_key_up(win, ev)) // no modifier change
			{
				_nk_pugl_modifiers(win, ev->state);
			}

			puglPostRedisplay(win->view);
		} break;
		case PUGL_MOTION:
		{
			const PuglEventMotion *ev = (const PuglEventMotion *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_motion(ctx, ev->x, ev->y);

			puglPostRedisplay(win->view);
		} break;
		case PUGL_SCROLL:
		{
			const PuglEventScroll *ev = (const PuglEventScroll *)e;

			_nk_pugl_modifiers(win, ev->state);
			nk_input_scroll(ctx, nk_vec2(0.f, ev->dy));

			puglPostRedisplay(win->view);
		} break;

		case PUGL_POINTER_OUT:
		{
			const PuglEventCrossing *ev = (const PuglEventCrossing *)e;

			_nk_pugl_modifiers(win, ev->state);
			win->has_left = true;
			puglPostRedisplay(win->view);
		} break;
		case PUGL_POINTER_IN:
		{
			const PuglEventCrossing *ev = (const PuglEventCrossing *)e;

			_nk_pugl_modifiers(win, ev->state);
			win->has_left = false;
			win->has_entered = true;
			puglPostRedisplay(win->view);
		} break;

		case PUGL_FOCUS_OUT:
		case PUGL_FOCUS_IN:
		{
			// nothing
		} break;

		case PUGL_TEXT:
		{
			const PuglEventText *ev = (const PuglEventText *)e;

			const bool control = ev->state & PUGL_MOD_CTRL;

			const int ch = control ? ev->character | 0x60 : ev->character;

			if(isprint(ch))
			{
				_nk_pugl_key_press(ctx, NK_KEY_TEXT_INSERT_MODE);
				nk_input_unicode(ctx, ch);
			}
		} break;

		case PUGL_CREATE:
		{
			// init glew
			_nk_pugl_glew_init();

			// init font system
			_nk_pugl_font_init(win);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
			win->glGenerateMipmap = GL_EXT(glGenerateMipmap);
#pragma GCC diagnostic pop
		} break;
		case PUGL_DESTROY:
		{
			// deinit font system
			_nk_pugl_font_deinit(win);
		} break;

		case PUGL_MAP:
		{
			 //nothing
		} break;
		case PUGL_UNMAP:
		{
			 //nothing
		} break;
		case PUGL_UPDATE:
		{
			 //nothing
		} break;
		case PUGL_CLIENT:
		{
			 //nothing
		} break;
		case PUGL_TIMER:
		{
			 //nothing
		} break;
		case PUGL_NOTHING:
		{
			// nothing
		} break;
	}

	return PUGL_SUCCESS;
}

static void
_nk_pugl_editor_paste(nk_handle userdata, struct nk_text_edit* editor)
{
	nk_pugl_window_t *win = userdata.ptr;

	size_t len;
	const char *selection = nk_pugl_paste_from_clipboard(win, &len);
	if(selection)
	{
		nk_textedit_paste(editor, selection, len);
	}
}

static void
_nk_pugl_editor_copy(nk_handle userdata, const char *buf, int len)
{
	nk_pugl_window_t *win = userdata.ptr;

	nk_pugl_copy_to_clipboard(win, buf, len);
}

NK_PUGL_API intptr_t
nk_pugl_init(nk_pugl_window_t *win)
{
	nk_pugl_config_t *cfg = &win->cfg;
	struct nk_convert_config *conv = &win->conv;

	const char *NK_SCALE = getenv("NK_SCALE");
	const float scale = NK_SCALE ? atof(NK_SCALE) : 1.f;
	const float dpi0 = 96.f; // reference DPI we're designing for

#if defined(__APPLE__)
	const float dpi1 = dpi0; //TODO implement this
#elif defined(_WIN32)
	// GetDpiForSystem/Monitor/Window is Win10 only
	HDC screen = GetDC(NULL);
	const float dpi1 = GetDeviceCaps(screen, LOGPIXELSX);
	ReleaseDC(NULL, screen);
#else
	win->async = (atomic_flag)ATOMIC_FLAG_INIT;
	win->disp = XOpenDisplay(0);
	// modern X actually lies here, but proprietary nvidia
	float dpi1 = XDisplayWidth(win->disp, 0) * 25.4f / XDisplayWidthMM(win->disp, 0);

	// read DPI from users's ~/.Xresources
	char *resource_string = XResourceManagerString(win->disp);
	XrmInitialize();
	if(resource_string)
	{
		XrmDatabase db = XrmGetStringDatabase(resource_string);
		if(db)
		{
			char *type = NULL;
			XrmValue value;

			XrmGetResource(db, "Xft.dpi", "String", &type, &value);
			if(value.addr)
			{
				dpi1 = atof(value.addr);
			}

			XrmDestroyDatabase(db);
		}
	}
#endif

	win->scale = scale * dpi1 / dpi0;
	if(win->scale < 0.5)
	{
		win->scale = 0.5;
	}
	win->has_left = true;

	cfg->width *= win->scale;
	cfg->height *= win->scale;
	cfg->min_width *= win->scale;
	cfg->min_height *= win->scale;

	// init pugl
	win->world = puglNewWorld(cfg->parent ? PUGL_MODULE : PUGL_PROGRAM,
		cfg->threads ? PUGL_WORLD_THREADS : 0);

#if defined(__APPLE__) || defined(_WIN32)
	uint8_t bytes [0x10];

	srand(time(NULL));
	for(unsigned i=0x0; i<0x10; i++)
		bytes[i] = rand() & 0xff;

	bytes[6] = (bytes[6] & 0b00001111) | 0b01000000; // set four most significant bits of 7th byte to 0b0100
	bytes[8] = (bytes[8] & 0b00111111) | 0b10000000; // set two most significant bits of 9th byte to 0b10

	snprintf(win->urn, sizeof(win->urn),
		"urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		bytes[0x0], bytes[0x1], bytes[0x2], bytes[0x3],
		bytes[0x4], bytes[0x5],
		bytes[0x6], bytes[0x7],
		bytes[0x8], bytes[0x9],
		bytes[0xa], bytes[0xb], bytes[0xc], bytes[0xd], bytes[0xe], bytes[0xf]);
	fprintf(stderr, "%s\n", win->urn);
	puglSetClassName(win->world, win->urn);
#else
	puglSetClassName(win->world, cfg->class ? cfg->class : "nuklear");
#endif

	// init nuklear
	nk_buffer_init_default(&win->cmds);
	nk_buffer_init_default(&win->vbuf);
	nk_buffer_init_default(&win->ebuf);
	nk_init_default(&win->ctx, 0);

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

	nk_input_begin(&win->ctx);

	win->ctx.clip.paste = _nk_pugl_editor_paste;
	win->ctx.clip.copy = _nk_pugl_editor_copy;
	win->ctx.clip.userdata.ptr = win;

	win->view = puglNewView(win->world);

	const PuglRect frame = {
		.x = 0,
		.y = 0,
		.width = cfg->width,
		.height = cfg->height
	};

	puglSetFrame(win->view, frame);
	if(cfg->min_width && cfg->min_height)
	{
		puglSetMinSize(win->view, cfg->min_width, cfg->min_height);
	}
	if(cfg->parent)
	{
		puglSetParentWindow(win->view, cfg->parent);
#if 0 // not yet implemented for mingw, darwin
		puglSetTransientFor(win->view, cfg->parent);
#endif
	}
	if(cfg->fixed_aspect)
	{
		puglSetAspectRatio(win->view, cfg->width, cfg->height,
			cfg->width, cfg->height);
	}
	puglSetViewHint(win->view, PUGL_RESIZABLE, cfg->resizable);
	puglSetViewHint(win->view, PUGL_DOUBLE_BUFFER, true);
	puglSetViewHint(win->view, PUGL_SWAP_INTERVAL, 1);
	puglSetHandle(win->view, win);
	puglSetEventFunc(win->view, _nk_pugl_event_func);
	puglSetBackend(win->view, puglGlBackend());
	puglSetWindowTitle(win->view,
		cfg->title ? cfg->title : "Nuklear");
	const int stat = puglRealize(win->view);
	assert(stat == 0);

	win->widget = puglGetNativeWindow(win->view);
	return win->widget;
}

NK_PUGL_API void
nk_pugl_show(nk_pugl_window_t *win)
{
	if(!win->view)
	{
		return;
	}

	puglShowWindow(win->view);
	_nk_pugl_host_resize(win);
}

NK_PUGL_API void
nk_pugl_hide(nk_pugl_window_t *win)
{
	if(!win->view)
	{
		return;
	}

	puglHideWindow(win->view);
}

NK_PUGL_API void
nk_pugl_shutdown(nk_pugl_window_t *win)
{
	if(!win->view)
	{
		return;
	}

	nk_input_end(&win->ctx);

	if(win->last.buffer)
	{
		free(win->last.buffer);
	}

	// shutdown nuklear
	nk_buffer_free(&win->cmds);
	nk_buffer_free(&win->vbuf);
	nk_buffer_free(&win->ebuf);
	nk_free(&win->ctx);

	// shutdown pugl
	if(win->world)
	{
		if(win->view)
		{
			puglFreeView(win->view);
		}

		puglFreeWorld(win->world);
	}

#if !defined(__APPLE__) && !defined(_WIN32)
	if(win->disp)
	{
		XCloseDisplay(win->disp);
	}
#endif
}

NK_PUGL_API void
nk_pugl_wait_for_event(nk_pugl_window_t *win)
{
	if(!win->view)
	{
		return;
	}

	puglUpdate(win->world, -1.0); // blocking pooll
}

NK_PUGL_API int
nk_pugl_process_events(nk_pugl_window_t *win)
{
	if(!win->view)
	{
		return 1; // quit
	}

	PuglStatus stat = puglUpdate(win->world, 0.0);
	(void)stat;

	return win->quit;
}

NK_PUGL_API int
nk_pugl_resize(nk_pugl_window_t *win, int width, int height)
{
	if(!win->view)
	{
		return 1; // quit
	}

	win->cfg.width = width;
	win->cfg.height = height;

	puglPostRedisplay(win->view);

	return 0;
}

NK_PUGL_API void
nk_pugl_post_redisplay(nk_pugl_window_t *win)
{
	if(!win->view)
	{
		return;
	}

	puglPostRedisplay(win->view);
}

NK_PUGL_API void
nk_pugl_async_redisplay(nk_pugl_window_t *win)
{
	if(!win->view)
	{
		return;
	}

#if defined(__APPLE__)
// TODO
#elif defined(_WIN32)
	const HWND widget = (HWND)win->widget;
	const int status = SendNotifyMessage(widget, WM_PAINT, 0, 0);
	(void)status;
#else
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

NK_PUGL_API void
nk_pugl_quit(nk_pugl_window_t *win)
{
	win->quit = 1;
}

NK_PUGL_API struct nk_image
nk_pugl_icon_load(nk_pugl_window_t *win, const char *filename)
{
	GLuint tex = 0;

	if(!win->view)
	{
		return nk_image_id(tex);
	}

	int w, h, n;
	uint8_t *data = stbi_load(filename, &w, &h, &n, 4);
	if(data)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		puglEnterContext(win->view, false);
#pragma GCC diagnostic pop
		{
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			if(!win->glGenerateMipmap) // for GL >= 1.4 && < 3.1
			{
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			if(win->glGenerateMipmap) // for GL >= 3.1
			{
				win->glGenerateMipmap(GL_TEXTURE_2D);
			}
		}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		puglLeaveContext(win->view, false);
#pragma GCC diagnostic pop

		stbi_image_free(data);
	}

	return nk_image_id(tex);
}

NK_PUGL_API void
nk_pugl_icon_unload(nk_pugl_window_t *win, struct nk_image img)
{
	if(!win->view)
	{
		return;
	}

	if(img.handle.id)
	{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		puglEnterContext(win->view, false);
#pragma GCC diagnostic pop
		{
			glDeleteTextures(1, (const GLuint *)&img.handle.id);
		}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		puglLeaveContext(win->view, false);
#pragma GCC diagnostic pop
	}
}

NK_PUGL_API bool
nk_pugl_is_shortcut_pressed(struct nk_input *in, char letter, bool clear)
{
	const bool control = nk_input_is_key_down(in, NK_KEY_CTRL);

	if(control && (in->keyboard.text_len == 1) )
	{
		if(in->keyboard.text[0] == letter)
		{
			if(clear)
			{
				in->keyboard.text_len = 0;
			}

			return true; // matching shortcut
		}
	}

	return false;
}

NK_PUGL_API void
nk_pugl_copy_to_clipboard(nk_pugl_window_t *win, const char *selection, size_t len)
{
	const char *type = "text/plain";

	puglSetClipboard(win->view, type, selection, len);
}

NK_PUGL_API const char *
nk_pugl_paste_from_clipboard(nk_pugl_window_t *win, size_t *len)
{
	const char *type = NULL;

	return puglGetClipboard(win->view, &type, len);
}

NK_PUGL_API float
nk_pugl_get_scale(nk_pugl_window_t *win)
{
	return win->scale;
}

#ifdef __cplusplus
}
#endif

#endif // NK_PUGL_IMPLEMENTATION
