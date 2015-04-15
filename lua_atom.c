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

typedef struct _lseq_t lseq_t;
typedef struct _lobj_t lobj_t;
typedef struct _ltuple_t ltuple_t;
typedef struct _lforge_t lforge_t;
typedef struct _lframe_t lframe_t;
typedef struct _latom_t latom_t;
typedef struct _Handle Handle;

struct _lseq_t {
	const LV2_Atom_Sequence *seq;
	const LV2_Atom_Event *itr;
};

struct _lobj_t {
	const LV2_Atom_Object *obj;
	const LV2_Atom_Property_Body *itr;
};

struct _ltuple_t {
	const LV2_Atom_Tuple *tuple;
	const LV2_Atom *itr;
};

struct _lforge_t {
	LV2_Atom_Forge *forge;
};

struct _lframe_t {
	LV2_Atom_Forge_Frame frame;
};

struct _latom_t {
	const LV2_Atom *atom;
};

struct _Handle {
	LV2_URID_Map *map;
	struct {
		LV2_URID lua_message;
		LV2_URID lua_code;
		LV2_URID lua_error;
	} uris;

	Lua_VM lvm;
	char *chunk;
	volatile int dirty_in;
	volatile int dirty_out;
	char error [1024];

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;

	LV2_Atom_Forge forge;
};

static const char *default_code =
	"function run(seq, forge)\n"
	"  for frames, atom in seq:foreach() do\n"
	"    -- your code here\n"
	"  end\n"
	"end";

static LV2_State_Status
state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	Handle *handle = (Handle *)instance;

	return store(
		state,
		handle->uris.lua_code,
		handle->chunk,
		strlen(handle->chunk)+1,
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
		if(handle->chunk)
			free(handle->chunk);
		handle->chunk = strdup(chunk);

		handle->dirty_in = 1;
	}

	return LV2_STATE_SUCCESS;
}

static const LV2_State_Interface state_iface = {
	.save = state_save,
	.restore = state_restore
};

static void
_latom_new(lua_State *L, const LV2_Atom *atom)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &handle->forge;

	if(atom->type == forge->Object)
	{
		lobj_t *lobj = lua_newuserdata(L, sizeof(lobj_t));
		lobj->obj = (const LV2_Atom_Object *)atom;
		luaL_getmetatable(L, "lobj");
	}
	else if(atom->type == forge->Tuple)
	{
		ltuple_t *ltuple = lua_newuserdata(L, sizeof(ltuple_t));
		ltuple->tuple = (const LV2_Atom_Tuple *)atom;
		luaL_getmetatable(L, "ltuple");
	}
	else
	{
		latom_t *latom = lua_newuserdata(L, sizeof(latom_t));
		latom->atom = atom;
		luaL_getmetatable(L, "latom");
	}

	lua_setmetatable(L, -2);
}

static int
_lseq_foreach_itr(lua_State *L)
{
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	if(!lv2_atom_sequence_is_end(&lseq->seq->body, lseq->seq->atom.size, lseq->itr))
	{
		// push frame time
		lua_pushinteger(L, lseq->itr->time.frames);
		// push atom
		_latom_new(L, &lseq->itr->body);
	
		// advance iterator
		lseq->itr = lv2_atom_sequence_next(lseq->itr);

		return 2;
	}

	// end of sequence reached
	lua_pushnil(L);
	return 1;
}

static int
_lseq__index(lua_State *L)
{
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");
	const char *key = luaL_checkstring(L, 2);
	//TODO allow indexing by index

	if(!strcmp(key, "type"))
	{
		lua_pushinteger(L, lseq->seq->atom.type);
	}
	else // look in metatable
	{
		lua_getmetatable(L, 1);
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
	}

	return 1;
}

