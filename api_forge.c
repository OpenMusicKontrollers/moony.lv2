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

#include <api_atom.h>
#include <api_forge.h>
#include <api_stash.h>

static inline int
_lforge_frame_time_inlined(lua_State *L, lforge_t *lforge, int64_t frames)
{
	if(frames >= lforge->last.frames)
	{
		if(!lv2_atom_forge_frame_time(lforge->forge, frames))
			luaL_error(L, forge_buffer_overflow);
		lforge->last.frames = frames;

		lua_settop(L, 1);
		return 1;
	}

	return luaL_error(L, "invalid frame time, must not decrease");
}

static int
_lforge_frame_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int64_t frames = luaL_checkinteger(L, 2);

	return _lforge_frame_time_inlined(L, lforge, frames);
}

static inline int
_lforge_beat_time_inlined(lua_State *L, lforge_t *lforge, double beats)
{
	if(beats >= lforge->last.beats)
	{
		if(!lv2_atom_forge_beat_time(lforge->forge, beats))
			luaL_error(L, forge_buffer_overflow);
		lforge->last.beats = beats;

		lua_settop(L, 1);
		return 1;
	}

	return luaL_error(L, "invalid beat time, must not decrease");
}

static int
_lforge_beat_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	double beats = luaL_checknumber(L, 2);

	return _lforge_beat_time_inlined(L, lforge, beats);
}

static int
_lforge_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(lua_isinteger(L, 2))
	{
		int64_t frames = lua_tointeger(L, 2);

		return _lforge_frame_time_inlined(L, lforge, frames);
	}
	else if(lua_isnumber(L, 2))
	{
		double beats = lua_tointeger(L, 2);

		return _lforge_beat_time_inlined(L, lforge, beats);
	}

	return luaL_error(L, "integer or number expected");
}

