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

#ifndef _LV2_TIMELY_H_
#define _LV2_TIMELY_H_

#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/time/time.h>

typedef enum _timely_mask_t timely_mask_t;
typedef struct _timely_t timely_t;
typedef void (*timely_cb_t)(timely_t *timely, int64_t frames, LV2_URID type,
	void *data);

enum _timely_mask_t {
	TIMELY_MASK_BAR_BEAT					= (1 << 0),
	TIMELY_MASK_BAR								= (1 << 1),
	TIMELY_MASK_BEAT_UNIT					= (1 << 2),
	TIMELY_MASK_BEATS_PER_BAR			= (1 << 3),
	TIMELY_MASK_BEATS_PER_MINUTE	= (1 << 4),
	TIMELY_MASK_FRAME							= (1 << 5),
	TIMELY_MASK_FRAMES_PER_SECOND	= (1 << 6),
	TIMELY_MASK_SPEED							= (1 << 7),
	TIMELY_MASK_BAR_BEAT_WHOLE		= (1 << 8),
	TIMELY_MASK_BAR_WHOLE					= (1 << 9)
};

struct _timely_t {
	struct {
		LV2_URID atom_object;
		LV2_URID atom_blank;
		LV2_URID atom_resource;

		LV2_URID time_position;
		LV2_URID time_barBeat;
		LV2_URID time_bar;
		LV2_URID time_beatUnit;
		LV2_URID time_beatsPerBar;
		LV2_URID time_beatsPerMinute;
		LV2_URID time_frame;
		LV2_URID time_framesPerSecond;
		LV2_URID time_speed;
	} urid;

	struct {
		float bar_beat;
		int64_t bar;

		int32_t beat_unit;
		float beats_per_bar;
		float beats_per_minute;

		int64_t frame;
		float frames_per_second;

		float speed;
	} pos;

	double frames_per_beat;
	double frames_per_bar;

	struct {
		double beat;
		double bar;
	} offset;

	struct {
		double beat;
		double bar;
	} window;

	bool first;
	timely_mask_t mask;
	timely_cb_t cb;
	void *data;
};

#define TIMELY_URI_BAR_BEAT(timely)						((timely)->urid.time_barBeat)
#define TIMELY_URI_BAR(timely)								((timely)->urid.time_bar)
#define TIMELY_URI_BEAT_UNIT(timely)					((timely)->urid.time_beatUnit)
#define TIMELY_URI_BEATS_PER_BAR(timely)			((timely)->urid.time_beatsPerBar)
#define TIMELY_URI_BEATS_PER_MINUTE(timely)		((timely)->urid.time_beatsPerMinute)
#define TIMELY_URI_FRAME(timely)							((timely)->urid.time_frame)
#define TIMELY_URI_FRAMES_PER_SECOND(timely)	((timely)->urid.time_framesPerSecond)
#define TIMELY_URI_SPEED(timely)							((timely)->urid.time_speed)

#define TIMELY_BAR_BEAT_RAW(timely)						((timely)->pos.bar_beat)
#define TIMELY_BAR_BEAT(timely)								(floor((timely)->pos.bar_beat) \
	+ (timely)->offset.beat / (timely)->frames_per_beat)
#define TIMELY_BAR(timely)										((timely)->pos.bar)
#define TIMELY_BEAT_UNIT(timely)							((timely)->pos.beat_unit)
#define TIMELY_BEATS_PER_BAR(timely)					((timely)->pos.beats_per_bar)
#define TIMELY_BEATS_PER_MINUTE(timely)				((timely)->pos.beats_per_minute)
#define TIMELY_FRAME(timely)									((timely)->pos.frame)
#define TIMELY_FRAMES_PER_SECOND(timely)			((timely)->pos.frames_per_second)
#define TIMELY_SPEED(timely)									((timely)->pos.speed)

#define TIMELY_FRAMES_PER_BEAT(timely)				((timely)->frames_per_beat)
#define TIMELY_FRAMES_PER_BAR(timely)					((timely)->frames_per_bar)

