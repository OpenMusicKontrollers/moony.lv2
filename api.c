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

#include <limits.h> // INT_MAX
#include <math.h> // INFINITY

#include <api_atom.h>
#include <api_forge.h>
#include <api_midi.h>
#include <api_osc.h>
#include <api_time.h>
#include <api_state.h>

#define RDF_PREFIX "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDFS_PREFIX "http://www.w3.org/2000/01/rdf-schema#"

#define RDF__value RDF_PREFIX"value"
#define RDFS__label RDFS_PREFIX"label"
#define RDFS__range RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

typedef struct _atom_ser_t atom_ser_t;
typedef struct _lstash_t lstash_t;

struct _atom_ser_t {
	tlsf_t tlsf;
	uint32_t size;
	uint8_t *buf;
	uint32_t offset;
};

struct _lstash_t {
	lforge_t lforge;
	LV2_Atom_Forge forge;
	atom_ser_t ser;
};

static const char *moony_ref [MOONY_UDATA_COUNT] = {
	[MOONY_UDATA_ATOM]	= "latom",
	[MOONY_UDATA_FORGE]	= "lforge",
	[MOONY_UDATA_STASH]	= "lforge"
};

static const size_t moony_sz [MOONY_UDATA_COUNT] = {
	[MOONY_UDATA_ATOM]	= sizeof(latom_t),
	[MOONY_UDATA_FORGE]	= sizeof(lforge_t),
	[MOONY_UDATA_STASH]	= sizeof(lstash_t)
};

static int
_hash_sort(const void *itm1, const void *itm2)
{
	const latom_driver_hash_t *hash1 = itm1;
	const latom_driver_hash_t *hash2 = itm2;

	if(hash1->type < hash2->type)
		return -1;
	else if(hash1->type > hash2->type)
		return 1;
	return 0;
}

static int
_lmap__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_pushvalue(L, 2);
	lua_rawget(L, 1); // rawget(self, uri)

	if(lua_isnil(L, -1)) // no yet cached
	{
		const char *uri = luaL_checkstring(L, 2);
		LV2_URID urid = moony->map->map(moony->map->handle, uri); // non-rt
		if(urid)
		{
			lua_pushinteger(L, urid);

			// cache it
			lua_pushvalue(L, 2); // uri
			lua_pushvalue(L, -2); // urid
			lua_rawset(L, 1);  // self
		}
		else
			lua_pushnil(L);
	}

	return 1;
}

static int
_lmap__call(lua_State *L)
{
	lua_settop(L, 2);
	lua_gettable(L, -2); // self[uri]

	return 1;
}

static const luaL_Reg lmap_mt [] = {
	{"__index", _lmap__index},
	{"__call", _lmap__index},
	{NULL, NULL}
};

static int
_lunmap__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_pushvalue(L, 2);
	lua_rawget(L, 1); // rawget(self, urid)

	if(lua_isnil(L, -1)) // no yet cached
	{
		LV2_URID urid = luaL_checkinteger(L, 2);
		const char *uri = moony->unmap->unmap(moony->unmap->handle, urid); // non-rt

		if(uri)
		{
			lua_pushstring(L, uri);

			// cache it
			lua_pushvalue(L, 2); // urid
			lua_pushvalue(L, -2); // uri
			lua_rawset(L, 1);  // self
		}
		else
			lua_pushnil(L);
	}

	return 1;
}

static int
_lunmap__call(lua_State *L)
{
	lua_settop(L, 2);
	lua_gettable(L, -2); // self[uri]

	return 1;
}

static const luaL_Reg lunmap_mt [] = {
	{"__index", _lunmap__index},
	{"__call", _lunmap__index},
	{NULL, NULL}
};

