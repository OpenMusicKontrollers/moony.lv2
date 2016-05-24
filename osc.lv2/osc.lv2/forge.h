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

#ifndef LV2_OSC_FORGE_H
#define LV2_OSC_FORGE_H

#include <inttypes.h>

#include <osc.lv2/osc.h>
#include <osc.lv2/util.h>
#include <osc.lv2/reader.h>

#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#ifdef __cplusplus
extern "C" {
#endif

#define lv2_osc_forge_int(forge, osc_urid, val) \
	lv2_atom_forge_int((forge), (val))

#define lv2_osc_forge_float(forge, osc_urid, val) \
	lv2_atom_forge_float((forge), (val))

#define lv2_osc_forge_string(forge, osc_urid, val, len) \
	lv2_atom_forge_string((forge), (val), (len))

#define lv2_osc_forge_long(forge, osc_urid, val) \
	lv2_atom_forge_long((forge), (val))

#define lv2_osc_forge_double(forge, osc_urid, val) \
	lv2_atom_forge_double((forge), (val))

#define lv2_osc_forge_true(forge, osc_urid) \
	lv2_atom_forge_bool((forge), 1)

#define lv2_osc_forge_false(forge, osc_urid) \
	lv2_atom_forge_bool((forge), 0)

#define lv2_osc_forge_nil(forge, osc_urid) \
	lv2_atom_forge_literal((forge), "", 0, (osc_urid)->OSC_Nil, 0)

#define lv2_osc_forge_impulse(forge, osc_urid) \
	lv2_atom_forge_literal((forge), "", 0, (osc_urid)->OSC_Impulse, 0)

#define lv2_osc_forge_symbol(forge, osc_urid, val) \
	lv2_atom_forge_urid((forge), (val))

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_chunk(LV2_Atom_Forge *forge, LV2_URID type,
	const uint8_t *buf, uint32_t size)
{
	LV2_Atom_Forge_Ref ref;

	if(  (ref = lv2_atom_forge_atom(forge, size, type))
		&& (ref = lv2_atom_forge_raw(forge, buf, size)) )
	{
		lv2_atom_forge_pad(forge, size);
		return ref;
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_midi(LV2_Atom_Forge *forge, LV2_OSC_URID *osc_urid,
	const uint8_t *buf, uint32_t size)
{
	assert(size <= 3);
	return lv2_osc_forge_chunk(forge, osc_urid->MIDI_MidiEvent, buf, size);
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_blob(LV2_Atom_Forge* forge, LV2_OSC_URID *osc_urid,
	const uint8_t *buf, uint32_t size)
{
	return lv2_osc_forge_chunk(forge, osc_urid->ATOM_Chunk, buf, size);
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_char(LV2_Atom_Forge* forge, LV2_OSC_URID *osc_urid,
	char val)
{
	return lv2_atom_forge_literal(forge, &val, 1, osc_urid->OSC_Char, 0);
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_rgba(LV2_Atom_Forge* forge, LV2_OSC_URID *osc_urid,
	uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	char val [9];
	sprintf(val, "%02"PRIx8"%02"PRIx8"%02"PRIx8"%02"PRIx8, r, g, b, a);
	return lv2_atom_forge_literal(forge, val, 8, osc_urid->OSC_RGBA, 0);
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_timetag(LV2_Atom_Forge *forge, LV2_OSC_URID *osc_urid,
	const LV2_OSC_Timetag *timetag)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge_Ref ref;

	if(  (ref = lv2_atom_forge_object(forge, &frame, 0, osc_urid->OSC_Timetag))
		&& (ref = lv2_atom_forge_key(forge, osc_urid->OSC_timetagIntegral))
		&& (ref = lv2_atom_forge_long(forge, timetag->integral))
		&& (ref = lv2_atom_forge_key(forge, osc_urid->OSC_timetagFraction))
		&& (ref = lv2_atom_forge_long(forge, timetag->fraction)) )
	{
		lv2_atom_forge_pop(forge, &frame);
		return ref;
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_bundle_head(LV2_Atom_Forge* forge, LV2_OSC_URID *osc_urid,
	LV2_Atom_Forge_Frame frame [2], const LV2_OSC_Timetag *timetag)
{
	LV2_Atom_Forge_Ref ref;

	if(  (ref = lv2_atom_forge_object(forge, &frame[0], 0, osc_urid->OSC_Bundle))
		&& (ref = lv2_atom_forge_key(forge, osc_urid->OSC_bundleTimetag))
		&& (ref = lv2_osc_forge_timetag(forge, osc_urid, timetag))
		&& (ref = lv2_atom_forge_key(forge, osc_urid->OSC_bundleItems))
		&& (ref = lv2_atom_forge_tuple(forge, &frame[1])) )
	{
		return ref;
	}

	return 0;
}

/**
   TODO
*/
static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_head(LV2_Atom_Forge *forge, LV2_OSC_URID *osc_urid,
	LV2_Atom_Forge_Frame frame [2], const char *path)
{
	assert(path);

	LV2_Atom_Forge_Ref ref;
	if(  (ref = lv2_atom_forge_object(forge, &frame[0], 0, osc_urid->OSC_Message))
		&& (ref = lv2_atom_forge_key(forge, osc_urid->OSC_messagePath))
		&& (ref = lv2_atom_forge_string(forge, path, strlen(path)))
		&& (ref = lv2_atom_forge_key(forge, osc_urid->OSC_messageArguments))
		&& (ref = lv2_atom_forge_tuple(forge, &frame[1])) )
	{
		return ref;
	}

	return 0;
}

/**
   TODO
*/
static inline void
lv2_osc_forge_pop(LV2_Atom_Forge *forge, LV2_Atom_Forge_Frame frame [2])
{
	lv2_atom_forge_pop(forge, &frame[1]); // a LV2_Atom_Tuple
	lv2_atom_forge_pop(forge, &frame[0]); // a LV2_Atom_Object
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_varlist(LV2_Atom_Forge *forge, LV2_OSC_URID *osc_urid,
	const char *path, const char *fmt, va_list args)
{
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	if(!lv2_osc_check_path(path) || !lv2_osc_check_fmt(fmt, 0))
		return 0;
	if(!(ref = lv2_osc_forge_message_head(forge, osc_urid, frame, path)))
		return 0;

	for(const char *type = fmt; *type; type++)
	{
		switch( (LV2_OSC_Type)*type)
		{
			case LV2_OSC_INT32:
			{
				if(!(ref = lv2_osc_forge_int(forge, osc_urid, va_arg(args, int32_t))))
					return 0;
				break;
			}
			case LV2_OSC_FLOAT:
			{
				if(!(ref = lv2_osc_forge_float(forge, osc_urid, (float)va_arg(args, double))))
					return 0;
				break;
			}
			case LV2_OSC_STRING:
			{
				const char *s = va_arg(args, const char *);
				if(!s || !(ref = lv2_osc_forge_string(forge, osc_urid, s, strlen(s))))
					return 0;
				break;
			}
			case LV2_OSC_BLOB:
			{
				const int32_t size = va_arg(args, int32_t);
				const uint8_t *b = va_arg(args, const uint8_t *);
				if(!b || !(ref = lv2_osc_forge_blob(forge, osc_urid, b, size)))
					return 0;
				break;
			}
			
			case LV2_OSC_INT64:
			{
				if(!(ref = lv2_osc_forge_long(forge, osc_urid, va_arg(args, int64_t))))
					return 0;
				break;
			}
			case LV2_OSC_DOUBLE:
			{
				if(!(ref = lv2_osc_forge_double(forge, osc_urid, va_arg(args, double))))
					return 0;
				break;
			}
			case LV2_OSC_TIMETAG:
			{
				const LV2_OSC_Timetag timetag = {
					.integral = va_arg(args, uint32_t),
					.fraction = va_arg(args, uint32_t)
				};
				if(!(ref = lv2_osc_forge_timetag(forge, osc_urid, &timetag)))
					return 0;
				break;
			}
			
			case LV2_OSC_TRUE:
			{
				if(!(ref = lv2_osc_forge_true(forge, osc_urid)))
					return 0;
				break;
			}
			case LV2_OSC_FALSE:
			{
				if(!(ref = lv2_osc_forge_false(forge, osc_urid)))
					return 0;
				break;
			}
			case LV2_OSC_NIL:
			{
				if(!(ref = lv2_osc_forge_nil(forge, osc_urid)))
					return 0;
				break;
			}
			case LV2_OSC_IMPULSE:
			{
				if(!(ref = lv2_osc_forge_impulse(forge, osc_urid)))
					return 0;
				break;
			}

			case LV2_OSC_SYMBOL:
			{
				if(!(ref = lv2_osc_forge_symbol(forge, osc_urid, va_arg(args, uint32_t))))
					return 0;
				break;
			}
			case LV2_OSC_MIDI:
			{
				const int32_t size = va_arg(args, int32_t);
				const uint8_t *m = va_arg(args, const uint8_t *);
				if(!m || !(ref = lv2_osc_forge_midi(forge, osc_urid, m, size)))
					return 0;
				break;
			}
			case LV2_OSC_CHAR:
			{
				if(!(ref = lv2_osc_forge_char(forge, osc_urid, (char)va_arg(args, int))))
					return 0;
				break;
			}
			case LV2_OSC_RGBA:
			{
				if(!(ref = lv2_osc_forge_rgba(forge, osc_urid,
						(uint8_t)va_arg(args, unsigned),
						(uint8_t)va_arg(args, unsigned),
						(uint8_t)va_arg(args, unsigned),
						(uint8_t)va_arg(args, unsigned))))
					return 0;
				break;
			}
		}
	}

	lv2_osc_forge_pop(forge, frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_message_vararg(LV2_Atom_Forge *forge, LV2_OSC_URID *osc_urid,
	const char *path, const char *fmt, ...)
{
	LV2_Atom_Forge_Ref ref;
	va_list args;

	va_start(args, fmt);

	ref = lv2_osc_forge_message_varlist(forge, osc_urid, path, fmt, args);

	va_end(args);

	return ref;
}

static inline LV2_Atom_Forge_Ref
lv2_osc_forge_packet(LV2_Atom_Forge *forge, LV2_OSC_URID *osc_urid,
	LV2_URID_Map *map, const uint8_t *buf, size_t size)
{
	LV2_OSC_Reader reader;
	LV2_Atom_Forge_Frame frame [2];
	LV2_Atom_Forge_Ref ref;

	lv2_osc_reader_initialize(&reader, buf, size);

	if(lv2_osc_reader_is_bundle(&reader))
	{
		LV2_OSC_Item *itm = OSC_READER_BUNDLE_BEGIN(&reader, size);
		
		if(itm && (ref = lv2_osc_forge_bundle_head(forge, osc_urid, frame, 
			LV2_OSC_TIMETAG_CREATE(itm->timetag))))
		{
			OSC_READER_BUNDLE_ITERATE(&reader, itm)
			{
				if(!(ref = lv2_osc_forge_packet(forge, osc_urid, map, itm->body, itm->size)))
					return 0;
			}

			lv2_osc_forge_pop(forge, frame);

			return ref;
		}
	}
	else if(lv2_osc_reader_is_message(&reader))
	{
		LV2_OSC_Arg *arg = OSC_READER_MESSAGE_BEGIN(&reader, size);

		if(arg && (ref = lv2_osc_forge_message_head(forge, osc_urid, frame, arg->path)))
		{
			OSC_READER_MESSAGE_ITERATE(&reader, arg)
			{
				switch( (LV2_OSC_Type)*arg->type)
				{
					case LV2_OSC_INT32:
					{
						if(!(ref = lv2_osc_forge_int(forge, osc_urid, arg->i)))
							return 0;
						break;
					}
					case LV2_OSC_FLOAT:
					{
						if(!(ref = lv2_osc_forge_float(forge, osc_urid, arg->f)))
							return 0;
						break;
					}
					case LV2_OSC_STRING:
					{
						if(!(ref = lv2_osc_forge_string(forge, osc_urid, arg->s, arg->size - 1)))
							return 0;
						break;
					}
					case LV2_OSC_BLOB:
					{
						if(!(ref = lv2_osc_forge_blob(forge, osc_urid, arg->b, arg->size)))
							return 0;
						break;
					}

					case LV2_OSC_INT64:
					{
						if(!(ref = lv2_osc_forge_long(forge, osc_urid, arg->h)))
							return 0;
						break;
					}
					case LV2_OSC_DOUBLE:
					{
						if(!(ref = lv2_osc_forge_double(forge, osc_urid, arg->d)))
							return 0;
						break;
					}
					case LV2_OSC_TIMETAG:
					{
						if(!(ref = lv2_osc_forge_timetag(forge, osc_urid, LV2_OSC_TIMETAG_CREATE(arg->t))))
							return 0;
						break;
					}

					case LV2_OSC_TRUE:
					{
						if(!(ref = lv2_osc_forge_true(forge, osc_urid)))
							return 0;
						break;
					}
					case LV2_OSC_FALSE:
					{
						if(!(ref = lv2_osc_forge_false(forge, osc_urid)))
							return 0;
						break;
					}
					case LV2_OSC_NIL:
					{
						if(!(ref = lv2_osc_forge_nil(forge, osc_urid)))
							return 0;
						break;
					}
					case LV2_OSC_IMPULSE:
					{
						if(!(ref = lv2_osc_forge_impulse(forge, osc_urid)))
							return 0;
						break;
					}

					case LV2_OSC_SYMBOL:
					{
						if(!(ref = lv2_osc_forge_symbol(forge, osc_urid,
								map->map(map->handle, arg->S))))
							return 0;
						break;
					}
					case LV2_OSC_MIDI:
					{
						if(!(ref = lv2_osc_forge_midi(forge, osc_urid, &arg->b[1], arg->size - 1)))
							return 0;
						break;
					}
					case LV2_OSC_CHAR:
					{
						if(!(ref = lv2_osc_forge_char(forge, osc_urid, arg->c)))
							return 0;
						break;
					}
					case LV2_OSC_RGBA:
					{
						if(!(ref = lv2_osc_forge_rgba(forge, osc_urid, arg->R, arg->G, arg->B, arg->A)))
							return 0;
						break;
					}
				}
			}

			lv2_osc_forge_pop(forge, frame);

			return ref;
		}
	}

	return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV2_OSC_FORGE_H
