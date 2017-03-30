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

#include <moony.h>
#include <api_atom.h>
#include <api_forge.h>

#include <lauxlib.h>

#define BUF_SIZE 8192
#define MAX_URIDS 512

typedef struct _urid_t urid_t;
typedef struct _handle_t handle_t;

struct _urid_t {
	LV2_URID urid;
	char *uri;
};

struct _handle_t {
	moony_t moony;

	const LV2_Worker_Interface *iface;

	LV2_Atom_Forge forge;

	uint8_t buf [BUF_SIZE] __attribute__((aligned(8)));
	uint8_t buf2 [BUF_SIZE] __attribute__((aligned(8)));

	urid_t urids [MAX_URIDS];
	LV2_URID urid;
};

__non_realtime static int
_test(lua_State *L)
{
	handle_t *handle = lua_touserdata(L, lua_upvalueindex(1));

	if(!lua_isfunction(L, 1) || !lua_isfunction(L, 2))
	{
		fprintf(stderr, "err: expected 2 function arguments\n");
		exit(-1);
	}

	LV2_Atom_Forge *forge = &handle->forge;
	LV2_Atom_Forge_Frame frame;

	// produce events
	lv2_atom_forge_set_buffer(forge, handle->buf, BUF_SIZE);
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	{
		lua_pushvalue(L, 1); // producer

		lforge_t *lforge = moony_newuserdata(L, &handle->moony, MOONY_UDATA_FORGE, true);
		lforge->depth = 0;
		lforge->last.frames = 0;
		lforge->forge = forge;

		lua_call(L, 1, 0);
	}
	if(&frame != forge->stack)
		fprintf(stderr, "forge frame mismatch\n");
	else
		lv2_atom_forge_pop(forge, &frame);

	// consume events
	lv2_atom_forge_set_buffer(forge, handle->buf2, BUF_SIZE);
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	{
		lua_pushvalue(L, 2); // consumer

		latom_t *latom = moony_newuserdata(L, &handle->moony, MOONY_UDATA_ATOM, true);
		latom->atom = (const LV2_Atom *)handle->buf;
		latom->body.raw = LV2_ATOM_BODY_CONST(latom->atom);
		
		lforge_t *lframe = moony_newuserdata(L, &handle->moony, MOONY_UDATA_FORGE, true);
		lframe->depth = 0;
		lframe->last.frames = 0;
		lframe->forge = forge;
			
		lua_call(L, 2, 0);
	}
	if(&frame != forge->stack)
		fprintf(stderr, "forge frame mismatch\n");
	else
		lv2_atom_forge_pop(forge, &frame);

	return 0;
}

__non_realtime static LV2_URID
_map(LV2_URID_Map_Handle instance, const char *uri)
{
	handle_t *handle = instance;

	urid_t *itm;
	for(itm=handle->urids; itm->urid; itm++)
	{
		if(!strcmp(itm->uri, uri))
			return itm->urid;
	}

	assert(handle->urid + 1 < MAX_URIDS);

	// create new
	itm->urid = ++handle->urid;
	itm->uri = strdup(uri);

	return itm->urid;
}

__non_realtime static const char *
_unmap(LV2_URID_Unmap_Handle instance, LV2_URID urid)
{
	handle_t *handle = instance;

	urid_t *itm;
	for(itm=handle->urids; itm->urid; itm++)
	{
		if(itm->urid == urid)
			return itm->uri;
	}

	// not found
	return NULL;
}

__non_realtime static LV2_Worker_Status
_respond(LV2_Worker_Respond_Handle instance, uint32_t size, const void *data)
{
	handle_t *handle = instance;

	return handle->iface->work_response(&handle->moony, size, data);
}

__non_realtime static LV2_Worker_Status
_sched(LV2_Worker_Schedule_Handle instance, uint32_t size, const void *data)
{
	handle_t *handle = instance;

	LV2_Worker_Status status = LV2_WORKER_SUCCESS;
	status |= handle->iface->work(&handle->moony, _respond, handle, size, data);
	status |= handle->iface->end_run(&handle->moony);

	return status;
}

__non_realtime static int
_vprintf(void *data, LV2_URID type, const char *fmt, va_list args)
{
	vfprintf(stderr, fmt, args);

	return 0;
}

__non_realtime static int
_printf(void *data, LV2_URID type, const char *fmt, ...)
{
  va_list args;
	int ret;

  va_start (args, fmt);
	ret = _vprintf(data, type, fmt, args);
  va_end(args);

	return ret;
}

__non_realtime int
main(int argc, char **argv)
{
	static handle_t handle;
	
	LV2_URID_Map map = {
		.handle = &handle,
		.map = _map
	};
	LV2_URID_Unmap unmap = {
		.handle = &handle,
		.unmap = _unmap
	};
	LV2_Worker_Schedule sched = {
		.handle = &handle,
		.schedule_work = _sched
	};
	LV2_Log_Log log = {
		.handle = &handle,
		.printf = _printf,
		.vprintf = _vprintf
	};

	const LV2_Feature feat_map = {
		.URI = LV2_URID__map,
		.data = &map
	};
	const LV2_Feature feat_unmap = {
		.URI = LV2_URID__unmap,
		.data = &unmap
	};
	const LV2_Feature feat_sched = {
		.URI = LV2_WORKER__schedule,
		.data = &sched
	};
	const LV2_Feature feat_log = {
		.URI = LV2_LOG__log,
		.data = &log
	};

	const LV2_Feature *const features [] = {
		&feat_map,
		&feat_unmap,
		&feat_sched,
		&feat_log,
		NULL
	};
	
	if(moony_init(&handle.moony, "http://open-music-kontrollers.ch/lv2/moony#test",
		48000, features, 0x800000, true)) // 8MB initial memory
	{
		return -1;
	}
	moony_vm_nrt_enter(handle.moony.vm);

	handle.iface = extension_data(LV2_WORKER__interface);

	lua_State *L = moony_current(&handle.moony);
	moony_open(&handle.moony, handle.moony.vm, L);

	lua_pushstring(L, "map");
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);
	
	lua_pushstring(L, "unmap");
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);

	lv2_atom_forge_init(&handle.forge, &map);

	// register test function
	lua_pushlightuserdata(L, &handle);
	lua_pushcclosure(L, _test, 1);
	lua_setglobal(L, "test");

	const int ret = luaL_dofile(L, argv[1]); // wraps around lua_pcall

	if(ret)
		fprintf(stderr, "err: %s\n", lua_tostring(L, -1));

	for(urid_t *itm=handle.urids; itm->urid; itm++)
		free(itm->uri);

	moony_vm_nrt_leave(handle.moony.vm);
	moony_deinit(&handle.moony);

	return ret;
}
