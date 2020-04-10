/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <props.h>

#define MAX_URIDS 512
#define STR_SIZE 32
#define CHUNK_SIZE 16
#define VEC_SIZE 13

#define PROPS_PREFIX		"http://open-music-kontrollers.ch/lv2/props#"
#define PROPS_TEST_URI	PROPS_PREFIX"test"

typedef struct _plugstate_t plugstate_t;
typedef struct _urid_t urid_t;
typedef struct _handle_t handle_t;
typedef void (*test_t)(handle_t *handle);
typedef void *(*ser_atom_realloc_t)(void *data, void *buf, size_t size);
typedef void  (*ser_atom_free_t)(void *data, void *buf);

typedef struct _ser_atom_t ser_atom_t;

struct _plugstate_t {
	int32_t b32;
	int32_t i32;
	int64_t i64;
	float f32;
	double f64;
	uint32_t urid;
	char str [STR_SIZE];
	char uri [STR_SIZE];
	char path [STR_SIZE];
	uint8_t chunk [CHUNK_SIZE];
	LV2_Atom_Literal_Body lit;
		char lit_body [STR_SIZE];
	LV2_Atom_Vector_Body vec;
		int32_t vec_body [VEC_SIZE];
	LV2_Atom_Object_Body obj; //FIXME
	LV2_Atom_Sequence_Body seq; //FIXME
};

struct _urid_t {
	LV2_URID urid;
	char *uri;
};

enum {
	PROP_b32 = 0,
	PROP_i32,
	PROP_i64,
	PROP_f32,
	PROP_f64,
	PROP_urid,
	PROP_str,
	PROP_uri,
	PROP_path,
	PROP_chunk,
	PROP_lit,
	PROP_vec,
	PROP_obj,
	PROP_seq,

	MAX_NPROPS
};

struct _handle_t {
	PROPS_T(props, MAX_NPROPS);
	plugstate_t state;
	plugstate_t stash;

	LV2_URID_Map map;

	urid_t urids [MAX_URIDS];
	LV2_URID urid;
};

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

static int
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

static int
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

static int
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

#if 0
static int
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
#endif

static LV2_Atom *
ser_atom_get(ser_atom_t *ser)
{
	if(!ser)
	{
		return NULL;
	}

	return ser->atom;
}

static LV2_URID
_map(LV2_URID_Map_Handle instance, const char *uri)
{
	handle_t *handle = instance;

	urid_t *itm;
	for(itm=handle->urids; itm->urid; itm++)
	{
		if(!strcmp(itm->uri, uri))
			return itm->urid;
	}

	assert(handle->urid + 1 < MAX_URIDS);

	// create new
	itm->urid = ++handle->urid;
	itm->uri = strdup(uri);

	return itm->urid;
}

static const props_def_t defs [MAX_NPROPS] = {
	[PROP_b32] = {
		.property = PROPS_PREFIX"b32",
		.offset = offsetof(plugstate_t, b32),
		.type = LV2_ATOM__Bool
	},
	[PROP_i32] = {
		.property = PROPS_PREFIX"i32",
		.offset = offsetof(plugstate_t, i32),
		.type = LV2_ATOM__Int
	},
	[PROP_i64] = {
		.property = PROPS_PREFIX"i64",
		.offset = offsetof(plugstate_t, i64),
		.type = LV2_ATOM__Long
	},
	[PROP_f32] = {
		.property = PROPS_PREFIX"f32",
		.offset = offsetof(plugstate_t, f32),
		.type = LV2_ATOM__Float
	},
	[PROP_f64] = {
		.property = PROPS_PREFIX"f64",
		.offset = offsetof(plugstate_t, f64),
		.type = LV2_ATOM__Double
	},
	[PROP_urid] = {
		.property = PROPS_PREFIX"urid",
		.offset = offsetof(plugstate_t, urid),
		.type = LV2_ATOM__URID
	},
	[PROP_str] = {
		.property = PROPS_PREFIX"str",
		.offset = offsetof(plugstate_t, str),
		.type = LV2_ATOM__String,
		.max_size = STR_SIZE
	},
	[PROP_uri] = {
		.property = PROPS_PREFIX"uri",
		.offset = offsetof(plugstate_t, uri),
		.type = LV2_ATOM__URI,
		.max_size = STR_SIZE
	},
	[PROP_path] = {
		.property = PROPS_PREFIX"path",
		.offset = offsetof(plugstate_t, path),
		.type = LV2_ATOM__Path,
		.max_size = STR_SIZE
	},
	[PROP_chunk] = {
		.property = PROPS_PREFIX"chunk",
		.offset = offsetof(plugstate_t, chunk),
		.type = LV2_ATOM__Chunk,
		.max_size = CHUNK_SIZE
	},
	[PROP_lit] = {
		.property = PROPS_PREFIX"lit",
		.offset = offsetof(plugstate_t, lit),
		.type = LV2_ATOM__Literal,
		.max_size = sizeof(LV2_Atom_Literal_Body) + STR_SIZE
	},
	[PROP_vec] = {
		.property = PROPS_PREFIX"vec",
		.offset = offsetof(plugstate_t, vec),
		.type = LV2_ATOM__Literal,
		.max_size = sizeof(LV2_Atom_Vector_Body) + VEC_SIZE*sizeof(int32_t)
	},
	[PROP_obj] = {
		.property = PROPS_PREFIX"obj",
		.offset = offsetof(plugstate_t, obj),
		.type = LV2_ATOM__Object,
		.max_size = sizeof(LV2_Atom_Object_Body) + 0 //FIXME
	},
	[PROP_seq] = {
		.property = PROPS_PREFIX"seq",
		.offset = offsetof(plugstate_t, seq),
		.type = LV2_ATOM__Sequence,
		.max_size = sizeof(LV2_Atom_Sequence_Body) + 0 //FIXME
	}
};

