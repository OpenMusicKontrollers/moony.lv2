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

#ifndef _LV2_OSC_H_
#define _LV2_OSC_H_

#include <math.h> // INFINITY

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>

#define OSC_URI								"http://open-music-kontrollers.ch/lv2/osc"
#define OSC_PREFIX						OSC_URI "#"	

#define OSC__Event						OSC_PREFIX "Event"						// event
#define OSC__schedule					OSC_PREFIX "schedule"					// feature

#define OSC__Bundle						OSC_PREFIX "Bundle"						// object otype
#define OSC__Message					OSC_PREFIX "Message"					// object otype
#define OSC__bundleTimestamp	OSC_PREFIX "bundleTimestamp"	// property key
#define OSC__bundleItems			OSC_PREFIX "bundleItems"			// property key
#define OSC__messagePath			OSC_PREFIX "messagePath"			// property key
#define OSC__messageFormat		OSC_PREFIX "messageFormat"		// property key
#define OSC__messageArguments	OSC_PREFIX "messageArguments"	// property key

typedef void *osc_schedule_handle_t;
typedef struct _osc_schedule_t osc_schedule_t;
typedef struct _osc_forge_t osc_forge_t;

typedef void (*osc_bundle_push_cb_t)(uint64_t timestamp, void *data);
typedef void (*osc_bundle_pop_cb_t)(void *data);
typedef void (*osc_message_cb_t)(const char *path, const char *fmt,
	const LV2_Atom_Tuple *arguments, void *data);

typedef double (*osc_schedule_osc2frames_t)(osc_schedule_handle_t handle,
	uint64_t timestamp);
typedef uint64_t (*osc_schedule_frames2osc_t)(osc_schedule_handle_t handle,
	double frames);

struct _osc_schedule_t {
	osc_schedule_osc2frames_t osc2frames;
	osc_schedule_frames2osc_t frames2osc;
	osc_schedule_handle_t handle;
};

struct _osc_forge_t {
	LV2_URID OSC_Event;

	LV2_URID OSC_Bundle;
	LV2_URID OSC_Message;

	LV2_URID OSC_bundleTimestamp;
	LV2_URID OSC_bundleItems;

	LV2_URID OSC_messagePath;
	LV2_URID OSC_messageFormat;
	LV2_URID OSC_messageArguments;

	LV2_URID MIDI_MidiEvent;
	
	LV2_URID ATOM_Object;
};

static inline void
osc_forge_init(osc_forge_t *oforge, LV2_URID_Map *map)
{
	oforge->OSC_Event = map->map(map->handle, OSC__Event);

	oforge->OSC_Bundle = map->map(map->handle, OSC__Bundle);
	oforge->OSC_Message = map->map(map->handle, OSC__Message);

	oforge->OSC_bundleTimestamp = map->map(map->handle, OSC__bundleTimestamp);
	oforge->OSC_bundleItems = map->map(map->handle, OSC__bundleItems);

	oforge->OSC_messagePath = map->map(map->handle, OSC__messagePath);
	oforge->OSC_messageFormat = map->map(map->handle, OSC__messageFormat);
	oforge->OSC_messageArguments = map->map(map->handle, OSC__messageArguments);
	
	oforge->MIDI_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
	
	oforge->ATOM_Object = map->map(map->handle, LV2_ATOM__Object);
}

static inline int
osc_atom_is_bundle(osc_forge_t *oforge, const LV2_Atom_Object *obj)
{
	return (obj->atom.type == oforge->ATOM_Object)
		&& (obj->body.otype == oforge->OSC_Bundle);
}

static inline void
osc_atom_bundle_unpack(osc_forge_t *oforge, const LV2_Atom_Object *obj,
	const LV2_Atom_Long **timestamp, const LV2_Atom_Tuple **items)
{
	*timestamp = NULL;
	*items = NULL;

	LV2_Atom_Object_Query q [] = {
		{ oforge->OSC_bundleTimestamp, (const LV2_Atom **)timestamp },
		{ oforge->OSC_bundleItems, (const LV2_Atom **)items },
		LV2_ATOM_OBJECT_QUERY_END
	};

	lv2_atom_object_query(obj, q);
}

