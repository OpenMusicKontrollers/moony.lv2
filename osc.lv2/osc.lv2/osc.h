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

#ifndef LV2_OSC_H
#define LV2_OSC_H

#include <stdint.h>

#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>

#define LV2_OSC_URI                 "http://open-music-kontrollers.ch/lv2/osc"
#define LV2_OSC_PREFIX              LV2_OSC_URI "#"	

#define LV2_OSC__Event              LV2_OSC_PREFIX "Event" // atom message type 
#define LV2_OSC__schedule           LV2_OSC_PREFIX "schedule" // feature

#define LV2_OSC__Packet             LV2_OSC_PREFIX "Packet" // atom object type

#define LV2_OSC__Bundle             LV2_OSC_PREFIX "Bundle" // atom object type
#define LV2_OSC__bundleTimetag      LV2_OSC_PREFIX "bundleTimetag" // atom object property
#define LV2_OSC__bundleItems        LV2_OSC_PREFIX "bundleItems"

#define LV2_OSC__Message            LV2_OSC_PREFIX "Message" // atom object type
#define LV2_OSC__messagePath        LV2_OSC_PREFIX "messagePath" // atom object property
#define LV2_OSC__messageArguments   LV2_OSC_PREFIX "messageArguments" // atom object property 

#define LV2_OSC__Timetag            LV2_OSC_PREFIX "Timetag" // atom object type
#define LV2_OSC__timetagIntegral    LV2_OSC_PREFIX "timetagIntegral" // atom object property
#define LV2_OSC__timetagFraction    LV2_OSC_PREFIX "timetagFraction" // atom object property 

#define LV2_OSC__Nil                LV2_OSC_PREFIX "Nil" // atom literal type
#define LV2_OSC__Impulse            LV2_OSC_PREFIX "Impulse" // atom literal type
#define LV2_OSC__Char               LV2_OSC_PREFIX "Char" // atom literal type
#define LV2_OSC__RGBA               LV2_OSC_PREFIX "RGBA" // atom literal type

#define LV2_OSC_PADDED_SIZE(size) ( ( (size_t)(size) + 3 ) & ( ~3 ) )
#define LV2_OSC_IMMEDIATE         1ULL