static int
_lforge_atom(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	latom_t *latom = luaL_checkudata(L, 2, "latom");
	const uint32_t size = latom->atom->size;

	if(!lv2_atom_forge_atom(lforge->forge, size, latom->atom->type))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_raw(lforge->forge, latom->body.raw, size))
		luaL_error(L, forge_buffer_overflow);
	lv2_atom_forge_pad(lforge->forge, size);

	lua_settop(L, 1);
	return 1;
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_int(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	int32_t val = luaL_checkinteger(L, pos);
	return lv2_atom_forge_int(forge, val);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_long(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	int64_t val = luaL_checkinteger(L, pos);
	return lv2_atom_forge_long(forge, val);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_float(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	float val = luaL_checknumber(L, pos);
	return lv2_atom_forge_float(forge, val);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_double(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	double val = luaL_checknumber(L, pos);
	return lv2_atom_forge_double(forge, val);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_bool(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	int32_t val = lua_toboolean(L, pos);
	return lv2_atom_forge_bool(forge, val);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_urid(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	LV2_URID val = luaL_checkinteger(L, pos);
	return lv2_atom_forge_urid(forge, val);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_string(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_string(forge, val, len);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_uri(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_uri(forge, val, len);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_path(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_path(forge, val, len);
}

static inline LV2_Atom_Forge_Ref
_lforge_basic_literal(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_literal(forge, val, len, 0, 0); //TODO context, lang
}

LV2_Atom_Forge_Ref
_lforge_basic(lua_State *L, int pos, LV2_Atom_Forge *forge, LV2_URID range)
{
	//FIXME binary lookup?
	if(range == forge->Int)
		return _lforge_basic_int(L, pos, forge);
	else if(range == forge->Long)
		return _lforge_basic_long(L, pos, forge);
	else if(range == forge->Float)
		return _lforge_basic_float(L, pos, forge);
	else if(range == forge->Double)
		return _lforge_basic_double(L, pos, forge);
	else if(range == forge->Bool)
		return _lforge_basic_bool(L, pos, forge);
	else if(range == forge->URID)
		return _lforge_basic_urid(L, pos, forge);
	else if(range == forge->String)
		return _lforge_basic_string(L, pos, forge);
	else if(range == forge->URI)
		return _lforge_basic_uri(L, pos, forge);
	else if(range == forge->Path)
		return _lforge_basic_path(L, pos, forge);
	else if(range == forge->Literal)
		return _lforge_basic_literal(L, pos, forge);

	return luaL_error(L, "not a basic type");
}

static int
_lforge_int(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_int(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_long(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_long(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_float(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_float(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_double(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_double(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_bool(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_bool(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_urid(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_urid(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_string(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_string(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_uri(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_uri(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_path(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_path(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_literal(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);
	LV2_URID datatype = luaL_checkinteger(L, 3);
	LV2_URID lang = luaL_checkinteger(L, 4);

	if(!lv2_atom_forge_literal(lforge->forge, val, size, datatype, lang))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_bytes(lua_State *L, moony_t *moony, LV2_URID type)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int ltype = lua_type(L, 2);
	if(ltype == LUA_TTABLE)
	{
		int size = lua_rawlen(L, 2);
		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		for(int i=1; i<=size; i++)
		{
			lua_rawgeti(L, -1, i);
			uint8_t byte = luaL_checkinteger(L, -1);
			lua_pop(L, 1);
			if(!lv2_atom_forge_raw(lforge->forge, &byte, 1))
				luaL_error(L, forge_buffer_overflow);
		}
		lv2_atom_forge_pad(lforge->forge, size);
	}
	else if(ltype == LUA_TSTRING)
	{
		size_t size;
		const char *str = lua_tolstring(L, 2, &size);
		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_raw(lforge->forge, str, size))
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pad(lforge->forge, size);
	}
	else if(luaL_testudata(L, 2, "latom")) //to convert between chunk <-> midi
	{
		latom_t *latom = lua_touserdata(L, 2);
		uint32_t size = latom->atom->size;
		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_raw(lforge->forge, latom->body.raw, size))
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pad(lforge->forge, size);
	}
	else // bytes as individual function arguments
	{
		int size = lua_gettop(L) - 1;

		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		for(int i=0; i<size; i++)
		{
			uint8_t byte = luaL_checkinteger(L, i+2);
			if(!lv2_atom_forge_raw(lforge->forge, &byte, 1))
				luaL_error(L, forge_buffer_overflow);
		}
		lv2_atom_forge_pad(lforge->forge, size);
	}

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_chunk(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	return _lforge_bytes(L, moony, moony->forge.Chunk);
}

static int
_lforge_midi(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	return _lforge_bytes(L, moony, moony->uris.midi_event);
}

static inline uint64_t
_lforge_to_timestamp(lua_State *L, moony_t *moony, lforge_t *lforge, int pos)
{
	uint64_t timestamp = 1ULL; // immediate timestamp
	if(lua_isinteger(L, pos))
	{
		// absolute timestamp
		timestamp = lua_tointeger(L, pos);
	}
	else if(lua_isnumber(L, pos) && moony->osc_sched)
	{
		// timestamp of current frame
		timestamp = moony->osc_sched->frames2osc(moony->osc_sched->handle,
			lforge->last.frames);
		volatile uint64_t sec = timestamp >> 32;
		volatile uint64_t frac = timestamp & 0xffffffff;
		
		// relative offset from current frame (in seconds)
		double offset_d = lua_tonumber(L, pos);
		double secs_d;
		double frac_d = modf(offset_d, &secs_d);

		// add relative offset to timestamp
		sec += secs_d;
		frac += frac_d * 0x1p32;
		if(frac >= 0x100000000ULL) // overflow
		{
			sec += 1;
			frac -= 0x100000000ULL;
		}
		timestamp = (sec << 32) | frac;

		/*
		// debug
		uint64_t t0 = moony->osc_sched->frames2osc(moony->osc_sched->handle, lforge->last.frames);
		uint64_t dt = timestamp - t0;
		double dd = dt * 0x1p-32;
		printf("%"PRIU64" %lf\n", dt, dd);
		*/
	}

	return timestamp;
}

static int
_lforge_osc_bundle(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	osc_forge_t *oforge = &moony->oforge;
	LV2_Atom_Forge *forge = lforge->forge;

	uint64_t timestamp = _lforge_to_timestamp(L, moony, lforge, 2);

	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 2;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!osc_forge_bundle_push(oforge, forge, lframe->frame, timestamp))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_osc_message(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	osc_forge_t *oforge = &moony->oforge;
	LV2_Atom_Forge *forge = lforge->forge;

	const char *path = luaL_checkstring(L, 2);
	const char *fmt = luaL_optstring(L, 3, "");

	LV2_Atom_Forge_Frame frame [2];

	if(!osc_forge_message_push(oforge, forge, frame, path, fmt))
		luaL_error(L, forge_buffer_overflow);

	int pos = 4;
	for(const char *type = fmt; *type; type++)
	{
		switch(*type)
		{
			case 'i':
			{
				int32_t i = luaL_checkinteger(L, pos++);
				if(!osc_forge_int32(oforge, forge, i))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'f':
			{
				float f = luaL_checknumber(L, pos++);
				if(!osc_forge_float(oforge, forge, f))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 's':
			case 'S':
			{
				const char *s = luaL_checkstring(L, pos++);
				if(!osc_forge_string(oforge, forge, s))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'b':
			{
				if(lua_istable(L, pos))
				{
					int n = lua_rawlen(L, pos);

					if(!lv2_atom_forge_atom(forge, n, forge->Chunk))
						luaL_error(L, forge_buffer_overflow);
					for(int i=1; i<=n; i++)
					{
						lua_rawgeti(L, pos, i);
						uint8_t b = luaL_checkinteger(L, -1);
						lua_pop(L, 1);
						if(!lv2_atom_forge_raw(forge, &b, 1))
							luaL_error(L, forge_buffer_overflow);
					}
					lv2_atom_forge_pad(forge, n);
				}
				pos += 1;

				break;
			}

			case 'T':
			case 'F':
			case 'N':
			case 'I':
			{
				break;
			}

			case 'h':
			{
				int64_t h = luaL_checkinteger(L, pos++);
				if(!osc_forge_int64(oforge, forge, h))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'd':
			{
				double d = luaL_checknumber(L, pos++);
				if(!osc_forge_double(oforge, forge, d))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 't':
			{
				uint64_t t = _lforge_to_timestamp(L, moony, lforge, pos++);
				if(!osc_forge_timestamp(oforge, forge, t))
					luaL_error(L, forge_buffer_overflow);
				break;
			}

			case 'c':
			{
				char c = luaL_checkinteger(L, pos++);
				if(!osc_forge_char(oforge, forge, c))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'm':
			{
				if(lua_istable(L, pos))
				{
					int n = lua_rawlen(L, pos);

					if(!lv2_atom_forge_atom(forge, n, moony->oforge.MIDI_MidiEvent))
						luaL_error(L, forge_buffer_overflow);
					for(int i=1; i<=n; i++)
					{
						lua_rawgeti(L, pos, i);
						uint8_t b = luaL_checkinteger(L, -1);
						lua_pop(L, 1);
						if(!lv2_atom_forge_raw(forge, &b, 1))
							luaL_error(L, forge_buffer_overflow);
					}
					lv2_atom_forge_pad(forge, n);
				}
				pos += 1;

				break;
			}
		}
	}

	osc_forge_message_pop(oforge, forge, frame);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_tuple(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_tuple(lforge->forge, &lframe->frame[0]))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_object(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID id = luaL_optinteger(L, 2, 0);
	LV2_URID otype = luaL_optinteger(L, 3, 0);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, &lframe->frame[0], id, otype))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_key(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);

	if(!lv2_atom_forge_key(lforge->forge, key))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_property(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);
	LV2_URID context = luaL_checkinteger(L, 3);

	if(!lv2_atom_forge_property_head(lforge->forge, key, context))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_vector(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID child_type = luaL_checkinteger(L, 2);
	uint32_t child_size = 0;
	LV2_Atom_Forge_Frame frame;

	int is_table = lua_istable(L, 3);

	int n = is_table
		? (int)lua_rawlen(L, 3)
		: lua_gettop(L) - 2;

	if(  (child_type == lforge->forge->Int)
		|| (child_type == lforge->forge->URID) )
	{
		child_size = sizeof(int32_t);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			int32_t v = luaL_checkinteger(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Bool)
	{
		child_size = sizeof(int32_t);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			int32_t v = lua_toboolean(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Long)
	{
		child_size = sizeof(int64_t);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			int64_t v = luaL_checkinteger(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Float)
	{
		child_size = sizeof(float);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			float v = luaL_checknumber(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Double)
	{
		child_size = sizeof(double);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			double v = luaL_checknumber(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else
		luaL_error(L, "vector supports only fixed sized atoms (e.g. Int, Long, Float, Double, URID, Bool)");

	if(&frame != lforge->forge->stack) // intercept assert
		luaL_error(L, "forge frame mismatch");
	lv2_atom_forge_pop(lforge->forge, &frame);
	lv2_atom_forge_pad(lforge->forge, n*child_size);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_sequence(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID unit = luaL_optinteger(L, 2, 0); //TODO use proper unit
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 1;
	lframe->last.frames = 0;
	lframe->forge = lforge->forge;
	
	if(!lv2_atom_forge_sequence_head(lforge->forge, &lframe->frame[0], unit))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_typed(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID urid = luaL_checkinteger(L, 2);
	lua_remove(L, 2); // urid

	lua_CFunction hook = NULL;

	//TODO keep this list updated
	//TODO use binary tree sorted by URID
	if(urid == lforge->forge->Int)
		hook = _lforge_int;
	else if(urid == lforge->forge->Long)
		hook = _lforge_long;
	else if(urid == lforge->forge->Float)
		hook = _lforge_float;
	else if(urid == lforge->forge->Double)
		hook = _lforge_double;
	else if(urid == lforge->forge->Bool)
		hook = _lforge_bool;
	else if(urid == lforge->forge->URID)
		hook = _lforge_urid;
	else if(urid == lforge->forge->String)
		hook = _lforge_string;
	else if(urid == lforge->forge->Literal)
		hook = _lforge_literal;
	else if(urid == lforge->forge->URI)
		hook = _lforge_uri;
	else if(urid == lforge->forge->Path)
		hook = _lforge_path;

	else if(urid == lforge->forge->Chunk)
		hook = _lforge_chunk;
	else if(urid == moony->uris.midi_event)
		hook = _lforge_midi;
	else if(urid == moony->oforge.OSC_Bundle)
		hook = _lforge_osc_bundle;
	else if(urid == moony->oforge.OSC_Message)
		hook = _lforge_osc_message;
	else if(urid == lforge->forge->Tuple)
		hook = _lforge_tuple;
	else if(urid == lforge->forge->Object)
		hook = _lforge_object;
	else if(urid == lforge->forge->Property)
		hook = _lforge_property;
	else if(urid == lforge->forge->Vector)
		hook = _lforge_vector;
	else if(urid == lforge->forge->Sequence)
		hook = _lforge_sequence;

	if(!hook)
		return luaL_error(L, "unknown atom type");

	return hook(L);
}

static int
_lforge_get(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	LV2_URID property = luaL_checkinteger(L, 3);

	LV2_Atom_Forge_Frame frame;

	if(!lv2_atom_forge_object(lforge->forge, &frame, 0, moony->uris.patch.get))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.property))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_urid(lforge->forge, property))
		luaL_error(L, forge_buffer_overflow);

	lv2_atom_forge_pop(lforge->forge, &frame);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_set(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	LV2_URID property = luaL_checkinteger(L, 3);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, moony->uris.patch.set))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.property))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_urid(lforge->forge, property))
		luaL_error(L, forge_buffer_overflow);

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.value))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_put(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 2;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, &lframe->frame[0], 0, moony->uris.patch.put))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.body))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_object(lforge->forge, &lframe->frame[1], 0, 0))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_patch(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, moony->uris.patch.patch))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	return 1; // derived forge
}

static int
_lforge_remove(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.remove))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, 0))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_add(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE, lforge->lheader.cache);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch.add))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, 0))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_pop(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	for(int i=lforge->depth; i>0; i--)
	{
		if(&lforge->frame[i-1] != lforge->forge->stack) // intercept assert
			luaL_error(L, "forge frame mismatch");
		lv2_atom_forge_pop(lforge->forge, &lforge->frame[i-1]);
	}
	lforge->depth = 0; // reset depth

	return 0;
}

static int
_lforge_read(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(lforge->lheader.type != MOONY_UDATA_STASH)
		luaL_error(L, "not a stash object");

	return _lstash_read(L);
}

static int
_lforge__gc(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(lforge->lheader.type == MOONY_UDATA_STASH)
		return _lstash__gc(L);

	return 0;
}

static int
_lforge__tostring(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lua_pushfstring(L, "(forge: %p)", lforge);
	return 1;
}

const luaL_Reg lforge_mt [] = {
	{"frame_time", _lforge_frame_time},
	{"beat_time", _lforge_beat_time},
	{"time", _lforge_time},

	{"atom", _lforge_atom},

	{"int", _lforge_int},
	{"long", _lforge_long},
	{"float", _lforge_float},
	{"double", _lforge_double},
	{"bool", _lforge_bool},
	{"urid", _lforge_urid},
	{"string", _lforge_string},
	{"literal", _lforge_literal},
	{"uri", _lforge_uri},
	{"path", _lforge_path},

	{"chunk", _lforge_chunk},
	{"midi", _lforge_midi},
	{"bundle", _lforge_osc_bundle},
	{"message", _lforge_osc_message},
	{"tuple", _lforge_tuple},
	{"object", _lforge_object},
	{"key", _lforge_key},
	{"property", _lforge_property},
	{"vector", _lforge_vector},
	{"sequence", _lforge_sequence},

	{"typed", _lforge_typed},

	{"get", _lforge_get},
	{"set", _lforge_set},
	{"put", _lforge_put},
	{"patch", _lforge_patch},
	{"remove", _lforge_remove},
	{"add", _lforge_add},

	{"pop", _lforge_pop},

	{"read", _lforge_read},

	{"__tostring", _lforge__tostring},
	{"__gc", _lforge__gc},

	{NULL, NULL}
};

const char *forge_buffer_overflow = "forge buffer overflow";