static inline void
_timely_deatomize_body(timely_t *timely, int64_t frames, uint32_t size,
	const LV2_Atom_Object_Body *body)
{
	const LV2_Atom_Float *bar_beat = NULL;
	const LV2_Atom_Long *bar = NULL;
	const LV2_Atom_Int *beat_unit = NULL;
	const LV2_Atom_Float *beats_per_bar = NULL;
	const LV2_Atom_Float *beats_per_minute = NULL;
	const LV2_Atom_Long *frame = NULL;
	const LV2_Atom_Float *frames_per_second = NULL;
	const LV2_Atom_Float *speed = NULL;

	lv2_atom_object_body_get(size, body,
		timely->urid.time_barBeat, &bar_beat,
		timely->urid.time_bar, &bar,
		timely->urid.time_beatUnit, &beat_unit,
		timely->urid.time_beatsPerBar, &beats_per_bar,
		timely->urid.time_beatsPerMinute, &beats_per_minute,
		timely->urid.time_frame, &frame,
		timely->urid.time_framesPerSecond, &frames_per_second,
		timely->urid.time_speed, &speed,
		0);

	// send speed first upon transport stop
	if(speed && (speed->body != timely->pos.speed) && (speed->body == 0.f) )
	{
		timely->pos.speed = speed->body;
		if(timely->mask & TIMELY_MASK_SPEED)
			timely->cb(timely, frames, timely->urid.time_speed, timely->data);
	}

	if(beat_unit && (beat_unit->body != timely->pos.beat_unit) )
	{
		timely->pos.beat_unit = beat_unit->body;
		if(timely->mask & TIMELY_MASK_BEAT_UNIT)
			timely->cb(timely, frames, timely->urid.time_beatUnit, timely->data);
	}

	if(beats_per_bar && (beats_per_bar->body != timely->pos.beats_per_bar) )
	{
		timely->pos.beats_per_bar = beats_per_bar->body;
		if(timely->mask & TIMELY_MASK_BEATS_PER_BAR)
			timely->cb(timely, frames, timely->urid.time_beatsPerBar, timely->data);
	}

	if(beats_per_minute && (beats_per_minute->body != timely->pos.beats_per_minute) )
	{
		timely->pos.beats_per_minute = beats_per_minute->body;
		if(timely->mask & TIMELY_MASK_BEATS_PER_MINUTE)
			timely->cb(timely, frames, timely->urid.time_beatsPerMinute, timely->data);
	}

	if(frame && (frame->body != timely->pos.frame) )
	{
		timely->pos.frame = frame->body;
		if(timely->mask & TIMELY_MASK_FRAME)
			timely->cb(timely, frames, timely->urid.time_frame, timely->data);
	}

	if(frames_per_second && (frames_per_second->body != timely->pos.frames_per_second) )
	{
		timely->pos.frames_per_second = frames_per_second->body;
		if(timely->mask & TIMELY_MASK_FRAMES_PER_SECOND)
			timely->cb(timely, frames, timely->urid.time_framesPerSecond, timely->data);
	}

	if(bar && (bar->body != timely->pos.bar) )
	{
		timely->pos.bar = bar->body;
		if(timely->mask & TIMELY_MASK_BAR)
			timely->cb(timely, frames, timely->urid.time_bar, timely->data);
	}

	if(bar_beat && (bar_beat->body != timely->pos.bar_beat) )
	{
		timely->pos.bar_beat = bar_beat->body;
		if(timely->mask & TIMELY_MASK_BAR_BEAT)
			timely->cb(timely, frames, timely->urid.time_barBeat, timely->data);
	}

	// send speed last upon transport start
	if(speed && (speed->body != timely->pos.speed) && (speed->body != 0.f) )
	{
		timely->pos.speed = speed->body;
		if(timely->mask & TIMELY_MASK_SPEED)
			timely->cb(timely, frames, timely->urid.time_speed, timely->data);
	}
}

static inline void
_timely_refresh(timely_t *timely)
{
	const float speed = (timely->pos.speed != 0.f)
		? timely->pos.speed
		: 1.f; // prevent divisions through zero later on

	timely->frames_per_beat = 240.0 * timely->pos.frames_per_second
		/ (timely->pos.beats_per_minute * timely->pos.beat_unit * speed);
	timely->frames_per_bar = timely->frames_per_beat * timely->pos.beats_per_bar;

	// bar
	timely->window.bar = timely->frames_per_bar;
	timely->offset.bar = timely->pos.bar_beat * timely->frames_per_beat;

	// beat
	timely->window.beat = timely->frames_per_beat;
	double integral;
	double beat_beat = modf(timely->pos.bar_beat, &integral);
	(void)integral;
	timely->offset.beat = beat_beat * timely->frames_per_beat;
}

static inline void
timely_init(timely_t *timely, LV2_URID_Map *map, double rate,
	timely_mask_t mask, timely_cb_t cb, void *data)
{
	assert(cb != NULL);

	timely->mask = mask;
	timely->cb = cb;
	timely->data = data;

	timely->urid.atom_object = map->map(map->handle, LV2_ATOM__Object);
	timely->urid.atom_blank = map->map(map->handle, LV2_ATOM__Blank);
	timely->urid.atom_resource = map->map(map->handle, LV2_ATOM__Resource);
	timely->urid.time_position = map->map(map->handle, LV2_TIME__Position);
	timely->urid.time_barBeat = map->map(map->handle, LV2_TIME__barBeat);
	timely->urid.time_bar = map->map(map->handle, LV2_TIME__bar);
	timely->urid.time_beatUnit = map->map(map->handle, LV2_TIME__beatUnit);
	timely->urid.time_beatsPerBar = map->map(map->handle, LV2_TIME__beatsPerBar);
	timely->urid.time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
	timely->urid.time_frame = map->map(map->handle, LV2_TIME__frame);
	timely->urid.time_framesPerSecond = map->map(map->handle, LV2_TIME__framesPerSecond);
	timely->urid.time_speed = map->map(map->handle, LV2_TIME__speed);

	timely->pos.speed = 0.f;
	timely->pos.bar_beat = 0.f;
	timely->pos.bar = 0;
	timely->pos.beat_unit = 4;
	timely->pos.beats_per_bar = 4.f;
	timely->pos.beats_per_minute = 120.f;
	timely->pos.frame = 0;
	timely->pos.frames_per_second = rate;

	_timely_refresh(timely);

	timely->first = true;
}

