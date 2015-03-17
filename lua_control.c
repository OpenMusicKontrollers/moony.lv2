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
	LV2_URID_Map *map;
	struct {
		LV2_URID atom_string;
		LV2_URID lua_chunk;
		LV2_URID state_default;
	} uris;

	Lua_VM lvm;

	const float *control_in [8];
	float *control_out [8];
	
	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *notify;
	LV2_Atom_Forge forge;
};

static const char *default_chunk =
	"function run(...)"
		"return ...;"
	"end";

static LV2_State_Status
state_save(LV2_Handle instance, LV2_State_Store_Function store, LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)instance;

	return store(
		state,
		handle->uris.lua_chunk,
		handle->lvm.chunk,
		strlen(handle->lvm.chunk)+1,
		handle->uris.atom_string,
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
		handle->uris.lua_chunk,
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
	}

	// load chunk
	if(luaL_dostring(handle->lvm.L, handle->lvm.chunk))
	{
		free(handle->lvm.chunk);
		handle->lvm.chunk = strdup(default_chunk);
		if(luaL_dostring(handle->lvm.L, handle->lvm.chunk))
		{
			fprintf(stderr, "Lua: %s\n", lua_tostring(handle->lvm.L, -1));
			//return NULL; //FIXME load default chunk?
		}
	}

	return LV2_STATE_SUCCESS;
}

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	int i;
	Handle *handle = (Handle *)calloc(1, sizeof(Handle));
	if(!handle)
		return NULL;

	for(i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = (LV2_URID_Map *)features[i]->data;

	if(!handle->map)
	{
		fprintf(stderr, "%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	handle->uris.atom_string = handle->map->map(handle->map->handle, LV2_ATOM__String);
	handle->uris.lua_chunk = handle->map->map(handle->map->handle, "http://lua.org#chunk");
	handle->uris.state_default = handle->map->map(handle->map->handle, LV2_STATE__loadDefaultState);

	if(lua_vm_init(&handle->lvm))
		return NULL;
	
	// load chunk
	handle->lvm.chunk = strdup(default_chunk);
	if(luaL_dostring(handle->lvm.L, handle->lvm.chunk))
	{
		fprintf(stderr, "Lua: %s\n", lua_tostring(handle->lvm.L, -1));
		return NULL;
	}

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
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			handle->control_in[port] = (const float *)data;
			break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			handle->control_out[port-8] = (float *)data;
			break;
		case 16:
			handle->event_in = (const LV2_Atom_Sequence *)data;
			break;
		case 17:
			handle->notify = (LV2_Atom_Sequence *)data;
			break;
	}
}

static void
activate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;
	//nothing
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	Handle *handle = (Handle *)instance;
	lua_State *L = handle->lvm.L;

	// prepare notify atom forge
	uint32_t capacity = handle->notify->atom.size;
	LV2_Atom_Forge *forge = &handle->forge;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->notify, capacity);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	//TODO
	lv2_atom_forge_pop(forge, &frame);

	lua_getglobal(L, "run");
	if(lua_isfunction(L, -1))
	{
		for(int i=0; i<8; i++)
			lua_pushnumber(L, *handle->control_in[i]);

		if(lua_pcall(L, 8, 8, 0))
			fprintf(stderr, "Lua: %s\n", lua_tostring(L, -1));

		for(int i=0; i<8; i++)
			if(lua_isnumber(L, i+1))
				*handle->control_out[i] = lua_tonumber(L, i+1);
			else
				*handle->control_out[i] = 0.f;
		lua_pop(L, 8);
	}
	else
		lua_pop(L, 1);
	lua_gc(L, LUA_GCSTEP, 0);
}

static void
deactivate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;
	//nothing
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

const LV2_Descriptor lv2_lua_control = {
	.URI						= LUA_CONTROL_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
