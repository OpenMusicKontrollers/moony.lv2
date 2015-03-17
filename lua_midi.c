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

typedef struct _Handle Handle;

struct _Handle {
	LV2_URID_Map *map;
	struct {
		LV2_URID atom_string;
		LV2_URID lua_chunk;
		LV2_URID state_default;
		LV2_URID midi_MidiEvent;
	} uris;

	Lua_VM lvm;

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
	LV2_Atom_Sequence *notify;
	LV2_Atom_Forge forge;
};

static const char *default_chunk =
	"function run(frame, ...)"
		"midi(frame, ...);"
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

static int
_midi(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &handle->forge;
		
	int len = lua_gettop(L);
	if(len > 0)
	{
		LV2_Atom midiatom;
		midiatom.type = handle->uris.midi_MidiEvent;
		midiatom.size = len-1;
			
		int64_t frames = luaL_checkint(L, 1);
		lv2_atom_forge_frame_time(forge, frames);
		lv2_atom_forge_raw(forge, &midiatom, sizeof(LV2_Atom));

		for(int i=0; i<len-1; i++)
		{
			uint8_t m = luaL_checkint(L, i+2);
			lv2_atom_forge_raw(forge, &m, 1);
		}
		
		lv2_atom_forge_pad(forge, len);
	}

	return 0;
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
	handle->uris.midi_MidiEvent = handle->map->map(handle->map->handle, LV2_MIDI__MidiEvent);

	if(lua_vm_init(&handle->lvm))
		return NULL;
	
	// load chunk
	handle->lvm.chunk = strdup(default_chunk);
	if(luaL_dostring(handle->lvm.L, handle->lvm.chunk))
	{
		fprintf(stderr, "Lua: %s\n", lua_tostring(handle->lvm.L, -1));
		return NULL;
	}

	// register midi function with handle as upvalue
	lua_pushlightuserdata(handle->lvm.L, handle);
	lua_pushcclosure(handle->lvm.L, _midi, 1);
	lua_setglobal(handle->lvm.L, "midi");
	
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
			handle->event_in = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->event_out = (LV2_Atom_Sequence *)data;
			break;
		case 2:
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

	// prepare midi atom forge
	capacity = handle->event_out->atom.size;
	forge = &handle->forge;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->event_out, capacity);
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	
	LV2_Atom_Event *ev = NULL;
	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		if(ev->body.type == handle->uris.midi_MidiEvent)
		{
			int64_t frames = ev->time.frames;
			uint32_t len = ev->body.size;
			const uint8_t *buf = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Event, ev);

			lua_getglobal(L, "run");
			if(lua_isfunction(L, -1))
			{
				lua_pushnumber(L, frames);
				for(int i=0; i<len; i++)
					lua_pushnumber(L, buf[i]);
				if(lua_pcall(L, 1+len, 0, 0))
					fprintf(stderr, "Lua: %s\n", lua_tostring(L, -1));
			}
			else
				lua_pop(L, 1);
		}
	}

	lv2_atom_forge_pop(forge, &frame);
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

const LV2_Descriptor lv2_lua_midi = {
	.URI						= LUA_MIDI_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
