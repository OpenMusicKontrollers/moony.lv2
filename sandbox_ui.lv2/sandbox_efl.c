/*
 * Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sandbox_slave.h>

#include <Elementary.h>

typedef struct _app_t app_t;

struct _app_t {
	sandbox_slave_t *sb;

	Evas_Object *win;
	Evas_Object *bg;
	Evas_Object *widget;
	Ecore_Fd_Handler *fd;
};

static Eina_Bool
_recv(void *data, Ecore_Fd_Handler *fd_handler)
{
	sandbox_slave_t *sb = data;

	sandbox_slave_recv(sb);

	return ECORE_CALLBACK_RENEW;
}

static void
_del_request(void *data, Evas_Object *obj, void *event_info)
{
	app_t *app = data;

	elm_exit();
	app->bg = NULL;
	app->win = NULL;
}

static inline int
_init(sandbox_slave_t *sb, void *data)
{
	app_t *app= data;

	int w = 640;
	int h = 360;

	const char *title = sandbox_slave_title_get(sb);
	app->win = elm_win_add(NULL, title, ELM_WIN_BASIC);
	if(!app->win)
	{
		fprintf(stderr, "elm_win_add failed\n");
		goto fail;
	}
	elm_win_title_set(app->win, title);
	elm_win_autodel_set(app->win, EINA_TRUE);
	evas_object_smart_callback_add(app->win, "delete,request", _del_request, app);

	app->bg = elm_bg_add(app->win);
	if(!app->bg)
	{
		fprintf(stderr, "elm_bg_add failed\n");
		goto fail;
	}
	elm_bg_color_set(app->bg, 64, 64, 64);
	evas_object_size_hint_weight_set(app->bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(app->bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(app->bg);
	elm_win_resize_object_add(app->win, app->bg);

	if(  sandbox_slave_instantiate(sb, (void *)app->win, (void *)&app->widget)
		|| !app->widget)
	{
		fprintf(stderr, "sandbox_slave_instantiate failed\n");
		goto fail;
	}
	evas_object_size_hint_weight_set(app->widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(app->widget, EVAS_HINT_FILL, EVAS_HINT_FILL);

	// get widget size hint
	int W, H;
	evas_object_size_hint_min_get(app->widget, &W, &H);
	if(W != 0)
		w = W;
	if(H != 0)
		h = H;
	evas_object_show(app->widget);
	elm_win_resize_object_add(app->win, app->widget);

	evas_object_resize(app->win, w, h);
	evas_object_show(app->win);

	int fd;
	sandbox_slave_fd_get(sb, &fd);
	if(fd == -1)
	{
		fprintf(stderr, "sandbox_slave_instantiate failed\n");
		goto fail;
	}

	app->fd= ecore_main_fd_handler_add(fd, ECORE_FD_READ,
		_recv, sb, NULL, NULL);
	if(!app->fd)
	{
		fprintf(stderr, "ecore_main_fd_handler_add failed\n");
		goto fail;
	}

	return 0;

fail:
	return -1;
}

static inline void
_run(sandbox_slave_t *sb, void *data)
{
	app_t *app = data;

	elm_run();
}

static inline void
_deinit(void *data)
{
	app_t *app = data;

	if(app->fd)
		ecore_main_fd_handler_del(app->fd);

	if(app->bg)
	{
		elm_win_resize_object_del(app->win, app->bg);
		evas_object_hide(app->bg);
		evas_object_del(app->bg);
	}

	if(app->win)
	{
		evas_object_hide(app->win);
		evas_object_del(app->win);
	}
}

static const sandbox_slave_driver_t driver = {
	.init_cb = _init,
	.run_cb = _run,
	.deinit_cb = _deinit,
	.resize_cb = NULL
};

static int
elm_main(int argc, char **argv)
{
	static app_t app;
	
#ifdef ELM_1_10
	elm_config_accel_preference_set("gl");
#endif

	app.sb = sandbox_slave_new(argc, argv, &driver, &app);
	if(app.sb)
	{
		sandbox_slave_run(app.sb);
		sandbox_slave_free(app.sb);
		printf("bye from %s\n", argv[0]);
		return 0;
	}

	printf("fail from %s\n", argv[0]);
	return -1;
}

ELM_MAIN();