static void
_test_1(handle_t *handle)
{
	assert(handle);

	props_t *props = &handle->props;
	plugstate_t *state = &handle->state;
	plugstate_t *stash = &handle->stash;
	LV2_URID_Map *map = &handle->map;

	for(unsigned i = 0; i < MAX_NPROPS; i++)
	{
		const props_def_t *def = &defs[i];

		const LV2_URID property = props_map(props, def->property);
		assert(property != 0);
		assert(property == map->map(map->handle, def->property));

		assert(strcmp(props_unmap(props, property), def->property) == 0);

		props_impl_t *impl = _props_impl_get(props, property);
		assert(impl);

		const LV2_URID type = map->map(map->handle, def->type);
		const LV2_URID access = map->map(map->handle, def->access
			? def->access : LV2_PATCH__writable);

		assert(impl->property == property);
		assert(impl->type == type);
		assert(impl->access == access);

		assert(impl->def == def);

		assert(atomic_load(&impl->state) == PROP_STATE_NONE);
		assert(impl->stashing == false);

		switch(i)
		{
			case PROP_b32:
			{
				assert(impl->value.size == sizeof(state->b32));
				assert(impl->value.body == &state->b32);

				assert(impl->stash.size == sizeof(stash->b32));
				assert(impl->stash.body == &stash->b32);
			} break;
			case PROP_i32:
			{
				assert(impl->value.size == sizeof(state->i32));
				assert(impl->value.body == &state->i32);

				assert(impl->stash.size == sizeof(stash->i32));
				assert(impl->stash.body == &stash->i32);
			} break;
			case PROP_i64:
			{
				assert(impl->value.size == sizeof(state->i64));
				assert(impl->value.body == &state->i64);

				assert(impl->stash.size == sizeof(stash->i64));
				assert(impl->stash.body == &stash->i64);
			} break;
			case PROP_f32:
			{
				assert(impl->value.size == sizeof(state->f32));
				assert(impl->value.body == &state->f32);

				assert(impl->stash.size == sizeof(stash->f32));
				assert(impl->stash.body == &stash->f32);
			} break;
			case PROP_f64:
			{
				assert(impl->value.size == sizeof(state->f64));
				assert(impl->value.body == &state->f64);

				assert(impl->stash.size == sizeof(stash->f64));
				assert(impl->stash.body == &stash->f64);
			} break;
			case PROP_urid:
			{
				assert(impl->value.size == sizeof(state->urid));
				assert(impl->value.body == &state->urid);

				assert(impl->stash.size == sizeof(stash->urid));
				assert(impl->stash.body == &stash->urid);
			} break;
			case PROP_str:
			{
				assert(impl->value.size == 0);
				assert(impl->value.body == &state->str);

				assert(impl->stash.size == 0);
				assert(impl->stash.body == &stash->str);
			} break;
			case PROP_uri:
			{
				assert(impl->value.size == 0);
				assert(impl->value.body == &state->uri);

				assert(impl->stash.size == 0);
				assert(impl->stash.body == &stash->uri);
			} break;
			case PROP_path:
			{
				assert(impl->value.size == 0);
				assert(impl->value.body == &state->path);

				assert(impl->stash.size == 0);
				assert(impl->stash.body == &stash->path);
			} break;
			case PROP_chunk:
			{
				assert(impl->value.size == 0);
				assert(impl->value.body == &state->chunk);

				assert(impl->stash.size == 0);
				assert(impl->stash.body == &stash->chunk);
			} break;
			case PROP_lit:
			{
				assert(impl->value.size == sizeof(state->lit));
				assert(impl->value.body == &state->lit);

				assert(impl->stash.size == sizeof(stash->lit));
				assert(impl->stash.body == &stash->lit);
			} break;
			case PROP_vec:
			{
				assert(impl->value.size == sizeof(state->vec));
				assert(impl->value.body == &state->vec);

				assert(impl->stash.size == sizeof(stash->vec));
				assert(impl->stash.body == &stash->vec);
			} break;
			case PROP_obj:
			{
				assert(impl->value.size == sizeof(state->obj));
				assert(impl->value.body == &state->obj);

				assert(impl->stash.size == sizeof(stash->obj));
				assert(impl->stash.body == &stash->obj);
			} break;
			case PROP_seq:
			{
				assert(impl->value.size == sizeof(state->seq));
				assert(impl->value.body == &state->seq);

				assert(impl->stash.size == sizeof(stash->seq));
				assert(impl->stash.body == &stash->seq);
			} break;
			default:
			{
				assert(false);
			} break;
		}
	}
}

