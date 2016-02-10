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

static inline void
_lstateresponder_register_access(lua_State *L, moony_t *moony, int64_t frames,
	lforge_t *lforge, const LV2_Atom_URID *subject, LV2_URID access)
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

		const char *label = "undefined"; // fallback
		LV2_URID range = moony->forge.Int; // fallback

		if(lua_geti(L, -1, moony->uris.rdfs_label) == LUA_TSTRING)
			label = lua_tostring(L, -1);
		lua_pop(L, 1); // label

		if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER)
			range = lua_tointeger(L, -1);
		lua_pop(L, 1); // range

		if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		  || !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_patch) )
			luaL_error(L, forge_buffer_overflow);
		{
			if(subject)
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
					|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
					luaL_error(L, forge_buffer_overflow);
			}

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_remove)
				|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0)

				|| !lv2_atom_forge_key(lforge->forge, access)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);
			lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_add)
				|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0)

				|| !lv2_atom_forge_key(lforge->forge, access)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);
			lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
		}
		lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

		if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
			|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_patch) )
			luaL_error(L, forge_buffer_overflow);
		{
			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_remove)
				|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0) )
				luaL_error(L, forge_buffer_overflow);
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_range)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_comment)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.core_minimum)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.core_maximum)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.units_unit)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.core_scale_point)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard) )
					luaL_error(L, forge_buffer_overflow);
			}
			lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_add)
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

				if(lua_geti(L, -1, moony->uris.core_minimum) != LUA_TNIL)
				{
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.core_minimum)
						|| !_lforge_basic(L, -1, lforge->forge, range) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // minimum

				if(lua_geti(L, -1, moony->uris.core_maximum) != LUA_TNIL)
				{
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.core_maximum)
						|| !_lforge_basic(L, -1, lforge->forge, range) )
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

				if(lua_geti(L, -1, moony->uris.core_scale_point) != LUA_TNIL)
				{
					// iterate over properties
					lua_pushnil(L);  // first key 
					while(lua_next(L, -2))
					{
						// uses 'key' (at index -2) and 'value' (at index -1)
						size_t point_size;
						const char *point = luaL_checklstring(L, -2, &point_size);
						LV2_Atom_Forge_Frame scale_point_frame;

						if(  !lv2_atom_forge_key(lforge->forge, moony->uris.core_scale_point)
							|| !lv2_atom_forge_object(lforge->forge, &scale_point_frame, 0, 0)

							|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
							|| !lv2_atom_forge_string(lforge->forge, point, point_size)

							|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdf_value)
							|| !_lforge_basic(L, -1, lforge->forge, range) )
							luaL_error(L, forge_buffer_overflow);
						
						lv2_atom_forge_pop(lforge->forge, &scale_point_frame); // core:scalePoint

						// removes 'value'; keeps 'key' for next iteration
						lua_pop(L, 1);
					}
				}
				lua_pop(L, 1); // scale_points
			}
			lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
		}
		lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

		// removes 'value'; keeps 'key' for next iteration
		lua_pop(L, 1);
	}
}

static inline void
_lstateresponder_register(lua_State *L, moony_t *moony, int64_t frames,
	lforge_t *lforge, const LV2_Atom_URID *subject)
{
	LV2_Atom_Forge_Frame obj_frame;
	LV2_Atom_Forge_Frame add_frame;
	LV2_Atom_Forge_Frame rem_frame;

	// clear all properties
	if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_patch) )
		luaL_error(L, forge_buffer_overflow);
	{
		if(subject)
		{
			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
				|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
				luaL_error(L, forge_buffer_overflow);
		}

		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_remove)
			|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0)

			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch_writable)
			|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch_readable)
			|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard) )
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_add)
			|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0) )
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
	}
	lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

	if(lua_geti(L, 1, moony->uris.patch_writable) != LUA_TNIL)
	{
		_lstateresponder_register_access(L, moony, frames, lforge, subject,
			moony->uris.patch_writable);
	}
	lua_pop(L, 1); // nil || table

	if(lua_geti(L, 1, moony->uris.patch_readable) != LUA_TNIL)
	{
		_lstateresponder_register_access(L, moony, frames, lforge, subject,
			moony->uris.patch_readable);
	}
	lua_pop(L, 1); // nil || table
}

