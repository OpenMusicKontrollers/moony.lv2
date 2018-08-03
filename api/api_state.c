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

#include <api_state.h>
#include <api_atom.h>
#include <api_forge.h>

__realtime static inline void
_lstateresponder_register_access(lua_State *L, moony_t *moony, int64_t frames,
	lforge_t *lforge, const LV2_Atom_URID *subject, LV2_URID access, int32_t sequence_num)
{
	LV2_Atom_Forge_Frame obj_frame;
	LV2_Atom_Forge_Frame add_frame;
	LV2_Atom_Forge_Frame rem_frame;

	// iterate over properties
	lua_pushnil(L);  // first key 
	while(lua_next(L, -2))
	{
		// uses 'key' (at index -2) and 'value' (at index -1)
		const LV2_URID key = luaL_checkinteger(L, -2);

		const char *label = ""; // fallback
		LV2_URID range = 0; // fallback
		LV2_URID child_type = 0; // fallback

		if(lua_geti(L, -1, moony->uris.rdfs_label) == LUA_TSTRING)
			label = lua_tostring(L, -1);
		lua_pop(L, 1); // label

		if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER)
			range = lua_tointeger(L, -1);
		lua_pop(L, 1); // range

		if(lua_geti(L, -1, moony->uris.atom_child_type) == LUA_TNUMBER)
			child_type= lua_tointeger(L, -1);
		lua_pop(L, 1); // child_type

		if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		  || !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch.patch) )
			luaL_error(L, forge_buffer_overflow);
		{
			if(subject)
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject)
					|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
					luaL_error(L, forge_buffer_overflow);
			}

			if(sequence_num)
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.sequence)
					|| !lv2_atom_forge_int(lforge->forge, sequence_num) )
					luaL_error(L, forge_buffer_overflow);
			}

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.remove)
				|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0)

				|| !lv2_atom_forge_key(lforge->forge, access)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);
			lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.add)
				|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0)

				|| !lv2_atom_forge_key(lforge->forge, access)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);
			lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
		}
		lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

		if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
			|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch.patch) )
			luaL_error(L, forge_buffer_overflow);
		{
			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);

			if(sequence_num)
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.sequence)
					|| !lv2_atom_forge_int(lforge->forge, sequence_num) )
					luaL_error(L, forge_buffer_overflow);
			}

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.remove)
				|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0) )
				luaL_error(L, forge_buffer_overflow);
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_range)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_comment)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.lv2_minimum)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.lv2_maximum)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.units_unit)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.units_symbol)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.moony_color)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.moony_syntax)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.lv2_scale_point)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard) )
					luaL_error(L, forge_buffer_overflow);
			}
			lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.add)
				|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0) )
				luaL_error(L, forge_buffer_overflow);
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
					|| !lv2_atom_forge_string(lforge->forge, label, strlen(label))

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_range)
					|| !lv2_atom_forge_urid(lforge->forge, range) )
					luaL_error(L, forge_buffer_overflow);

				if(lua_geti(L, -1, moony->uris.rdfs_comment) == LUA_TSTRING)
				{
					size_t comment_size;
					const char *comment = lua_tolstring(L, -1, &comment_size);
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_comment)
						|| !lv2_atom_forge_string(lforge->forge, comment, comment_size) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // comment

				const LV2_URID range2 = (range == lforge->forge->Vector)
					? child_type
					: range;

				if(lua_geti(L, -1, moony->uris.lv2_minimum) != LUA_TNIL)
				{
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.lv2_minimum)
						|| !_lforge_basic(L, -1, lforge->forge, range2, 0) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // minimum

				if(lua_geti(L, -1, moony->uris.lv2_maximum) != LUA_TNIL)
				{
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.lv2_maximum)
						|| !_lforge_basic(L, -1, lforge->forge, range2, 0) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // maximum

				if(lua_geti(L, -1, moony->uris.units_unit) == LUA_TNUMBER)
				{
					const LV2_URID unit = lua_tointeger(L, -1);
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.units_unit)
						|| !lv2_atom_forge_urid(lforge->forge, unit) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // unit

				if(lua_geti(L, -1, moony->uris.units_symbol) == LUA_TSTRING)
				{
					size_t len;
					const char *symbol = lua_tolstring(L, -1, &len);
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.units_symbol)
						|| !lv2_atom_forge_string(lforge->forge, symbol, len) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // symbol

				if(lua_geti(L, -1, moony->uris.moony_color) == LUA_TNUMBER)
				{
					const uint32_t col = lua_tointeger(L, -1);
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.moony_color)
						|| !lv2_atom_forge_long(lforge->forge, col) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // symbol

				if(lua_geti(L, -1, moony->uris.moony_syntax) == LUA_TNUMBER)
				{
					const LV2_URID syntax = lua_tointeger(L, -1);
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.moony_syntax)
						|| !lv2_atom_forge_urid(lforge->forge, syntax) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // symbol

				if(lua_geti(L, -1, moony->uris.lv2_scale_point) != LUA_TNIL)
				{
					LV2_Atom_Forge_Frame tuple_frame;
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.lv2_scale_point)
						|| !lv2_atom_forge_tuple(lforge->forge, &tuple_frame) )
						luaL_error(L, forge_buffer_overflow);

					// iterate over properties
					lua_pushnil(L);  // first key 
					while(lua_next(L, -2))
					{
						// uses 'key' (at index -2) and 'value' (at index -1)
						size_t point_size;
						const char *point = luaL_checklstring(L, -2, &point_size);
						LV2_Atom_Forge_Frame scale_point_frame;

						if(  !lv2_atom_forge_object(lforge->forge, &scale_point_frame, 0, 0)

							|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
							|| !lv2_atom_forge_string(lforge->forge, point, point_size)

							|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdf_value)
							|| !_lforge_basic(L, -1, lforge->forge, range, 0) )
							luaL_error(L, forge_buffer_overflow);
						
						lv2_atom_forge_pop(lforge->forge, &scale_point_frame); // core:scalePoint

						// removes 'value'; keeps 'key' for next iteration
						lua_pop(L, 1);
					}

					lv2_atom_forge_pop(lforge->forge, &tuple_frame);
				}
				lua_pop(L, 1); // scale_points
			}
			lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
		}
		lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

		//FIXME also send patch:set ?

		// removes 'value'; keeps 'key' for next iteration
		lua_pop(L, 1);
	}
}

