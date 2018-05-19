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

#ifndef LV2_OSC_WRITER_H
#define LV2_OSC_WRITER_H

#include <stdbool.h>
#include <string.h>

#include <osc.lv2/osc.h>
#include <osc.lv2/util.h>
#include <osc.lv2/endian.h>

#include <lv2/lv2plug.in/ns/ext/atom/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef LV2_ATOM_TUPLE_FOREACH // there is a bug in LV2 1.10.0
#define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))

typedef struct _LV2_OSC_Writer LV2_OSC_Writer;
typedef struct _LV2_OSC_Writer_Frame LV2_OSC_Writer_Frame;

struct _LV2_OSC_Writer {
	uint8_t *buf;
	uint8_t *ptr;
	const uint8_t *end;
};

struct _LV2_OSC_Writer_Frame {
	uint8_t *ref;
};

static inline void
lv2_osc_writer_initialize(LV2_OSC_Writer *writer, uint8_t *buf, size_t size)
{
	writer->buf = buf;
	writer->ptr = buf;
	writer->end = buf + size;
}

static inline size_t
lv2_osc_writer_get_size(LV2_OSC_Writer *writer)
{
	if(writer->ptr > writer->buf)
		return writer->ptr - writer->buf;

	return 0;
}

static inline uint8_t * 
lv2_osc_writer_finalize(LV2_OSC_Writer *writer, size_t *size)
{
	*size = lv2_osc_writer_get_size(writer);

	if(*size)
		return writer->buf;

	return NULL;
}

static inline bool
lv2_osc_writer_overflow(LV2_OSC_Writer *writer, size_t size)
{
	return writer->ptr + size >= writer->end;
}

static inline bool
lv2_osc_writer_htobe32(LV2_OSC_Writer *writer, union swap32_t *s32)
{
	if(lv2_osc_writer_overflow(writer, 4))
		return false;

	s32->u = htobe32(s32->u);
	*(uint32_t *)writer->ptr = s32->u;
	writer->ptr += 4;

	return true;
}

static inline bool 
lv2_osc_writer_htobe64(LV2_OSC_Writer *writer, union swap64_t *s64)
{
	if(lv2_osc_writer_overflow(writer, 8))
		return false;

	s64->u = htobe64(s64->u);
	*(uint64_t *)writer->ptr = s64->u;
	writer->ptr += 8;

	return true;
}

static inline bool
lv2_osc_writer_add_int32(LV2_OSC_Writer *writer, int32_t i)
{
	return lv2_osc_writer_htobe32(writer, &(union swap32_t){ .i = i });
}

static inline bool 
lv2_osc_writer_add_float(LV2_OSC_Writer *writer, float f)
{
	return lv2_osc_writer_htobe32(writer, &(union swap32_t){ .f = f });
}

static inline bool 
lv2_osc_writer_add_string(LV2_OSC_Writer *writer, const char *s)
{
	const size_t rawlen = strlen(s) + 1;
	const size_t padded = LV2_OSC_PADDED_SIZE(rawlen);
	if(lv2_osc_writer_overflow(writer, padded))
		return false;

	const uint32_t blank = 0;
	memcpy(writer->ptr + padded - sizeof(uint32_t), &blank, sizeof(uint32_t));
	memcpy(writer->ptr, s, rawlen);
	writer->ptr += padded;

	return true;
}

static inline bool
lv2_osc_writer_add_symbol(LV2_OSC_Writer *writer, const char *S)
{
	return lv2_osc_writer_add_string(writer, S);
}

static inline bool
lv2_osc_writer_add_int64(LV2_OSC_Writer *writer, int64_t h)
{
	return lv2_osc_writer_htobe64(writer, &(union swap64_t){ .h = h });
}

static inline bool 
lv2_osc_writer_add_double(LV2_OSC_Writer *writer, double d)
{
	return lv2_osc_writer_htobe64(writer, &(union swap64_t){ .d = d });
}

static inline bool
lv2_osc_writer_add_timetag(LV2_OSC_Writer *writer, uint64_t u)
{
	return lv2_osc_writer_htobe64(writer, &(union swap64_t){ .u = u });
}

