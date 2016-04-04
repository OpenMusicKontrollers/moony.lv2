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

#ifndef _SANDBOX_MASTER_H
#define _SANDBOX_MASTER_H

#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _sandbox_master_t sandbox_master_t;
typedef struct _sandbox_master_driver_t sandbox_master_driver_t;
typedef void (*sandbox_master_recv_cb_t)(void *data, uint32_t index, uint32_t size,
	uint32_t format, const void *buf);
typedef void (*sandbox_master_subscribe_cb_t)(void *data, uint32_t index,
	uint32_t protocol, bool state);

struct _sandbox_master_driver_t {
	const char *socket_path;
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	sandbox_master_recv_cb_t recv_cb;
	sandbox_master_subscribe_cb_t subscribe_cb;
};

sandbox_master_t *
sandbox_master_new(sandbox_master_driver_t *driver, void *data);

void
sandbox_master_free(sandbox_master_t *sb);

void
sandbox_master_recv(sandbox_master_t *sb);

bool
sandbox_master_send(sandbox_master_t *sb, uint32_t index, uint32_t size,
	uint32_t format, const void *buf);

bool
sandbox_master_flush(sandbox_master_t *sb);

void
sandbox_master_fd_get(sandbox_master_t *sb, int *fd);

#ifdef __cplusplus
}
#endif

#endif