static inline int
osc_atom_is_message(osc_forge_t *oforge, const LV2_Atom_Object *obj)
{
	return (obj->atom.type == oforge->ATOM_Object)
		&& (obj->body.otype == oforge->OSC_Message);
}

static inline void
osc_atom_message_unpack(osc_forge_t *oforge, const LV2_Atom_Object *obj,
	const LV2_Atom_String **path, const LV2_Atom_String **format,
	const LV2_Atom_Tuple **arguments)
{
	*path = NULL;
	*format = NULL;
	*arguments = NULL;

	LV2_Atom_Object_Query q [] = {
		{ oforge->OSC_messagePath, (const LV2_Atom **)path },
		{ oforge->OSC_messageFormat, (const LV2_Atom **)format },
		{ oforge->OSC_messageArguments, (const LV2_Atom **)arguments },
		LV2_ATOM_OBJECT_QUERY_END
	};

	lv2_atom_object_query(obj, q);
}

static inline void osc_atom_event_unroll(osc_forge_t *oforge,
	const LV2_Atom_Object *obj, osc_bundle_push_cb_t bundle_push_cb,
	osc_bundle_pop_cb_t bundle_pop_cb, osc_message_cb_t message_cb, void *data);

static inline void
osc_atom_message_unroll(osc_forge_t *oforge, const LV2_Atom_Object *obj,
	osc_message_cb_t message_cb, void *data)
{
	const LV2_Atom_String* path;
	const LV2_Atom_String* fmt;
	const LV2_Atom_Tuple* args;

	osc_atom_message_unpack(oforge, obj, &path, &fmt, &args);

	const char *path_str = path ? LV2_ATOM_BODY_CONST(path) : NULL;
	const char *fmt_str = fmt ? LV2_ATOM_BODY_CONST(fmt) : NULL;

	if(message_cb)
		message_cb(path_str, fmt_str, args, data);
}

static inline void
osc_atom_bundle_unroll(osc_forge_t *oforge, const LV2_Atom_Object *obj,
	osc_bundle_push_cb_t bundle_push_cb, osc_bundle_pop_cb_t bundle_pop_cb,
	osc_message_cb_t message_cb, void *data)
{
	const LV2_Atom_Long* timestamp;
	const LV2_Atom_Tuple* items;

	osc_atom_bundle_unpack(oforge, obj, &timestamp, &items);

	uint64_t timestamp_body = timestamp ? (uint64_t)timestamp->body : 1ULL;

	if(bundle_push_cb)
		bundle_push_cb(timestamp_body, data);

	// iterate over tuple body
	if(items)
	{
		for(const LV2_Atom *itr = lv2_atom_tuple_begin(items);
			!lv2_atom_tuple_is_end(LV2_ATOM_BODY(items), items->atom.size, itr);
			itr = lv2_atom_tuple_next(itr))
		{
			osc_atom_event_unroll(oforge, (const LV2_Atom_Object *)itr,
				bundle_push_cb, bundle_pop_cb, message_cb, data);
		}
	}

	if(bundle_pop_cb)
		bundle_pop_cb(data);
}

static inline void
osc_atom_event_unroll(osc_forge_t *oforge, const LV2_Atom_Object *obj,
	osc_bundle_push_cb_t bundle_push_cb, osc_bundle_pop_cb_t bundle_pop_cb,
	osc_message_cb_t message_cb, void *data)
{
	if(osc_atom_is_bundle(oforge, obj))
	{
		osc_atom_bundle_unroll(oforge, obj, bundle_push_cb, bundle_pop_cb,
			message_cb, data);
	}
	else if(osc_atom_is_message(oforge, obj))
	{
		osc_atom_message_unroll(oforge, obj, message_cb, data);
	}
}

