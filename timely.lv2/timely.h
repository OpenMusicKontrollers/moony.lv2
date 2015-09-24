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
	TIMELY_MASK_SPEED							= (1 << 7)
};

struct _timely_t {
	struct {
		LV2_URID atom_object;
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

#define TIMELY_BAR_BEAT(timely)								((timely)->pos.bar_beat)
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
_timely_deatomize(timely_t *timely, int64_t frames, const LV2_Atom_Object *obj)
{
	const LV2_Atom_Float *bar_beat = NULL;
	const LV2_Atom_Long *bar = NULL;
	const LV2_Atom_Int *beat_unit = NULL;
	const LV2_Atom_Float *beats_per_bar = NULL;
	const LV2_Atom_Float *beats_per_minute = NULL;
	const LV2_Atom_Long *frame = NULL;
	const LV2_Atom_Float *frames_per_second = NULL;
	const LV2_Atom_Float *speed = NULL;

	LV2_Atom_Object_Query q [] = {
		{ timely->urid.time_barBeat, (const LV2_Atom **)&bar_beat },
		{ timely->urid.time_bar, (const LV2_Atom **)&bar },
		{ timely->urid.time_beatUnit, (const LV2_Atom **)&beat_unit },
		{ timely->urid.time_beatsPerBar, (const LV2_Atom **)&beats_per_bar },
		{ timely->urid.time_beatsPerMinute, (const LV2_Atom **)&beats_per_minute },
		{ timely->urid.time_frame, (const LV2_Atom **)&frame },
		{ timely->urid.time_framesPerSecond, (const LV2_Atom **)&frames_per_second },
		{ timely->urid.time_speed, (const LV2_Atom **)&speed },
		LV2_ATOM_OBJECT_QUERY_END
	};

	lv2_atom_object_query(obj, q);

	// send speed first upon transport stop
	if(speed && (speed->body != timely->pos.speed) && (speed->body == 0.f) )
	{
		timely->pos.speed = speed->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_SPEED) )
			timely->cb(timely, frames, timely->urid.time_speed, timely->data);
	}

	if(beat_unit && (beat_unit->body != timely->pos.beat_unit) )
	{
		timely->pos.beat_unit = beat_unit->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_BEAT_UNIT) )
			timely->cb(timely, frames, timely->urid.time_beatUnit, timely->data);
	}

	if(beats_per_bar && (beats_per_bar->body != timely->pos.beats_per_bar) )
	{
		timely->pos.beats_per_bar = beats_per_bar->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_BEATS_PER_BAR) )
			timely->cb(timely, frames, timely->urid.time_beatsPerBar, timely->data);
	}

	if(beats_per_minute && (beats_per_minute->body != timely->pos.beats_per_minute) )
	{
		timely->pos.beats_per_minute = beats_per_minute->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_BEATS_PER_MINUTE) )
			timely->cb(timely, frames, timely->urid.time_beatsPerMinute, timely->data);
	}

	if(frame && (frame->body != timely->pos.frame) )
	{
		timely->pos.frame = frame->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_FRAME) )
			timely->cb(timely, frames, timely->urid.time_frame, timely->data);
	}

	if(frames_per_second && (frames_per_second->body != timely->pos.frames_per_second) )
	{
		timely->pos.frames_per_second = frames_per_second->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_FRAMES_PER_SECOND) )
			timely->cb(timely, frames, timely->urid.time_framesPerSecond, timely->data);
	}

	if(bar && (bar->body != timely->pos.bar) )
	{
		timely->pos.bar = bar->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_BAR) )
			timely->cb(timely, frames, timely->urid.time_bar, timely->data);
	}

	if(bar_beat && (bar_beat->body != timely->pos.bar_beat) )
	{
		timely->pos.bar_beat = bar_beat->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_BAR_BEAT) )
			timely->cb(timely, frames, timely->urid.time_barBeat, timely->data);
	}

	// send speed last upon transport start
	if(speed && (speed->body != timely->pos.speed) && (speed->body != 0.f) )
	{
		timely->pos.speed = speed->body;
		if(timely->cb && (timely->mask & TIMELY_MASK_SPEED) )
			timely->cb(timely, frames, timely->urid.time_speed, timely->data);
	}
}

static inline void
_timely_refresh(timely_t *timely)
{
	timely->frames_per_beat = 240.0 / (timely->pos.beats_per_minute * timely->pos.beat_unit)
		* timely->pos.frames_per_second;
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

static void
timely_init(timely_t *timely, LV2_URID_Map *map, double rate,
	timely_mask_t mask, timely_cb_t cb, void *data)
{
	timely->mask = mask;
	timely->cb = cb;
	timely->data = data;

	timely->urid.atom_object = map->map(map->handle, LV2_ATOM__Object);
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

static int
timely_advance(timely_t *timely, const LV2_Atom_Object *obj,
	uint32_t from, uint32_t to)
{
	if(timely->first)
	{
		timely->first = false;

		// send initial values
		if(timely->cb)
		{
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
	}

	// are we rolling?
	if(timely->pos.speed > 0.f)
	{
		if( (timely->offset.bar == 0) && (timely->pos.bar == 0) )
		{
			if(timely->cb && (timely->mask & TIMELY_MASK_BAR) )
				timely->cb(timely, from, timely->urid.time_bar, timely->data);
		}

		if( (timely->offset.beat == 0) && (timely->pos.bar_beat == 0) )
		{
			if(timely->cb && (timely->mask & TIMELY_MASK_BAR_BEAT) )
				timely->cb(timely, from, timely->urid.time_barBeat, timely->data); 
		}

		for(unsigned i=from; i<to; i++)
		{
			if(timely->offset.bar >= timely->window.bar)
			{
				timely->pos.bar += 1;
				timely->offset.bar -= timely->window.bar;

				if(timely->cb && (timely->mask & TIMELY_MASK_BAR) )
					timely->cb(timely, i, timely->urid.time_bar, timely->data);
			}

			if( (timely->offset.beat >= timely->window.beat) )
			{
				timely->pos.bar_beat = floor(timely->pos.bar_beat) + 1;
				timely->offset.beat -= timely->window.beat;

				if(timely->pos.bar_beat >= timely->pos.beats_per_bar)
					timely->pos.bar_beat = 0;

				if(timely->cb && (timely->mask & TIMELY_MASK_BAR_BEAT) )
					timely->cb(timely, i, timely->urid.time_barBeat, timely->data); 
			}

			timely->offset.bar += 1;
			timely->offset.beat += 1;
		}
		
		timely->pos.frame += to - from;
	}

	// is this a time position event?
	if(  obj
		&& (obj->atom.type == timely->urid.atom_object)
		&& (obj->body.otype == timely->urid.time_position) )
	{
		_timely_deatomize(timely, to, obj);
		_timely_refresh(timely);

		return 1; // handled a time position event
	}

	return 0; // did not handle a time position event
}

#endif // _LV2_TIMELY_H_