static inline bool 
lv2_osc_writer_add_blob_inline(LV2_OSC_Writer *writer, int32_t len, uint8_t **body)
{
	const size_t len_padded = LV2_OSC_PADDED_SIZE(len);
	const size_t size = 4 + len_padded;
	if(lv2_osc_writer_overflow(writer, size))
		return false;

	if(!lv2_osc_writer_add_int32(writer, len))
		return false;

	*body = writer->ptr;
	//memset(&writer->ptr[len], 0x0, len_padded - len);
	writer->ptr += len_padded;

	return true;
}

static inline bool 
lv2_osc_writer_add_blob(LV2_OSC_Writer *writer, int32_t len, const uint8_t *body)
{
	uint8_t *dst;
	if(!lv2_osc_writer_add_blob_inline(writer, len, &dst))
		return false;

	memcpy(dst, body, len);

	return true;
}

static inline bool
lv2_osc_writer_add_midi_inline(LV2_OSC_Writer *writer, int32_t len, uint8_t **m)
{
	if( (len > 4) || lv2_osc_writer_overflow(writer, 4))
		return false;

	*m = writer->ptr;
	//memset(&writer->ptr[len], 0x0, 4 - len);
	writer->ptr += 4;

	return true;
}

static inline bool 
lv2_osc_writer_add_midi(LV2_OSC_Writer *writer, int32_t len, const uint8_t *m)
{
	uint8_t *dst;
	if(!lv2_osc_writer_add_midi_inline(writer, len, &dst))
		return false;

	memcpy(dst, m, len);

	return true;
}

static inline bool
lv2_osc_writer_add_rgba(LV2_OSC_Writer *writer, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if(lv2_osc_writer_overflow(writer, 4))
		return false;

	writer->ptr[0] = r;
	writer->ptr[1] = g;
	writer->ptr[2] = b;
	writer->ptr[3] = a;
	writer->ptr += 4;

	return true;
}

static inline bool
lv2_osc_writer_add_char(LV2_OSC_Writer *writer, char c)
{
	return lv2_osc_writer_add_int32(writer, (int32_t)c);
}

static inline bool 
lv2_osc_writer_push_bundle(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame, uint64_t t)
{
	if(lv2_osc_writer_overflow(writer, 16))
		return false;

	frame->ref = writer->ptr;

	strncpy((char *)writer->ptr, "#bundle", 8);
	writer->ptr += 8;

	return lv2_osc_writer_add_timetag(writer, t);
}

static inline bool 
lv2_osc_writer_pop_bundle(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame)
{
	union swap32_t s32 = { .i = writer->ptr - frame->ref - 16};

	if(s32.i <= 0)
	{
		writer->ptr = frame->ref;
		return false;
	}

	return true;
}

static inline  bool
lv2_osc_writer_push_item(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame)
{
	if(lv2_osc_writer_overflow(writer, 4))
		return false;

	frame->ref = writer->ptr;
	writer->ptr += 4;

	return true;
}

static inline bool
lv2_osc_writer_pop_item(LV2_OSC_Writer *writer, LV2_OSC_Writer_Frame *frame)
{
	union swap32_t s32 = { .i = writer->ptr - frame->ref - 4};

	if(s32.i <= 0)
	{
		writer->ptr = frame->ref;
		return false;
	}

	s32.u = htobe32(s32.u);
	*(uint32_t *)frame->ref = s32.u;

	return true;
}

static inline bool 
lv2_osc_writer_add_path(LV2_OSC_Writer *writer, const char *path)
{
	return lv2_osc_writer_add_string(writer, path);
}

static inline bool 
lv2_osc_writer_add_format(LV2_OSC_Writer *writer, const char *fmt)
{
	const size_t rawlen = strlen(fmt) + 1;
	const size_t padded = LV2_OSC_PADDED_SIZE(rawlen + 1);
	if(lv2_osc_writer_overflow(writer, padded))
		return false;

	const uint32_t blank = 0;
	memcpy(writer->ptr + padded - sizeof(uint32_t), &blank, sizeof(uint32_t));
	*writer->ptr++ = ',';
	memcpy(writer->ptr, fmt, rawlen);
	writer->ptr += padded - 1;

	return true;
}