static inline int
timely_advance_body(timely_t *timely, uint32_t size, uint32_t type,
	const LV2_Atom_Object_Body *body, uint32_t from, uint32_t to)
{
	if(timely->first)
	{
		timely->first = false;

		// send initial values
		if(timely->mask & TIMELY_MASK_SPEED)
			timely->cb(timely, 0, timely->urid.time_speed, timely->data);

		if(timely->mask & TIMELY_MASK_BEAT_UNIT)
			timely->cb(timely, 0, timely->urid.time_beatUnit, timely->data);

		if(timely->mask & TIMELY_MASK_BEATS_PER_BAR)
			timely->cb(timely, 0, timely->urid.time_beatsPerBar, timely->data);

		if(timely->mask & TIMELY_MASK_BEATS_PER_MINUTE)
			timely->cb(timely, 0, timely->urid.time_beatsPerMinute, timely->data);

		if(timely->mask & TIMELY_MASK_FRAME)
			timely->cb(timely, 0, timely->urid.time_frame, timely->data);

		if(timely->mask & TIMELY_MASK_FRAMES_PER_SECOND)
			timely->cb(timely, 0, timely->urid.time_framesPerSecond, timely->data);

		if(timely->mask & TIMELY_MASK_BAR)
			timely->cb(timely, 0, timely->urid.time_bar, timely->data);

		if(timely->mask & TIMELY_MASK_BAR_BEAT)
			timely->cb(timely, 0, timely->urid.time_barBeat, timely->data);
	}

	// are we rolling?
	if(timely->pos.speed != 0.f)
	{
		if( (timely->offset.bar == 0) && (timely->pos.bar == 0) )
		{
			if(timely->mask & (TIMELY_MASK_BAR | TIMELY_MASK_BAR_WHOLE) )
				timely->cb(timely, from, timely->urid.time_bar, timely->data);
		}

		if( (timely->offset.beat == 0) && (timely->pos.bar_beat == 0) )
		{
			if(timely->mask & (TIMELY_MASK_BAR_BEAT | TIMELY_MASK_BAR_BEAT_WHOLE) )
				timely->cb(timely, from, timely->urid.time_barBeat, timely->data);
		}

		unsigned update_frame = to;
		for(unsigned i=from; i<to; i++)
		{
			if(timely->offset.bar >= timely->window.bar)
			{
				timely->pos.bar += 1;
				timely->offset.bar -= timely->window.bar;

				if(timely->mask & TIMELY_MASK_FRAME)
					timely->cb(timely, (update_frame = i), timely->urid.time_frame, timely->data);

				if(timely->mask & TIMELY_MASK_BAR_WHOLE)
					timely->cb(timely, i, timely->urid.time_bar, timely->data);
			}

			if( (timely->offset.beat >= timely->window.beat) )
			{
				timely->pos.bar_beat = floor(timely->pos.bar_beat) + 1;
				timely->offset.beat -= timely->window.beat;

				if(timely->pos.bar_beat >= timely->pos.beats_per_bar)
					timely->pos.bar_beat -= timely->pos.beats_per_bar;

				if( (timely->mask & TIMELY_MASK_FRAME) && (update_frame != i) )
					timely->cb(timely, (update_frame = i), timely->urid.time_frame, timely->data);

				if(timely->mask & TIMELY_MASK_BAR_BEAT_WHOLE)
					timely->cb(timely, i, timely->urid.time_barBeat, timely->data);
			}

			timely->offset.bar += 1;
			timely->offset.beat += 1;
			timely->pos.frame += 1;
		}
	}

	// is this a time position event?
	if(  ( (type == timely->urid.atom_object)
			|| (type == timely->urid.atom_blank)
			|| (type == timely->urid.atom_resource) )
		&& body && (body->otype == timely->urid.time_position) )
	{
		_timely_deatomize_body(timely, to, size, body);
		_timely_refresh(timely);

		return 1; // handled a time position event
	}

	return 0; // did not handle a time position event
}

static inline int
timely_advance(timely_t *timely, const LV2_Atom_Object *obj,
	uint32_t from, uint32_t to)
{
	if(obj)
		return timely_advance_body(timely, obj->atom.size, obj->atom.type, &obj->body, from, to);

	return timely_advance_body(timely, 0, 0, NULL, from, to);
}

#endif // _LV2_TIMELY_H_
