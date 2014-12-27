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

#include <osc.h>

typedef struct _Handle Handle;

struct _Handle {
	LV2_URID_Map *map;
	struct {
		LV2_URID atom_string;
		LV2_URID lua_chunk;
		LV2_URID state_default;
		LV2_URID osc_OscEvent;
	} uris;

	Lua_VM lvm;

	const LV2_Atom_Sequence *osc_in;
	LV2_Atom_String *osc_out;
	LV2_Atom_Forge forge;
};

static const char *default_chunk =
	"function run(frame, path, fmt, ...)"
		"osc(frame, path, fmt, ...);"
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
_osc(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &handle->forge;
	osc_data_t buf[1024]; // TODO how big?
	osc_data_t *ptr = buf;
	osc_data_t *end = buf+1024;
		
	int len = lua_gettop(L);
	if(len > 2)
	{
		int64_t frames = luaL_checkint(L, 1);
		const char *path = luaL_checkstring(L, 2);
		const char *fmt = luaL_checkstring(L, 3);

		ptr = osc_set_path(ptr, end, path);
		ptr = osc_set_fmt(ptr, end, fmt);

		if(len-3 <= strlen(fmt)) // I,N,T,F have no arguments
		{
			int pos = 4;
			for(const char *type=fmt; *type; type++)
				switch(*type)
				{
					case OSC_INT32:
					{
						int32_t i = luaL_checkint(L, pos++);
						ptr = osc_set_int32(ptr, end, i);
						break;
					}
					case OSC_FLOAT:
					{
						float f = luaL_checknumber(L, pos++);
						ptr = osc_set_float(ptr, end, f);
						break;
					}
					case OSC_STRING:
					case OSC_SYMBOL:
					{
						const char *s = luaL_checkstring(L, pos++);
						ptr = osc_set_string(ptr, end, s);
						break;
					}
					case OSC_INT64:
					{
						int64_t h = luaL_checkint(L, pos++);
						ptr = osc_set_int64(ptr, end, h);
						break;
					}
					case OSC_TIMETAG:
					{
						osc_time_t t = luaL_checkint(L, pos++);
						ptr = osc_set_timetag(ptr, end, t);
						break;
					}
					case OSC_DOUBLE:
					{
						double d = luaL_checknumber(L, pos++);
						ptr = osc_set_double(ptr, end, d);
						break;
					}
					case OSC_MIDI:
					{
						uint8_t m[4];
						for(int i=0; i<4; i++)
						{
							lua_rawgeti(L, pos, i+1);
							m[i] = luaL_checkint(L, -1);
							lua_pop(L, 1);
						}
						pos++;
						ptr = osc_set_midi(ptr, end, m);
						break;
					}
					case OSC_CHAR:
					{
						char c = luaL_checkint(L, pos++);
						ptr = osc_set_char(ptr, end, c);
						break;
					}
					case OSC_BLOB:
						//FIXME
						break;
					case OSC_NIL:
					case OSC_BANG:
					case OSC_TRUE:
					case OSC_FALSE:
						break;
					default:
						break;
				}
	
			size_t size;
			if(ptr && osc_check_packet(buf, size=ptr-buf))
			{
				LV2_Atom oscatom;
				oscatom.type = handle->uris.osc_OscEvent;
				oscatom.size = size;
				lv2_atom_forge_frame_time(forge, frames);
				lv2_atom_forge_raw(forge, &oscatom, sizeof(LV2_Atom));
				lv2_atom_forge_raw(forge, buf, size);
				lv2_atom_forge_pad(forge, size);
			}
		}
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
	handle->uris.osc_OscEvent = handle->map->map(handle->map->handle, "http://opensoundcontrol.org#OscEvent");

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
	lua_pushcclosure(handle->lvm.L, _osc, 1);
	lua_setglobal(handle->lvm.L, "osc");
	
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
			handle->osc_in = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->osc_out = (LV2_Atom_String *)data;
			break;
		default:
			break;
	}
}

static void
activate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;
	//nothing
}

static int
_through(osc_time_t time, const char *path, const char *fmt, osc_data_t *arg, size_t size, void *data)
{
	Handle *handle = data;
	lua_State *L = handle->lvm.L;

	lua_getglobal(L, "run");
	if(lua_isfunction(L, -1))
	{
		osc_data_t *ptr = arg;
		lua_pushnumber(L, time);
		lua_pushstring(L, path);
		lua_pushstring(L, fmt);
		for(const char *type=fmt; *type; type++)
			switch(*type)
			{
				case OSC_INT32:
				{
					int32_t i;
					ptr = osc_get_int32(ptr, &i);
					lua_pushnumber(L, i);
					break;
				}
				case OSC_FLOAT:
				{
					float f;
					ptr = osc_get_float(ptr, &f);
					lua_pushnumber(L, f);
					break;
				}
				case OSC_STRING:
				case OSC_SYMBOL:
				{
					const char *s;
					ptr = osc_get_string(ptr, &s);
					lua_pushstring(L, s);
					break;
				}
				case OSC_INT64:
				{
					int64_t h;
					ptr = osc_get_int64(ptr, &h);
					lua_pushnumber(L, h);
					break;
				}
				case OSC_DOUBLE:
				{
					double d;
					ptr = osc_get_double(ptr, &d);
					lua_pushnumber(L, d);
					break;
				}
				case OSC_TIMETAG:
				{
					osc_time_t t;
					ptr = osc_get_timetag(ptr, &t);
					lua_pushnumber(L, t);
					break;
				}
				case OSC_CHAR:
				{
					char c;
					ptr = osc_get_char(ptr, &c);
					lua_pushnumber(L, c);
					break;
				}
				case OSC_MIDI:
				{
					uint8_t *m;
					ptr = osc_get_midi(ptr, &m);
					lua_createtable(L, 4, 0);
					for(int i=0; i<4; i++)
					{
						lua_pushnumber(L, m[i]);
						lua_rawseti(L, -2, i+1);
					}
					break;
				}
				case OSC_BLOB:
					//FIXME
					break;
				case OSC_NIL:
				case OSC_BANG:
				case OSC_TRUE:
				case OSC_FALSE:
					break;
				default:
					break;
			}
		if(lua_pcall(L, 3+strlen(fmt), 0, 0))
			fprintf(stderr, "Lua: %s\n", lua_tostring(L, -1));
	}
	else
		lua_pop(L, 1);

	return 1;
}

static const osc_method_t methods [] = {
	{NULL, NULL, _through},
	{NULL, NULL, NULL}
};

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	Handle *handle = (Handle *)instance;
	lua_State *L = handle->lvm.L;

	// prepare osc atom forge
	const uint32_t capacity = handle->osc_out->atom.size;
	LV2_Atom_Forge *forge = &handle->forge;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->osc_out, capacity);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	
	LV2_Atom_Event *ev = NULL;
	LV2_ATOM_SEQUENCE_FOREACH(handle->osc_in, ev)
	{
		if(ev->body.type == handle->uris.osc_OscEvent)
		{
			osc_time_t frames = ev->time.frames;
			size_t len = ev->body.size;
			const osc_data_t *buf = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Event, ev);

			if(osc_check_packet((osc_data_t *)buf, len))
				osc_dispatch_method(frames, (osc_data_t *)buf, len, (osc_method_t *)methods, NULL, NULL, handle);
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

const LV2_Descriptor lv2_lua_osc = {
	.URI						= LUA_OSC_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
