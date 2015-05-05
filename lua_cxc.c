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
	lua_atom_t lua_atom;

	Lua_VM lvm;
	char chunk [MAX_CHUNK_LEN];
	volatile int dirty_in;
	volatile int dirty_out;
	volatile int error_out;
	char error [MAX_ERROR_LEN];

	int max_val;

	const float *val_in [4];
	float *val_out [4];
	
	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;
};

static const char *default_code [4] = {
	"function run(a)\n"
	"  -- your code here\n"
	"  return a\n"
	"end",

	"function run(a, b)\n"
	"  -- your code here\n"
	"  return a, b\n"
	"end",

	NULL,

	"function run(a, b, c, d)\n"
	"  -- your code here\n"
	"  return a, b, c, d\n"
	"end"
};

static LV2_State_Status
state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)instance;

	return store(
		state,
		handle->lua_atom.uris.lua_code,
		handle->chunk,
		strlen(handle->chunk)+1,
		handle->lua_atom.forge.String,
		LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}

static LV2_State_Status
state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)instance;

	size_t size;
	uint32_t type;
	uint32_t flags2;
	const char *chunk = retrieve(
		state,
		handle->lua_atom.uris.lua_code,
		&size,
		&type,
		&flags2
	);

	//TODO check type, flags2

	if(chunk && size && type)
	{
		strncpy(handle->chunk, chunk, size);
		handle->dirty_in = 1;
	}

	return LV2_STATE_SUCCESS;
}

static const LV2_State_Interface state_iface = {
	.save = state_save,
	.restore = state_restore
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)calloc(1, sizeof(Handle));
	if(!handle)
		return NULL;

	if(  lua_vm_init(&handle->lvm)
		|| lua_atom_init(&handle->lua_atom, features) )
	{
		free(handle);
		return NULL;
	}
	
	if(!strcmp(descriptor->URI, LUA_C1XC1_URI))
		handle->max_val = 1;
	else if(!strcmp(descriptor->URI, LUA_C2XC2_URI))
		handle->max_val = 2;
	else if(!strcmp(descriptor->URI, LUA_C4XC4_URI))
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
	else if( (port - 2) < handle->max_val)
		handle->val_in[port - 2] = (const float *)data;
	else if( (port - 2) < handle->max_val*2)
		handle->val_out[port - 2 - handle->max_val] = (float *)data;
}

static void
activate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;
	
	// load default chunk
	strcpy(handle->chunk, default_code[handle->max_val-1]);
	luaL_dostring(handle->lvm.L, handle->chunk); // cannot fail

	handle->dirty_out = 1; // trigger update of UI
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	Handle *handle = (Handle *)instance;
	lua_State *L = handle->lvm.L;

	// read control atom port
	LV2_ATOM_SEQUENCE_FOREACH(handle->control, ev)
	{
		const LV2_Atom *atom = &ev->body;

		if(atom->type == handle->lua_atom.forge.String)
		{
			if(atom->size)
			{
				strncpy(handle->chunk, LV2_ATOM_BODY_CONST(atom), atom->size);
				handle->dirty_in = 1;
			}
			else
				handle->dirty_out = 1;
		}
	}

	// handle dirty state
	if(handle->dirty_in)
	{
		// load chunk
		if(luaL_dostring(handle->lvm.L, handle->chunk))
		{
			strcpy(handle->error, lua_tostring(L, -1));
			lua_pop(L, 1);

			handle->error_out = 1;
		}
		else
			handle->error[0] = 0x0; // reset error flag

		handle->dirty_in = 0;
	}

	// run
	if(handle->error[0] == 0x0) // bypass if errored
	{
		int top = lua_gettop(L);
		lua_getglobal(L, "run");
		if(lua_isfunction(L, -1))
		{
			for(int i=0; i<handle->max_val; i++)
				lua_pushnumber(L, *handle->val_in[i]);

			if(lua_pcall(L, handle->max_val, LUA_MULTRET, 0))
			{
				strcpy(handle->error, lua_tostring(L, -1));
				lua_pop(L, 1);

				handle->error_out = 1;
			}

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

	// prepare notify atom forge
	LV2_Atom_Forge *forge = &handle->lua_atom.forge;
	LV2_Atom_Forge_Frame notify_frame;
	
	uint32_t capacity = handle->notify->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->notify, capacity);
	lv2_atom_forge_sequence_head(forge, &notify_frame, 0);
	if(handle->dirty_out)
	{
		uint32_t len = strlen(handle->chunk) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, 0);
		lv2_atom_forge_object(forge, &obj_frame, 0, handle->lua_atom.uris.lua_message);
		lv2_atom_forge_key(forge, handle->lua_atom.uris.lua_code);
		lv2_atom_forge_string(forge, handle->chunk, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		handle->dirty_out = 0; // reset flag
	}
	if(handle->error_out)
	{
		uint32_t len = strlen(handle->error) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, 0);
		lv2_atom_forge_object(forge, &obj_frame, 0, handle->lua_atom.uris.lua_message);
		lv2_atom_forge_key(forge, handle->lua_atom.uris.lua_error);
		lv2_atom_forge_string(forge, handle->error, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		handle->error_out = 0; // reset flag
	}
	lv2_atom_forge_pop(forge, &notify_frame);
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

	lua_vm_deinit(&handle->lvm);
	free(handle);
}

static const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;
	else
		return NULL;
}

const LV2_Descriptor c1xc1 = {
	.URI						= LUA_C1XC1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c2xc2 = {
	.URI						= LUA_C2XC2_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c4xc4 = {
	.URI						= LUA_C4XC4_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
