/*
 * Copyright (c) 2014 Hanspeter Portner (dev@open-music-kontrollers.ch)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
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

	if(size && type)
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

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	Handle *handle = (Handle *)instance;

	if(port < 8)
		handle->control_in[port] = (const float *)data;
	else
		handle->control_out[port-8] = (float *)data;
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
