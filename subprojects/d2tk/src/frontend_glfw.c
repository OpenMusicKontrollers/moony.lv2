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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "core_internal.h"
#include <d2tk/frontend_glfw.h>

#include <d2tk/backend.h>

#define GLFW_INCLUDE_ES3
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>

struct _d2tk_frontend_t {
	const d2tk_glfw_config_t *config;
	int w;
	int h;
	GLFWwindow * window;
	d2tk_base_t *base;
	void *ctx;
};

static inline void
_d2tk_frontend_expose(d2tk_frontend_t *dglfw)
{
	d2tk_base_t *base = dglfw->base;

	d2tk_coord_t w;
	d2tk_coord_t h;
	d2tk_base_get_dimensions(base, &w, &h);

	if(d2tk_base_pre(base, NULL) == 0)
	{
		dglfw->config->expose(dglfw->config->data, w, h);

		d2tk_base_post(base);
	}
}

D2TK_API int
d2tk_frontend_poll(d2tk_frontend_t *dglfw, double timeout)
{
	d2tk_base_t *base = dglfw->base;
	int width;
	int height;

	if(timeout < 0.0)
	{
		glfwWaitEvents();
	}
	else if(timeout == 0.0)
	{
		glfwPollEvents();
	}
	else
	{
		glfwWaitEventsTimeout(timeout);
	}

	glfwGetFramebufferSize(dglfw->window, &width, &height);
	if( (dglfw->w != width) || (dglfw->h != height) )
	{
		dglfw->w = width;
		dglfw->h = height;

		d2tk_base_set_dimensions(base, dglfw->w, dglfw->h);
	}

	_d2tk_frontend_expose(dglfw);
	glfwSwapBuffers(dglfw->window);

	return 0;
}

D2TK_API int
d2tk_frontend_get_file_descriptors(d2tk_frontend_t *dglfw, int *fds, int numfds)
{
	return d2tk_base_get_file_descriptors(dglfw->base, fds, numfds);
}

D2TK_API int
d2tk_frontend_step(d2tk_frontend_t *dglfw)
{
	return d2tk_frontend_poll(dglfw, 0.0);
}

D2TK_API void
d2tk_frontend_run(d2tk_frontend_t *dglfw, const sig_atomic_t *done)
{
	while(!*done)
	{
		if(d2tk_frontend_poll(dglfw, -1.0))
		{
			break;
		}
	}
}

D2TK_API void
d2tk_frontend_free(d2tk_frontend_t *dglfw)
{
	glfwDestroyWindow(dglfw->window);
	free(dglfw);

	glfwTerminate();
}

D2TK_API float
d2tk_frontend_get_scale()
{
#if 0
	float xscale;
	float yscale;

	glfwGetWindowContentScale(dglfw->window, &xscale, &yscale);

	return xscale;
#else
	return 1.f;
#endif
}

static void
_d2tk_frontend_modifiers(d2tk_base_t *base, int mods)
{
	d2tk_base_set_modmask(base, D2TK_MODMASK_SHIFT,
		(mods & GLFW_MOD_SHIFT) ? true : false);
	d2tk_base_set_modmask(base, D2TK_MODMASK_CTRL,
		(mods & GLFW_MOD_CONTROL) ? true : false);
	d2tk_base_set_modmask(base, D2TK_MODMASK_ALT,
		(mods & GLFW_MOD_ALT) ? true : false);
}

static void
_d2tk_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	d2tk_frontend_t *dglfw = glfwGetWindowUserPointer(window);
	d2tk_base_t *base = dglfw->base;
	const bool pressed = action == GLFW_PRESS;

	_d2tk_frontend_modifiers(base, mods);

	fprintf(stderr, "[%s] %i %i %i %i\n", __func__, key, scancode, action, mods);

	d2tk_keymask_t mask = D2TK_KEYMASK_NONE;
	unsigned int codepoint = 0;

	switch(key)
	{
		case GLFW_KEY_ENTER:
		{
			mask = D2TK_KEYMASK_ENTER;
			codepoint = '\n';
		} break; 
		case GLFW_KEY_TAB:
		{
			mask = D2TK_KEYMASK_TAB;
			codepoint = '\t';
		} break; 
		case GLFW_KEY_BACKSPACE:
		{
			mask = D2TK_KEYMASK_BACKSPACE;
			codepoint = '\b';
		} break; 
		case GLFW_KEY_ESCAPE:
		{
			mask = D2TK_KEYMASK_ESCAPE;
			codepoint = 0x1B;
		} break; 

		case GLFW_KEY_UP:
		{
			mask = D2TK_KEYMASK_UP;
		} break; 
		case GLFW_KEY_DOWN:
		{
			mask = D2TK_KEYMASK_DOWN;
		} break; 
		case GLFW_KEY_LEFT:
		{
			mask = D2TK_KEYMASK_LEFT;
		} break; 
		case GLFW_KEY_RIGHT:
		{
			mask = D2TK_KEYMASK_RIGHT;
		} break; 

		case GLFW_KEY_INSERT:
		{
			mask = D2TK_KEYMASK_INS;
		} break; 
		case GLFW_KEY_DELETE:
		{
			mask = D2TK_KEYMASK_DEL;
		} break; 
		case GLFW_KEY_HOME:
		{
			mask = D2TK_KEYMASK_HOME;
		} break; 
		case GLFW_KEY_END:
		{
			mask = D2TK_KEYMASK_END;
		} break; 
		case GLFW_KEY_PAGE_UP:
		{
			mask = D2TK_KEYMASK_PAGEUP;
		} break; 
		case GLFW_KEY_PAGE_DOWN:
		{
			mask = D2TK_KEYMASK_PAGEDOWN;
		} break; 
	}

	if(mask != D2TK_KEYMASK_NONE)
	{
		d2tk_base_set_keymask(base, mask, pressed);
	}

	if(codepoint && pressed)
	{
		d2tk_base_append_utf8(base, codepoint);
	}
}