static inline bool
lv2_osc_writer_arg_varlist(LV2_OSC_Writer *writer, const char *fmt, va_list args)
{
	for(const char *type = fmt; *type; type++)
	{
		switch( (LV2_OSC_Type)*type)
		{
			case LV2_OSC_INT32:
				if(!lv2_osc_writer_add_int32(writer, va_arg(args, int32_t)))
					return false;
				break;
			case LV2_OSC_FLOAT:
				if(!lv2_osc_writer_add_float(writer, (float)va_arg(args, double)))
					return false;
				break;
			case LV2_OSC_STRING:
				if(!lv2_osc_writer_add_string(writer, va_arg(args, const char *)))
					return false;
				break;
			case LV2_OSC_BLOB:
			{
				const int32_t len = va_arg(args, int32_t);
				if(!lv2_osc_writer_add_blob(writer, len, va_arg(args, const uint8_t *)))
					return false;
			}	break;

			case LV2_OSC_TRUE:
			case LV2_OSC_FALSE:
			case LV2_OSC_NIL:
			case LV2_OSC_IMPULSE:
				break;

			case LV2_OSC_INT64:
				if(!lv2_osc_writer_add_int64(writer, va_arg(args, int64_t)))
					return false;
				break;
			case LV2_OSC_DOUBLE:
				if(!lv2_osc_writer_add_double(writer, va_arg(args, double)))
					return false;
				break;
			case LV2_OSC_TIMETAG:
				if(!lv2_osc_writer_add_timetag(writer, va_arg(args, uint64_t)))
					return false;
				break;

			case LV2_OSC_MIDI:
			{
				const int32_t len = va_arg(args, int32_t);
				if(!lv2_osc_writer_add_midi(writer, len, va_arg(args, const uint8_t *)))
					return false;
			}	break;
			case LV2_OSC_SYMBOL:
				if(!lv2_osc_writer_add_symbol(writer, va_arg(args, const char *)))
					return false;
				break;
			case LV2_OSC_CHAR:
				if(!lv2_osc_writer_add_char(writer, va_arg(args, int)))
					return false;
				break;
			case LV2_OSC_RGBA:
			{
				const uint8_t r = va_arg(args, unsigned);
				const uint8_t g = va_arg(args, unsigned);
				const uint8_t b = va_arg(args, unsigned);
				const uint8_t a = va_arg(args, unsigned);
				if(!lv2_osc_writer_add_rgba(writer, r, g, b, a))
					return false;
			}	break;
		}
	}

	return true;
}

static inline bool
lv2_osc_writer_arg_vararg(LV2_OSC_Writer *writer, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

	const bool res = lv2_osc_writer_arg_varlist(writer, fmt, args);

	va_end(args);

	return res;
}

static inline bool
lv2_osc_writer_message_varlist(LV2_OSC_Writer *writer, const char *path, const char *fmt, va_list args)
{
	if(!lv2_osc_writer_add_path(writer, path))
		return false;

	if(!lv2_osc_writer_add_format(writer, fmt))
		return false;

	return lv2_osc_writer_arg_varlist(writer, fmt, args);
}

static inline bool
lv2_osc_writer_message_vararg(LV2_OSC_Writer *writer, const char *path, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

	const bool res = lv2_osc_writer_message_varlist(writer, path, fmt, args);

	va_end(args);

	return res;
}

