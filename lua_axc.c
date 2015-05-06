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

#include <lua_lv2.h>

#include <lauxlib.h>

typedef struct _Handle Handle;

struct _Handle {
	lua_handle_t lua_handle;

	int max_val;

	const LV2_Atom_Sequence *event_in;
	float *val_out [4];

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;
};

static const char *default_code [4] = {
	"function run(seq)\n"
	"  for frames, atom in seq:foreach() do\n"
	"    -- your code here\n"
	"  end\n"
	"\n"
	"  return 0.0\n"
	"end",

	"function run(seq)\n"
	"  for frames, atom in seq:foreach() do\n"
	"    -- your code here\n"
	"  end\n"
	"\n"
	"  return 0.0, 0.0\n"
	"end",

	NULL,

	"function run(seq)\n"
	"  for frames, atom in seq:foreach() do\n"
	"    -- your code here\n"
	"  end\n"
	"\n"
	"  return 0.0, 0.0, 0.0, 0.0\n"
	"end"
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)calloc(1, sizeof(Handle));
	if(!handle)
		return NULL;

	if(lua_handle_init(&handle->lua_handle, features))
	{
		free(handle);
		return NULL;
	}
	lua_handle_open(&handle->lua_handle, handle->lua_handle.lvm.L);
	
	if(!strcmp(descriptor->URI, LUA_A1XC1_URI))
		handle->max_val = 1;
	else if(!strcmp(descriptor->URI, LUA_A1XC2_URI))
		handle->max_val = 2;
	else if(!strcmp(descriptor->URI, LUA_A1XC4_URI))
		handle->max_val = 4;
	else
		handle->max_val = 0; // never reached

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	Handle *handle = (Handle *)instance;

	if(port == 0)
		handle->control = (const LV2_Atom_Sequence *)data;
	else if(port == 1)
		handle->notify = (LV2_Atom_Sequence *)data;
	else if(port == 2)
		handle->event_in = (const LV2_Atom_Sequence *)data;
	else if( (port - 3) < handle->max_val)
		handle->val_out[port - 3] = (float *)data;
}

static void
activate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	lua_handle_activate(&handle->lua_handle, default_code[handle->max_val-1]);
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	Handle *handle = (Handle *)instance;
	lua_State *L = handle->lua_handle.lvm.L;

	// handle UI comm
	lua_handle_in(&handle->lua_handle, handle->control);

	// run
	if(!lua_handle_bypass(&handle->lua_handle))
	{
		int top = lua_gettop(L);
		lua_getglobal(L, "run");
		if(lua_isfunction(L, -1))
		{
			// push sequence
			lseq_t *lseq = lua_newuserdata(L, sizeof(lseq_t));
			lseq->seq = handle->event_in;
			lseq->itr = NULL;
			luaL_getmetatable(L, "lseq");
			lua_setmetatable(L, -2);

			if(lua_pcall(L, 1, LUA_MULTRET, 0))
				lua_handle_error(&handle->lua_handle);

			int ret = lua_gettop(L) - top;
			int max = ret > handle->max_val ? handle->max_val : ret; // discard superfluous returns
			for(int i=0; i<max; i++)
				*handle->val_out[i] = luaL_optnumber(L, i+1, 0.f);
			for(int i=ret; i<handle->max_val; i++) // fill in missing returns with 0.f
				*handle->val_out[i] = 0.f;

			lua_pop(L, ret);
		}
		else
			lua_pop(L, 1);

		lua_gc(L, LUA_GCSTEP, 0);
	}

	// handle UI comm
	lua_handle_out(&handle->lua_handle, handle->notify, nsamples - 1);
}

static void
deactivate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	// nothing
}

static void
cleanup(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	lua_handle_deinit(&handle->lua_handle);
	free(handle);
}

static const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_STATE__interface))
		return &lua_handle_state_iface;
	else
		return NULL;
}

const LV2_Descriptor a1xc1 = {
	.URI						= LUA_A1XC1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor a1xc2 = {
	.URI						= LUA_A1XC2_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor a1xc4 = {
	.URI						= LUA_A1XC4_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