static void
_test_2(handle_t *handle)
{
	assert(handle);

	props_t *props = &handle->props;
	plugstate_t *state = &handle->state;
	plugstate_t *stash = &handle->stash;
	LV2_URID_Map *map = &handle->map;

	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge_Ref ref;
	ser_atom_t ser;

	lv2_atom_forge_init(&forge, map);
	assert(ser_atom_init(&ser) == 0);

	lv2_atom_forge_set_sink(&forge, _ser_atom_sink, _ser_atom_deref, &ser);

	ref = lv2_atom_forge_sequence_head(&forge, &frame, 0);
	assert(ref);

	props_idle(props, &forge, 0, &ref);
	assert(ref);

	const LV2_URID property = props_map(props, defs[0].property);
	assert(property);

	state->b32 = true;

	props_set(props, &forge, 1, property, &ref);
	assert(ref);

	state->b32 = false;

	lv2_atom_forge_pop(&forge, &frame);

	const LV2_Atom_Sequence *seq = (const LV2_Atom_Sequence *)ser_atom_get(&ser);
	assert(seq);

	unsigned nevs = 0;
	LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
	{
		const LV2_Atom *atom = &ev->body;

		assert(ev->time.frames == 1);
		assert(atom->type == forge.Object);

		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)atom;
		assert(obj->body.id == 0);
		assert(obj->body.otype == props->urid.patch_set);

		unsigned nprops = 0;
		LV2_ATOM_OBJECT_FOREACH(obj, prop)
		{
			assert(prop->context == 0);

			if(prop->key == props->urid.patch_subject)
			{
				const LV2_Atom_URID *val = (const LV2_Atom_URID *)&prop->value;

				assert(val->atom.type == forge.URID);
				assert(val->atom.size == sizeof(uint32_t));
				assert(val->body == props->urid.subject);

				nprops |= 0x1;
			}
			else if(prop->key == props->urid.patch_property)
			{
				const LV2_Atom_URID *val = (const LV2_Atom_URID *)&prop->value;

				assert(val->atom.type == forge.URID);
				assert(val->atom.size == sizeof(uint32_t));
				assert(val->body == property);

				nprops |= 0x2;
			}
			else if(prop->key == props->urid.patch_value)
			{
				const LV2_Atom_Bool *val = (const LV2_Atom_Bool *)&prop->value;

				assert(val->atom.type == forge.Bool);
				assert(val->atom.size == sizeof(int32_t));
				assert(val->body == true);

				nprops |= 0x4;
			}
			else
			{
				assert(false);
			}
		}
		assert(nprops == 0x7);

		assert(props_advance(props, &forge, ev->time.frames, obj, &ref) == 1);

		assert(state->b32 == true);
		assert(stash->b32 == true);

		nevs |= 0x1;
	}
	assert(nevs == 0x1);

	assert(ser_atom_deinit(&ser) == 0);
}

static const test_t tests [] = {
	_test_1,
	_test_2,
	NULL
};

int
main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	static handle_t handle;

	for(const test_t *test = tests; *test; test++)
	{
		memset(&handle, 0, sizeof(handle));

		handle.map.handle = &handle;
		handle.map.map = _map;

		assert(props_init(&handle.props, PROPS_PREFIX"subj", defs, MAX_NPROPS,
			&handle.state, &handle.stash, &handle.map, NULL) == 1);

		(*test)(&handle);
	}

	for(urid_t *itm=handle.urids; itm->urid; itm++)
	{
		free(itm->uri);
	}

	return 0;
}