__realtime static inline void
_lstateresponder_reg(lua_State *L, moony_t *moony, int64_t frames,
	lforge_t *lforge, const LV2_Atom_URID *subject, int32_t sequence_num)
{
	LV2_Atom_Forge_Frame obj_frame;
	LV2_Atom_Forge_Frame add_frame;
	LV2_Atom_Forge_Frame rem_frame;

	// clear all properties
	if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch.patch) )
		luaL_error(L, forge_buffer_overflow);
	{
		if(subject)
		{
			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject)
				|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
				luaL_error(L, forge_buffer_overflow);
		}

		if(sequence_num)
		{
			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.sequence)
				|| !lv2_atom_forge_int(lforge->forge, sequence_num) )
				luaL_error(L, forge_buffer_overflow);
		}

		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.remove)
			|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0)

			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch.writable)
			|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard)

			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch.readable)
			|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.wildcard) )
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.add)
			|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0) )
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
	}
	lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

	if(lua_geti(L, 1, moony->uris.patch.writable) != LUA_TNIL)
	{
		_lstateresponder_register_access(L, moony, frames, lforge, subject,
			moony->uris.patch.writable, sequence_num);
	}
	lua_pop(L, 1); // nil || table

	if(lua_geti(L, 1, moony->uris.patch.readable) != LUA_TNIL)
	{
		_lstateresponder_register_access(L, moony, frames, lforge, subject,
			moony->uris.patch.readable, sequence_num);
	}
	lua_pop(L, 1); // nil || table
}

__realtime static void
_lstateresponder_error(lua_State *L, lforge_t *lforge, moony_t *moony,
	int64_t frames, int32_t sequence_num)
{
	LV2_Atom_Forge_Frame obj_frame;
	if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch.error) )
		luaL_error(L, forge_buffer_overflow);

	if(sequence_num)
	{
		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.sequence)
			|| !lv2_atom_forge_int(lforge->forge, sequence_num) )
			luaL_error(L, forge_buffer_overflow);
	}

	lv2_atom_forge_pop(lforge->forge, &obj_frame);
}

__realtime static void
_lstateresponder_ack(lua_State *L, lforge_t *lforge, moony_t *moony,
	int64_t frames, int32_t sequence_num)
{
	LV2_Atom_Forge_Frame obj_frame;
	if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch.ack) )
		luaL_error(L, forge_buffer_overflow);

	if(sequence_num)
	{
		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.sequence)
			|| !lv2_atom_forge_int(lforge->forge, sequence_num) )
			luaL_error(L, forge_buffer_overflow);
	}

	lv2_atom_forge_pop(lforge->forge, &obj_frame);
}

