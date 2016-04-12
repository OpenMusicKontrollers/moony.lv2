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

#ifndef _SANDBOX_SLAVE_H
#define _SANDBOX_SLAVE_H

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sandbox_slave_t sandbox_slave_t;
typedef struct _sandbox_slave_driver_t sandbox_slave_driver_t;
	
typedef int (*sandbox_slave_driver_init_t)(sandbox_slave_t *sb, void *data);
typedef void (*sandbox_slave_driver_run_t)(sandbox_slave_t *sb, void *data);
typedef void (*sandbox_slave_driver_deinit_t)(void *data);
typedef int (*sandbox_slave_driver_resize_t)(void *data, int width, int height);

struct _sandbox_slave_driver_t {
	sandbox_slave_driver_init_t init_cb;
	sandbox_slave_driver_run_t run_cb;
	sandbox_slave_driver_deinit_t deinit_cb;
	sandbox_slave_driver_resize_t resize_cb;
};

sandbox_slave_t *
sandbox_slave_new(int argc, char **argv, const sandbox_slave_driver_t *driver, void *data);

void
sandbox_slave_free(sandbox_slave_t *sb);

void *
sandbox_slave_instantiate(sandbox_slave_t *sb, const LV2_Feature *parent_feature, void *widget);

void
sandbox_slave_recv(sandbox_slave_t *sb);

bool
sandbox_slave_flush(sandbox_slave_t *sb);

const void *
sandbox_slave_extension_data(sandbox_slave_t *sb, const char *URI);

void
sandbox_slave_run(sandbox_slave_t *sb);

void
sandbox_slave_fd_get(sandbox_slave_t *sb, int *fd);

const char *
sandbox_slave_title_get(sandbox_slave_t *sb);

#ifdef __cplusplus
}
#endif

#endif