#ifdef __cplusplus
extern "C" {
#endif

typedef void *LV2_OSC_Schedule_Handle;

typedef double (*LV2_OSC_Schedule_OSC2Frames)(
	LV2_OSC_Schedule_Handle handle,
	uint64_t timetag);

typedef uint64_t (*LV2_OSC_Schedule_Frames2OSC)(
	LV2_OSC_Schedule_Handle handle,
	double frames);

typedef struct _LV2_OSC_Schedule {
	LV2_OSC_Schedule_Handle handle;
	LV2_OSC_Schedule_OSC2Frames osc2frames;
	LV2_OSC_Schedule_Frames2OSC frames2osc;
} LV2_OSC_Schedule;

typedef enum LV2_OSC_Type {
	LV2_OSC_INT32   =	'i',
	LV2_OSC_FLOAT   =	'f',
	LV2_OSC_STRING  =	's',
	LV2_OSC_BLOB    =	'b',
	
	LV2_OSC_TRUE    =	'T',
	LV2_OSC_FALSE   =	'F',
	LV2_OSC_NIL     =	'N',
	LV2_OSC_IMPULSE =	'I',
	
	LV2_OSC_INT64   =	'h',
	LV2_OSC_DOUBLE  =	'd',
	LV2_OSC_TIMETAG =	't',
	
	LV2_OSC_SYMBOL  =	'S',
	LV2_OSC_CHAR    =	'c',
	LV2_OSC_MIDI    =	'm',
	LV2_OSC_RGBA    =	'r'
} LV2_OSC_Type;

union swap32_t {
	uint32_t u;

	int32_t i;
	float f;
};

union swap64_t {
	uint64_t u;

	int64_t h;
	uint64_t t;
	double d;
};

typedef struct _LV2_OSC_Timetag {
	uint32_t integral;
	uint32_t fraction;
} LV2_OSC_Timetag;

typedef struct _LV2_OSC_URID {
	LV2_URID OSC_Packet;

	LV2_URID OSC_Bundle;
	LV2_URID OSC_bundleTimetag;
	LV2_URID OSC_bundleItems;

	LV2_URID OSC_Message;
	LV2_URID OSC_messagePath;
	LV2_URID OSC_messageArguments;

	LV2_URID OSC_Timetag;
	LV2_URID OSC_timetagIntegral;
	LV2_URID OSC_timetagFraction;

	LV2_URID OSC_Nil;
	LV2_URID OSC_Impulse;
	LV2_URID OSC_Char;
	LV2_URID OSC_RGBA;

	LV2_URID MIDI_MidiEvent;

	LV2_URID ATOM_Int;
	LV2_URID ATOM_Long;
	LV2_URID ATOM_String;
	LV2_URID ATOM_Literal;
	LV2_URID ATOM_Float;
	LV2_URID ATOM_Double;
	LV2_URID ATOM_URID;
	LV2_URID ATOM_Bool;
	LV2_URID ATOM_Tuple;
	LV2_URID ATOM_Object;
	LV2_URID ATOM_Chunk;
} LV2_OSC_URID;

static inline void
lv2_osc_urid_init(LV2_OSC_URID *osc_urid, LV2_URID_Map *map)
{
	osc_urid->OSC_Packet = map->map(map->handle, LV2_OSC__Packet);

	osc_urid->OSC_Bundle = map->map(map->handle, LV2_OSC__Bundle);
	osc_urid->OSC_bundleTimetag = map->map(map->handle, LV2_OSC__bundleTimetag);
	osc_urid->OSC_bundleItems = map->map(map->handle, LV2_OSC__bundleItems);

	osc_urid->OSC_Message = map->map(map->handle, LV2_OSC__Message);
	osc_urid->OSC_messagePath = map->map(map->handle, LV2_OSC__messagePath);
	osc_urid->OSC_messageArguments = map->map(map->handle, LV2_OSC__messageArguments);

	osc_urid->OSC_Timetag = map->map(map->handle, LV2_OSC__Timetag);
	osc_urid->OSC_timetagIntegral = map->map(map->handle, LV2_OSC__timetagIntegral);
	osc_urid->OSC_timetagFraction = map->map(map->handle, LV2_OSC__timetagFraction);

	osc_urid->OSC_Nil = map->map(map->handle, LV2_OSC__Nil);
	osc_urid->OSC_Impulse = map->map(map->handle, LV2_OSC__Impulse);
	osc_urid->OSC_Char = map->map(map->handle, LV2_OSC__Char);
	osc_urid->OSC_RGBA = map->map(map->handle, LV2_OSC__RGBA);

	osc_urid->MIDI_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);

	osc_urid->ATOM_Int = map->map(map->handle, LV2_ATOM__Int);
	osc_urid->ATOM_Long = map->map(map->handle, LV2_ATOM__Long);
	osc_urid->ATOM_String = map->map(map->handle, LV2_ATOM__String);
	osc_urid->ATOM_Literal = map->map(map->handle, LV2_ATOM__Literal);
	osc_urid->ATOM_Float = map->map(map->handle, LV2_ATOM__Float);
	osc_urid->ATOM_Double = map->map(map->handle, LV2_ATOM__Double);
	osc_urid->ATOM_URID = map->map(map->handle, LV2_ATOM__URID);
	osc_urid->ATOM_Bool = map->map(map->handle, LV2_ATOM__Bool);
	osc_urid->ATOM_Tuple = map->map(map->handle, LV2_ATOM__Tuple);
	osc_urid->ATOM_Object = map->map(map->handle, LV2_ATOM__Object);
	osc_urid->ATOM_Chunk = map->map(map->handle, LV2_ATOM__Chunk);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV2_OSC_H