static int
_log(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	int n = lua_gettop(L);

	if(!n)
		return 0;

	luaL_Buffer buf;
	luaL_buffinit(L, &buf);

	for(int i=1; i<=n; i++)
	{
		const char *str = NULL;
		switch(lua_type(L, i))
		{
			case LUA_TNIL:
				str = "(nil)";
				break;
			case LUA_TBOOLEAN:
				str = lua_toboolean(L, i) ? "true" : "false";
				break;
			case LUA_TLIGHTUSERDATA:
				str = lua_tostring(L, i);
				if(!str)
					str = "(lightuserdata)";
				break;
			case LUA_TNUMBER:
				str = lua_tostring(L, i);
				break;
			case LUA_TSTRING:
				str = lua_tostring(L, i);
				break;
			case LUA_TTABLE:
				str = lua_tostring(L, i);
				if(!str)
					str = "(table)";
				break;
			case LUA_TFUNCTION:
				str = "(function)";
				break;
			case LUA_TUSERDATA:
				str = lua_tostring(L, i);
				if(!str)
					str = "(userdata)";
				break;
			case LUA_TTHREAD:
				str = "(thread)";
				break;
		};
		if(!str)
			continue;
		luaL_addstring(&buf, str);
		if(i < n)
			luaL_addchar(&buf, '\t');
	}

	luaL_pushresult(&buf);

	const char *res = lua_tostring(L, -1);
	if(moony->log)
		lv2_log_trace(&moony->logger, "%s", res);

	// feedback to UI
	char *end = strrchr(moony->trace, '\0'); // search end of string
	if(end)
	{
		if(end != moony->trace)
			*end++ = '\n'; // append to
		snprintf(end, MOONY_MAX_ERROR_LEN - (end - moony->trace), "%s", res);
		moony->trace_out = 1; // set flag
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
{
	atom_ser_t *ser = handle;

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;

	if(ser->offset + size > ser->size)
	{
		const uint32_t new_size = ser->size * 2;
		if(ser->tlsf)
		{
			if(!(ser->buf = tlsf_realloc(ser->tlsf, ser->buf, new_size)))
				return 0; // realloc failed
		}
		else
		{
			if(!(ser->buf = realloc(ser->buf, new_size)))
				return 0; // realloc failed
		}

		ser->size = new_size;
	}

	memcpy(ser->buf + ser->offset, buf, size);
	ser->offset += size;

	return ref;
}

static inline LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

/*FIXME
static int
_lstash(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lstash_t *lstash = moony_newuserdata(L, moony, MOONY_UDATA_STASH);
	lforge_t *lforge = &lstash->lforge;
	LV2_Atom_Forge *forge = &lstash->forge;
	atom_ser_t *ser = &lstash->ser;

	lforge->depth = 0;
	lforge->last.frames = 0;
	lforge->forge = forge;

	ser->tlsf = moony->vm.tlsf;
	ser->size = 256;
	ser->buf = tlsf_malloc(moony->vm.tlsf, 256);
	ser->offset = 0;

	if(ser->buf)
	{
		LV2_Atom *atom = (LV2_Atom *)ser->buf;
		atom->size = 0;
		atom->type = 0;

		// initialize forge (URIDs)
		memcpy(forge, &moony->forge, sizeof(LV2_Atom_Forge));
		lv2_atom_forge_set_sink(forge, _sink, _deref, ser);

		//FIXME needs __gc
	}
	else
		lua_pushnil(L); // failed to allocate memory

	return 1;
}
*/

int
moony_init(moony_t *moony, const char *subject, double sample_rate,
	const LV2_Feature *const *features)
{
	if(moony_vm_init(&moony->vm))
	{
		fprintf(stderr, "Lua VM cannot be initialized\n");
		return -1;
	}
	
	LV2_Options_Option *opts = NULL;
	bool load_default_state = false;

	for(unsigned i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			moony->map = (LV2_URID_Map *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			moony->unmap = (LV2_URID_Unmap *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_WORKER__schedule))
			moony->sched = (LV2_Worker_Schedule *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			moony->log = (LV2_Log_Log *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_OPTIONS__options))
			opts = (LV2_Options_Option *)features[i]->data;
		else if(!strcmp(features[i]->URI, OSC__schedule))
			moony->osc_sched = (osc_schedule_t *)features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_STATE__loadDefaultState))
			load_default_state = true;
	}

	if(!moony->map)
	{
		fprintf(stderr, "Host does not support urid:map\n");
		return -1;
	}
	if(!moony->unmap)
	{
		fprintf(stderr, "Host does not support urid:unmap\n");
		return -1;
	}
	if(!moony->sched)
	{
		fprintf(stderr, "Host does not support worker:schedule\n");
		return -1;
	}
	if(!load_default_state)
	{
		strcpy(moony->chunk,
			"-- host does not support state:loadDefaultState feature\n\n"
			"function run(...)\n"
			"end");
	}

	moony->uris.subject = moony->map->map(moony->map->handle, subject);

	moony->uris.moony_message = moony->map->map(moony->map->handle, MOONY_MESSAGE_URI);
	moony->uris.moony_code = moony->map->map(moony->map->handle, MOONY_CODE_URI);
	moony->uris.moony_selection = moony->map->map(moony->map->handle, MOONY_SELECTION_URI);
	moony->uris.moony_error = moony->map->map(moony->map->handle, MOONY_ERROR_URI);
	moony->uris.moony_trace = moony->map->map(moony->map->handle, MOONY_TRACE_URI);
	moony->uris.moony_state = moony->map->map(moony->map->handle, MOONY_STATE_URI);

	moony->uris.midi_event = moony->map->map(moony->map->handle, LV2_MIDI__MidiEvent);
	
	moony->uris.bufsz_min_block_length = moony->map->map(moony->map->handle,
		LV2_BUF_SIZE__minBlockLength);
	moony->uris.bufsz_max_block_length = moony->map->map(moony->map->handle,
		LV2_BUF_SIZE__maxBlockLength);
	moony->uris.bufsz_sequence_size = moony->map->map(moony->map->handle,
		LV2_BUF_SIZE__sequenceSize);

	moony->uris.patch_get = moony->map->map(moony->map->handle, LV2_PATCH__Get);
	moony->uris.patch_set = moony->map->map(moony->map->handle, LV2_PATCH__Set);
	moony->uris.patch_put = moony->map->map(moony->map->handle, LV2_PATCH__Put);
	moony->uris.patch_patch = moony->map->map(moony->map->handle, LV2_PATCH__Patch);
	moony->uris.patch_body = moony->map->map(moony->map->handle, LV2_PATCH__body);
	moony->uris.patch_subject = moony->map->map(moony->map->handle, LV2_PATCH__subject);
	moony->uris.patch_property = moony->map->map(moony->map->handle, LV2_PATCH__property);
	moony->uris.patch_value = moony->map->map(moony->map->handle, LV2_PATCH__value);
	moony->uris.patch_add = moony->map->map(moony->map->handle, LV2_PATCH__add);
	moony->uris.patch_remove = moony->map->map(moony->map->handle, LV2_PATCH__remove);
	moony->uris.patch_wildcard = moony->map->map(moony->map->handle, LV2_PATCH__wildcard);
	moony->uris.patch_writable = moony->map->map(moony->map->handle, LV2_PATCH__writable);
	moony->uris.patch_readable = moony->map->map(moony->map->handle, LV2_PATCH__readable);

	moony->uris.rdfs_label = moony->map->map(moony->map->handle, RDFS__label);
	moony->uris.rdfs_range = moony->map->map(moony->map->handle, RDFS__range);
	moony->uris.rdfs_comment = moony->map->map(moony->map->handle, RDFS__comment);

	moony->uris.rdf_value = moony->map->map(moony->map->handle, RDF__value);

	moony->uris.core_minimum = moony->map->map(moony->map->handle, LV2_CORE__minimum);
	moony->uris.core_maximum = moony->map->map(moony->map->handle, LV2_CORE__maximum);
	moony->uris.core_scale_point = moony->map->map(moony->map->handle, LV2_CORE__scalePoint);

	moony->uris.units_unit = moony->map->map(moony->map->handle, LV2_UNITS__unit);

	osc_forge_init(&moony->oforge, moony->map);
	lv2_atom_forge_init(&moony->forge, moony->map);
	lv2_atom_forge_init(&moony->state_forge, moony->map);
	lv2_atom_forge_init(&moony->stash_forge, moony->map);
	if(moony->log)
		lv2_log_logger_init(&moony->logger, moony->map, moony->log);

	latom_driver_hash_t *latom_driver_hash = moony->atom_driver_hash;
	unsigned pos = 0;

	latom_driver_hash[pos].type = moony->forge.Int;
	latom_driver_hash[pos++].driver = &latom_int_driver;

	latom_driver_hash[pos].type = moony->forge.Long;
	latom_driver_hash[pos++].driver = &latom_long_driver;

	latom_driver_hash[pos].type = moony->forge.Float;
	latom_driver_hash[pos++].driver = &latom_float_driver;

	latom_driver_hash[pos].type = moony->forge.Double;
	latom_driver_hash[pos++].driver = &latom_double_driver;

	latom_driver_hash[pos].type = moony->forge.Bool;
	latom_driver_hash[pos++].driver = &latom_bool_driver;

	latom_driver_hash[pos].type = moony->forge.URID;
	latom_driver_hash[pos++].driver = &latom_urid_driver;

	latom_driver_hash[pos].type = moony->forge.String;
	latom_driver_hash[pos++].driver = &latom_string_driver;

	latom_driver_hash[pos].type = moony->forge.URI;
	latom_driver_hash[pos++].driver = &latom_string_driver;

	latom_driver_hash[pos].type = moony->forge.Path;
	latom_driver_hash[pos++].driver = &latom_string_driver;

	latom_driver_hash[pos].type = moony->forge.Literal;
	latom_driver_hash[pos++].driver = &latom_literal_driver;

	latom_driver_hash[pos].type = moony->forge.Tuple;
	latom_driver_hash[pos++].driver = &latom_tuple_driver;

	latom_driver_hash[pos].type = moony->forge.Object;
	latom_driver_hash[pos++].driver = &latom_object_driver;

	latom_driver_hash[pos].type = moony->forge.Vector;
	latom_driver_hash[pos++].driver = &latom_vector_driver;

	latom_driver_hash[pos].type = moony->forge.Sequence;
	latom_driver_hash[pos++].driver = &latom_sequence_driver;

	latom_driver_hash[pos].type = moony->forge.Chunk;
	latom_driver_hash[pos++].driver = &latom_chunk_driver;

	latom_driver_hash[pos].type = moony->uris.midi_event;
	latom_driver_hash[pos++].driver = &latom_chunk_driver;

	assert(pos++ == DRIVER_HASH_MAX);
	qsort(latom_driver_hash, DRIVER_HASH_MAX, sizeof(latom_driver_hash_t), _hash_sort);
	
	if(opts)
	{
		for(LV2_Options_Option *opt = opts;
			(opt->key != 0) && (opt->value != NULL);
			opt++)
		{
			if(opt->key == moony->uris.bufsz_min_block_length)
				moony->opts.min_block_length = *(int32_t *)opt->value;
			else if(opt->key == moony->uris.bufsz_max_block_length)
				moony->opts.max_block_length = *(int32_t *)opt->value;
			else if(opt->key == moony->uris.bufsz_sequence_size)
				moony->opts.sequence_size = *(int32_t *)opt->value;
		}
	}
	moony->opts.sample_rate = sample_rate;

	moony->dirty_out = 1; // trigger update of UI

	atomic_flag_clear_explicit(&moony->lock.chunk, memory_order_relaxed);
	atomic_flag_clear_explicit(&moony->lock.error, memory_order_relaxed);
	atomic_flag_clear_explicit(&moony->lock.state, memory_order_relaxed);

	return 0;
}

