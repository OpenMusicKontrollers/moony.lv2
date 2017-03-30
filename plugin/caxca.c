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
#include <lock_stash.h>

#include <lauxlib.h>

typedef struct _Handle Handle;

struct _Handle {
	moony_t moony;
	bool once;

	unsigned max_val;

	uint32_t sample_count;
	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
	const float *val_in [4];
	float *val_out [4];

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;
	
	LV2_Atom_Forge forge;

	lock_stash_t stash [2];
	bool stashed;
	uint32_t stash_nsamples;
};

__non_realtime static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)calloc(1, sizeof(Handle));
	if(!handle)
		return NULL;
	mlock(handle, sizeof(Handle));

	if(moony_init(&handle->moony, descriptor->URI, rate, features, 0x80000, false)) // 512KB initial memory
	{
		free(handle);
		return NULL;
	}
	moony_vm_nrt_enter(handle->moony.vm);
	moony_open(&handle->moony, handle->moony.vm, moony_current(&handle->moony));
	moony_vm_nrt_leave(handle->moony.vm);
	
	if(!strcmp(descriptor->URI, MOONY_C1A1XC1A1_URI))
		handle->max_val = 1;
	else if(!strcmp(descriptor->URI, MOONY_C2A1XC2A1_URI))
		handle->max_val = 2;
	else if(!strcmp(descriptor->URI, MOONY_C4A1XC4A1_URI))
		handle->max_val = 4;
	else
		handle->max_val = 1;

	lv2_atom_forge_init(&handle->forge, handle->moony.map);
	
	for(unsigned i=0; i<2; i++)
	{
		_stash_init(&handle->stash[i], handle->moony.map);
		_stash_reset(&handle->stash[i]);
	}

	return handle;
}

__realtime static void
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

__realtime static inline void
_run_period(lua_State *L, const char *cmd, Handle *handle, uint32_t nsamples,
	const LV2_Atom_Sequence *event_in, const LV2_Atom_Sequence *control)
{
	int top = lua_gettop(L);
	if(lua_getglobal(L, cmd) != LUA_TNIL)
	{
		lua_pushinteger(L, nsamples);

		// push control / notify pair
		{
			latom_t *latom = moony_newuserdata(L, &handle->moony, MOONY_UDATA_ATOM, true);
			latom->atom = (const LV2_Atom *)control;
			latom->body.raw = LV2_ATOM_BODY_CONST(latom->atom);

			lforge_t *lforge = moony_newuserdata(L, &handle->moony, MOONY_UDATA_FORGE, true);
			lforge->depth = 0;
			lforge->last.frames = 0;
			lforge->forge = &handle->moony.notify_forge;
		}

		// push sequence / forge pair
		{
			latom_t *latom = moony_newuserdata(L, &handle->moony, MOONY_UDATA_ATOM, true);
			latom->atom = (const LV2_Atom *)event_in;
			latom->body.raw = LV2_ATOM_BODY_CONST(latom->atom);

			lforge_t *lforge = moony_newuserdata(L, &handle->moony, MOONY_UDATA_FORGE, true);
			lforge->depth = 0;
			lforge->last.frames = 0;
			lforge->forge = &handle->forge;
		}

		// push values
		for(unsigned i=0; i<handle->max_val; i++)
			lua_pushnumber(L, *handle->val_in[i]);

		lua_call(L, 3 + handle->max_val + 2, LUA_MULTRET);

		unsigned ret = lua_gettop(L) - top;
		unsigned max = ret > handle->max_val ? handle->max_val : ret; // discard superfluous returns
		for(unsigned i=0; i<max; i++)
			*handle->val_out[i] = luaL_optnumber(L, i+1, 0.f);
		for(unsigned i=ret; i<handle->max_val; i++) // fill in missing returns with 0.f
			*handle->val_out[i] = 0.f;
	}
}

