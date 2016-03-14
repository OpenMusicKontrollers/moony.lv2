/*
 * Copyright (c) 2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the voiceied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdatomic.h>

#include <xpress.h>

#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>

#define XPRESS_TEST_URI	XPRESS_PREFIX"test"

#define MAX_NVOICES 32

typedef struct _target_t target_t;
typedef struct _plughandle_t plughandle_t;

struct _target_t {
	xpress_uuid_t uuid;
};

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_Log_Log *log;
	LV2_Log_Logger logger;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Ref ref;

	XPRESS_T(xpress, MAX_NVOICES);
	target_t target [MAX_NVOICES];

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
};

static _Atomic xpress_uuid_t voice_uuid = ATOMIC_VAR_INIT(0);

static xpress_uuid_t
_voice_map_new_uuid(void *handle)
{
	_Atomic xpress_uuid_t *uuid = handle;
	return atomic_fetch_add_explicit(uuid, 1, memory_order_relaxed);
}

static xpress_map_t voice_map_fallback = {
	.handle = &voice_uuid,
	.new_uuid = _voice_map_new_uuid
};

static void
_dump(plughandle_t *handle)
{
	XPRESS_VOICE_FOREACH(&handle->xpress, voice)
	{
		lv2_log_trace(&handle->logger, "%"PRIi64, voice->uuid);
	}
	lv2_log_trace(&handle->logger, "");
}

static void
_add(void *data, int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	LV2_Atom_Forge *forge = &handle->forge;
	target_t *src = target;

	lv2_log_trace(&handle->logger, "ADD: %"PRIi64, uuid);

	src->uuid = xpress_map(&handle->xpress);

	xpress_state_t new_state;
	memcpy(&new_state, state, sizeof(xpress_state_t));
	new_state.position[0] *= 2;

	if(handle->ref)
		handle->ref = xpress_put(&handle->xpress, forge, frames, src->uuid, &new_state);

	_dump(handle);
}

static void
_put(void *data, int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	LV2_Atom_Forge *forge = &handle->forge;
	target_t *src = target;

	lv2_log_trace(&handle->logger, "PUT: %"PRIi64, uuid);

	xpress_state_t new_state;
	memcpy(&new_state, state, sizeof(xpress_state_t));
	new_state.position[0] *= 2;

	if(handle->ref)
		handle->ref = xpress_put(&handle->xpress, forge, frames, src->uuid, &new_state);

	_dump(handle);
}

static void
_del(void *data, int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	LV2_Atom_Forge *forge = &handle->forge;
	target_t *src = target;

	lv2_log_trace(&handle->logger, "DEL: %"PRIi64, uuid);

	if(handle->ref)
		handle->ref = xpress_del(&handle->xpress, forge, frames, src->uuid);

	_dump(handle);
}

static const xpress_iface_t iface = {
	.size = sizeof(target_t),
	.add = _add,
	.put = _put,
	.del = _del
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate,
	const char *bundle_path, const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	xpress_map_t *voice_map = NULL;

	for(unsigned i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			handle->log = features[i]->data;
		else if(!strcmp(features[i]->URI, XPRESS_VOICE_MAP))
			voice_map = features[i]->data;
	}

	if(!handle->map)
	{
		fprintf(stderr,
			"%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!handle->log)
	{
		fprintf(stderr,
			"%s: Host does not support log:log\n", descriptor->URI);
		free(handle);
		return NULL;
	}
	if(!voice_map)
		voice_map = &voice_map_fallback;

	lv2_log_logger_init(&handle->logger, handle->map, handle->log);

	lv2_atom_forge_init(&handle->forge, handle->map);

	if(!xpress_init(&handle->xpress, MAX_NVOICES, handle->map, voice_map,
			XPRESS_EVENT_ALL, &iface, handle->target, handle) )
	{
		fprintf(stderr, "failed to initialize xpress structure\n");
		free(handle);
		return NULL;
	}

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	plughandle_t *handle = (plughandle_t *)instance;

	switch(port)
	{
		case 0:
			handle->event_in = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->event_out = (LV2_Atom_Sequence *)data;
			break;
		default:
			break;
	}
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	plughandle_t *handle = instance;

	uint32_t capacity = handle->event_out->atom.size;
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_set_buffer(&handle->forge, (uint8_t *)handle->event_out, capacity);
	handle->ref = lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		if(handle->ref)
			xpress_advance(&handle->xpress, &handle->forge, ev->time.frames, obj, &handle->ref); //TODO handle return
	}
	if(handle->ref)
		lv2_atom_forge_pop(&handle->forge, &frame);
	else
		lv2_atom_sequence_clear(handle->event_out);
}

static void
cleanup(LV2_Handle instance)
{
	plughandle_t *handle = instance;

	free(handle);
}

const LV2_Descriptor xpress_test = {
	.URI						= XPRESS_TEST_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= NULL
};

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &xpress_test;
		default:
			return NULL;
	}
}