void
moony_deinit(moony_t *moony)
{
	if(moony->state_atom)
		free(moony->state_atom);
	moony->state_atom = NULL;

	if(moony->stash_atom)
		tlsf_free(moony->vm.tlsf, moony->stash_atom);
	moony->stash_atom = NULL;

	moony_vm_deinit(&moony->vm);
}

void
moony_open(moony_t *moony, lua_State *L, bool use_assert)
{
	luaL_newmetatable(L, "latom");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, latom_mt, 1);
	lua_pop(L, 1);
	
	luaL_newmetatable(L, "lforge");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lforge_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	// lv2.map
	lua_newtable(L);
	lua_newtable(L);
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs(L, lmap_mt, 1);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Map");

	// lv2.unmap
	lua_newtable(L);
	lua_newtable(L);
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs(L, lunmap_mt, 1);
	lua_setmetatable(L, -2);
	lua_setglobal(L, "Unmap");

#define SET_MAP(L, PREFIX, PROPERTY) \
({ \
	lua_getglobal(L, "Map"); \
	lua_getfield(L, -1, PREFIX ## PROPERTY); \
	LV2_URID urid = luaL_checkinteger(L, -1); \
	lua_remove(L, -2); /* Map */ \
	lua_setfield(L, -2, #PROPERTY); \
	urid; \
})

	LV2_URID core_sample_rate;

	lua_newtable(L);
	{
		SET_MAP(L, LV2_CORE__, ControlPort);
	}
	lua_setglobal(L, "State");

	lua_newtable(L);
	{
		//SET_MAP(L, LV2_ATOM__, Blank); // is depracated
		SET_MAP(L, LV2_ATOM__, Bool);
		SET_MAP(L, LV2_ATOM__, Chunk);
		SET_MAP(L, LV2_ATOM__, Double);
		SET_MAP(L, LV2_ATOM__, Float);
		SET_MAP(L, LV2_ATOM__, Int);
		SET_MAP(L, LV2_ATOM__, Long);
		SET_MAP(L, LV2_ATOM__, Literal);
		SET_MAP(L, LV2_ATOM__, Object);
		SET_MAP(L, LV2_ATOM__, Path);
		SET_MAP(L, LV2_ATOM__, Property);
		//SET_MAP(L, LV2_ATOM__, Resource); // is deprecated
		SET_MAP(L, LV2_ATOM__, Sequence);
		SET_MAP(L, LV2_ATOM__, String);
		SET_MAP(L, LV2_ATOM__, Tuple);
		SET_MAP(L, LV2_ATOM__, URI);
		SET_MAP(L, LV2_ATOM__, URID);
		SET_MAP(L, LV2_ATOM__, Vector);
	}
	lua_setglobal(L, "Atom");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_MIDI__, MidiEvent);

		for(const midi_msg_t *msg=midi_msgs; msg->key; msg++)
		{
			lua_pushinteger(L, msg->type);
			lua_setfield(L, -2, msg->key);
		}
		for(const midi_msg_t *msg=controllers; msg->key; msg++)
		{
			lua_pushinteger(L, msg->type);
			lua_setfield(L, -2, msg->key);
		}
	}
	lua_setglobal(L, "MIDI");

	lua_createtable(L, 0x80, 0x80);
	{
		const char *keys [12] = {
			"C", "C#",
			"D", "D#",
			"E",
			"F", "F#",
			"G", "G#",
			"A", "A#",
			"H"
		};
		char name [8];

		for(unsigned i=0; i<0x80; i++)
		{
			const unsigned octave = i / 12;
			const unsigned key = i % 12;
			snprintf(name, 8, "%s%u", keys[key], octave);

			lua_pushinteger(L, i);
			lua_setfield(L, -2, name);

			lua_pushstring(L, name);
			lua_rawseti(L, -2, i);
		}
	}
	lua_setglobal(L, "Note");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_TIME__, Position);
		SET_MAP(L, LV2_TIME__, barBeat);
		SET_MAP(L, LV2_TIME__, bar);
		SET_MAP(L, LV2_TIME__, beat);
		SET_MAP(L, LV2_TIME__, beatUnit);
		SET_MAP(L, LV2_TIME__, beatsPerBar);
		SET_MAP(L, LV2_TIME__, beatsPerMinute);
		SET_MAP(L, LV2_TIME__, frame);
		SET_MAP(L, LV2_TIME__, framesPerSecond);
		SET_MAP(L, LV2_TIME__, speed);
	}
	lua_setglobal(L, "Time");

	lua_newtable(L);
	{
		SET_MAP(L, OSC__, Event);
		SET_MAP(L, OSC__, Bundle);
		SET_MAP(L, OSC__, Message);
		SET_MAP(L, OSC__, bundleTimestamp);
		SET_MAP(L, OSC__, bundleItems);
		SET_MAP(L, OSC__, messagePath);
		SET_MAP(L, OSC__, messageFormat);
		SET_MAP(L, OSC__, messageArguments);
	}
	lua_setglobal(L, "OSC");

	lua_newtable(L);
	{
		core_sample_rate = SET_MAP(L, LV2_CORE__, sampleRate);
		SET_MAP(L, LV2_CORE__, minimum);
		SET_MAP(L, LV2_CORE__, maximum);
		SET_MAP(L, LV2_CORE__, scalePoint);
	}
	lua_setglobal(L, "Core");
	
	lua_newtable(L);
	{
		SET_MAP(L, LV2_BUF_SIZE__, minBlockLength);
		SET_MAP(L, LV2_BUF_SIZE__, maxBlockLength);
		SET_MAP(L, LV2_BUF_SIZE__, sequenceSize);
	}
	lua_setglobal(L, "Buf_Size");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_PATCH__, Ack);
		SET_MAP(L, LV2_PATCH__, Delete);
		SET_MAP(L, LV2_PATCH__, Copy);
		SET_MAP(L, LV2_PATCH__, Error);
		SET_MAP(L, LV2_PATCH__, Get);
		SET_MAP(L, LV2_PATCH__, Message);
		SET_MAP(L, LV2_PATCH__, Move);
		SET_MAP(L, LV2_PATCH__, Patch);
		SET_MAP(L, LV2_PATCH__, Post);
		SET_MAP(L, LV2_PATCH__, Put);
		SET_MAP(L, LV2_PATCH__, Request);
		SET_MAP(L, LV2_PATCH__, Response);
		SET_MAP(L, LV2_PATCH__, Set);
		SET_MAP(L, LV2_PATCH__, add);
		SET_MAP(L, LV2_PATCH__, body);
		SET_MAP(L, LV2_PATCH__, destination);
		SET_MAP(L, LV2_PATCH__, property);
		SET_MAP(L, LV2_PATCH__, readable);
		SET_MAP(L, LV2_PATCH__, remove);
		SET_MAP(L, LV2_PATCH__, request);
		SET_MAP(L, LV2_PATCH__, subject);
		SET_MAP(L, LV2_PATCH__, sequenceNumber);
		SET_MAP(L, LV2_PATCH__, value);
		SET_MAP(L, LV2_PATCH__, wildcard);
		SET_MAP(L, LV2_PATCH__, writable);
	}
	lua_setglobal(L, "Patch");

	lua_newtable(L);
	{
		SET_MAP(L, RDF__, value);
	}
	lua_setglobal(L, "RDF");

	lua_newtable(L);
	{
		SET_MAP(L, RDFS__, label);
		SET_MAP(L, RDFS__, range);
		SET_MAP(L, RDFS__, comment);
	}
	lua_setglobal(L, "RDFS");

	lua_newtable(L);
	{
		SET_MAP(L, LV2_UNITS__, Conversion);
		SET_MAP(L, LV2_UNITS__, Unit);
		SET_MAP(L, LV2_UNITS__, bar);
		SET_MAP(L, LV2_UNITS__, beat);
		SET_MAP(L, LV2_UNITS__, bpm);
		SET_MAP(L, LV2_UNITS__, cent);
		SET_MAP(L, LV2_UNITS__, cm);
		SET_MAP(L, LV2_UNITS__, coef);
		SET_MAP(L, LV2_UNITS__, conversion);
		SET_MAP(L, LV2_UNITS__, db);
		SET_MAP(L, LV2_UNITS__, degree);
		SET_MAP(L, LV2_UNITS__, frame);
		SET_MAP(L, LV2_UNITS__, hz);
		SET_MAP(L, LV2_UNITS__, inch);
		SET_MAP(L, LV2_UNITS__, khz);
		SET_MAP(L, LV2_UNITS__, km);
		SET_MAP(L, LV2_UNITS__, m);
		SET_MAP(L, LV2_UNITS__, mhz);
		SET_MAP(L, LV2_UNITS__, midiNote);
		SET_MAP(L, LV2_UNITS__, mile);
		SET_MAP(L, LV2_UNITS__, min);
		SET_MAP(L, LV2_UNITS__, mm);
		SET_MAP(L, LV2_UNITS__, ms);
		SET_MAP(L, LV2_UNITS__, name);
		SET_MAP(L, LV2_UNITS__, oct);
		SET_MAP(L, LV2_UNITS__, pc);
		SET_MAP(L, LV2_UNITS__, prefixConversion);
		SET_MAP(L, LV2_UNITS__, render);
		SET_MAP(L, LV2_UNITS__, s);
		SET_MAP(L, LV2_UNITS__, semitone12TET);
		SET_MAP(L, LV2_UNITS__, symbol);
		SET_MAP(L, LV2_UNITS__, unit);
	}
	lua_setglobal(L, "Units");

	lua_newtable(L);
	{
		if(moony->opts.min_block_length)
		{
			lua_pushinteger(L, moony->uris.bufsz_min_block_length);
			lua_pushinteger(L, moony->opts.min_block_length);
			lua_rawset(L, -3);
		}
	
		if(moony->opts.max_block_length)
		{
			lua_pushinteger(L, moony->uris.bufsz_max_block_length);
			lua_pushinteger(L, moony->opts.max_block_length);
			lua_rawset(L, -3);
		}
		
		if(moony->opts.sequence_size)
		{
			lua_pushinteger(L, moony->uris.bufsz_sequence_size);
			lua_pushinteger(L, moony->opts.sequence_size);
			lua_rawset(L, -3);
		}
	
		if(moony->opts.sample_rate)
		{
			lua_pushinteger(L, core_sample_rate);
			lua_pushinteger(L, moony->opts.sample_rate);
			lua_rawset(L, -3);
		}
	}
	lua_setglobal(L, "Options");