__realtime static int
_lstateresponder__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 4); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: forge
	// 4: atom

	int64_t frames = luaL_checkinteger(L, 2);
	lforge_t *lforge = luaL_checkudata(L, 3, "lforge");
	latom_t *latom = luaL_checkudata(L, 4, "latom");
	lua_pop(L, 1); // atom

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	if(lv2_atom_forge_is_object_type(lforge->forge, latom->atom->type))
	{
		if(latom->body.obj->otype == moony->uris.patch.get)
		{
			const LV2_Atom_URID *subject = NULL;
			const LV2_Atom_URID *property = NULL;
			const LV2_Atom_Int *sequence = NULL;

			lv2_atom_object_body_get(latom->atom->size, latom->body.obj,
				moony->uris.patch.subject, &subject,
				moony->uris.patch.property, &property,
				moony->uris.patch.sequence, &sequence,
				0);

			int32_t sequence_num = 0;
			if(sequence && (sequence->atom.type == moony->forge.Int) )
				sequence_num = sequence->body;

			if(!subject || ((subject->atom.type == moony->forge.URID) && (subject->body == moony->uris.patch.self)) )
			{
				if(!property)
				{
					// register state
					_lstateresponder_reg(L, moony, frames, lforge, subject, sequence_num);

					lua_pushboolean(L, 1); // handled
					return 1;
				}
				else if(property->atom.type == moony->forge.URID)
				{
					bool found_it = false;

					if(lua_geti(L, 1, moony->uris.patch.writable) != LUA_TNIL)
					{
						if(lua_geti(L, -1, property->body) != LUA_TNIL)
							found_it = true;
						else
							lua_pop(L, 1); // nil
					}
					else
						lua_pop(L, 1); // nil

					if(!found_it)
					{
						if(lua_geti(L, 1, moony->uris.patch.readable) != LUA_TNIL)
						{
							if(lua_geti(L, -1, property->body) != LUA_TNIL)
								found_it = true;
							else
								lua_pop(L, 1); // readable, property nil
						}
						else
							lua_pop(L, 1); // nil
					}

					if(found_it)
					{
						LV2_URID range = 0; // fallback
						LV2_URID child_type = 0; //fallback

						// get atom type
						if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER)
							range = lua_tointeger(L, -1);
						lua_pop(L, 1); // range

						// get child type
						if(lua_geti(L, -1, moony->uris.atom_child_type) == LUA_TNUMBER)
							child_type = lua_tointeger(L, -1);
						lua_pop(L, 1); // child_type

						LV2_Atom_Forge_Frame obj_frame;

						if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
							|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch.set) )
							luaL_error(L, forge_buffer_overflow);
						{
							if(subject)
							{
								if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject)
									|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
									luaL_error(L, forge_buffer_overflow);
							}

							if(sequence_num)
							{
								if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.sequence)
									|| !lv2_atom_forge_int(lforge->forge, sequence_num) )
									luaL_error(L, forge_buffer_overflow);
							}

							if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.property)
								|| !lv2_atom_forge_urid(lforge->forge, property->body) )
								luaL_error(L, forge_buffer_overflow);

							if(lua_geti(L, -1, moony->uris.rdf_value) != LUA_TNIL)
							{
								if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.value)
									|| !_lforge_basic(L, -1, lforge->forge, range, child_type) )
									luaL_error(L, forge_buffer_overflow);
							}
							lua_pop(L, 1); // value
						}
						lv2_atom_forge_pop(lforge->forge, &obj_frame);

						lua_pushboolean(L, 1); // handled
						return 1;
					}
				}

				if(sequence_num)
					_lstateresponder_error(L, lforge, moony, frames, sequence_num);
			}
		}
		else if(latom->body.obj->otype == moony->uris.patch.set)
		{
			const LV2_Atom_URID *subject = NULL;
			const LV2_Atom_URID *property = NULL;
			const LV2_Atom_Int *sequence = NULL;
			const LV2_Atom *value = NULL;

			lv2_atom_object_body_get(latom->atom->size, latom->body.obj,
				moony->uris.patch.subject, &subject,
				moony->uris.patch.property, &property,
				moony->uris.patch.sequence, &sequence,
				moony->uris.patch.value, &value,
				0);

			int32_t sequence_num = 0;
			if(sequence && (sequence->atom.type == moony->forge.Int) )
				sequence_num = sequence->body;

			if(!subject || ((subject->atom.type == moony->forge.URID) && (subject->body == moony->uris.patch.self)) )
			{
				if(property && (property->atom.type == moony->forge.URID) && value)
				{
					if(  (lua_geti(L, 1, moony->uris.patch.writable) != LUA_TNIL)
						&& (lua_geti(L, -1, property->body) != LUA_TNIL) ) // self[property]
					{
						_latom_value(L, value);
						lua_seti(L, -2, moony->uris.rdf_value); // self[property][RDF.value] = value

						if(sequence_num)
							_lstateresponder_ack(L, lforge, moony, frames, sequence_num);

						lua_pushboolean(L, 1); // handled
						return 1;
					}
				}

				if(sequence_num)
					_lstateresponder_error(L, lforge, moony, frames, sequence_num);
			}
		}
		else if(latom->body.obj->otype == moony->uris.patch.put)
		{
			const LV2_Atom_Object *patch_body = NULL;
			const LV2_Atom_Int *sequence = NULL;

			lv2_atom_object_body_get(latom->atom->size, latom->body.obj,
				moony->uris.patch.body, &patch_body,
				moony->uris.patch.sequence, &sequence,
				NULL);

			int32_t sequence_num = 0;
			if(sequence && (sequence->atom.type == moony->forge.Int) )
				sequence_num = sequence->body;

			if(patch_body && (patch_body->atom.type == moony->forge.Object) )
			{
				LV2_ATOM_OBJECT_FOREACH(patch_body, prop)
				{
					const LV2_URID property = prop->key;
					const LV2_Atom *value = &prop->value;

					if(  (lua_geti(L, 1, moony->uris.patch.writable) != LUA_TNIL)
						&& (lua_geti(L, -1, property) != LUA_TNIL) ) // self[property]
					{
						_latom_value(L, value);
						lua_seti(L, -2, moony->uris.rdf_value); // self[property][RDF.value] = value
					}
				}

				if(sequence_num)
					_lstateresponder_ack(L, lforge, moony, frames, sequence_num);

				lua_pushboolean(L, 1); // handled
				return 1;
			}

			if(sequence_num)
				_lstateresponder_error(L, lforge, moony, frames, sequence_num);
		}
	}

	lua_pushboolean(L, 0); // not handled
	return 1;
}