static inline LV2_Atom_Forge_Ref
osc_forge_bundle_push(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	LV2_Atom_Forge_Frame frame [2], uint64_t timestamp)
{
	if(!lv2_atom_forge_object(forge, &frame[0], 0, oforge->OSC_Bundle))
		return 0;

	if(!lv2_atom_forge_key(forge, oforge->OSC_bundleTimestamp))
		return 0;
	if(!lv2_atom_forge_long(forge, timestamp))
		return 0;

	if(!lv2_atom_forge_key(forge, oforge->OSC_bundleItems))
		return 0;

	return lv2_atom_forge_tuple(forge, &frame[1]);
}

static inline void
osc_forge_bundle_pop(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	LV2_Atom_Forge_Frame frame [2])
{
	lv2_atom_forge_pop(forge, &frame[1]);
	lv2_atom_forge_pop(forge, &frame[0]);
}

static inline LV2_Atom_Forge_Ref
osc_forge_message_push(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	LV2_Atom_Forge_Frame frame [2], const char *path, const char *fmt)
{
	if(!lv2_atom_forge_object(forge, &frame[0], 0, oforge->OSC_Message))
		return 0;

	if(!lv2_atom_forge_key(forge, oforge->OSC_messagePath))
		return 0;
	if(!lv2_atom_forge_string(forge, path, strlen(path)))
		return 0;

	if(!lv2_atom_forge_key(forge, oforge->OSC_messageFormat))
		return 0;
	if(!lv2_atom_forge_string(forge, fmt, strlen(fmt)))
		return 0;

	if(!lv2_atom_forge_key(forge, oforge->OSC_messageArguments))
		return 0;

	return lv2_atom_forge_tuple(forge, &frame[1]);
}

static inline void
osc_forge_message_pop(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	LV2_Atom_Forge_Frame frame [2])
{
	lv2_atom_forge_pop(forge, &frame[1]);
	lv2_atom_forge_pop(forge, &frame[0]);
}

static inline LV2_Atom_Forge_Ref
osc_forge_int32(osc_forge_t *oforge, LV2_Atom_Forge *forge, int32_t i)
{
	return lv2_atom_forge_int(forge, i);
}

static inline LV2_Atom_Forge_Ref
osc_forge_float(osc_forge_t *oforge, LV2_Atom_Forge *forge, float f)
{
	return lv2_atom_forge_float(forge, f);
}

static inline LV2_Atom_Forge_Ref
osc_forge_string(osc_forge_t *oforge, LV2_Atom_Forge *forge, const char *s)
{
	return lv2_atom_forge_string(forge, s, strlen(s));
}

static inline LV2_Atom_Forge_Ref
osc_forge_symbol(osc_forge_t *oforge, LV2_Atom_Forge *forge, const char *s)
{
	return lv2_atom_forge_string(forge, s, strlen(s));
}

static inline LV2_Atom_Forge_Ref
osc_forge_blob(osc_forge_t *oforge, LV2_Atom_Forge *forge, uint32_t size,
	const uint8_t *b)
{
	LV2_Atom_Forge_Ref ref;
	if(!(ref = lv2_atom_forge_atom(forge, size, forge->Chunk)))
		return 0;
	if(!(ref = lv2_atom_forge_raw(forge, b, size)))
		return 0;
	lv2_atom_forge_pad(forge, size);

	return ref;
}

static inline LV2_Atom_Forge_Ref
osc_forge_int64(osc_forge_t *oforge, LV2_Atom_Forge *forge, int64_t h)
{
	return lv2_atom_forge_long(forge, h);
}

static inline LV2_Atom_Forge_Ref
osc_forge_double(osc_forge_t *oforge, LV2_Atom_Forge *forge, double d)
{
	return lv2_atom_forge_double(forge, d);
}

static inline LV2_Atom_Forge_Ref
osc_forge_timestamp(osc_forge_t *oforge, LV2_Atom_Forge *forge, uint64_t t)
{
	return lv2_atom_forge_long(forge, t);
}

