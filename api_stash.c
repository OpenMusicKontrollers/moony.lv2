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

#include <api_stash.h>

int
_lstash__gc(lua_State *L)
{
	lstash_t *lstash = lua_touserdata(L, 1);
	atom_ser_t *ser = &lstash->ser;

	if(ser->buf)
		tlsf_free(ser->tlsf, ser->buf);

	return 0;
}

int
_lstash_write(lua_State *L)
{
	lstash_t *lstash = lua_touserdata(L, 1);
	atom_ser_t *ser = &lstash->ser;
	lforge_t *lforge = &lstash->lforge;

	lforge->depth = 0;
	lforge->last.frames = 0;
	lforge->forge = &lstash->forge;
	
	ser->offset = 0; // reset stash pointer
	lv2_atom_forge_set_sink(&lstash->forge, _sink, _deref, ser);

	LV2_Atom *atom = (LV2_Atom *)ser->buf;
	atom->type = 0;
	atom->size = 0;

	luaL_getmetatable(L, "lforge");
	lua_setmetatable(L, 1);

	return 1;
}

int
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

int
_lstash(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lstash_t *lstash = moony_newuserdata(L, moony, MOONY_UDATA_STASH);

	// initialize forge (URIDs)
	memcpy(&lstash->forge, &moony->forge, sizeof(LV2_Atom_Forge));

	// initialize memory pool
	atom_ser_t *ser = &lstash->ser;
	ser->tlsf = moony->vm.tlsf;
	ser->size = 256;
	ser->buf = tlsf_malloc(moony->vm.tlsf, ser->size);
	ser->offset = 0; // reset stash pointer

	if(!ser->buf)
		lua_pushnil(L); // memory allocation failed

	// start off as forge
	_lstash_write(L);

	return 1;
}

const luaL_Reg lstash_mt [] = {
	{NULL, NULL}
};