__realtime static int
_run(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));

	// apply stash, if any
	if(handle->stashed)
	{
		const LV2_Atom_Sequence *event_in = &handle->stash[0].seq;
		const LV2_Atom_Sequence *control = &handle->stash[1].seq;

		_run_period(L, "run", handle, handle->stash_nsamples, event_in, control);

		LV2_ATOM_SEQUENCE_FOREACH(handle->event_out, ev)
			ev->time.frames = 0; // overwrite time stamps
		LV2_ATOM_SEQUENCE_FOREACH(handle->notify, ev)
			ev->time.frames = 0; // overwrite time stamps
	}

	if(handle->once)
	{
		_run_period(L, "once", handle, handle->sample_count, handle->event_in, handle->control);
		handle->once = false;
	}

	_run_period(L, "run", handle, handle->sample_count, handle->event_in, handle->control);

	return 0;
}

__realtime static void
run(LV2_Handle instance, uint32_t nsamples)
{
	Handle *handle = (Handle *)instance;

	handle->sample_count = nsamples;

	// prepare event_out sequence
	LV2_Atom_Forge_Frame frame;
	uint32_t capacity = handle->event_out->atom.size;
	lv2_atom_forge_set_buffer(&handle->forge, (uint8_t *)handle->event_out, capacity);
	lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

	moony_pre(&handle->moony, handle->notify);

	if(_try_lock(&handle->moony.lock.state))
	{
		// apply stash, if any
		if(handle->stashed)
		{
			lock_stash_t *stash = &handle->stash[1];
			handle->once = moony_in(&handle->moony, &stash->seq, handle->notify);

			LV2_ATOM_SEQUENCE_FOREACH(handle->notify, ev)
				ev->time.frames = 0; // overwrite time stamps
		}

		// handle UI comm
		handle->once = moony_in(&handle->moony, handle->control, handle->notify)
			|| handle->once;

		// run
		if(!moony_bypass(&handle->moony))
		{
			lua_State *L = moony_current(&handle->moony);

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
#ifdef USE_MANUAL_GC
			lua_gc(L, LUA_GCSTEP, 0);
#endif
		}

		if(handle->stashed)
		{
			for(unsigned i=0; i<2; i++)
				_stash_reset(&handle->stash[i]);

			handle->stash_nsamples = 0;
			handle->stashed = false;
		}

		_unlock(&handle->moony.lock.state);
	}
	else
	{
		// stash incoming events to later apply

		lock_stash_t *stash = &handle->stash[0];
		LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
		{
			if(stash->ref)
				stash->ref = lv2_atom_forge_frame_time(&stash->forge, handle->stash_nsamples + ev->time.frames);
			if(stash->ref)
				stash->ref = lv2_atom_forge_write(&stash->forge, &ev->body, lv2_atom_total_size(&ev->body));
		}

		stash = &handle->stash[1];
		LV2_ATOM_SEQUENCE_FOREACH(handle->control, ev)
		{
			if(stash->ref)
				stash->ref = lv2_atom_forge_frame_time(&stash->forge, handle->stash_nsamples + ev->time.frames);
			if(stash->ref)
				stash->ref = lv2_atom_forge_write(&stash->forge, &ev->body, lv2_atom_total_size(&ev->body));
		}

		handle->stash_nsamples += nsamples;
		handle->stashed = true;
	}

	if(&frame != handle->forge.stack) // intercept assert
		moony_err(&handle->moony, "forge frame mismatch");

	// clear output sequence buffer upon error
	if(moony_bypass(&handle->moony))
		lv2_atom_sequence_clear(handle->event_out);
	else
		lv2_atom_forge_pop(&handle->forge, &frame);

	// clear output ports upon error
	if(moony_bypass(&handle->moony))
		for(unsigned i=0; i<handle->max_val; i++)
			*handle->val_out[i] = 0.f;

	// handle UI comm
	moony_out(&handle->moony, handle->notify, nsamples - 1);
}

__non_realtime static void
cleanup(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	moony_deinit(&handle->moony);
	munlock(handle, sizeof(Handle));
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
