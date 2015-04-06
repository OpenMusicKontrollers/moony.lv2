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

#include <osc.h>

typedef struct _Handle Handle;

struct _Handle {
	LV2_URID_Map *map;
	struct {
		LV2_URID lua_code;
		LV2_URID state_default;
		LV2_URID osc_OscEvent;
	} uris;

	volatile int dirty_in;
	volatile int dirty_out;
	Lua_VM lvm;

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;

	LV2_Atom_Forge forge;
};

static const char *default_code =
	"<code>function run(seq, forge)</br>"
	"</tab>for frames, atom, in seq:foreach() do</br>"
	"</tab></tab>forge:append(frames, atom)</br>"
	"</tab>end</br>"
	"end</code>";

static LV2_State_Status
state_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)instance;

	return store(
		state,
		handle->uris.lua_code,
		handle->lvm.chunk,
		strlen(handle->lvm.chunk)+1,
		handle->forge.String,
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
		handle->uris.lua_code,
		&size,
		&type,
		&flags2
	);

	//TODO check type, flags2

	if(chunk && size && type)
	{
		if(handle->lvm.chunk)
			free(handle->lvm.chunk);
		handle->lvm.chunk = strdup(chunk);

		handle->dirty_in = 1;
	}

	return LV2_STATE_SUCCESS;
}

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)calloc(1, sizeof(Handle));
	if(!handle)
		return NULL;

	for(int i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = (LV2_URID_Map *)features[i]->data;

	if(!handle->map)
	{
		fprintf(stderr, "%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	handle->uris.lua_code = handle->map->map(handle->map->handle, LUA_CODE_URI);
	handle->uris.state_default = handle->map->map(handle->map->handle, LV2_STATE__loadDefaultState);

	if(lua_vm_init(&handle->lvm))
		return NULL;
	
	lv2_atom_forge_init(&handle->forge, handle->map);

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	Handle *handle = (Handle *)instance;

	switch(port)
	{
		case 0:
			handle->control = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->notify = (LV2_Atom_Sequence *)data;
			break;
		case 2:
			handle->event_in = (const LV2_Atom_Sequence *)data;
			break;
		case 3:
			handle->event_out = (LV2_Atom_Sequence *)data;
			break;
		default:
			break;
	}
}

static void
activate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;
	
	// load default chunk
	handle->lvm.chunk = strdup(default_code);
	luaL_dostring(handle->lvm.L, handle->lvm.chunk);

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

		if(atom->type == handle->forge.String)
		{
			if(atom->size)
			{
				handle->lvm.chunk = strndup(LV2_ATOM_BODY_CONST(atom), atom->size);
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
		if(luaL_dostring(handle->lvm.L, handle->lvm.chunk))
		{
			free(handle->lvm.chunk);
			handle->lvm.chunk = strdup(default_code);
			luaL_dostring(handle->lvm.L, handle->lvm.chunk);
		}

		handle->dirty_in = 0;
	}

	// prepare notify atom forge
	uint32_t capacity = handle->notify->atom.size;
	LV2_Atom_Forge *forge = &handle->forge;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->notify, capacity);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	if(handle->dirty_out)
	{
		lv2_atom_forge_frame_time(forge, 0);
		lv2_atom_forge_string(forge, handle->lvm.chunk, strlen(handle->lvm.chunk) + 1);

		handle->dirty_out = 0;
	}
	lv2_atom_forge_pop(forge, &frame);

	// prepare event_out sequence
	capacity = handle->event_out->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->event_out, capacity);
	lv2_atom_forge_sequence_head(forge, &frame, 0);

	// run
	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		const LV2_Atom *atom = &ev->body;

		//TODO
	}

	lua_gc(L, LUA_GCSTEP, 0);

	lv2_atom_forge_pop(forge, &frame);
}

static void
deactivate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	//free(handle->lvm.chunk);
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
	const static LV2_State_Interface state_iface = {
		.save = state_save,
		.restore = state_restore
	};

	if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;
	else
		return NULL;
}

const LV2_Descriptor lv2_lua_atom = {
	.URI						= LUA_ATOM_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