__realtime static int
_lstateresponder_register(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 3); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: forge

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	int64_t frames = luaL_checkinteger(L, 2);
	lforge_t *lforge = luaL_checkudata(L, 3, "lforge");

	// fake subject
	const LV2_Atom_URID subject = {
		.atom = {
			.size = sizeof(uint32_t),
			.type = lforge->forge->URID
		},
		.body = moony->uris.patch.self
	};

	// register state
	_lstateresponder_reg(L, moony, frames, lforge, &subject, 0); //TODO use patch:sequenceNumber

	return 1; // forge
}

__realtime static int
_lstateresponder_sync(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 3); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: forge
	//FIXME support syncing specific parameter only

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	int64_t frames = luaL_checkinteger(L, 2);
	lforge_t *lforge = luaL_checkudata(L, 3, "lforge");

	LV2_Atom_Forge_Frame obj_frame;

	if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch.put) )
		luaL_error(L, forge_buffer_overflow);
	{
		LV2_Atom_Forge_Frame body_frame;

		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch.subject)
			|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch.self)
			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch.sequence)
			|| !lv2_atom_forge_int(lforge->forge, 0) //TODO
			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch.body)
			|| !lv2_atom_forge_object(lforge->forge, &body_frame, 0, 0) )
			luaL_error(L, forge_buffer_overflow);
		{
			if(lua_geti(L, 1, moony->uris.patch.writable) != LUA_TNIL)
			{
				// iterate over writable properties
				lua_pushnil(L);
				while(lua_next(L, -2))
				{
					const LV2_URID key = lua_tointeger(L, -2); // key
					LV2_URID range = 0; // fallback
					LV2_URID child_type = 0; // fallback

					if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER) // prop[RDFS.range]
						range = lua_tointeger(L, -1);
					lua_pop(L, 1); // range

					if(lua_geti(L, -1, moony->uris.atom_child_type) == LUA_TNUMBER) // prop[Atom.childType]
						child_type = lua_tointeger(L, -1);
					lua_pop(L, 1); // child_type

					if(lua_geti(L, -1, moony->uris.rdf_value) != LUA_TNIL)
					{
						if(  !lv2_atom_forge_key(lforge->forge, key)
							|| !_lforge_basic(L, -1, lforge->forge, range, child_type) )
							luaL_error(L, forge_buffer_overflow);
					}
					lua_pop(L, 1); // nil || rdf_value

					lua_pop(L, 1); // removes 'value', keeps 'key' for next iteration
				}
			}
			lua_pop(L, 1); // nil || writable

			if(lua_geti(L, 1, moony->uris.patch.readable) != LUA_TNIL)
			{
				// iterate over writable properties
				lua_pushnil(L);
				while(lua_next(L, -2))
				{
					const LV2_URID key = lua_tointeger(L, -2); // key
					LV2_URID range = 0; // fallback
					LV2_URID child_type = 0; // fallback

					if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER) // prop[RDFS.range]
						range = lua_tointeger(L, -1);
					lua_pop(L, 1); // range

					if(lua_geti(L, -1, moony->uris.atom_child_type) == LUA_TNUMBER) // prop[Atom.childType]
						child_type = lua_tointeger(L, -1);
					lua_pop(L, 1); // child_type

					if(lua_geti(L, -1, moony->uris.rdf_value) != LUA_TNIL)
					{
						if(  !lv2_atom_forge_key(lforge->forge, key)
							|| !_lforge_basic(L, -1, lforge->forge, range, child_type) )
							luaL_error(L, forge_buffer_overflow);
					}
					lua_pop(L, 1); // nil || rdf_value

					lua_pop(L, 1); // removes 'value', keeps 'key' for next iteration
				}
			}
			lua_pop(L, 1); // nil || readable
		}
		lv2_atom_forge_pop(lforge->forge, &body_frame);
	}
	lv2_atom_forge_pop(lforge->forge, &obj_frame);

	return 0;
}

