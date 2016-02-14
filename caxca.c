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

#include <lauxlib.h>

typedef struct _Handle Handle;

struct _Handle {
	moony_t moony;

	unsigned max_val;

	uint32_t sample_count;
	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
	const float *val_in [4];
	float *val_out [4];

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;
	
	LV2_Atom_Forge forge;
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)calloc(1, sizeof(Handle));
	if(!handle)
		return NULL;

	if(moony_init(&handle->moony, descriptor->URI, rate, features))
	{
		free(handle);
		return NULL;
	}
	moony_open(&handle->moony, handle->moony.vm.L, false);
	
	if(!strcmp(descriptor->URI, MOONY_C1A1XC1A1_URI))
		handle->max_val = 1;
	else if(!strcmp(descriptor->URI, MOONY_C2A1XC2A1_URI))
		handle->max_val = 2;
	else if(!strcmp(descriptor->URI, MOONY_C4A1XC4A1_URI))
		handle->max_val = 4;
	else
		handle->max_val = 1;
	
	lv2_atom_forge_init(&handle->forge, handle->moony.map);

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	Handle *handle = (Handle *)instance;

	if(port == 0)
		handle->event_in = (const LV2_Atom_Sequence *)data;
	else if(port == 1)
		handle->event_out = (LV2_Atom_Sequence *)data;
	else if( (port - 2) < handle->max_val)
		handle->val_in[port - 2] = (const float *)data;
	else if( (port - 2) < handle->max_val*2)
		handle->val_out[port - 2 - handle->max_val] = (float *)data;
	else if(port == handle->max_val*2 + 2)
		handle->control = (const LV2_Atom_Sequence *)data;
	else if(port == handle->max_val*2 + 3)
		handle->notify = (LV2_Atom_Sequence *)data;
}

static int
_run(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
		
	int top = lua_gettop(L);
	if(lua_getglobal(L, "run") != LUA_TNIL)
	{
		lua_pushinteger(L, handle->sample_count);

		// push sequence
		lseq_t *lseq = moony_newuserdata(L, &handle->moony, MOONY_UDATA_ATOM);
		lseq->seq = handle->event_in;
		lseq->itr = NULL;
		
		// push forge
		lforge_t *lforge = moony_newuserdata(L, &handle->moony, MOONY_UDATA_FORGE);
		lforge->depth = 0;
		lforge->last.frames = 0;
		lforge->forge = &handle->forge;

		// push values
		for(unsigned i=0; i<handle->max_val; i++)
			lua_pushnumber(L, *handle->val_in[i]);

		lua_call(L, 3 + handle->max_val, LUA_MULTRET);

		unsigned ret = lua_gettop(L) - top;
		unsigned max = ret > handle->max_val ? handle->max_val : ret; // discard superfluous returns
		for(unsigned i=0; i<max; i++)
			*handle->val_out[i] = luaL_optnumber(L, i+1, 0.f);
		for(unsigned i=ret; i<handle->max_val; i++) // fill in missing returns with 0.f
			*handle->val_out[i] = 0.f;
	}

	return 0;
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	Handle *handle = (Handle *)instance;
	lua_State *L = handle->moony.vm.L;

	handle->sample_count = nsamples;

	// handle UI comm
	moony_in(&handle->moony, handle->control);

	// prepare event_out sequence
	LV2_Atom_Forge_Frame frame;
	uint32_t capacity = handle->event_out->atom.size;
	lv2_atom_forge_set_buffer(&handle->forge, (uint8_t *)handle->event_out, capacity);
	lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

	// run
	if(!moony_bypass(&handle->moony) && _try_lock(&handle->moony.lock.state))
	{
		if(lua_gettop(L) != 1)
		{
			// cache for reuse
			lua_settop(L, 0);
			lua_pushlightuserdata(L, handle);
			lua_pushcclosure(L, _run, 1);
		}
		lua_pushvalue(L, 1); // _run with upvalue
		if(lua_pcall(L, 0, 0, 0))
			moony_error(&handle->moony);

		moony_freeuserdata(&handle->moony);
		lua_gc(L, LUA_GCSTEP, 0);

		_unlock(&handle->moony.lock.state);
	}

	if(&frame != handle->forge.stack) // intercept assert
		moony_err(&handle->moony, "forge frame mismatch");

	// clear output sequence buffer upon error
	if(moony_bypass(&handle->moony))
		lv2_atom_sequence_clear(handle->event_out);
	else
		lv2_atom_forge_pop(&handle->forge, &frame);

	// handle UI comm
	moony_out(&handle->moony, handle->notify, nsamples - 1);
}

static void
cleanup(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	moony_deinit(&handle->moony);
	free(handle);
}

const LV2_Descriptor c1a1xc1a1 = {
	.URI						= MOONY_C1A1XC1A1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c2a1xc2a1 = {
	.URI						= MOONY_C2A1XC2A1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor c4a1xc4a1 = {
	.URI						= MOONY_C4A1XC4A1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
