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

#include <sandbox_master.h>
#include <sandbox_io.h>

#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

struct _sandbox_master_t {
	sandbox_io_t io;

	sandbox_master_driver_t *driver;
	void *data;
};

sandbox_master_t *
sandbox_master_new(sandbox_master_driver_t *driver, void *data)
{
	sandbox_master_t *sb = calloc(1, sizeof(sandbox_master_t));
	if(!sb)
		goto fail;

	sb->driver = driver;
	sb->data = data;

	if(_sandbox_io_init(&sb->io, driver->map, driver->unmap, driver->socket_path, true))
		goto fail;

	return sb;

fail:
	sandbox_master_free(sb);
	return NULL;
}

void
sandbox_master_free(sandbox_master_t *sb)
{
	if(sb)
	{
		_sandbox_io_deinit(&sb->io);
		free(sb);
	}
}

void
sandbox_master_recv(sandbox_master_t *sb)
{
	if(sb)
		_sandbox_io_recv(&sb->io, sb->driver->recv_cb, sb->driver->subscribe_cb, sb->data);
}

bool
sandbox_master_send(sandbox_master_t *sb, uint32_t index, uint32_t size,
	uint32_t format, const void *buf)
{
	if(sb)
		return _sandbox_io_send(&sb->io, index, size, format, buf);

	return false;
}

bool
sandbox_master_flush(sandbox_master_t *sb)
{
	if(sb)
		return _sandbox_io_flush(&sb->io);

	return false;
}

void
sandbox_master_fd_get(sandbox_master_t *sb, int *fd)
{
	if(sb && fd)
		*fd = _sandbox_io_fd_get(&sb->io);
	else if(fd)
		*fd = -1;
}