__realtime static int
_lstateresponder_stash(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: forge

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	lforge_t *lforge = luaL_checkudata(L, 2, "lforge");

	// ignore patch:readable's
	if(lua_geti(L, 1, moony->uris.patch.writable) == LUA_TNIL)
	{
		lua_pop(L, 1); // nil
		return 1; // forge
	}

	LV2_Atom_Forge_Frame frame;
	if(!lv2_atom_forge_object(lforge->forge, &frame, 0, 0))
		luaL_error(L, forge_buffer_overflow);

	// iterate over writable properties
	lua_pushnil(L);  // first key 
	while(lua_next(L, -2))
	{
		// uses 'key' (at index -2) and 'value' (at index -1)
		const LV2_URID key = luaL_checkinteger(L, -2);
		LV2_URID range = 0; // fallback
		LV2_URID child_type = 0; // fallback

		if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER) // prop[RDFS.range]
			range = lua_tointeger(L, -1);
		lua_pop(L, 1); // range

		if(lua_geti(L, -1, moony->uris.atom_child_type) == LUA_TNUMBER) // prop[Atom.child_type]
			child_type = lua_tointeger(L, -1);
		lua_pop(L, 1); // child_type

		if(lua_geti(L, -1, moony->uris.rdf_value) != LUA_TNIL) // prop[RDF.value]
		{
			if(  !lv2_atom_forge_key(lforge->forge, key)
				|| !_lforge_basic(L, -1, lforge->forge, range, child_type) )
				luaL_error(L, forge_buffer_overflow);
		}
		lua_pop(L, 1); // value

		// removes 'value'; keeps 'key' for next iteration
		lua_pop(L, 1);
	}

	lv2_atom_forge_pop(lforge->forge, &frame);

	lua_pop(L, 1); // patch:writable
	return 1; // forge
}

__realtime static int
_lstateresponder_apply(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: obj

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);
	
	latom_t *latom = luaL_checkudata(L, 2, "latom");

	// check for atom object
	if(!lv2_atom_forge_is_object_type(&moony->forge, latom->atom->type))
	{
		lua_pushboolean(L, false);
		return 1;
	}

	// ignore patch:readable's
	if(lua_geti(L, 1, moony->uris.patch.writable) != LUA_TNIL)
	{
		LV2_ATOM_OBJECT_BODY_FOREACH(latom->body.obj, latom->atom->size, prop)
		{
			if(lua_geti(L, -1, prop->key) != LUA_TNIL)
			{
				_latom_value(L, &prop->value);
				lua_seti(L, -2, moony->uris.rdf_value); // set prop[RDF.value]
			}
			lua_pop(L, 1); // nil || prop
		}
	}

	lua_pushboolean(L, true);
	return 1;
}

__realtime int
_lstateresponder(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	// o = new 
	int32_t *dummy = lua_newuserdata(L, sizeof(int32_t));
	(void)dummy;

	// o.uservalue = uservalue
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "lstateresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

const luaL_Reg lstateresponder_mt [] = {
	{"__call", _lstateresponder__call},
	{"register", _lstateresponder_register},
	{"sync", _lstateresponder_sync},
	{"stash", _lstateresponder_stash},
	{"apply", _lstateresponder_apply},
	{NULL, NULL}
};
