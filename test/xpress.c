/*
 * Copyright (c) 2016-2017 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <xpress.lv2/xpress.h>

#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>

#define XPRESS_TEST_URI	XPRESS_PREFIX"test"

#define MAX_NVOICES 32

typedef struct _targetI_t targetI_t;
typedef struct _targetO_t targetO_t;
typedef struct _plughandle_t plughandle_t;

struct _targetI_t {
	xpress_uuid_t uuidO;
};

struct _targetO_t {
	uint8_t dummy;
};

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_Log_Log *log;
	LV2_Log_Logger logger;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Ref ref;

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;

	XPRESS_T(xpressI, MAX_NVOICES);
	XPRESS_T(xpressO, MAX_NVOICES);
	targetI_t targetI [MAX_NVOICES];
	targetO_t targetO [MAX_NVOICES];
};

static void
_dump(plughandle_t *handle)
{
	XPRESS_VOICE_FOREACH(&handle->xpressI, voice)
	{
		lv2_log_trace(&handle->logger, "\t%"PRIu32" (%"PRIu32")", voice->uuid, voice->source);
	}
}

static void
_add(void *data, int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	LV2_Atom_Forge *forge = &handle->forge;
	targetI_t *src = target;

	lv2_log_trace(&handle->logger, "ADD: %"PRIu32, uuid);

	targetO_t *dst = xpress_create(&handle->xpressO, &src->uuidO);
	(void)dst;

	xpress_state_t new_state = *state;
	new_state.pitch *= 2;

	if(handle->ref)
		handle->ref = xpress_token(&handle->xpressO, forge, frames, src->uuidO, &new_state);

	_dump(handle);
}

static void
_set(void *data, int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	LV2_Atom_Forge *forge = &handle->forge;
	targetI_t *src = target;

	lv2_log_trace(&handle->logger, "PUT: %"PRIu32, uuid);

	targetO_t *dst = xpress_get(&handle->xpressO, src->uuidO);
	(void)dst;

	xpress_state_t new_state = *state;
	new_state.pitch *= 2;

	if(handle->ref)
		handle->ref = xpress_token(&handle->xpressO, forge, frames, src->uuidO, &new_state);

	_dump(handle);
}

static void
_del(void *data, int64_t frames,
	xpress_uuid_t uuid, void *target)
{
	plughandle_t *handle = data;
	LV2_Atom_Forge *forge = &handle->forge;
	targetI_t *src = target;

	lv2_log_trace(&handle->logger, "DEL: %"PRIu32, uuid);

	xpress_free(&handle->xpressO, src->uuidO);

	if(handle->ref)
		handle->ref = xpress_alive(&handle->xpressO, forge, frames);

	_dump(handle);
}

static const xpress_iface_t ifaceI = {
	.size = sizeof(targetI_t),
	.add = _add,
	.set = _set,
	.del = _del
};

static const xpress_iface_t ifaceO = {
	.size = sizeof(targetO_t)
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor,
	double rate __attribute__((unused)),
	const char *bundle_path __attribute__((unused)),
	const LV2_Feature *const *features)
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
		else if(!strcmp(features[i]->URI, XPRESS__voiceMap))
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

	lv2_log_logger_init(&handle->logger, handle->map, handle->log);

	lv2_atom_forge_init(&handle->forge, handle->map);

	if(!xpress_init(&handle->xpressI, MAX_NVOICES, handle->map, voice_map,
			XPRESS_EVENT_ALL, &ifaceI, handle->targetI, handle) )
	{
		fprintf(stderr, "failed to initialize xpressI structure\n");
		free(handle);
		return NULL;
	}
	if(!xpress_init(&handle->xpressO, MAX_NVOICES, handle->map, voice_map,
			XPRESS_EVENT_NONE, &ifaceO, handle->targetO, handle) )
	{
		fprintf(stderr, "failed to initialize xpressO structure\n");
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

	xpress_pre(&handle->xpressI);
	xpress_rst(&handle->xpressO);

	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		if(handle->ref)
			xpress_advance(&handle->xpressI, &handle->forge, ev->time.frames, obj, &handle->ref); //TODO handle return
	}

	xpress_post(&handle->xpressI, nsamples-1);
	if(handle->ref && !xpress_synced(&handle->xpressO))
		handle->ref = xpress_alive(&handle->xpressO, &handle->forge, nsamples-1);

	if(handle->ref)
		lv2_atom_forge_pop(&handle->forge, &frame);
	else
		lv2_atom_sequence_clear(handle->event_out);
}

static void
cleanup(LV2_Handle instance)
{
	plughandle_t *handle = instance;

	xpress_deinit(&handle->xpressI);
	xpress_deinit(&handle->xpressO);
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