static int
_lseq_foreach(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
	lseq_t *lseq = luaL_checkudata(L, 1, "lseq");

	// reset iterator to beginning of sequence
	lseq->itr = lv2_atom_sequence_begin(&lseq->seq->body);

	lua_pushlightuserdata(L, handle);
	lua_pushcclosure(L, _lseq_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const luaL_Reg lseq_mt [] = {
	{"__index", _lseq__index},
	{"foreach", _lseq_foreach},
	{NULL, NULL}
};

static int
_ltuple_foreach_itr(lua_State *L)
{
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	if(!lv2_atom_tuple_is_end(LV2_ATOM_BODY(ltuple->tuple), ltuple->tuple->atom.size, ltuple->itr))
	{
		// push atom
		_latom_new(L, ltuple->itr);
	
		// advance iterator
		ltuple->itr = lv2_atom_tuple_next(ltuple->itr);

		return 1;
	}

	// end of sequence reached
	lua_pushnil(L);
	return 1;
}

static int
_ltuple__index(lua_State *L)
{
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");
	const char *key = luaL_checkstring(L, 2);
	//TODO allow indexing by index

	if(!strcmp(key, "type"))
	{
		lua_pushinteger(L, ltuple->tuple->atom.type);
	}
	else // look in metatable
	{
		lua_getmetatable(L, 1);
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
	}

	return 1;
}

static int
_ltuple_foreach(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
	ltuple_t *ltuple = luaL_checkudata(L, 1, "ltuple");

	// reset iterator to beginning of tuple
	ltuple->itr = lv2_atom_tuple_begin(ltuple->tuple);

	lua_pushlightuserdata(L, handle);
	lua_pushcclosure(L, _ltuple_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const luaL_Reg ltuple_mt [] = {
	{"__index", _ltuple__index},
	{"foreach", _ltuple_foreach},
	{NULL, NULL}
};

static int
_lobj_foreach_itr(lua_State *L)
{
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	if(!lv2_atom_object_is_end(&lobj->obj->body, lobj->obj->atom.size, lobj->itr))
	{
		// push atom
		lua_pushinteger(L, lobj->itr->key);
		_latom_new(L, &lobj->itr->value);
	
		// advance iterator
		lobj->itr = lv2_atom_object_next(lobj->itr);

		return 2;
	}

	// end of sequence reached
	lua_pushnil(L);
	return 1;
}

static int
_lobj__index(lua_State *L)
{
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");
	const char *key = luaL_checkstring(L, 2);
	//TODO allow indexing by keys

	if(!strcmp(key, "type"))
	{
		lua_pushinteger(L, lobj->obj->atom.type);
	}
	else // look in metatable
	{
		lua_getmetatable(L, 1);
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
	}

	return 1;
}

static int
_lobj_foreach(lua_State *L)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
	lobj_t *lobj = luaL_checkudata(L, 1, "lobj");

	// reset iterator to beginning of tuple
	lobj->itr = lv2_atom_object_begin(&lobj->obj->body);

	lua_pushlightuserdata(L, handle);
	lua_pushcclosure(L, _lobj_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const luaL_Reg lobj_mt [] = {
	{"__idnex", _lobj__index},
	{"foreach", _lobj_foreach},
	{NULL, NULL}
};

static int
_lforge_frame_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int64_t val = luaL_checkinteger(L, 2);

	lv2_atom_forge_frame_time(lforge->forge, val);

	return 0;
}

static int
_lforge_int(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int32_t val = luaL_checkinteger(L, 2);

	lv2_atom_forge_int(lforge->forge, val);

	return 0;
}

static int
_lforge_long(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int64_t val = luaL_checkinteger(L, 2);

	lv2_atom_forge_long(lforge->forge, val);

	return 0;
}

static int
_lforge_float(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	float val = luaL_checkinteger(L, 2);

	lv2_atom_forge_float(lforge->forge, val);

	return 0;
}

static int
_lforge_double(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	double val = luaL_checkinteger(L, 2);

	lv2_atom_forge_double(lforge->forge, val);

	return 0;
}

static int
_lforge_bool(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	uint32_t val = luaL_checkinteger(L, 2);

	lv2_atom_forge_bool(lforge->forge, val);

	return 0;
}

static int
_lforge_urid(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID val = luaL_checkinteger(L, 2);

	lv2_atom_forge_urid(lforge->forge, val);

	return 0;
}

static int
_lforge_string(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	lv2_atom_forge_string(lforge->forge, val, size);

	return 0;
}

static int
_lforge_uri(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	lv2_atom_forge_uri(lforge->forge, val, size);

	return 0;
}

static int
_lforge_path(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);

	lv2_atom_forge_path(lforge->forge, val, size);

	return 0;
}

static int
_lforge_tuple(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lframe_t *lframe = lua_newuserdata(L, sizeof(lframe_t));
	luaL_getmetatable(L, "lframe");
	lua_setmetatable(L, -2);

	lv2_atom_forge_tuple(lforge->forge, &lframe->frame);

	return 1;
}

static int
_lforge_object(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID id = luaL_checkinteger(L, 2);
	LV2_URID otype = luaL_checkinteger(L, 3);
	lframe_t *lframe = lua_newuserdata(L, sizeof(lframe_t));
	luaL_getmetatable(L, "lframe");
	lua_setmetatable(L, -2);

	lv2_atom_forge_object(lforge->forge, &lframe->frame, 0, otype);

	return 1;
}

static int
_lforge_key(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);

	lv2_atom_forge_key(lforge->forge, key);

	return 1;
}

static int
_lforge_pop(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lframe_t *lframe = luaL_checkudata(L, 2, "lframe");

	lv2_atom_forge_pop(lforge->forge, &lframe->frame);

	return 0;
}

static const luaL_Reg lframe_mt [] = {
	{NULL, NULL}
};

static const luaL_Reg lforge_mt [] = {
	{"frame_time", _lforge_frame_time},

	{"int", _lforge_int},
	{"long", _lforge_long},
	{"float", _lforge_float},
	{"double", _lforge_double},
	{"bool", _lforge_bool},
	{"urid", _lforge_urid},
	{"string", _lforge_string},
	{"uri", _lforge_uri},
	{"path", _lforge_path},

	//TODO vector
	{"tuple", _lforge_tuple},
	{"object", _lforge_object},
	{"key", _lforge_key},

	{"pop", _lforge_pop},
	{NULL, NULL}
};

static void
_latom_value(lua_State *L, const LV2_Atom *atom)
{
	Handle *handle = lua_touserdata(L, lua_upvalueindex(1));
	LV2_Atom_Forge *forge = &handle->forge;

	if(atom->type == forge->Int)
		lua_pushinteger(L, ((const LV2_Atom_Int *)atom)->body);
	else if(atom->type == forge->Long)
		lua_pushinteger(L, ((const LV2_Atom_Long *)atom)->body);
	else if(atom->type == forge->Float)
		lua_pushnumber(L, ((const LV2_Atom_Float *)atom)->body);
	else if(atom->type == forge->Double)
		lua_pushnumber(L, ((const LV2_Atom_Double *)atom)->body);
	else if(atom->type == forge->Bool)
		lua_pushinteger(L, ((const LV2_Atom_Bool *)atom)->body);
	else if(atom->type == forge->URID)
		lua_pushinteger(L, ((const LV2_Atom_URID *)atom)->body);
	else if(atom->type == forge->String)
		lua_pushstring(L, LV2_ATOM_BODY_CONST(atom));
	else if(atom->type == forge->URI)
		lua_pushstring(L, LV2_ATOM_BODY_CONST(atom));
	else if(atom->type == forge->Path)
		lua_pushstring(L, LV2_ATOM_BODY_CONST(atom));
	else
		lua_pushnil(L);
}

static int
_latom__index(lua_State *L)
{
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const char *key = lua_tostring(L, 2);

	if(!strcmp(key, "type"))
		lua_pushinteger(L, latom->atom->type);
	else if(!strcmp(key, "size"))
		lua_pushinteger(L, latom->atom->size);
	else if(!strcmp(key, "value"))
		_latom_value(L, latom->atom);
	else // look in metatable
	{
		lua_getmetatable(L, 1);
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
	}

	return 1;
}

static int
_latom__len(lua_State *L)
{
	latom_t *latom = luaL_checkudata(L, 1, "latom");

	lua_pushinteger(L, latom->atom->size);

	return 1;
}

static int
_latom__tostring(lua_State *L)
{
	latom_t *latom = luaL_checkudata(L, 1, "latom");

	_latom_value(L, latom->atom);
	if(!lua_isstring(L, -1))
		lua_tostring(L, -1);

	return 1;
}

static int
_latom__concat(lua_State *L)
{
	latom_t *latom;
	if((latom = luaL_testudata(L, 1, "latom")))
	{
		_latom_value(L, latom->atom);
		if(!lua_isstring(L, -1))
			lua_tostring(L, -1);
	}
	else
		lua_pushvalue(L, 1);
	
	if((latom = luaL_testudata(L, 2, "latom")))
	{
		_latom_value(L, latom->atom);
		if(!lua_isstring(L, -1))
			lua_tostring(L, -1);
	}
	else
		lua_pushvalue(L, 2);

	lua_concat(L, 2);

	return 1;
}

static const luaL_Reg latom_mt [] = {
	{"__index", _latom__index},
	{"__len", _latom__len},
	{"__tostring", _latom__tostring},
	{"__concat", _latom__concat},
	{NULL, NULL}
};

static void
luaopen_atom(Handle *handle, lua_State *L)
{
	luaL_newmetatable(L, "lseq");
	lua_pushlightuserdata(L, handle); // @ upvalueindex 1
	luaL_setfuncs (L, lseq_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lobj");
	lua_pushlightuserdata(L, handle); // @ upvalueindex 1
	luaL_setfuncs (L, lobj_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "ltuple");
	lua_pushlightuserdata(L, handle); // @ upvalueindex 1
	luaL_setfuncs (L, ltuple_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "latom");
	lua_pushlightuserdata(L, handle); // @ upvalueindex 1
	luaL_setfuncs (L, latom_mt, 1);
	//lua_pushvalue(L, -1);
	//lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lforge");
	lua_pushlightuserdata(L, handle); // @ upvalueindex 1
	luaL_setfuncs (L, lforge_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lframe");
	lua_pushlightuserdata(L, handle); // @ upvalueindex 1
	luaL_setfuncs (L, lframe_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
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

	handle->uris.lua_message = handle->map->map(handle->map->handle, LUA_MESSAGE_URI);
	handle->uris.lua_code = handle->map->map(handle->map->handle, LUA_CODE_URI);
	handle->uris.lua_error = handle->map->map(handle->map->handle, LUA_ERROR_URI);

	if(lua_vm_init(&handle->lvm))
		return NULL;
	luaopen_atom(handle, handle->lvm.L);

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
	handle->chunk = strdup(default_code);
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

		if(atom->type == handle->forge.String)
		{
			if(atom->size)
			{
				handle->chunk = strndup(LV2_ATOM_BODY_CONST(atom), atom->size);
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
			printf("load error\n");
			strcpy(handle->error, lua_tostring(L, -1));
			lua_pop(L, 1);

			// load default code
			free(handle->chunk);
			handle->chunk = strdup(default_code);
			luaL_dostring(handle->lvm.L, handle->chunk); // cannot fail
		}

		handle->dirty_in = 0;
	}

	// prepare event_out sequence
	uint32_t capacity = handle->event_out->atom.size;
	LV2_Atom_Forge *forge = &handle->forge;
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->event_out, capacity);
	lv2_atom_forge_sequence_head(forge, &frame, 0);

	// run
	lua_getglobal(L, "run");
	if(lua_isfunction(L, -1))
	{
		// push sequence
		lseq_t *lseq = lua_newuserdata(handle->lvm.L, sizeof(lseq_t));
		lseq->seq = handle->event_in;
		lseq->itr = NULL;
		luaL_getmetatable(L, "lseq");
		lua_setmetatable(L, -2);

		// push forge
		lforge_t *lforge = lua_newuserdata(handle->lvm.L, sizeof(lforge_t));
		lforge->forge = forge;
		luaL_getmetatable(L, "lforge");
		lua_setmetatable(L, -2);
			
		if(lua_pcall(L, 2, 0, 0))
		{
			printf("runtime error\n");
			strcpy(handle->error, lua_tostring(L, -1));
			lua_pop(L, 1);

			// load default code
			free(handle->chunk);
			handle->chunk = strdup(default_code);
			luaL_dostring(handle->lvm.L, handle->chunk); // cannot fail
		}
	}
	else
		lua_pop(L, 1);

	lua_gc(L, LUA_GCSTEP, 0);
	lv2_atom_forge_pop(forge, &frame);

	// prepare notify atom forge
	capacity = handle->notify->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->notify, capacity);
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	if(handle->dirty_out)
	{
		uint32_t len = strlen(handle->chunk) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, 0);
		lv2_atom_forge_object(forge, &obj_frame, 0, handle->uris.lua_message);
		lv2_atom_forge_key(forge, handle->uris.lua_code);
		lv2_atom_forge_string(forge, handle->chunk, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		handle->dirty_out = 0; // reset flag
	}
	if(handle->error[0])
	{
		uint32_t len = strlen(handle->error) + 1;
		LV2_Atom_Forge_Frame obj_frame;
		lv2_atom_forge_frame_time(forge, 0);
		lv2_atom_forge_object(forge, &obj_frame, 0, handle->uris.lua_message);
		lv2_atom_forge_key(forge, handle->uris.lua_error);
		lv2_atom_forge_string(forge, handle->error, len);
		lv2_atom_forge_pop(forge, &obj_frame);

		handle->error[0] = '\0'; // reset flag
	}
	lv2_atom_forge_pop(forge, &frame);
}

static void
deactivate(LV2_Handle instance)
{
	Handle *handle = (Handle *)instance;

	if(handle->chunk)
		free(handle->chunk);
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
