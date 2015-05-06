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

	const float *val_in [4];
	LV2_Atom_Sequence *event_out;

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;

	LV2_Atom_Forge forge;
};

static const char *default_code [4] = {
	"function run(forge, a)\n"
	"  -- your code here\n"
	"end",

	"function run(forge, a, b)\n"
	"  -- your code here\n"
	"end",

	NULL,

	"function run(forge, a, b, c, d)\n"
	"  -- your code here\n"
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
	
	if(!strcmp(descriptor->URI, LUA_C1XA1_URI))
		handle->max_val = 1;
	else if(!strcmp(descriptor->URI, LUA_C2XA1_URI))
		handle->max_val = 2;
	else if(!strcmp(descriptor->URI, LUA_C4XA1_URI))
		handle->max_val = 4;
	else
		handle->max_val = 0; // never reached

	lv2_atom_forge_init(&handle->forge, handle->lua_handle.map);

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
		handle->event_out = (LV2_Atom_Sequence *)data;
	else if( (port - 3) < handle->max_val)
		handle->val_in[port - 3] = (const float *)data;
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

	// prepare event_out sequence
	LV2_Atom_Forge_Frame frame;
	uint32_t capacity = handle->event_out->atom.size;
	lv2_atom_forge_set_buffer(&handle->forge, (uint8_t *)handle->event_out, capacity);
	lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

	// run
	if(!lua_handle_bypass(&handle->lua_handle))
	{
		lua_getglobal(L, "run");
		if(lua_isfunction(L, -1))
		{
			// push forge
			lforge_t *lforge = lua_newuserdata(L, sizeof(lforge_t));
			lforge->forge = &handle->forge;
			luaL_getmetatable(L, "lforge");
			lua_setmetatable(L, -2);

			// push values
			for(int i=0; i<handle->max_val; i++)
				lua_pushnumber(L, *handle->val_in[i]);
				
			if(lua_pcall(L, 1 + handle->max_val, 0, 0))
				lua_handle_error(&handle->lua_handle);
		}
		else
			lua_pop(L, 1);

		lua_gc(L, LUA_GCSTEP, 0);
	}

	lv2_atom_forge_pop(&handle->forge, &frame);

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

const LV2_Descriptor c1xa1 = {
	.URI						= LUA_C1XA1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c2xa1 = {
	.URI						= LUA_C2XA1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c4xa1 = {
	.URI						= LUA_C4XA1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