static inline LV2_Atom_Forge_Ref
osc_forge_char(osc_forge_t *oforge, LV2_Atom_Forge *forge, char c)
{
	return lv2_atom_forge_int(forge, c);
}

static inline LV2_Atom_Forge_Ref
osc_forge_midi(osc_forge_t *oforge, LV2_Atom_Forge *forge, uint32_t size,
	const uint8_t *m)
{
	LV2_Atom_Forge_Ref ref;
	if(!(ref = lv2_atom_forge_atom(forge, size, oforge->MIDI_MidiEvent)))
		return 0;
	if(!(ref = lv2_atom_forge_raw(forge, m, size)))
		return 0;
	lv2_atom_forge_pad(forge, size);

	return ref;
}

static inline LV2_Atom_Forge_Ref
osc_forge_message_varlist(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const char *path, const char *fmt, va_list args)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	if(!(ref = osc_forge_message_push(oforge, forge, frame, path, fmt)))
		return 0;

	for(const char *type = fmt; *type; type++)
	{
		switch(*type)
		{
			case 'i':
			{
				if(!(ref =osc_forge_int32(oforge, forge, va_arg(args, int32_t))))
					return 0;
				break;
			}
			case 'f':
			{
				if(!(ref = osc_forge_float(oforge, forge, (float)va_arg(args, double))))
					return 0;
				break;
			}
			case 's':
			{
				if(!(ref = osc_forge_string(oforge, forge, va_arg(args, const char *))))
					return 0;
				break;
			}
			case 'S':
			{
				if(!(ref = osc_forge_symbol(oforge, forge, va_arg(args, const char *))))
					return 0;
				break;
			}
			case 'b':
			{
				uint32_t size = va_arg(args, uint32_t);
				const uint8_t *b = va_arg(args, const uint8_t *);
				if(!(ref = osc_forge_blob(oforge, forge, size, b)))
					return 0;
				break;
			}
			
			case 'h':
			{
				if(!(ref = osc_forge_int64(oforge, forge, va_arg(args, int64_t))))
					return 0;
				break;
			}
			case 'd':
			{
				if(!(ref = osc_forge_double(oforge, forge, va_arg(args, double))))
					return 0;
				break;
			}
			case 't':
			{
				if(!(ref = osc_forge_timestamp(oforge, forge, va_arg(args, uint64_t))))
					return 0;
				break;
			}
			
			case 'c':
			{
				if(!(ref = osc_forge_char(oforge, forge, (char)va_arg(args, unsigned int))))
					return 0;
				break;
			}
			case 'm':
			{
				int32_t size = va_arg(args, int32_t);
				const uint8_t *m = va_arg(args, const uint8_t *);
				if(!(ref = osc_forge_midi(oforge, forge, size, m)))
					return 0;
				break;
			}
			
			case 'T':
			case 'F':
			case 'N':
			case 'I':
			{
				break;
			}

			default: // unknown argument type
			{
				return 0;
			}
		}
	}

	osc_forge_message_pop(oforge, forge, frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
osc_forge_message_vararg(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const char *path, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	LV2_Atom_Forge_Ref ref;
	ref = osc_forge_message_varlist(oforge, forge, path, fmt, args);

	va_end(args);

	return ref;
}

static inline const LV2_Atom *
osc_deforge_int32(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, int32_t *i)
{
	if(!atom || (atom->type != forge->Int) )
		return NULL;

	*i = ((const LV2_Atom_Int *)atom)->body;

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_float(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, float *f)
{
	if(!atom || (atom->type != forge->Float) )
		return NULL;

	*f = ((const LV2_Atom_Float *)atom)->body;

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_string(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, const char **s)
{
	if(!atom || (atom->type != forge->String) )
		return NULL;

	*s = LV2_ATOM_BODY_CONST(atom);

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_symbol(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, const char **s)
{
	if(!atom || (atom->type != forge->String) )
		return NULL;

	*s = LV2_ATOM_BODY_CONST(atom);

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_blob(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, uint32_t *size, const uint8_t **b)
{
	if(!atom || (atom->type != forge->Chunk) )
		return NULL;

	*size = atom->size;
	*b = LV2_ATOM_BODY_CONST(atom);

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_int64(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, int64_t *h)
{
	if(!atom || (atom->type != forge->Long) )
		return NULL;

	*h = ((const LV2_Atom_Long *)atom)->body;

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_double(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, double *d)
{
	if(!atom || (atom->type != forge->Double) )
		return NULL;

	*d = ((const LV2_Atom_Double *)atom)->body;

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_timestamp(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, uint64_t *t)
{
	if(!atom || (atom->type != forge->Long) )
		return NULL;

	*t = ((const LV2_Atom_Long *)atom)->body;

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_char(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, char *c)
{
	if(!atom || (atom->type != forge->Int) )
		return NULL;

	*c = ((const LV2_Atom_Int *)atom)->body;

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_midi(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, uint32_t *size, const uint8_t **m)
{
	if(!atom || (atom->type != oforge->MIDI_MidiEvent) )
		return NULL;

	*size = atom->size;
	*m = LV2_ATOM_BODY_CONST(atom);

	return lv2_atom_tuple_next(atom);
}

static inline const LV2_Atom *
osc_deforge_message_varlist(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, const char *fmt, va_list args)
{
	for(const char *type = fmt; *type; type++)
	{
		switch(*type)
		{
			case 'i':
			{
				int32_t *i = va_arg(args, int32_t *);
				if(!(atom = osc_deforge_int32(oforge, forge, atom, i)))
					return NULL;
				break;
			}
			case 'f':
			{
				float *f = va_arg(args, float *);
				if(!(atom = osc_deforge_float(oforge, forge, atom, f)))
					return NULL;
				break;
			}
			case 's':
			{
				const char **s = va_arg(args, const char **);
				if(!(atom = osc_deforge_string(oforge, forge, atom, s)))
					return NULL;
				break;
			}
			case 'S':
			{
				const char **s = va_arg(args, const char **);
				if(!(atom = osc_deforge_symbol(oforge, forge, atom, s)))
					return NULL;
				break;
			}
			case 'b':
			{
				uint32_t *size = va_arg(args, uint32_t *);
				const uint8_t **b = va_arg(args, const uint8_t **);
				if(!(atom = osc_deforge_blob(oforge, forge, atom, size, b)))
					return NULL;
				break;
			}
			
			case 'h':
			{
				int64_t *h = va_arg(args, int64_t *);
				if(!(atom = osc_deforge_int64(oforge, forge, atom, h)))
					return NULL;
				break;
			}
			case 'd':
			{
				double *d = va_arg(args, double *);
				if(!(atom = osc_deforge_double(oforge, forge, atom, d)))
					return NULL;
				break;
			}
			case 't':
			{
				uint64_t *t = va_arg(args, uint64_t *);
				if(!(atom = osc_deforge_timestamp(oforge, forge, atom, t)))
					return NULL;
				break;
			}
			
			case 'c':
			{
				char *c = va_arg(args, char *);
				if(!(atom = osc_deforge_char(oforge, forge, atom, c)))
					return NULL;
				break;
			}
			case 'm':
			{
				uint32_t *size = va_arg(args, uint32_t *);
				const uint8_t **m = va_arg(args, const uint8_t **);
				if(!(atom = osc_deforge_midi(oforge, forge, atom, size, m)))
					return NULL;
				break;
			}
			
			case 'T':
			case 'F':
			case 'N':
			case 'I':
			{
				break;
			}

			default: // unknown argument type
			{
				return NULL;
			}
		}
	}

	return atom;
}

static inline const LV2_Atom *
osc_deforge_message_vararg(osc_forge_t *oforge, LV2_Atom_Forge *forge,
	const LV2_Atom *atom, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	atom = osc_deforge_message_varlist(oforge, forge, atom, fmt, args);

	va_end(args);

	return atom;
}

#endif // _LV2_OSC_H_
