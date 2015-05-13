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

#include <Eina.h>
#include <ext_urid.h>

#include <lauxlib.h>

#define BUF_SIZE 8191

typedef struct _handle_t handle_t;

struct _handle_t {
	moony_t moony;

	ext_urid_t *ext_urid;

	LV2_Atom_Forge forge;

	uint8_t buf [8192] __attribute__((aligned(8)));
};

static int
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

		lforge_t *lforge = lua_newuserdata(L, sizeof(lforge_t));
		lforge->forge = forge;
		luaL_getmetatable(L, "lforge");
		lua_setmetatable(L, -2);

		if(lua_pcall(L, 1, 0, 0))
		{
			fprintf(stderr, "err: %s\n", lua_tostring(L, -1));
			exit(-1);
		}
	}
	lv2_atom_forge_pop(forge, &frame);

	// consume events
	{
		lua_pushvalue(L, 2); // consumer

		lseq_t *lseq = lua_newuserdata(L, sizeof(lseq_t));
		lseq->seq = (const LV2_Atom_Sequence *)handle->buf;
		lseq->itr = NULL;
		luaL_getmetatable(L, "lseq");
		lua_setmetatable(L, -2);
			
		if(lua_pcall(L, 1, 0, 0))
		{
			fprintf(stderr, "err: %s\n", lua_tostring(L, -1));
			exit(-1);
		}
	}

	return 0;
}

static LV2_URID
_map(LV2_URID_Map_Handle instance, const char *uri)
{
	handle_t *handle = instance;

	return ext_urid_map(handle->ext_urid, uri);
}

static const char *
_unmap(LV2_URID_Unmap_Handle instance, LV2_URID urid)
{
	handle_t *handle = instance;

	return ext_urid_unmap(handle->ext_urid, urid);
}

static LV2_Worker_Status
_sched(LV2_Worker_Schedule_Handle instance, uint32_t size, const void *data)
{
	handle_t *handle = instance;

	printf("_sched %u\n", size);
	//TODO

	return LV2_WORKER_SUCCESS;
}

static int
_vprintf(void *data, LV2_URID type, const char *fmt, va_list args)
{
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);

	return 0;
}

static int
_printf(void *data, LV2_URID type, const char *fmt, ...)
{
  va_list args;
	int ret;

  va_start (args, fmt);
	ret = _vprintf(data, type, fmt, args);
  va_end(args);

	return ret;
}

int
main(int argc, char **argv)
{
	static handle_t handle;
	
	eina_init();

	handle.ext_urid = ext_urid_new();
	if(!handle.ext_urid)
		return -1;

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
	
	if(moony_init(&handle.moony, features))
		return -1;

	lua_State *L = handle.moony.vm.L;
	moony_open(&handle.moony, L);

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

	if(luaL_dofile(L, argv[1]))
	{
		fprintf(stderr, "err: %s\n", lua_tostring(L, -1));
		return -1;
	}

	moony_deinit(&handle.moony);

	ext_urid_free(handle.ext_urid);
	eina_shutdown();

	printf("\n[test] Sucessful\n");

	return 0;
}
