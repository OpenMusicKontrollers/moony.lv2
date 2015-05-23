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

#include <lauxlib.h>

typedef struct _Handle Handle;

struct _Handle {
	moony_t moony;

	int max_val;

	const float *val_in [4];
	float *val_out [4];
	
	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;
};

static const char *default_code [3] = {
	"function run(a)\n"
	"  -- your code here\n"
	"  return a\n"
	"end",

	"function run(a, b)\n"
	"  -- your code here\n"
	"  return a, b\n"
	"end",

	"function run(a, b, c, d)\n"
	"  -- your code here\n"
	"  return a, b, c, d\n"
	"end"
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)calloc(1, sizeof(Handle));
	if(!handle)
		return NULL;

	if(moony_init(&handle->moony, features))
	{
		free(handle);
		return NULL;
	}
	moony_open(&handle->moony, handle->moony.vm.L);
	
	if(!strcmp(descriptor->URI, MOONY_C1XC1_URI))
		handle->max_val = 1;
	else if(!strcmp(descriptor->URI, MOONY_C2XC2_URI))
		handle->max_val = 2;
	else if(!strcmp(descriptor->URI, MOONY_C4XC4_URI))
		handle->max_val = 4;
	else
		handle->max_val = 1;
	moony_activate(&handle->moony, default_code[handle->max_val >> 1]);

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	Handle *handle = (Handle *)instance;

	if(port < handle->max_val)
		handle->val_in[port] = (const float *)data;
	else if(port < handle->max_val*2)
		handle->val_out[port - handle->max_val] = (float *)data;
	else if(port == handle->max_val*2)
		handle->control = (const LV2_Atom_Sequence *)data;
	else if(port == handle->max_val*2 + 1)
		handle->notify = (LV2_Atom_Sequence *)data;
}

static void
activate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	// nothing
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	Handle *handle = (Handle *)instance;
	lua_State *L = handle->moony.vm.L;

	// handle UI comm
	moony_in(&handle->moony, handle->control);

	// run
	if(!moony_bypass(&handle->moony))
	{
		int top = lua_gettop(L);
		lua_getglobal(L, "run");
		if(lua_isfunction(L, -1))
		{
			for(int i=0; i<handle->max_val; i++)
				lua_pushnumber(L, *handle->val_in[i]);

			if(lua_pcall(L, handle->max_val, LUA_MULTRET, 0))
				moony_error(&handle->moony);

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

		//moony_freeuserdata(&handle->moony);
		lua_gc(L, LUA_GCSTEP, 0);
	}

	// handle UI comm
	moony_out(&handle->moony, handle->notify, nsamples - 1);
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

	moony_deinit(&handle->moony);
	free(handle);
}

const LV2_Descriptor c1xc1 = {
	.URI						= MOONY_C1XC1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c2xc2 = {
	.URI						= MOONY_C2XC2_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c4xc4 = {
	.URI						= MOONY_C4XC4_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
