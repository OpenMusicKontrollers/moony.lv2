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

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#define SER_ATOM_IMPLEMENTATION
#include <ser_atom.lv2/ser_atom.h>

#define MAX_ITEMS 0x100000

static uint32_t
_map(void *data, const char *uri)
{
	(void)uri;

	uint32_t *urid = data;

	return ++(*urid);
}

static void *
_realloc(void *data, void *buf, size_t size)
{
	(void)data;
	(void)buf;
	(void)size;

	return NULL;
}

static void
_free(void *data, void *buf)
{
	(void)data;
	(void)buf;
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	ser_atom_t ser;
	LV2_Atom_Forge forge;

	uint32_t urid = 0;
	LV2_URID_Map map = { 
		.handle = &urid,
		.map = _map
	};

	lv2_atom_forge_init(&forge, &map);

	assert(ser_atom_init(NULL) != 0);
	assert(ser_atom_init(&ser) == 0);

	assert(ser.size == 0);
	assert(ser.offset == 0);
	assert(ser.buf == NULL);

	assert(ser_atom_funcs(NULL, NULL, NULL, NULL) != 0);
	assert(ser_atom_funcs(&ser, _realloc, _free, NULL) == 0);
	assert(ser_atom_reset(&ser, &forge) == 0);
	assert(lv2_atom_forge_bool(&forge, true) == 0);
	_free(NULL, NULL);

	assert(ser_atom_init(&ser) == 0);

	assert(ser.realloc == _ser_atom_realloc);
	assert(ser.free == _ser_atom_free);
	assert(ser.data == NULL);

	assert(ser.size == 0);
	assert(ser.offset == 0);
	assert(ser.buf == NULL);

	assert(ser_atom_reset(NULL, &forge) != 0);
	assert(ser_atom_reset(&ser, NULL) != 0);
	assert(ser_atom_reset(NULL, NULL) != 0);
	assert(ser_atom_reset(&ser, &forge) == 0);

	LV2_Atom_Forge_Frame frame;
	int64_t i;

	assert(lv2_atom_forge_tuple(&forge, &frame) != 0);
	for(i = 0; i < MAX_ITEMS; i++)
	{
		assert(lv2_atom_forge_long(&forge, i) != 0);
	}
	lv2_atom_forge_pop(&forge, &frame);

	i = 0;
	LV2_ATOM_TUPLE_FOREACH((const LV2_Atom_Tuple *)ser.buf, atom)
	{
		assert(atom->size == sizeof(int64_t));
		assert(atom->type == forge.Long);

		const int64_t *i64 = LV2_ATOM_BODY_CONST(atom);
		assert(*i64 == i++);
	}

	const size_t tot_size = MAX_ITEMS*sizeof(LV2_Atom_Long) + sizeof(LV2_Atom_Tuple);
	assert(ser.offset == tot_size);
	assert(ser.size >= tot_size);

	assert(lv2_atom_forge_deref(&forge, 0) == NULL);

	assert(ser_atom_get(NULL) == NULL);
	assert(ser_atom_get(&ser) == ser.atom);

	assert(ser_atom_deinit(NULL) != 0);
	assert(ser_atom_deinit(&ser) == 0);

	assert(ser.size == 0);
	assert(ser.offset == 0);
	assert(ser.buf == NULL);

	return 0;
}
