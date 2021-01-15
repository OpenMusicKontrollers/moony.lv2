/*
 * Copyright (c) 2015-2021 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <api_stash.h>

__realtime int
_lstash__gc(lua_State *L)
{
	lstash_t *lstash = lua_touserdata(L, 1);
	atom_ser_t *ser = &lstash->ser;

	if(ser->buf)
		moony_rt_free(ser->data, ser->buf, ser->size);

	return 0;
}

__realtime int
_lstash_write(lua_State *L)
{
	lstash_t *lstash = lua_touserdata(L, 1);
	atom_ser_t *ser = &lstash->ser;
	lforge_t *lforge = &lstash->lforge;

	lforge->depth = 0;
	lforge->last.frames = 0;
	lforge->forge = &lstash->forge;
	
	ser->offset = 0; // reset stash pointer
	lv2_atom_forge_set_sink(&lstash->forge, _sink_rt, _deref, ser);

	LV2_Atom *atom = (LV2_Atom *)ser->buf;
	atom->type = 0;
	atom->size = 0;

	luaL_getmetatable(L, "lforge");
	lua_setmetatable(L, 1);

	return 1;
}

__realtime int
_lstash_read(lua_State *L)
{
	lstash_t *lstash = lua_touserdata(L, 1);
	atom_ser_t *ser = &lstash->ser;
	latom_t *latom = &lstash->latom;

	latom->atom = (const LV2_Atom *)ser->buf;
	latom->body.raw = LV2_ATOM_BODY_CONST(latom->atom);

	luaL_getmetatable(L, "latom");
	lua_setmetatable(L, 1);

	return 1;
}

__realtime int
_lstash(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	moony_vm_t *vm = lua_touserdata(L, lua_upvalueindex(2));

	lstash_t *lstash = moony_newuserdata(L, moony, MOONY_UDATA_STASH, false);

	// initialize forge (URIDs)
	memcpy(&lstash->forge, &moony->forge, sizeof(LV2_Atom_Forge));

	// initialize memory pool
	atom_ser_t *ser = &lstash->ser;
	ser->data = vm;
	ser->size = 1024;
	ser->offset = 0; // reset stash pointer
	ser->buf = moony_rt_alloc(vm, ser->size);

	if(!ser->buf)
		lua_pushnil(L); // memory allocation failed

	// start off as forge
	_lstash_write(L);

	return 1;
}

const luaL_Reg lstash_mt [] = {
	{NULL, NULL}
};
