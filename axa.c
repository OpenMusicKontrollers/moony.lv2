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

	unsigned max_val;

	uint32_t sample_count;
	const LV2_Atom_Sequence *event_in [4];
	LV2_Atom_Sequence *event_out [4];

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;

	LV2_Atom_Forge forge [4];
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
	
	if(!strcmp(descriptor->URI, MOONY_A1XA1_URI))
		handle->max_val = 1;
	else if(!strcmp(descriptor->URI, MOONY_A2XA2_URI))
		handle->max_val = 2;
	else if(!strcmp(descriptor->URI, MOONY_A4XA4_URI))
		handle->max_val = 4;
	else
		handle->max_val = 1;

	for(unsigned i=0; i<handle->max_val; i++)
		lv2_atom_forge_init(&handle->forge[i], handle->moony.map);

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	Handle *handle = (Handle *)instance;

	if(port < handle->max_val)
		handle->event_in[port] = (const LV2_Atom_Sequence *)data;
	else if(port < handle->max_val*2)
		handle->event_out[port - handle->max_val] = (LV2_Atom_Sequence *)data;
	else if(port == handle->max_val*2)
		handle->control = (const LV2_Atom_Sequence *)data;
	else if(port == handle->max_val*2 + 1)
		handle->notify = (LV2_Atom_Sequence *)data;
}

static int
_run(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));

	lua_getglobal(L, "run");
	if(lua_isfunction(L, -1))
	{
		lua_pushinteger(L, handle->sample_count);

		// push sequence
		for(unsigned i=0; i<handle->max_val; i++)
		{
			lseq_t *lseq = moony_newuserdata(L, &handle->moony, MOONY_UDATA_ATOM);
			lseq->seq = handle->event_in[i];
			lseq->itr = NULL;
		}

		// push forge
		for(unsigned i=0; i<handle->max_val; i++)
		{
			lforge_t *lforge = moony_newuserdata(L, &handle->moony, MOONY_UDATA_FORGE);
			lforge->depth = 0;
			lforge->last.frames = 0;
			lforge->forge = &handle->forge[i];
		}
			
		lua_call(L, 1 + 2*handle->max_val, 0);
	}
	else
		lua_pop(L, 1);

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
	LV2_Atom_Forge_Frame frame [4];

	for(unsigned i=0; i<handle->max_val; i++)
	{
		uint32_t capacity = handle->event_out[i]->atom.size;
		lv2_atom_forge_set_buffer(&handle->forge[i], (uint8_t *)handle->event_out[i], capacity);
		lv2_atom_forge_sequence_head(&handle->forge[i], &frame[i], 0);
	}

	// run
	if(!moony_bypass(&handle->moony) && _try_lock(&handle->moony.lock.state))
	{
		lua_pushlightuserdata(L, handle);
		lua_pushcclosure(L, _run, 1);
		if(lua_pcall(L, 0, 0, 0))
			moony_error(&handle->moony);

		moony_freeuserdata(&handle->moony);
		lua_gc(L, LUA_GCSTEP, 0);

		_unlock(&handle->moony.lock.state);
	}

	for(unsigned i=0; i<handle->max_val; i++)
		if(&frame[i] != handle->forge[i].stack) // intercept assert
			moony_err(&handle->moony, "forge frame mismatch");

	// clear output sequence buffers upon error
	if(moony_bypass(&handle->moony))
	{
		for(unsigned i=0; i<handle->max_val; i++)
			lv2_atom_sequence_clear(handle->event_out[i]);
	}
	else
	{
		for(unsigned i=0; i<handle->max_val; i++)
			lv2_atom_forge_pop(&handle->forge[i], &frame[i]);
	}

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

const LV2_Descriptor a1xa1 = {
	.URI						= MOONY_A1XA1_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor a2xa2 = {
	.URI						= MOONY_A2XA2_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};

const LV2_Descriptor a4xa4 = {
	.URI						= MOONY_A4XA4_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= NULL,
	.run						= run,
	.deactivate			= NULL,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