#define UDATA_OFFSET (LUA_RIDX_LAST + 1)
	// create userdata caches
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_ATOM);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_FORGE);
	lua_newtable(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + MOONY_UDATA_STASH);

	// overwrite print function with LV2 log
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _log, 1);
	lua_setglobal(L, "print");

	if(!use_assert)
	{
		// clear assert
		lua_pushnil(L);
		lua_setglobal(L, "assert");
	}

	// clear dofile
	lua_pushnil(L);
	lua_setglobal(L, "dofile");

	// clear loadfile
	lua_pushnil(L);
	lua_setglobal(L, "loadfile");

	// MIDIResponder metatable
	luaL_newmetatable(L, "lmidiresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lmidiresponder_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	// MIDIResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lmidiresponder, 1);
	lua_setglobal(L, "MIDIResponder");

	// OSCResponder metatable
	luaL_newmetatable(L, "loscresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, loscresponder_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	// OSCResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _loscresponder, 1);
	lua_setglobal(L, "OSCResponder");

	// OSCResponder pattern matcher
	luaL_dostring(L, loscresponder_match); // cannot fail

	// TimeResponder metatable
	luaL_newmetatable(L, "ltimeresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, ltimeresponder_mt, 1);
	// we have a __index function, thus no __index table here
	lua_pop(L, 1);

	// TimeResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _ltimeresponder, 1);
	lua_setglobal(L, "TimeResponder");

	// StateResponder metatable
	luaL_newmetatable(L, "lstateresponder");
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	luaL_setfuncs (L, lstateresponder_mt, 1);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	// StateResponder factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lstateresponder, 1);
	lua_setglobal(L, "StateResponder");

	/*FIXME
	// Stash factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lstash, 1);
	lua_setglobal(L, "Stash");
	*/

#undef SET_MAP
}

void *
moony_newuserdata(lua_State *L, moony_t *moony, moony_udata_t type)
{
	assert( (type >= MOONY_UDATA_ATOM) && (type < MOONY_UDATA_COUNT) );

	int *itr = &moony->itr[type];
	void *data = NULL;

	lua_rawgeti(L, LUA_REGISTRYINDEX, UDATA_OFFSET + type); // ref
	lua_rawgeti(L, -1, *itr); // udata?
	if(lua_isnil(L, -1)) // no cached udata, create one!
	{
		lua_pop(L, 1); // nil

		data = lua_newuserdata(L, moony_sz[type]);
		luaL_getmetatable(L, moony_ref[type]);
		lua_setmetatable(L, -2);
		lua_pushvalue(L, -1);
		lua_rawseti(L, -3, *itr); // store in cache
	}
	else // there is a cached udata, use it!
	{
		data = lua_touserdata(L, -1);
		//printf("moony_newuserdata: %s %i %p\n", moony_ref[type], *itr, data);
	}
	lua_remove(L, -2); // ref
	*itr += 1;

	return data;

#undef UDATA_OFFSET
}

static int
_stash(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_getglobal(L, "stash");
	if(lua_isfunction(L, -1))
	{
		lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
		lframe->depth = 0;
		lframe->last.frames = 0;
		lframe->forge = &moony->stash_forge;

		atom_ser_t ser = {
			.tlsf = moony->vm.tlsf, // use tlsf_realloc
			.size = 256,
			.buf = tlsf_malloc(moony->vm.tlsf, 256)
		};

		if(ser.buf)
		{
			LV2_Atom *atom = (LV2_Atom *)ser.buf;
			atom->type = 0;
			atom->size = 0;

			lv2_atom_forge_set_sink(lframe->forge, _sink, _deref, &ser);
			lua_call(L, 1, 0);

			if(moony->stash_atom)
				tlsf_free(moony->vm.tlsf, moony->stash_atom);
			moony->stash_atom = atom;
		}
	}
	else
		lua_pop(L, 1);

	return 0;
}

static int
_apply(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_getglobal(L, "apply");
	if(lua_isfunction(L, -1))
	{
		_latom_new(L, moony->stash_atom);
		lua_call(L, 1, 0);
	}
	else
		lua_pop(L, 1);

	return 0;
}

static int
_save(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	if(lua_getglobal(L, "save") == LUA_TFUNCTION)
	{
		lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
		lframe->depth = 0;
		lframe->last.frames = 0;
		lframe->forge = &moony->state_forge;

		lua_call(L, 1, 0);
	}

	return 0;
}

static LV2_State_Status
_state_save(LV2_Handle instance, LV2_State_Store_Function store,
	LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	moony_t *moony = (moony_t *)instance;

	LV2_State_Status status = LV2_STATE_SUCCESS;
	char *chunk = NULL;

	_spin_lock(&moony->lock.chunk);
	chunk = strdup(moony->chunk);
	_unlock(&moony->lock.chunk);

	if(chunk)
	{
		status = store(
			state,
			moony->uris.moony_code,
			chunk,
			strlen(chunk) + 1,
			moony->forge.String,
			LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

		free(chunk);
	}

	atom_ser_t ser = {
		.tlsf = NULL,
		.size = 256,
		.buf = malloc(256)
	};

	if(ser.buf)
	{
		LV2_Atom *atom = (LV2_Atom *)ser.buf;
		atom->type = 0;
		atom->size = 0;

		lv2_atom_forge_set_sink(&moony->state_forge, _sink, _deref, &ser);

		// lock Lua state, so it cannot be accessed by realtime thread
		_spin_lock(&moony->lock.state);

		// restore Lua defined properties
		lua_State *L = moony->vm.L;
		lua_pushlightuserdata(L, moony);
		lua_pushcclosure(L, _save, 1);
		if(lua_pcall(L, 0, 0, 0))
		{
			if(moony->log) //TODO send to UI, too
				lv2_log_error(&moony->logger, "%s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}

		_unlock(&moony->lock.state);

		if( (atom->type) && (atom->size) )
		{
			status = store(
				state,
				moony->uris.moony_state,
				LV2_ATOM_BODY(atom),
				atom->size,
				atom->type,
				LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
		}

		if(moony->state_atom)
			free(moony->state_atom);
		moony->state_atom = atom;
	}

	return status;
}

static int
_restore(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	if(lua_getglobal(L, "restore") == LUA_TFUNCTION)
	{
		_latom_new(L, moony->state_atom);
		lua_call(L, 1, 0);
	}

	return 0;
}

static LV2_State_Status
_state_restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve, LV2_State_Handle state, uint32_t flags, const LV2_Feature *const *features)
{
	moony_t *moony = (moony_t *)instance;
	lua_State *L = moony->vm.L;

	size_t size;
	uint32_t type;
	uint32_t flags2;

	const char *chunk = retrieve(
		state,
		moony->uris.moony_code,
		&size,
		&type,
		&flags2
	);

	//TODO check type, flags2

	if(chunk && size && type)
	{
		//_spin_lock(&moony->lock.chunk);
		strncpy(moony->chunk, chunk, size);
		//_unlock(&moony->lock.chunk);

		if(luaL_dostring(L, moony->chunk))
			moony_error(moony);
		else
			moony->error[0] = 0x0; // reset error flag

		moony->dirty_out = 1;
	}

	const uint8_t *body = retrieve(
		state,
		moony->uris.moony_state,
		&size,
		&type,
		&flags2
	);

	if(body && size && type)
	{
		if(moony->state_atom) // clear old state_atom
			free(moony->state_atom);

		// allocate new state_atom
		moony->state_atom = malloc(sizeof(LV2_Atom) + size);
		if(moony->state_atom) // fill new restore atom
		{
			moony->state_atom->size = size;
			moony->state_atom->type = type;
			memcpy(LV2_ATOM_BODY(moony->state_atom), body, size);

			// restore Lua defined properties
			lua_pushlightuserdata(L, moony);
			lua_pushcclosure(L, _restore, 1);
			if(lua_pcall(L, 0, 0, 0))
			{
				if(moony->log) //TODO send to UI, too
					lv2_log_error(&moony->logger, "%s", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
			lua_gc(L, LUA_GCSTEP, 0);
		}
	}

	return LV2_STATE_SUCCESS;
}

static const LV2_State_Interface state_iface = {
	.save = _state_save,
	.restore = _state_restore
};

// non-rt thread
static LV2_Worker_Status
_work(LV2_Handle instance,
	LV2_Worker_Respond_Function respond,
	LV2_Worker_Respond_Handle target,
	uint32_t size,
	const void *body)
{
	moony_t *moony = instance;

	assert(size == sizeof(moony_mem_t));
	const moony_mem_t *request = body;
	uint32_t i = request->i;

	if(request->mem) // request to free memory from _work_response
	{
		moony_vm_mem_free(request->mem, moony->vm.size[i]);
	}
	else // request to allocate memory from moony_vm_mem_extend
	{
		moony->vm.area[i] = moony_vm_mem_alloc(moony->vm.size[i]);
	
		respond(target, size, body); // signal to _work_response
	}
	
	return LV2_WORKER_SUCCESS;
}

// rt-thread
static LV2_Worker_Status
_work_response(LV2_Handle instance, uint32_t size, const void *body)
{
	moony_t *moony = instance;

	assert(size == sizeof(moony_mem_t));
	const moony_mem_t *request = body;
	uint32_t i = request->i;
	
	if(!moony->vm.area[i]) // allocation failed
		return LV2_WORKER_ERR_UNKNOWN;

	// tlsf add pool
	moony->vm.pool[i] = tlsf_add_pool(moony->vm.tlsf,
		moony->vm.area[i], moony->vm.size[i]);

	if(!moony->vm.pool[i]) // pool addition failed
	{
		const moony_mem_t req = {
			.i = i,
			.mem = moony->vm.area[i]
		};
		moony->vm.area[i] = NULL;

		// schedule freeing of memory to _work
		LV2_Worker_Status status = moony->sched->schedule_work(
			moony->sched->handle, sizeof(moony_mem_t), &req);
		if( (status != LV2_WORKER_SUCCESS) && moony->log)
			lv2_log_warning(&moony->logger, "moony: schedule_work failed");

		return LV2_WORKER_ERR_UNKNOWN;
	}
		
	moony->vm.space += moony->vm.size[i];
	//printf("mem extended to %zu KB\n", moony->vm.space / 1024);

	return LV2_WORKER_SUCCESS;
}

// rt-thread
static LV2_Worker_Status
_end_run(LV2_Handle instance)
{
	moony_t *moony = instance;

	// do nothing
	moony->working = 0;

	return LV2_WORKER_SUCCESS;
}

static const LV2_Worker_Interface work_iface = {
	.work = _work,
	.work_response = _work_response,
	.end_run = _end_run
};

const void*
extension_data(const char* uri)
{
	if(!strcmp(uri, LV2_WORKER__interface))
		return &work_iface;
	else if(!strcmp(uri, LV2_STATE__interface))
		return &state_iface;
	else
		return NULL;
}

void
moony_in(moony_t *moony, const LV2_Atom_Sequence *seq)
{
	lua_State *L = moony->vm.L;

	LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
	{
		const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

		if(  lv2_atom_forge_is_object_type(&moony->forge, obj->atom.type)
			&& (obj->body.otype == moony->uris.moony_message) )
		{
			const LV2_Atom_String *moony_code = NULL;
			const LV2_Atom_String *moony_selection = NULL;
			
			LV2_Atom_Object_Query q[] = {
				{ moony->uris.moony_code, (const LV2_Atom **)&moony_code },
				{ moony->uris.moony_selection, (const LV2_Atom **)&moony_selection },
				{ 0, NULL}
			};
			lv2_atom_object_query(obj, q);

			if(moony_code && moony_code->atom.size)
			{
				// stash
				lua_pushlightuserdata(L, moony);
				lua_pushcclosure(L, _stash, 1);
				if(lua_pcall(L, 0, 0, 0))
					moony_error(moony);

				// load chunk
				const char *str = LV2_ATOM_BODY_CONST(&moony_code->atom);
				if(luaL_dostring(L, str)) // failed loading chunk
				{
					moony_error(moony);
				}
				else // succeeded loading chunk
				{
					_spin_lock(&moony->lock.chunk);
					strncpy(moony->chunk, str, moony_code->atom.size);
					_unlock(&moony->lock.chunk);

					moony->error[0] = 0x0; // reset error flag

					if(moony->state_atom)
					{
						// restore Lua defined properties
						lua_pushlightuserdata(L, moony);
						lua_pushcclosure(L, _restore, 1);
						if(lua_pcall(L, 0, 0, 0))
							moony_error(moony);
					}
				}

				// apply stash
				if(moony->stash_atom) // something has been stashed previously
				{
					lua_pushlightuserdata(L, moony);
					lua_pushcclosure(L, _apply, 1);
					if(lua_pcall(L, 0, 0, 0))
						moony_error(moony);

					tlsf_free(moony->vm.tlsf, moony->stash_atom);
					moony->stash_atom = NULL;
				}
			}
			else if(moony_selection && moony_selection->atom.size)
			{
				// we do not do any stash, apply and restore for selections

				// load chunk
				const char *str = LV2_ATOM_BODY_CONST(&moony_selection->atom);
				if(luaL_dostring(L, str)) // failed loading chunk
				{
					moony_error(moony);
				}
				else // succeeded loading chunk
				{
					moony->error[0] = 0x0; // reset error flag
				}
			}
			else
				moony->dirty_out = 1;
		}
	}
}

void
moony_out(moony_t *moony, LV2_Atom_Sequence *seq, uint32_t frames)
{
	// prepare notify atom forge
	LV2_Atom_Forge *forge = &moony->forge;
	LV2_Atom_Forge_Frame notify_frame;
	LV2_Atom_Forge_Ref ref;
	
	uint32_t capacity = seq->atom.size;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)seq, capacity);
	ref = lv2_atom_forge_sequence_head(forge, &notify_frame, 0);

	if(moony->dirty_out)
	{
		_spin_lock(&moony->lock.chunk);

		uint32_t len = strlen(moony->chunk);
		LV2_Atom_Forge_Frame obj_frame;
		if(ref)
			ref = lv2_atom_forge_frame_time(forge, frames);
		if(ref)
			ref = _moony_message_forge(forge, moony->uris.moony_message,
				moony->uris.moony_code, len, moony->chunk);

		_unlock(&moony->lock.chunk);

		moony->dirty_out = 0; // reset flag
	}

	if(moony->error_out)
	{
		uint32_t len = strlen(moony->error);
		LV2_Atom_Forge_Frame obj_frame;
		if(ref)
			ref = lv2_atom_forge_frame_time(forge, frames);
		if(ref)
			ref = _moony_message_forge(forge, moony->uris.moony_message,
				moony->uris.moony_error, len, moony->error);

		moony->error_out = 0; // reset flag
	}

	if(moony->trace_out)
	{
		char *pch = strtok(moony->trace, "\n");
		while(pch)
		{
			uint32_t len = strlen(pch);
			LV2_Atom_Forge_Frame obj_frame;
			if(ref)
				ref = lv2_atom_forge_frame_time(forge, frames);
			if(ref)
				ref = _moony_message_forge(forge, moony->uris.moony_message,
					moony->uris.moony_trace, len, pch);

			pch = strtok(NULL, "\n");
		}

		moony->trace[0] = '\0';
		moony->trace_out = 0; // reset flag
	}

	if(ref)
		lv2_atom_forge_pop(forge, &notify_frame);
	else
		lv2_atom_sequence_clear(seq);
}