static void
_d2tk_char(GLFWwindow *window, unsigned int codepoint)
{
	d2tk_frontend_t *dglfw = glfwGetWindowUserPointer(window);
	d2tk_base_t *base = dglfw->base;

	fprintf(stderr, "[%s] %u\n", __func__, codepoint);

	d2tk_base_append_utf8(base, codepoint);
}

static void
_d2tk_cursor_pos(GLFWwindow *window, double xpos, double ypos)
{
	d2tk_frontend_t *dglfw = glfwGetWindowUserPointer(window);
	d2tk_base_t *base = dglfw->base;

	d2tk_base_set_mouse_pos(base, xpos, ypos);
}

static void
_d2tk_cursor_enter(GLFWwindow *window, int entered)
{
	d2tk_frontend_t *dglfw = glfwGetWindowUserPointer(window);
	d2tk_base_t *base = dglfw->base;

	if(entered)
	{
		d2tk_base_set_full_refresh(base);
	}
}

static void
_d2tk_mouse_button(GLFWwindow *window, int button, int action, int mods)
{
	d2tk_frontend_t *dglfw = glfwGetWindowUserPointer(window);
	d2tk_base_t *base = dglfw->base;
	const bool pressed = action == GLFW_PRESS;

	_d2tk_frontend_modifiers(base, mods);

	switch(button)
	{
		case 3:
		{
			d2tk_base_set_butmask(base, D2TK_BUTMASK_RIGHT, pressed);
		} break;
		case 2:
		{
			d2tk_base_set_butmask(base, D2TK_BUTMASK_MIDDLE, pressed);
		} break;
		case 1:
			// fall-through
		default:
		{
			d2tk_base_set_butmask(base, D2TK_BUTMASK_LEFT, pressed);
		} break;
	}
}

static void
_d2tk_scroll(GLFWwindow *window, double xoffset, double yoffset)
{
	d2tk_frontend_t *dglfw = glfwGetWindowUserPointer(window);
	d2tk_base_t *base = dglfw->base;

	d2tk_base_add_mouse_scroll(base, xoffset, yoffset);
}

D2TK_API d2tk_frontend_t *
d2tk_glfw_new(const d2tk_glfw_config_t *config)
{
	if(glfwInit() != GLFW_TRUE)
	{
		return NULL;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);  
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	d2tk_frontend_t *dglfw = calloc(1, sizeof(d2tk_frontend_t));
	if(!dglfw)
	{
		goto fail;
	}

	dglfw->config = config;

	dglfw->window = glfwCreateWindow(dglfw->config->w, dglfw->config->h, "d2tk",
		NULL, NULL);

	if(!dglfw->window)
	{
		goto fail;
	}

	glfwSetWindowUserPointer(dglfw->window, dglfw);

	glfwSetKeyCallback(dglfw->window, _d2tk_key);
	glfwSetCharCallback(dglfw->window, _d2tk_char);
	glfwSetCursorPosCallback(dglfw->window, _d2tk_cursor_pos);
	glfwSetCursorEnterCallback(dglfw->window, _d2tk_cursor_enter);
	glfwSetMouseButtonCallback(dglfw->window, _d2tk_mouse_button);
	glfwSetScrollCallback(dglfw->window, _d2tk_scroll);
	glfwMakeContextCurrent(dglfw->window);
	glfwSwapInterval(1);

	dglfw->ctx = d2tk_core_driver.new(dglfw->config->bundle_path);

	if(!dglfw->ctx)
	{
		goto fail;
	}

	dglfw->base = d2tk_base_new(&d2tk_core_driver, dglfw->ctx);
	if(!dglfw->base)
	{
		goto fail;
	}

	return dglfw;

fail:
	if(dglfw)
	{
		if(dglfw->window)
		{
			glfwDestroyWindow(dglfw->window);
		}

		free(dglfw);

		glfwTerminate();
	}

	return NULL;
}

D2TK_API void
d2tk_frontend_redisplay(d2tk_frontend_t *dglfw)
{
	(void)dglfw;
	//FIXME
}

D2TK_API int
d2tk_frontend_set_size(d2tk_frontend_t *dglfw, d2tk_coord_t w, d2tk_coord_t h)
{
	d2tk_base_set_dimensions(dglfw->base, w, h);
	d2tk_frontend_redisplay(dglfw);

	return 0;
}

D2TK_API int
d2tk_frontend_get_size(d2tk_frontend_t *dglfw, d2tk_coord_t *w, d2tk_coord_t *h)
{
	int width;
	int height;

	glfwGetWindowSize(dglfw->window, &width, &height);

	*w = width;
	*h = height;

	return 0;
}

D2TK_API d2tk_base_t *
d2tk_frontend_get_base(d2tk_frontend_t *dglfw)
{
	return dglfw->base;
}
