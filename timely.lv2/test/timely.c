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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <timely.h>

#include <lv2/lv2plug.in/ns/ext/log/log.h>

#define TIMELY_PREFIX		"http://open-music-kontrollers.ch/lv2/timely#"
#define TIMELY_TEST_URI	TIMELY_PREFIX"test"

typedef struct _plughandle_t plughandle_t;

struct _plughandle_t {
	LV2_URID_Map *map;
	LV2_Log_Log *log;
	timely_t timely;

	LV2_URID log_trace;

	const LV2_Atom_Sequence *event_in;
};

static int
_log_vprintf(plughandle_t *handle, LV2_URID type, const char *fmt, va_list args)
{
	return handle->log->vprintf(handle->log->handle, type, fmt, args);
}

// non-rt || rt with LV2_LOG__Trace
static int
_log_printf(plughandle_t *handle, LV2_URID type, const char *fmt, ...)
{
  va_list args;
	int ret;

  va_start (args, fmt);
	ret = _log_vprintf(handle, type, fmt, args);
  va_end(args);

	return ret;
}

static void
_timely_cb(timely_t *timely, int64_t frames, LV2_URID type, void *data)
{
	plughandle_t *handle = data;

	const char *uri = NULL;

	if(type == TIMELY_URI_BAR_BEAT(timely))
		uri = LV2_TIME__barBeat;
	else if(type == TIMELY_URI_BAR(timely))
		uri = LV2_TIME__bar;
	else if(type == TIMELY_URI_BEAT_UNIT(timely))
		uri = LV2_TIME__beatUnit;
	else if(type == TIMELY_URI_BEATS_PER_BAR(timely))
		uri = LV2_TIME__beatsPerBar;
	else if(type == TIMELY_URI_BEATS_PER_MINUTE(timely))
		uri = LV2_TIME__beatsPerMinute;
	else if(type == TIMELY_URI_FRAME(timely))
		uri = LV2_TIME__frame;
	else if(type == TIMELY_URI_FRAMES_PER_SECOND(timely))
		uri = LV2_TIME__framesPerSecond;
	else if(type == TIMELY_URI_SPEED(timely))
		uri = LV2_TIME__speed;

	const int64_t frame = TIMELY_FRAME(timely);
	_log_printf(data, handle->log_trace, "0x%08"PRIx64" %4"PRIi64" %s (%i)", frame, frames, uri, type);
}

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate,
	const char *bundle_path, const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	for(unsigned i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			handle->log = features[i]->data;
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

	handle->log_trace = handle->map->map(handle->map->handle, LV2_LOG__Trace);

	timely_mask_t mask = TIMELY_MASK_BAR_BEAT
		| TIMELY_MASK_BAR
		| TIMELY_MASK_BEAT_UNIT
		| TIMELY_MASK_BEATS_PER_BAR
		| TIMELY_MASK_BEATS_PER_MINUTE
		| TIMELY_MASK_FRAME
		| TIMELY_MASK_FRAMES_PER_SECOND
		| TIMELY_MASK_SPEED
		| TIMELY_MASK_BAR_BEAT_WHOLE
		| TIMELY_MASK_BAR_WHOLE;
	timely_init(&handle->timely, handle->map, rate, mask, _timely_cb, handle);

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
		default:
			break;
	}
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	plughandle_t *handle = instance;
	int64_t from = 0;

	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		const int64_t to = ev->time.frames;
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		const int handled = timely_advance(&handle->timely, obj, from, to);
		(void)handled;
		from = to;
	}

	timely_advance(&handle->timely, NULL, from, nsamples);
}

static void
cleanup(LV2_Handle instance)
{
	plughandle_t *handle = instance;

	free(handle);
}

const LV2_Descriptor timely_test = {
	.URI						= TIMELY_TEST_URI,
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
			return &timely_test;
		default:
			return NULL;
	}
}