static int
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
	lobj_t *lobj = NULL;
	if(luaL_testudata(L, 4, "latom"))
		lobj = lua_touserdata(L, 4);
	lua_pop(L, 1); // atom

	if(!lobj) // not a valid atom, abort
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	if(lobj->obj->body.otype == moony->uris.patch_get)
	{
		const LV2_Atom_URID *subject = NULL;
		const LV2_Atom_URID *property = NULL;

		LV2_Atom_Object_Query q [] = {
			{ moony->uris.patch_subject, (const LV2_Atom **)&subject },
			{ moony->uris.patch_property, (const LV2_Atom **)&property },
			{ 0, NULL }
		};
		lv2_atom_object_query(lobj->obj, q);

		if(!subject || ((subject->atom.type == moony->forge.URID) && (subject->body == moony->uris.subject)) )
		{
			if(!property)
			{
				// register state
				_lstateresponder_register(L, moony, frames, lforge, subject);

				lua_pushboolean(L, 1); // handled
				return 1;
			}
			else if(property->atom.type == moony->forge.URID)
			{
				bool found_it = false;

				if(lua_geti(L, 1, moony->uris.patch_writable) != LUA_TNIL)
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
					if(lua_geti(L, 1, moony->uris.patch_readable) != LUA_TNIL)
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
					LV2_URID range = moony->forge.Int; // fallback

					// get atom type
					if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER)
						range = lua_tointeger(L, -1);
					lua_pop(L, 1); // range

					LV2_Atom_Forge_Frame obj_frame;

					if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
						|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_set) )
						luaL_error(L, forge_buffer_overflow);
					{
						if(subject)
						{
							if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
								|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
								luaL_error(L, forge_buffer_overflow);
						}

						if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_property)
							|| !lv2_atom_forge_urid(lforge->forge, property->body) )
							luaL_error(L, forge_buffer_overflow);

						if(lua_geti(L, -1, moony->uris.patch_get) != LUA_TNIL)
						{
							lua_pushvalue(L, -2); // self[property]
							lua_pushvalue(L, 2); // frames
							lua_pushvalue(L, 3); // forge
							lua_call(L, 3, 1);
						}
						else
						{
							lua_pop(L, 1); // nil
							lua_geti(L, -1, moony->uris.rdf_value);
						}

						if(!lua_isnil(L, -1))
						{
							if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_value)
								|| !_lforge_basic(L, -1, lforge->forge, range) )
								luaL_error(L, forge_buffer_overflow);
						}
					}
					lv2_atom_forge_pop(lforge->forge, &obj_frame);

					lua_pushboolean(L, 1); // handled
					return 1;
				}
			}
		}
	}
	else if(lobj->obj->body.otype == moony->uris.patch_set)
	{
		const LV2_Atom_URID *subject = NULL;
		const LV2_Atom_URID *property = NULL;
		const LV2_Atom *value = NULL;

		LV2_Atom_Object_Query q [] = {
			{ moony->uris.patch_subject, (const LV2_Atom **)&subject },
			{ moony->uris.patch_property, (const LV2_Atom **)&property },
			{ moony->uris.patch_value, &value },
			{ 0, NULL }
		};
		lv2_atom_object_query(lobj->obj, q);

		if(!subject || ((subject->atom.type == moony->forge.URID) && (subject->body == moony->uris.subject)) )
		{
			if(property && (property->atom.type == moony->forge.URID) && value)
			{
				if(  (lua_geti(L, 1, moony->uris.patch_writable) != LUA_TNIL)
					&& (lua_geti(L, -1, property->body) != LUA_TNIL) ) // self[property]
				{
					if(lua_geti(L, -1, moony->uris.patch_set) != LUA_TNIL)
					{
						lua_pushvalue(L, -2); // self[property]
						lua_pushvalue(L, 2); // frames
						lua_pushvalue(L, 3); // forge
						_latom_value(L, value);
						lua_call(L, 4, 0);
					}
					else
					{
						lua_pop(L, 1); // nil
						_latom_value(L, value);
						lua_seti(L, -2, moony->uris.rdf_value); // self[property].value = value
					}

					lua_pushboolean(L, 1); // handled
					return 1;
				}
			}
		}
	}

	lua_pushboolean(L, 0); // not handled
	return 1;
}

static int
_lstateresponder_reg(lua_State *L)
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
		.body = moony->uris.subject
	};

	// register state
	_lstateresponder_register(L, moony, frames, lforge, &subject);

	return 0;
}

static int
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
	if(lua_geti(L, 1, moony->uris.patch_writable) == LUA_TNIL)
		return 0;

	LV2_Atom_Forge_Frame frame;
	if(!lv2_atom_forge_object(lforge->forge, &frame, 0, 0))
		luaL_error(L, forge_buffer_overflow);

	// iterate over writable properties
	lua_pushnil(L);  // first key 
	while(lua_next(L, -2))
	{
		// uses 'key' (at index -2) and 'value' (at index -1)
		const LV2_URID key = luaL_checkinteger(L, -2);
		LV2_URID range = lforge->forge->Int; // fallback

		if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER) // prop[RDFS.range]
			range = lua_tointeger(L, -1);
		lua_pop(L, 1); // range

		if(lua_geti(L, -1, moony->uris.rdf_value) != LUA_TNIL) // prop[RDF.value]
		{
			//TODO call prop[Patch.Get] ?
			if(  !lv2_atom_forge_key(lforge->forge, key)
				|| !_lforge_basic(L, -1, lforge->forge, range) )
				luaL_error(L, forge_buffer_overflow);
		}
		lua_pop(L, 1); // nil || prop[RDF.value]

		// removes 'value'; keeps 'key' for next iteration
		lua_pop(L, 1);
	}

	lv2_atom_forge_pop(lforge->forge, &frame);

	return 0;
}

static int
_lstateresponder_apply(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: obj

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);
	
	lobj_t *lobj = luaL_checkudata(L, 2, "latom");

	// ignore patch:readable's
	if(lua_geti(L, 1, moony->uris.patch_writable) == LUA_TNIL)
		return 0;

	LV2_ATOM_OBJECT_FOREACH(lobj->obj, prop)
	{
		if(lua_geti(L, -1, prop->key) != LUA_TNIL)
		{
			_latom_value(L, &prop->value);
			lua_seti(L, -2, moony->uris.rdf_value); // set prop[RDF.value]
			//TODO call prop[Patch.Set] ?
		}

		lua_pop(L, 1); // nil || prop
	}

	return 0;
}

int
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
	{"register", _lstateresponder_reg},
	{"stash", _lstateresponder_stash},
	{"apply", _lstateresponder_apply},
	{NULL, NULL}
};
