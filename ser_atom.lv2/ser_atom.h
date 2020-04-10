/*
 * Copyright (c) 2018 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _SER_ATOM_H
#define _SER_ATOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#ifndef SER_ATOM_API
#	define SER_ATOM_API static
#endif

typedef void *(*ser_atom_realloc_t)(void *data, void *buf, size_t size);
typedef void  (*ser_atom_free_t)(void *data, void *buf);

typedef struct _ser_atom_t ser_atom_t;

SER_ATOM_API int
ser_atom_init(ser_atom_t *ser);

SER_ATOM_API int
ser_atom_funcs(ser_atom_t *ser, ser_atom_realloc_t realloc,
	ser_atom_free_t free, void *data);

SER_ATOM_API int
ser_atom_reset(ser_atom_t *ser, LV2_Atom_Forge *forge);

SER_ATOM_API LV2_Atom *
ser_atom_get(ser_atom_t *ser);

SER_ATOM_API int
ser_atom_deinit(ser_atom_t *ser);

#ifdef SER_ATOM_IMPLEMENTATION

struct _ser_atom_t {
	ser_atom_realloc_t realloc;
	ser_atom_free_t free;
	void *data;

	size_t size;
	size_t offset;
	union {
		uint8_t *buf;
		LV2_Atom *atom;
	};
};

static LV2_Atom_Forge_Ref
_ser_atom_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf,
	uint32_t size)
{
	ser_atom_t *ser = handle;
	const size_t needed = ser->offset + size;

	while(needed > ser->size)
	{
		const size_t augmented = ser->size
			? ser->size << 1
			: 1024;
		uint8_t *grown = ser->realloc(ser->data, ser->buf, augmented);
		if(!grown) // out-of-memory
		{
			return 0;
		}

		ser->buf = grown;
		ser->size = augmented;
	}

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;
	memcpy(&ser->buf[ser->offset], buf, size);
	ser->offset += size;

	return ref;
}

static LV2_Atom *
_ser_atom_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	ser_atom_t *ser = handle;

	if(!ref) // invalid reference
	{
		return NULL;
	}

	const size_t offset = ref - 1;
	return (LV2_Atom *)&ser->buf[offset];
}

static void *
_ser_atom_realloc(void *data, void *buf, size_t size)
{
	(void)data;

	return realloc(buf, size);
}

static void
_ser_atom_free(void *data, void *buf)
{
	(void)data;

	free(buf);
}

SER_ATOM_API int
ser_atom_funcs(ser_atom_t *ser, ser_atom_realloc_t realloc,
	ser_atom_free_t free, void *data)
{
	if(!ser || !realloc || !free || ser_atom_deinit(ser))
	{
		return -1;
	}

	ser->realloc = realloc;
	ser->free = free;
	ser->data = data;

	return 0;
}

SER_ATOM_API int
ser_atom_init(ser_atom_t *ser)
{
	if(!ser)
	{
		return -1;
	}

	ser->size = 0;
	ser->offset = 0;
	ser->buf = NULL;

	return ser_atom_funcs(ser, _ser_atom_realloc, _ser_atom_free, NULL);
}

SER_ATOM_API int
ser_atom_reset(ser_atom_t *ser, LV2_Atom_Forge *forge)
{
	if(!ser || !forge)
	{
		return -1;
	}

	lv2_atom_forge_set_sink(forge, _ser_atom_sink, _ser_atom_deref, ser);

	ser->offset = 0;

	return 0;
}

SER_ATOM_API LV2_Atom *
ser_atom_get(ser_atom_t *ser)
{
	if(!ser)
	{
		return NULL;
	}

	return ser->atom;
}

SER_ATOM_API int
ser_atom_deinit(ser_atom_t *ser)
{
	if(!ser)
	{
		return -1;
	}

	if(ser->buf)
	{
		ser->free(ser->data, ser->buf);
	}

	ser->size = 0;
	ser->offset = 0;
	ser->buf = NULL;

	return 0;
}

#endif // SER_ATOM_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

#endif //_SER_ATOM_H