static inline bool
lv2_osc_writer_packet(LV2_OSC_Writer *writer, LV2_OSC_URID *osc_urid,
	LV2_URID_Unmap *unmap, uint32_t size, const LV2_Atom_Object_Body *body)
{
	if(body->otype == osc_urid->OSC_Bundle)
	{
		const LV2_Atom_Object *timetag = NULL;
		const LV2_Atom_Tuple *items = NULL;

		if(!lv2_osc_bundle_body_get(osc_urid, size, body, &timetag, &items))
			return false;

		LV2_OSC_Timetag tt;
		LV2_OSC_Writer_Frame bndl = { .ref = 0 };

		lv2_osc_timetag_get(osc_urid, &timetag->atom, &tt);
		if(!lv2_osc_writer_push_bundle(writer, &bndl, lv2_osc_timetag_parse(&tt)))
			return false;

		LV2_ATOM_TUPLE_FOREACH(items, atom)
		{
			const LV2_Atom_Object *obj= (const LV2_Atom_Object *)atom;
			LV2_OSC_Writer_Frame itm = { .ref = 0 };

			if(  !lv2_osc_writer_push_item(writer, &itm)
				|| !lv2_osc_writer_packet(writer, osc_urid, unmap, obj->atom.size, &obj->body)
				|| !lv2_osc_writer_pop_item(writer, &itm) )
			{
				return false;
			}
		}

		return lv2_osc_writer_pop_bundle(writer, &bndl);
	}
	else if(body->otype == osc_urid->OSC_Message)
	{
		const LV2_Atom_String *path = NULL;
		const LV2_Atom_Tuple *arguments = NULL;

		if(lv2_osc_message_body_get(osc_urid, size, body, &path, &arguments))
		{
			if(!lv2_osc_writer_add_path(writer, LV2_ATOM_BODY_CONST(path)))
				return false;

			char fmt [128]; //TODO how big?
			char *ptr = fmt;
			LV2_ATOM_TUPLE_FOREACH(arguments, atom)
			{
				*ptr++ = lv2_osc_argument_type(osc_urid, atom);
			}
			*ptr = '\0';
			if(!lv2_osc_writer_add_format(writer, fmt))
				return false;

			LV2_ATOM_TUPLE_FOREACH(arguments, atom)
			{
				const LV2_Atom_Object *obj= (const LV2_Atom_Object *)atom;

				if(atom->type == osc_urid->ATOM_Int)
				{
					if(!lv2_osc_writer_add_int32(writer, ((const LV2_Atom_Int *)atom)->body))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_Float)
				{
					if(!lv2_osc_writer_add_float(writer, ((const LV2_Atom_Float *)atom)->body))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_String)
				{
					if(!lv2_osc_writer_add_string(writer, LV2_ATOM_BODY_CONST(atom)))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_Chunk)
				{
					if(!lv2_osc_writer_add_blob(writer, atom->size, LV2_ATOM_BODY_CONST(atom)))
						return false;
				}

				else if(atom->type == osc_urid->ATOM_Long)
				{
					if(!lv2_osc_writer_add_int64(writer, ((const LV2_Atom_Long *)atom)->body))
						return false;
				}
				else if(atom->type == osc_urid->ATOM_Double)
				{
					if(!lv2_osc_writer_add_double(writer, ((const LV2_Atom_Double *)atom)->body))
						return false;
				}
				else if( (atom->type == osc_urid->ATOM_Object) && (obj->body.otype == osc_urid->OSC_Timetag) )
				{
					LV2_OSC_Timetag tt;
					lv2_osc_timetag_get(osc_urid, &obj->atom, &tt);
					if(!lv2_osc_writer_add_timetag(writer, lv2_osc_timetag_parse(&tt)))
						return false;
				}

				// there is nothing to do for: true, false, nil, impulse

				else if(atom->type == osc_urid->ATOM_URID)
				{
					const char *symbol = unmap->unmap(unmap->handle, ((const LV2_Atom_URID *)atom)->body);
					if(!symbol || !lv2_osc_writer_add_symbol(writer, symbol))
						return false;
				}
				else if(atom->type == osc_urid->MIDI_MidiEvent)
				{
					uint8_t *m = NULL;
					if(!lv2_osc_writer_add_midi_inline(writer, atom->size + 1, &m))
						return false;
					m[0] = 0x0; // port
					memcpy(&m[1], LV2_ATOM_BODY_CONST(atom), atom->size);
				}
				else if(atom->type == osc_urid->ATOM_Literal)
				{
					const LV2_Atom_Literal *lit = (LV2_Atom_Literal *)atom;

					if(lit->body.datatype == osc_urid->OSC_Char)
					{
						const char c = *(const char *)LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, lit);
						if(!lv2_osc_writer_add_char(writer, c))
							return false;
					}
					else if(lit->body.datatype == osc_urid->OSC_RGBA)
					{
						const char *rgba = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, atom);
						uint8_t r, g, b, a;
						if(sscanf(rgba, "%02"SCNx8"%02"SCNx8"%02"SCNx8"%02"SCNx8, &r, &g, &b, &a) != 4)
							return false;
						if(!lv2_osc_writer_add_rgba(writer, r, g, b, a))
							return false;
					}
				}
			}
		}

		return true;
	}

	return false;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // LV2_OSC_WRITER_H
