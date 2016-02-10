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

#include <moony.h>
#include <timely.h>

#include <lauxlib.h>

// there is a bug in LV2 <= 0.10
#if defined(LV2_ATOM_TUPLE_FOREACH)
#	undef LV2_ATOM_TUPLE_FOREACH
#	define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))
#endif

#define LV2_ATOM_VECTOR_ITEM_CONST(vec, i) \
	(LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, (vec)) + (i)*(vec)->body.child_size)

#define RDF_PREFIX "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDFS_PREFIX "http://www.w3.org/2000/01/rdf-schema#"

#define RDF__value RDF_PREFIX"value"
#define RDFS__label RDFS_PREFIX"label"
#define RDFS__range RDFS_PREFIX"range"
#define RDFS__comment RDFS_PREFIX"comment"

typedef struct _midi_msg_t midi_msg_t;
typedef struct _lobj_t lobj_t;
typedef struct _ltuple_t ltuple_t;
typedef struct _lvec_t lvec_t;
typedef struct _atom_ser_t atom_ser_t;
typedef struct _lstash_t lstash_t;

struct _midi_msg_t {
	uint8_t type;
	const char *key;
};

struct _atom_ser_t {
	tlsf_t tlsf;
	uint32_t size;
	uint8_t *buf;
	uint32_t offset;
};

static const midi_msg_t midi_msgs [] = {
	{ LV2_MIDI_MSG_NOTE_OFF							, "NoteOff" },
	{ LV2_MIDI_MSG_NOTE_ON							, "NoteOn" },
	{ LV2_MIDI_MSG_NOTE_PRESSURE				, "NotePressure" },
	{ LV2_MIDI_MSG_CONTROLLER						, "Controller" },
	{ LV2_MIDI_MSG_PGM_CHANGE						, "ProgramChange" },
	{ LV2_MIDI_MSG_CHANNEL_PRESSURE			, "ChannelPressure" },
	{ LV2_MIDI_MSG_BENDER								, "Bender" },
	{ LV2_MIDI_MSG_SYSTEM_EXCLUSIVE			, "SystemExclusive" },
	{ LV2_MIDI_MSG_MTC_QUARTER					, "QuarterFrame" },
	{ LV2_MIDI_MSG_SONG_POS							, "SongPosition" },
	{ LV2_MIDI_MSG_SONG_SELECT					, "SongSelect" },
	{ LV2_MIDI_MSG_TUNE_REQUEST					, "TuneRequest" },
	{ LV2_MIDI_MSG_CLOCK								, "Clock" },
	{ LV2_MIDI_MSG_START								, "Start" },
	{ LV2_MIDI_MSG_CONTINUE							, "Continue" },
	{ LV2_MIDI_MSG_STOP									, "Stop" },
	{ LV2_MIDI_MSG_ACTIVE_SENSE					, "ActiveSense" },
	{ LV2_MIDI_MSG_RESET								, "Reset" },

	{ 0, NULL} // sentinel
};

static const midi_msg_t controllers [] = {
	{ LV2_MIDI_CTL_MSB_BANK             , "BankSelection_MSB" },
	{ LV2_MIDI_CTL_MSB_MODWHEEL         , "Modulation_MSB" },
	{ LV2_MIDI_CTL_MSB_BREATH           , "Breath_MSB" },
	{ LV2_MIDI_CTL_MSB_FOOT             , "Foot_MSB" },
	{ LV2_MIDI_CTL_MSB_PORTAMENTO_TIME  , "PortamentoTime_MSB" },
	{ LV2_MIDI_CTL_MSB_DATA_ENTRY       , "DataEntry_MSB" },
	{ LV2_MIDI_CTL_MSB_MAIN_VOLUME      , "MainVolume_MSB" },
	{ LV2_MIDI_CTL_MSB_BALANCE          , "Balance_MSB" },
	{ LV2_MIDI_CTL_MSB_PAN              , "Panpot_MSB" },
	{ LV2_MIDI_CTL_MSB_EXPRESSION       , "Expression_MSB" },
	{ LV2_MIDI_CTL_MSB_EFFECT1          , "Effect1_MSB" },
	{ LV2_MIDI_CTL_MSB_EFFECT2          , "Effect2_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE1 , "GeneralPurpose1_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE2 , "GeneralPurpose2_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE3 , "GeneralPurpose3_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE4 , "GeneralPurpose4_MSB" },

	{ LV2_MIDI_CTL_LSB_BANK             , "BankSelection_LSB" },
	{ LV2_MIDI_CTL_LSB_MODWHEEL         , "Modulation_LSB" },
	{ LV2_MIDI_CTL_LSB_BREATH           , "Breath_LSB" },
	{ LV2_MIDI_CTL_LSB_FOOT             , "Foot_LSB" },
	{ LV2_MIDI_CTL_LSB_PORTAMENTO_TIME  , "PortamentoTime_LSB" },
	{ LV2_MIDI_CTL_LSB_DATA_ENTRY       , "DataEntry_LSB" },
	{ LV2_MIDI_CTL_LSB_MAIN_VOLUME      , "MainVolume_LSB" },
	{ LV2_MIDI_CTL_LSB_BALANCE          , "Balance_LSB" },
	{ LV2_MIDI_CTL_LSB_PAN              , "Panpot_LSB" },
	{ LV2_MIDI_CTL_LSB_EXPRESSION       , "Expression_LSB" },
	{ LV2_MIDI_CTL_LSB_EFFECT1          , "Effect1_LSB" },
	{ LV2_MIDI_CTL_LSB_EFFECT2          , "Effect2_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE1 , "GeneralPurpose1_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE2 , "GeneralPurpose2_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE3 , "GeneralPurpose3_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE4 , "GeneralPurpose4_LSB" },

	{ LV2_MIDI_CTL_SUSTAIN              , "SustainPedal" },
	{ LV2_MIDI_CTL_PORTAMENTO           , "Portamento" },
	{ LV2_MIDI_CTL_SOSTENUTO            , "Sostenuto" },
	{ LV2_MIDI_CTL_SOFT_PEDAL           , "SoftPedal" },
	{ LV2_MIDI_CTL_LEGATO_FOOTSWITCH    , "LegatoFootSwitch" },
	{ LV2_MIDI_CTL_HOLD2                , "Hold2" },

	{ LV2_MIDI_CTL_SC1_SOUND_VARIATION  , "SoundVariation" },
	{ LV2_MIDI_CTL_SC3_RELEASE_TIME     , "ReleaseTime" },
	{ LV2_MIDI_CTL_SC2_TIMBRE           , "Timbre" },
	{ LV2_MIDI_CTL_SC4_ATTACK_TIME      , "AttackTime" },
	{ LV2_MIDI_CTL_SC5_BRIGHTNESS       , "Brightness" },
	{ LV2_MIDI_CTL_SC1_SOUND_VARIATION  , "SC1" },
	{ LV2_MIDI_CTL_SC2_TIMBRE           , "SC2" },
	{ LV2_MIDI_CTL_SC3_RELEASE_TIME     , "SC3" },
	{ LV2_MIDI_CTL_SC4_ATTACK_TIME      , "SC4" },
	{ LV2_MIDI_CTL_SC5_BRIGHTNESS       , "SC5" },
	{ LV2_MIDI_CTL_SC6                  , "SC6" },
	{ LV2_MIDI_CTL_SC7                  , "SC7" },
	{ LV2_MIDI_CTL_SC8                  , "SC8" },
	{ LV2_MIDI_CTL_SC9                  , "SC9" },
	{ LV2_MIDI_CTL_SC10                 , "SC10" },

	{ LV2_MIDI_CTL_GENERAL_PURPOSE5     , "GeneralPurpose5" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE6     , "GeneralPurpose6" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE7     , "GeneralPurpose7" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE8     , "GeneralPurpose8" },
	{ LV2_MIDI_CTL_PORTAMENTO_CONTROL   , "PortamentoControl" },

	{ LV2_MIDI_CTL_E1_REVERB_DEPTH      , "ReverbDepth" },
	{ LV2_MIDI_CTL_E2_TREMOLO_DEPTH     , "TremoloDepth" },
	{ LV2_MIDI_CTL_E3_CHORUS_DEPTH      , "ChorusDepth" },
	{ LV2_MIDI_CTL_E4_DETUNE_DEPTH      , "DetuneDepth" },
	{ LV2_MIDI_CTL_E5_PHASER_DEPTH      , "PhaserDepth" },
	{ LV2_MIDI_CTL_E1_REVERB_DEPTH      , "E1" },
	{ LV2_MIDI_CTL_E2_TREMOLO_DEPTH     , "E2" },
	{ LV2_MIDI_CTL_E3_CHORUS_DEPTH      , "E3" },
	{ LV2_MIDI_CTL_E4_DETUNE_DEPTH      , "E4" },
	{ LV2_MIDI_CTL_E5_PHASER_DEPTH      , "E5" },

	{ LV2_MIDI_CTL_DATA_INCREMENT       , "DataIncrement" },
	{ LV2_MIDI_CTL_DATA_DECREMENT       , "DataDecrement" },

	{ LV2_MIDI_CTL_NRPN_LSB             , "NRPN_LSB" },
	{ LV2_MIDI_CTL_NRPN_MSB             , "NRPN_MSB" },

	{ LV2_MIDI_CTL_RPN_LSB              , "RPN_LSB" },
	{ LV2_MIDI_CTL_RPN_MSB              , "RPN_MSB" },

	{ LV2_MIDI_CTL_ALL_SOUNDS_OFF       , "AllSoundsOff" },
	{ LV2_MIDI_CTL_RESET_CONTROLLERS    , "ResetControllers" },
	{ LV2_MIDI_CTL_LOCAL_CONTROL_SWITCH , "LocalControlSwitch" },
	{ LV2_MIDI_CTL_ALL_NOTES_OFF        , "AllNotesOff" },
	{ LV2_MIDI_CTL_OMNI_OFF             , "OmniOff" },
	{ LV2_MIDI_CTL_OMNI_ON              , "OmniOn" },
	{ LV2_MIDI_CTL_MONO1                , "Mono1" },
	{ LV2_MIDI_CTL_MONO2                , "Mono2" },

	{ 0, NULL} // sentinel
};

struct _lobj_t {
	const LV2_Atom_Object *obj;
	const LV2_Atom_Property_Body *itr;
};

struct _ltuple_t {
	const LV2_Atom_Tuple *tuple;
	int pos;
	const LV2_Atom *itr;
};

struct _lvec_t {
	const LV2_Atom_Vector *vec;
	int count;
	int pos;
};

struct _latom_t {
	union {
		const LV2_Atom *atom;

		const LV2_Atom_Int *i32;
		const LV2_Atom_Long *i64;
		const LV2_Atom_Float *f32;
		const LV2_Atom_Double *f64;
		const LV2_Atom_URID *u32;
		const LV2_Atom_Literal *lit;

		lseq_t seq;
		lobj_t obj;
		ltuple_t tuple;
		lvec_t vec;
	};

	LV2_Atom body [0];
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

static const char *forge_buffer_overflow = "forge buffer overflow";

static inline void
_latom_new(lua_State *L, const LV2_Atom *atom)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	latom_t *latom = moony_newuserdata(L, moony, MOONY_UDATA_ATOM);
	latom->atom = atom;
}

static inline void
_latom_body_new(lua_State *L, uint32_t size, LV2_URID type, const void *body)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	size_t atom_size = sizeof(LV2_Atom) + size;

	latom_t *latom = lua_newuserdata(L, sizeof(latom_t) + atom_size);
	latom->atom = (const LV2_Atom *)latom->body;
	latom->body->size = size;
	latom->body->type = type;
	memcpy(LV2_ATOM_BODY(latom->body), body, size);
	luaL_getmetatable(L, "latom");

	lua_setmetatable(L, -2);
}

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

static inline const latom_driver_t *
_latom_driver(moony_t *moony, LV2_URID type)
{
	const latom_driver_hash_t *base = moony->atom_driver_hash;
	size_t lim;
	const latom_driver_hash_t *p;

	for(lim = DRIVER_HASH_MAX; lim != 0; lim >>= 1)
	{
		p = base + (lim >> 1);

		if(type == p->type)
			return p->driver;

		if(type > p->type)
		{
			base = p + 1;
			lim--;
		}
	}

	return NULL;
}

static void
_latom_value(lua_State *L, const LV2_Atom *atom)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	const latom_driver_t *driver = _latom_driver(moony, atom->type);

	// dummy wrapping
	latom_t latom = {
		.atom = atom
	};

	if(driver && driver->value)
		driver->value(L, &latom);
	else
		lua_pushnil(L); // unknown type
}

// Int driver
static int
_latom_int__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_int__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "%i", latom->i32->body);
	return 1;
}

static inline int
_latom_int_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->i32->body);
	return 1;
}

static int
_latom_int_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	return _latom_int_value(L, latom);
}

static const latom_driver_t latom_int_driver = {
	.__len = _latom_int__len,
	.__tostring = _latom_int__tostring,
	.value = _latom_int_value,
	.unpack = _latom_int_unpack
};

// Long driver
static int
_latom_long__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_long__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "%li", latom->i64->body);
	return 1;
}

static inline int
_latom_long_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->i64->body);
	return 1;
}

static int
_latom_long_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	return _latom_long_value(L, latom);
}

static const latom_driver_t latom_long_driver = {
	.__len = _latom_long__len,
	.__tostring = _latom_long__tostring,
	.value = _latom_long_value,
	.unpack = _latom_long_unpack
};

// Float driver
static int
_latom_float__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_float__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "%f", latom->f32->body);
	return 1;
}

static inline int
_latom_float_value(lua_State *L, latom_t *latom)
{
	lua_pushnumber(L, latom->f32->body);
	return 1;
}

static int
_latom_float_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	return _latom_float_value(L, latom);
}

static const latom_driver_t latom_float_driver = {
	.__len = _latom_float__len,
	.__tostring = _latom_float__tostring,
	.value = _latom_float_value,
	.unpack = _latom_float_unpack
};

// Double driver
static int
_latom_double__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_double__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "%lf", latom->f64->body);
	return 1;
}

static inline int
_latom_double_value(lua_State *L, latom_t *latom)
{
	lua_pushnumber(L, latom->f64->body);
	return 1;
}

static int
_latom_double_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	return _latom_double_value(L, latom);
}

static const latom_driver_t latom_double_driver = {
	.__len = _latom_double__len,
	.__tostring = _latom_double__tostring,
	.value = _latom_double_value,
	.unpack = _latom_double_unpack
};

// Bool driver
static int
_latom_bool__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_bool__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "%s", latom->i32->body ? "true" : "false");
	return 1;
}

static inline int
_latom_bool_value(lua_State *L, latom_t *latom)
{
	lua_pushboolean(L, latom->i32->body);
	return 1;
}

static int
_latom_bool_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	return _latom_bool_value(L, latom);
}

static const latom_driver_t latom_bool_driver = {
	.__len = _latom_bool__len,
	.__tostring = _latom_bool__tostring,
	.value = _latom_bool_value,
	.unpack = _latom_bool_unpack
};

// URID driver
static int
_latom_urid__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_urid__tostring(lua_State *L, latom_t *latom)
{
	lua_pushfstring(L, "%u", latom->u32->body);
	return 1;
}

static inline int
_latom_urid_value(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->u32->body);
	return 1;
}

static int
_latom_urid_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	return _latom_urid_value(L, latom);
}

static const latom_driver_t latom_urid_driver = {
	.__len = _latom_urid__len,
	.__tostring = _latom_urid__tostring,
	.value = _latom_urid_value,
	.unpack = _latom_urid_unpack
};

// String driver
static int
_latom_string__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static inline int
_latom_string_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L, LV2_ATOM_BODY_CONST(latom->atom), latom->atom->size - 1);
	return 1;
}

static int
_latom_string_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	return _latom_string_value(L, latom);
}

static const latom_driver_t latom_string_driver = {
	.__len = _latom_string__len,
	.__tostring = _latom_string_value,
	.value = _latom_string_value,
	.unpack = _latom_string_unpack
};

// Literal driver
static int
_latom_literal__indexk(lua_State *L, latom_t *latom)
{
	const char *key = lua_tostring(L, 2);
	if(!strcmp(key, "datatype"))
		lua_pushinteger(L, latom->lit->body.datatype);
	else if(!strcmp(key, "lang"))
		lua_pushinteger(L, latom->lit->body.lang);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_literal_value(lua_State *L, latom_t *latom)
{
	lua_pushlstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, latom->atom), latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	return 1;
}

static int
_latom_literal_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	lua_pushlstring(L, LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, latom->atom), latom->atom->size - 1 - sizeof(LV2_Atom_Literal_Body));
	lua_pushinteger(L, latom->lit->body.datatype);
	lua_pushinteger(L, latom->lit->body.lang);
	return 1;
}

static const latom_driver_t latom_literal_driver = {
	.__indexk = _latom_literal__indexk,
	.__len = _latom_string__len,
	.__tostring = _latom_literal_value,
	.value = _latom_literal_value,
	.unpack = _latom_literal_unpack
};

static int
_latom_tuple__indexi(lua_State *L, latom_t *latom)
{
	ltuple_t *ltuple = &latom->tuple;
	const int idx = lua_tointeger(L, 2);

	int count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
	{
		if(++count == idx)
		{
			_latom_new(L, atom);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

static int
_latom_tuple__len(lua_State *L, latom_t *latom)
{
	ltuple_t *ltuple = &latom->tuple;

	int count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_tuple__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(tuple)");
	return 1;
}

static int
_latom_tuple_unpack(lua_State *L)
{
	ltuple_t *ltuple = lua_touserdata(L, 1);

	int n = lua_gettop(L);
	int min = n > 1
		? luaL_checkinteger(L, 2)
		: 1;
	int max = n > 2
		? luaL_checkinteger(L, 3)
		: INT_MAX;

	int pos = 1;
	int count = 0;
	LV2_ATOM_TUPLE_FOREACH(ltuple->tuple, atom)
	{
		if(pos >= min)
		{
			if(pos <= max)
			{
				_latom_new(L, atom);
				count += 1;
			}
			else
				break;
		}

		pos += 1;
	}

	return count;
}

static int
_latom_tuple_foreach_itr(lua_State *L)
{
	ltuple_t *ltuple = lua_touserdata(L, 1);

	if(!lv2_atom_tuple_is_end(LV2_ATOM_BODY(ltuple->tuple), ltuple->tuple->atom.size, ltuple->itr))
	{
		// push index
		lua_pushinteger(L, ltuple->pos);

		// push atom
		_latom_new(L, ltuple->itr);
	
		// advance iterator
		ltuple->pos += 1;
		ltuple->itr = lv2_atom_tuple_next(ltuple->itr);

		return 2;
	}

	// end of tuple reached
	lua_pushnil(L);
	return 1;
}

static int
_latom_tuple_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	ltuple_t *ltuple = lua_touserdata(L, 1);

	// reset iterator to beginning of tuple
	ltuple->pos = 1;
	ltuple->itr = lv2_atom_tuple_begin(ltuple->tuple);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_tuple_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const latom_driver_t latom_tuple_driver = {
	.__indexi = _latom_tuple__indexi,
	.__len = _latom_tuple__len,
	.__tostring = _latom_tuple__tostring,
	.unpack = _latom_tuple_unpack,
	.foreach = _latom_tuple_foreach
};

static int
_latom_obj__indexi(lua_State *L, latom_t *latom)
{
	lobj_t *lobj = &latom->obj;
	const LV2_URID urid = lua_tointeger(L, 2);

	// start a query for given URID
	const LV2_Atom *atom = NULL;
	LV2_Atom_Object_Query q [] = {
		{ urid, &atom },
		{	0, NULL }
	};
	lv2_atom_object_query(lobj->obj, q);

	if(atom) // query returned a matching atom
		_latom_new(L, atom);
	else // query returned no matching atom
		lua_pushnil(L);

	return 1;
}

static int
_latom_obj__indexk(lua_State *L, latom_t *latom)
{
	const char *key = lua_tostring(L, 2);
	if(!strcmp(key, "id"))
		lua_pushinteger(L, latom->obj.obj->body.id);
	else if(!strcmp(key, "otype"))
		lua_pushinteger(L, latom->obj.obj->body.otype);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_obj__len(lua_State *L, latom_t *latom)
{
	lobj_t *lobj = &latom->obj;

	int count = 0;
	LV2_ATOM_OBJECT_FOREACH(lobj->obj, prop)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_obj__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(object)");
	return 1;
}

static int
_latom_obj_foreach_itr(lua_State *L)
{
	lobj_t *lobj = lua_touserdata(L, 1);

	if(!lv2_atom_object_is_end(&lobj->obj->body, lobj->obj->atom.size, lobj->itr))
	{
		// push atom
		lua_pushinteger(L, lobj->itr->key);
		lua_pushinteger(L, lobj->itr->context);
		_latom_new(L, &lobj->itr->value);
	
		// advance iterator
		lobj->itr = lv2_atom_object_next(lobj->itr);

		return 3;
	}

	// end of object reached
	lua_pushnil(L);
	return 1;
}

static int
_latom_obj_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lobj_t *lobj = lua_touserdata(L, 1);

	// reset iterator to beginning of object
	lobj->itr = lv2_atom_object_begin(&lobj->obj->body);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_obj_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const latom_driver_t latom_object_driver = {
	.__indexi = _latom_obj__indexi,
	.__indexk = _latom_obj__indexk,
	.__len = _latom_obj__len,
	.__tostring = _latom_obj__tostring,
	.foreach = _latom_obj_foreach
};

static int
_latom_seq__indexi(lua_State *L, latom_t *latom)
{
	lseq_t *lseq = lua_touserdata(L, 1);
	int index = lua_tointeger(L, 2); // indexing start from 1

	int count = 0;
	LV2_ATOM_SEQUENCE_FOREACH(lseq->seq, ev)
	{
		if(++count == index) 
		{
			_latom_new(L, &ev->body);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

static int
_latom_seq__len(lua_State *L, latom_t *latom)
{
	lseq_t *lseq = lua_touserdata(L, 1);

	int count = 0;
	LV2_ATOM_SEQUENCE_FOREACH(lseq->seq, ev)
		++count;

	lua_pushinteger(L, count);
	return 1;
}

static int
_latom_seq__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(sequence)");
	return 1;
}

static int
_latom_seq_foreach_itr(lua_State *L)
{
	lseq_t *lseq = lua_touserdata(L, 1);

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
_latom_seq_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lseq_t *lseq = lua_touserdata(L, 1);

	// reset iterator to beginning of sequence
	lseq->itr = lv2_atom_sequence_begin(&lseq->seq->body);

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_seq_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const latom_driver_t latom_sequence_driver = {
	.__indexi = _latom_seq__indexi,
	.__len = _latom_seq__len,
	.__tostring = _latom_seq__tostring,
	.foreach = _latom_seq_foreach
};

static int
_latom_vec__indexi(lua_State *L, latom_t *latom)
{
	lvec_t *lvec = lua_touserdata(L, 1);
	int index = lua_tointeger(L, 2); // indexing start from 1

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

	if( (index > 0) && (index <= lvec->count) )
	{
		_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
			LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, index - 1));
	}
	else // index is out of bounds
		lua_pushnil(L);

	return 1;
}

static int
_latom_vec__indexk(lua_State *L, latom_t *latom)
{
	const char *key = lua_tostring(L, 2);
	if(!strcmp(key, "child_type"))
		lua_pushinteger(L, latom->vec.vec->body.child_type);
	else if(!strcmp(key, "child_size"))
		lua_pushinteger(L, latom->vec.vec->body.child_size);
	else
		lua_pushnil(L);
	return 1;
}

static int
_latom_vec__len(lua_State *L, latom_t *latom)
{
	lvec_t *lvec = lua_touserdata(L, 1);

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

	lua_pushinteger(L, lvec->count);
	return 1;
}

static int
_latom_vec__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(vector)");
	return 1;
}

static int
_latom_vec_unpack(lua_State *L)
{
	lvec_t *lvec = lua_touserdata(L, 1);

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

	int n = lua_gettop(L);
	int min = 1;
	int max = lvec->count;

	if(n > 1) // check provided index ranges
	{
		min = luaL_checkinteger(L, 2);
		min = min < 1
			? 1
			: (min > lvec->count
				? lvec->count
				: min);

		if(n > 2)
		{
			max = luaL_checkinteger(L, 3);
			max = max < 1
				? 1
				: (max > lvec->count
					? lvec->count
					: max);
		}
	}

	for(int i=min; i<=max; i++)
	{
		_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
			LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, i-1));
	}

	return max - min + 1;
}

static int
_latom_vec_foreach_itr(lua_State *L)
{
	lvec_t *lvec = lua_touserdata(L, 1);

	lvec->count = (lvec->vec->atom.size - sizeof(LV2_Atom_Vector_Body))
		/ lvec->vec->body.child_size;

	if(lvec->pos < lvec->count)
	{
		// push index
		lua_pushinteger(L, lvec->pos + 1);

		// push atom
		_latom_body_new(L, lvec->vec->body.child_size, lvec->vec->body.child_type,
			LV2_ATOM_VECTOR_ITEM_CONST(lvec->vec, lvec->pos));

		// advance iterator
		lvec->pos += 1;

		return 2;
	}

	// end of vector reached
	lua_pushnil(L);
	return 1;
}

static int
_latom_vec_foreach(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lvec_t *lvec = lua_touserdata(L, 1);

	// reset iterator to beginning of vector
	lvec->pos = 0;

	lua_pushlightuserdata(L, moony);
	lua_pushcclosure(L, _latom_vec_foreach_itr, 1);
	lua_pushvalue(L, 1);

	return 2;
}

static const latom_driver_t latom_vector_driver = {
	.__indexi = _latom_vec__indexi,
	.__indexk = _latom_vec__indexk,
	.__len = _latom_vec__len,
	.__tostring = _latom_vec__tostring,
	.unpack = _latom_vec_unpack,
	.foreach = _latom_vec_foreach
};

static int
_latom_chunk__indexi(lua_State *L, latom_t *latom)
{
	const uint8_t *payload = LV2_ATOM_BODY_CONST(latom->atom);
	int index = lua_tointeger(L, 2); // indexing start from 1

	if( (index > 0) && (index <= (int)latom->atom->size) )
		lua_pushinteger(L, payload[index-1]);
	else // index is out of bounds
		lua_pushnil(L);

	return 1;
}

static int
_latom_chunk__len(lua_State *L, latom_t *latom)
{
	lua_pushinteger(L, latom->atom->size);
	return 1;
}

static int
_latom_chunk__tostring(lua_State *L, latom_t *latom)
{
	lua_pushstring(L, "(chunk)");
	return 1;
}

static int
_latom_chunk_value(lua_State *L, latom_t *latom)
{
	const uint8_t *payload = LV2_ATOM_BODY_CONST(latom->atom);

	lua_newtable(L);
	for(unsigned i=0; i<latom->atom->size; i++)
	{
		lua_pushinteger(L, payload[i]);
		lua_rawseti(L, -2, i+1);
	}

	return 1;
}

static int
_latom_chunk_unpack(lua_State *L)
{
	latom_t *latom = lua_touserdata(L, 1);
	const uint8_t *payload = LV2_ATOM_BODY_CONST(latom->atom);

	int n = lua_gettop(L);
	int min = 1;
	int max = latom->atom->size;

	if(n > 1) // check provided index ranges
	{
		min = luaL_checkinteger(L, 2);
		min = min < 1
			? 1
			: (min > (int)latom->atom->size
				? (int)latom->atom->size
				: min);

		if(n > 2)
		{
			max = luaL_checkinteger(L, 3);
			max = max < 1
				? 1
				: (max > (int)latom->atom->size
					? (int)latom->atom->size
					: max);
		}
	}

	for(int i=min; i<=max; i++)
		lua_pushinteger(L, payload[i-1]);

	return max - min + 1;
}

static const latom_driver_t latom_chunk_driver = {
	.__indexi = _latom_chunk__indexi,
	.__len = _latom_chunk__len,
	.__tostring = _latom_chunk__tostring,
	.value = _latom_chunk_value,
	.unpack = _latom_chunk_unpack,
};

static int
_latom__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver)
	{
		const int type = lua_type(L, 2);
		if(type == LUA_TSTRING)
		{
			const char *key = lua_tostring(L, 2);
			if(!strcmp(key, "type"))
			{
				lua_pushinteger(L, latom->atom->type);
				return 1;
			}
			else if(driver->value && !strcmp(key, "value"))
			{
				return driver->value(L, latom);
			}
			else if(driver->foreach && !strcmp(key, "foreach"))
			{
				lua_pushlightuserdata(L, moony);
				lua_pushcclosure(L, driver->foreach, 1);
				return 1;
			}
			else if(driver->unpack && !strcmp(key, "unpack"))
			{
				lua_pushlightuserdata(L, moony);
				lua_pushcclosure(L, driver->unpack, 1);
				return 1;
			}
			else if(driver->__indexk)
			{
				return driver->__indexk(L, latom);
			}
		}
		else if(driver->__indexi && (type == LUA_TNUMBER) )
		{
			return driver->__indexi(L, latom);
		}
	}

	lua_pushnil(L);
	return 1;
}

static int
_latom__len(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver && driver->__len)
		return driver->__len(L, latom);

	lua_pushnil(L);
	return 1;
}

static int
_latom__tostring(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	latom_t *latom = luaL_checkudata(L, 1, "latom");
	const latom_driver_t *driver = _latom_driver(moony, latom->atom->type);

	if(driver && driver->__tostring)
		return driver->__tostring(L, latom);

	lua_pushnil(L);
	return 1;
}

static const luaL_Reg latom_mt [] = {
	{"__index", _latom__index},
	{"__len", _latom__len},
	{"__tostring", _latom__tostring},

	{NULL, NULL}
};

static inline int
_lforge_frame_time_inlined(lua_State *L, lforge_t *lforge, int64_t frames)
{
	if(frames >= lforge->last.frames)
	{
		if(!lv2_atom_forge_frame_time(lforge->forge, frames))
			luaL_error(L, forge_buffer_overflow);
		lforge->last.frames = frames;

		lua_settop(L, 1);
		return 1;
	}

	return luaL_error(L, "invalid frame time, must not decrease");
}

static int
_lforge_frame_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	int64_t frames = luaL_checkinteger(L, 2);

	return _lforge_frame_time_inlined(L, lforge, frames);
}

static inline int
_lforge_beat_time_inlined(lua_State *L, lforge_t *lforge, double beats)
{
	if(beats >= lforge->last.beats)
	{
		if(!lv2_atom_forge_beat_time(lforge->forge, beats))
			luaL_error(L, forge_buffer_overflow);
		lforge->last.beats = beats;

		lua_settop(L, 1);
		return 1;
	}

	return luaL_error(L, "invalid beat time, must not decrease");
}

static int
_lforge_beat_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	double beats = luaL_checknumber(L, 2);

	return _lforge_beat_time_inlined(L, lforge, beats);
}

static int
_lforge_time(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(lua_isinteger(L, 2))
	{
		int64_t frames = lua_tointeger(L, 2);

		return _lforge_frame_time_inlined(L, lforge, frames);
	}
	else if(lua_isnumber(L, 2))
	{
		double beats = lua_tointeger(L, 2);

		return _lforge_beat_time_inlined(L, lforge, beats);
	}

	return luaL_error(L, "integer or number expected");
}

static int
_lforge_atom(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	latom_t *latom = luaL_checkudata(L, 2, "latom");

	if(!lv2_atom_forge_raw(lforge->forge, latom->atom, sizeof(LV2_Atom) + latom->atom->size))
		luaL_error(L, forge_buffer_overflow);
	lv2_atom_forge_pad(lforge->forge, latom->atom->size);

	lua_settop(L, 1);
	return 1;
}

static inline int
_lforge_basic_int(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	int32_t val = luaL_checkinteger(L, pos);
	return lv2_atom_forge_int(forge, val);
}

static inline int
_lforge_basic_long(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	int64_t val = luaL_checkinteger(L, pos);
	return lv2_atom_forge_long(forge, val);
}

static inline int
_lforge_basic_float(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	float val = luaL_checknumber(L, pos);
	return lv2_atom_forge_float(forge, val);
}

static inline int
_lforge_basic_double(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	double val = luaL_checknumber(L, pos);
	return lv2_atom_forge_double(forge, val);
}

static inline int
_lforge_basic_bool(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	int32_t val = lua_toboolean(L, pos);
	return lv2_atom_forge_bool(forge, val);
}

static inline int
_lforge_basic_urid(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	LV2_URID val = luaL_checkinteger(L, pos);
	return lv2_atom_forge_urid(forge, val);
}

static inline int
_lforge_basic_string(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_string(forge, val, len);
}

static inline int
_lforge_basic_uri(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_uri(forge, val, len);
}

static inline int
_lforge_basic_path(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_path(forge, val, len);
}

static inline int
_lforge_basic_literal(lua_State *L, int pos, LV2_Atom_Forge *forge)
{
	size_t len;
	const char *val = luaL_checklstring(L, pos, &len);
	return lv2_atom_forge_literal(forge, val, len, 0, 0); //TODO context, lang
}

static inline int
_lforge_basic(lua_State *L, int pos, LV2_Atom_Forge *forge, LV2_URID range)
{
	if(range == forge->Int)
		return _lforge_basic_int(L, pos, forge);
	else if(range == forge->Long)
		return _lforge_basic_long(L, pos, forge);
	else if(range == forge->Float)
		return _lforge_basic_float(L, pos, forge);
	else if(range == forge->Double)
		return _lforge_basic_double(L, pos, forge);
	else if(range == forge->Bool)
		return _lforge_basic_bool(L, pos, forge);
	else if(range == forge->URID)
		return _lforge_basic_urid(L, pos, forge);
	else if(range == forge->String)
		return _lforge_basic_string(L, pos, forge);
	else if(range == forge->URI)
		return _lforge_basic_uri(L, pos, forge);
	else if(range == forge->Path)
		return _lforge_basic_path(L, pos, forge);
	else if(range == forge->Literal)
		return _lforge_basic_literal(L, pos, forge);

	return luaL_error(L, "not a basic type");
}

static int
_lforge_int(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_int(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_long(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_long(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_float(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_float(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_double(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_double(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_bool(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_bool(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_urid(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_urid(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_string(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_string(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_uri(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_uri(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_path(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	if(!_lforge_basic_path(L, 2, lforge->forge))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_literal(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	size_t size;
	const char *val = luaL_checklstring(L, 2, &size);
	LV2_URID datatype = luaL_checkinteger(L, 3);
	LV2_URID lang = luaL_checkinteger(L, 4);

	if(!lv2_atom_forge_literal(lforge->forge, val, size, datatype, lang))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_bytes(lua_State *L, moony_t *moony, LV2_URID type)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	if(lua_istable(L, 2))
	{
		int size = lua_rawlen(L, 2);
		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		for(int i=1; i<=size; i++)
		{
			lua_rawgeti(L, -1, i);
			uint8_t byte = luaL_checkinteger(L, -1);
			lua_pop(L, 1);
			if(!lv2_atom_forge_raw(lforge->forge, &byte, 1))
				luaL_error(L, forge_buffer_overflow);
		}
		lv2_atom_forge_pad(lforge->forge, size);
	}
	else if(luaL_testudata(L, 2, "latom")) //to convert between chunk <-> midi
	{
		latom_t *lchunk = lua_touserdata(L, 2);
		uint32_t size = lchunk->atom->size;
		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_raw(lforge->forge, LV2_ATOM_BODY_CONST(lchunk->atom), size))
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pad(lforge->forge, size);
	}
	else // bytes as individual function arguments
	{
		int size = lua_gettop(L) - 1;

		if(!lv2_atom_forge_atom(lforge->forge, size, type))
			luaL_error(L, forge_buffer_overflow);
		for(int i=0; i<size; i++)
		{
			uint8_t byte = luaL_checkinteger(L, i+2);
			if(!lv2_atom_forge_raw(lforge->forge, &byte, 1))
				luaL_error(L, forge_buffer_overflow);
		}
		lv2_atom_forge_pad(lforge->forge, size);
	}

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_chunk(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	return _lforge_bytes(L, moony, moony->forge.Chunk);
}

static int
_lforge_midi(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	return _lforge_bytes(L, moony, moony->uris.midi_event);
}

static inline uint64_t
_lforge_to_timestamp(lua_State *L, moony_t *moony, lforge_t *lforge, int pos)
{
	uint64_t timestamp = 1ULL; // immediate timestamp
	if(lua_isinteger(L, pos))
	{
		// absolute timestamp
		timestamp = lua_tointeger(L, pos);
	}
	else if(lua_isnumber(L, pos) && moony->osc_sched)
	{
		// timestamp of current frame
		timestamp = moony->osc_sched->frames2osc(moony->osc_sched->handle,
			lforge->last.frames);
		volatile uint64_t sec = timestamp >> 32;
		volatile uint64_t frac = timestamp & 0xffffffff;
		
		// relative offset from current frame (in seconds)
		double offset_d = lua_tonumber(L, pos);
		double secs_d;
		double frac_d = modf(offset_d, &secs_d);

		// add relative offset to timestamp
		sec += secs_d;
		frac += frac_d * 0x1p32;
		if(frac >= 0x100000000ULL) // overflow
		{
			sec += 1;
			frac -= 0x100000000ULL;
		}
		timestamp = (sec << 32) | frac;

		/*
		// debug
		uint64_t t0 = moony->osc_sched->frames2osc(moony->osc_sched->handle, lforge->last.frames);
		uint64_t dt = timestamp - t0;
		double dd = dt * 0x1p-32;
		printf("%lu %lf\n", dt, dd);
		*/
	}

	return timestamp;
}

static int
_lforge_osc_bundle(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	osc_forge_t *oforge = &moony->oforge;
	LV2_Atom_Forge *forge = lforge->forge;

	uint64_t timestamp = _lforge_to_timestamp(L, moony, lforge, 2);

	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 2;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!osc_forge_bundle_push(oforge, forge, lframe->frame, timestamp))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_osc_message(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	osc_forge_t *oforge = &moony->oforge;
	LV2_Atom_Forge *forge = lforge->forge;

	const char *path = luaL_checkstring(L, 2);
	const char *fmt = luaL_optstring(L, 3, "");

	LV2_Atom_Forge_Frame frame [2];

	if(!osc_forge_message_push(oforge, forge, frame, path, fmt))
		luaL_error(L, forge_buffer_overflow);

	int pos = 4;
	for(const char *type = fmt; *type; type++)
	{
		switch(*type)
		{
			case 'i':
			{
				int32_t i = luaL_checkinteger(L, pos++);
				if(!osc_forge_int32(oforge, forge, i))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'f':
			{
				float f = luaL_checknumber(L, pos++);
				if(!osc_forge_float(oforge, forge, f))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 's':
			case 'S':
			{
				const char *s = luaL_checkstring(L, pos++);
				if(!osc_forge_string(oforge, forge, s))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'b':
			{
				if(lua_istable(L, pos))
				{
					int n = lua_rawlen(L, pos);

					if(!lv2_atom_forge_atom(forge, n, forge->Chunk))
						luaL_error(L, forge_buffer_overflow);
					for(int i=1; i<=n; i++)
					{
						lua_rawgeti(L, pos, i);
						uint8_t b = luaL_checkinteger(L, -1);
						lua_pop(L, 1);
						if(!lv2_atom_forge_raw(forge, &b, 1))
							luaL_error(L, forge_buffer_overflow);
					}
					lv2_atom_forge_pad(forge, n);
				}
				pos += 1;

				break;
			}

			case 'T':
			case 'F':
			case 'N':
			case 'I':
			{
				break;
			}

			case 'h':
			{
				int64_t h = luaL_checkinteger(L, pos++);
				if(!osc_forge_int64(oforge, forge, h))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'd':
			{
				double d = luaL_checknumber(L, pos++);
				if(!osc_forge_double(oforge, forge, d))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 't':
			{
				uint64_t t = _lforge_to_timestamp(L, moony, lforge, pos++);
				if(!osc_forge_timestamp(oforge, forge, t))
					luaL_error(L, forge_buffer_overflow);
				break;
			}

			case 'c':
			{
				char c = luaL_checkinteger(L, pos++);
				if(!osc_forge_char(oforge, forge, c))
					luaL_error(L, forge_buffer_overflow);
				break;
			}
			case 'm':
			{
				if(lua_istable(L, pos))
				{
					int n = lua_rawlen(L, pos);

					if(!lv2_atom_forge_atom(forge, n, moony->oforge.MIDI_MidiEvent))
						luaL_error(L, forge_buffer_overflow);
					for(int i=1; i<=n; i++)
					{
						lua_rawgeti(L, pos, i);
						uint8_t b = luaL_checkinteger(L, -1);
						lua_pop(L, 1);
						if(!lv2_atom_forge_raw(forge, &b, 1))
							luaL_error(L, forge_buffer_overflow);
					}
					lv2_atom_forge_pad(forge, n);
				}
				pos += 1;

				break;
			}
		}
	}

	osc_forge_message_pop(oforge, forge, frame);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_tuple(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_tuple(lforge->forge, &lframe->frame[0]))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_object(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID id = luaL_optinteger(L, 2, 0);
	LV2_URID otype = luaL_optinteger(L, 3, 0);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, &lframe->frame[0], id, otype))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_key(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);

	if(!lv2_atom_forge_key(lforge->forge, key))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_property(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID key = luaL_checkinteger(L, 2);
	LV2_URID context = luaL_checkinteger(L, 3);

	if(!lv2_atom_forge_property_head(lforge->forge, key, context))
		luaL_error(L, forge_buffer_overflow);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_vector(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID child_type = luaL_checkinteger(L, 2);
	uint32_t child_size = 0;
	LV2_Atom_Forge_Frame frame;

	int is_table = lua_istable(L, 3);

	int n = is_table
		? (int)lua_rawlen(L, 3)
		: lua_gettop(L) - 2;

	if(  (child_type == lforge->forge->Int)
		|| (child_type == lforge->forge->URID) )
	{
		child_size = sizeof(int32_t);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			int32_t v = luaL_checkinteger(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Bool)
	{
		child_size = sizeof(int32_t);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			int32_t v = lua_toboolean(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Long)
	{
		child_size = sizeof(int64_t);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			int64_t v = luaL_checkinteger(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Float)
	{
		child_size = sizeof(float);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			float v = luaL_checknumber(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else if(child_type == lforge->forge->Double)
	{
		child_size = sizeof(double);
		if(!lv2_atom_forge_vector_head(lforge->forge, &frame, child_size, child_type))
			luaL_error(L, forge_buffer_overflow);

		for(int i=1; i<=n; i++)
		{
			if(is_table)
				lua_rawgeti(L, 3, i);
			double v = luaL_checknumber(L, is_table ? -1 : i+2);
			if(is_table)
				lua_pop(L, 1);

			if(!lv2_atom_forge_raw(lforge->forge, &v, child_size))
				luaL_error(L, forge_buffer_overflow);
		}
	}
	else
		luaL_error(L, "vector supports only fixed sized atoms (e.g. Int, Long, Float, Double, URID, Bool)");

	if(&frame != lforge->forge->stack) // intercept assert
		luaL_error(L, "forge frame mismatch");
	lv2_atom_forge_pop(lforge->forge, &frame);
	lv2_atom_forge_pad(lforge->forge, n*child_size);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_sequence(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID unit = luaL_optinteger(L, 2, 0); //TODO use proper unit
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = 0;
	lframe->forge = lforge->forge;
	
	if(!lv2_atom_forge_sequence_head(lforge->forge, &lframe->frame[0], unit))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_typed(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID urid = luaL_checkinteger(L, 2);
	lua_remove(L, 2); // urid

	lua_CFunction hook = NULL;

	//TODO keep this list updated
	//TODO use binary tree sorted by URID
	if(urid == lforge->forge->Int)
		hook = _lforge_int;
	else if(urid == lforge->forge->Long)
		hook = _lforge_long;
	else if(urid == lforge->forge->Float)
		hook = _lforge_float;
	else if(urid == lforge->forge->Double)
		hook = _lforge_double;
	else if(urid == lforge->forge->Bool)
		hook = _lforge_bool;
	else if(urid == lforge->forge->URID)
		hook = _lforge_urid;
	else if(urid == lforge->forge->String)
		hook = _lforge_string;
	else if(urid == lforge->forge->Literal)
		hook = _lforge_literal;
	else if(urid == lforge->forge->URI)
		hook = _lforge_uri;
	else if(urid == lforge->forge->Path)
		hook = _lforge_path;

	else if(urid == lforge->forge->Chunk)
		hook = _lforge_chunk;
	else if(urid == moony->uris.midi_event)
		hook = _lforge_midi;
	else if(urid == moony->oforge.OSC_Bundle)
		hook = _lforge_osc_bundle;
	else if(urid == moony->oforge.OSC_Message)
		hook = _lforge_osc_message;
	else if(urid == lforge->forge->Tuple)
		hook = _lforge_tuple;
	else if(urid == lforge->forge->Object)
		hook = _lforge_object;
	else if(urid == lforge->forge->Property)
		hook = _lforge_property;
	else if(urid == lforge->forge->Vector)
		hook = _lforge_vector;
	else if(urid == lforge->forge->Sequence)
		hook = _lforge_sequence;

	if(!hook)
		return luaL_error(L, "unknown atom type");

	return hook(L);
}

static int
_lforge_get(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	LV2_URID property = luaL_checkinteger(L, 3);

	LV2_Atom_Forge_Frame frame;

	if(!lv2_atom_forge_object(lforge->forge, &frame, 0, moony->uris.patch_get))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_property))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_urid(lforge->forge, property))
		luaL_error(L, forge_buffer_overflow);

	lv2_atom_forge_pop(lforge->forge, &frame);

	lua_settop(L, 1);
	return 1;
}

static int
_lforge_set(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	LV2_URID property = luaL_checkinteger(L, 3);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, moony->uris.patch_set))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_property))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_urid(lforge->forge, property))
		luaL_error(L, forge_buffer_overflow);

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_value))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_put(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 2;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, &lframe->frame[0], 0, moony->uris.patch_put))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_body))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_object(lforge->forge, &lframe->frame[1], 0, 0))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_patch(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	LV2_URID subject = luaL_optinteger(L, 2, 0);
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, moony->uris.patch_patch))
		luaL_error(L, forge_buffer_overflow);

	if(subject) // is optional
	{
		if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject))
			luaL_error(L, forge_buffer_overflow);
		if(!lv2_atom_forge_urid(lforge->forge, subject))
			luaL_error(L, forge_buffer_overflow);
	}

	return 1; // derived forge
}

static int
_lforge_remove(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_remove))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, 0))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_add(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");
	lforge_t *lframe = moony_newuserdata(L, moony, MOONY_UDATA_FORGE);
	lframe->depth = 1;
	lframe->last.frames = lforge->last.frames;
	lframe->forge = lforge->forge;

	if(!lv2_atom_forge_key(lforge->forge, moony->uris.patch_add))
		luaL_error(L, forge_buffer_overflow);
	if(!lv2_atom_forge_object(lforge->forge, lframe->frame, 0, 0))
		luaL_error(L, forge_buffer_overflow);

	return 1; // derived forge
}

static int
_lforge_pop(lua_State *L)
{
	lforge_t *lforge = luaL_checkudata(L, 1, "lforge");

	for(int i=lforge->depth; i>0; i--)
	{
		if(&lforge->frame[i-1] != lforge->forge->stack) // intercept assert
			luaL_error(L, "forge frame mismatch");
		lv2_atom_forge_pop(lforge->forge, &lforge->frame[i-1]);
	}
	lforge->depth = 0; // reset depth

	return 0;
}

static int
_lstash_clear(lua_State *L)
{
	lstash_t *lstash = luaL_checkudata(L, 1, "lforge");

	LV2_Atom *atom = (LV2_Atom *)lstash->ser.buf;
	atom->type = 0;
	atom->size = 0;
	lstash->ser.offset = 0;

	return 1;
}

static int
_lstash_handle(lua_State *L)
{
	lstash_t *lstash = luaL_checkudata(L, 1, "lforge");

	const LV2_Atom *atom = (const LV2_Atom *)lstash->ser.buf;
	_latom_new(L, atom);

	return 1;
}

static const luaL_Reg lforge_mt [] = {
	{"frame_time", _lforge_frame_time},
	{"beat_time", _lforge_beat_time},
	{"time", _lforge_time},

	{"atom", _lforge_atom},

	{"int", _lforge_int},
	{"long", _lforge_long},
	{"float", _lforge_float},
	{"double", _lforge_double},
	{"bool", _lforge_bool},
	{"urid", _lforge_urid},
	{"string", _lforge_string},
	{"literal", _lforge_literal},
	{"uri", _lforge_uri},
	{"path", _lforge_path},

	{"chunk", _lforge_chunk},
	{"midi", _lforge_midi},
	{"bundle", _lforge_osc_bundle},
	{"message", _lforge_osc_message},
	{"tuple", _lforge_tuple},
	{"object", _lforge_object},
	{"key", _lforge_key},
	{"property", _lforge_property},
	{"vector", _lforge_vector},
	{"sequence", _lforge_sequence},

	{"typed", _lforge_typed},

	{"get", _lforge_get},
	{"set", _lforge_set},
	{"put", _lforge_put},
	{"patch", _lforge_patch},
	{"remove", _lforge_remove},
	{"add", _lforge_add},

	{"pop", _lforge_pop},

	//FIXME are only valid for stash
	{"handle", _lstash_handle},
	{"clear", _lstash_clear},

	{NULL, NULL}
};

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

static int
_lmidiresponder__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 4); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: data
	// 4: atom
	
	latom_t *lchunk = NULL;
	if(luaL_testudata(L, 4, "latom"))
		lchunk = lua_touserdata(L, 4);
	lua_pop(L, 1); // atom

	// check for valid atom and event type
	if(!lchunk || (lchunk->atom->type != moony->uris.midi_event))
	{
		lua_pushboolean(L, 0); // not handled
		return 1;
	}

	const uint8_t *midi = LV2_ATOM_BODY_CONST(lchunk->atom);
	const uint8_t status = midi[0];
	const uint8_t command = status & 0xf0;
	const bool is_system = command == 0xf0;

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	if(lua_geti(L, 1, is_system ? status : command) != LUA_TNIL)
	{
		lua_insert(L, 1);
		if(is_system)
			lua_pushnil(L); // system messages have no channel
		else
			lua_pushinteger(L, status & 0x0f); // 4: channel

		for(unsigned i=1; i<lchunk->atom->size; i++)
			lua_pushinteger(L, midi[i]);

		lua_call(L, 4 + lchunk->atom->size - 1, 0);
	}

	lua_pushboolean(L, 1); // handled
	return 1;
}

static int
_lmidiresponder(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	// o = new
	int32_t *dummy = lua_newuserdata(L, sizeof(int32_t));
	(void)dummy;

	// o.uservalue = uservalue
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "lmidiresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

static const luaL_Reg lmidiresponder_mt [] = {
	{"__call", _lmidiresponder__call},
	{NULL, NULL}
};

static inline bool
_osc_path_has_wildcards(const char *path)
{
	for(const char *ptr=path+1; *ptr; ptr++)
		if(strchr("?*[{", *ptr))
			return true;
	return false;
}

//TODO can we rewrite this directly in C?
static const char *loscresponder_match =
	"function __expand(v)\n"
	"	return coroutine.wrap(function()\n"
	"		local item = string.match(v, '%b{}')\n"
	"		if item then\n"
	"			for key in string.gmatch(item, '{?([^,}]*)') do\n"
	"				local vv = string.gsub(v, item, key)\n"
	"				for w in __expand(vv) do\n"
	"					coroutine.yield(w)\n"
	"				end\n"
	"			end\n"
	"		else\n"
	"			coroutine.yield(v)\n"
	"		end\n"
	"	end)\n"
	"end\n"
	"\n"
	"function __match(v, o, ...)\n"
	"	local handled = false\n"
	"	v = string.gsub(v, '%?', '.')\n"
	"	v = string.gsub(v, '%*', '.*')\n"
	"	v = string.gsub(v, '%[%!', '[^')\n"
	"	v = '^' .. v .. '$'\n"
	"	for w in __expand(v) do\n"
	"		for k, x in pairs(o) do\n"
	"			if string.match(k, w) then\n"
	"				handled = x(o, ...) or handled\n"
	"			end\n"
	"		end\n"
	"	end\n"
	"	return handled\n"
	"end";

static void
_loscresponder_msg(const char *path, const char *fmt, const LV2_Atom_Tuple *args,
	void *data)
{
	moony_t *moony = data;
	lua_State *L = moony->vm.L;
	LV2_Atom_Forge *forge = &moony->forge;
	osc_forge_t *oforge = &moony->oforge;

	// 1: uservalue
	// 2: frames
	// 3: data

	int matching = 0;
	if(_osc_path_has_wildcards(path))
	{
		lua_getglobal(L, "__match"); // push pattern matching function
		lua_pushstring(L, path); // path
		matching = 1;
	}
	else if(lua_getfield(L, 1, path) == LUA_TNIL) // raw string match
	{
		lua_pop(L, 1); // nil
		return; // no match
	}

	lua_pushvalue(L, 1); // self
	lua_pushvalue(L, 2); // frames
	lua_pushvalue(L, 3); // data
	lua_pushstring(L, fmt);

	int oldtop = lua_gettop(L);
	const LV2_Atom *atom = lv2_atom_tuple_begin(args);

	for(const char *type = fmt; *type && atom; type++)
	{
		switch(*type)
		{
			case 'i':
			{
				int32_t i = 0;
				atom = osc_deforge_int32(oforge, forge, atom, &i);
				lua_pushinteger(L, i);
				break;
			}
			case 'f':
			{
				float f = 0.f;
				atom = osc_deforge_float(oforge, forge, atom, &f);
				lua_pushnumber(L, f);
				break;
			}
			case 's':
			{
				const char *s = NULL;
				atom = osc_deforge_string(oforge, forge, atom, &s);
				lua_pushstring(L, s);
				break;
			}
			case 'S':
			{
				const char *s = NULL;
				atom = osc_deforge_symbol(oforge, forge, atom, &s);
				lua_pushstring(L, s);
				break;
			}
			case 'b':
			{
				uint32_t size = 0;
				const uint8_t *b = NULL;
				atom = osc_deforge_blob(oforge, forge, atom, &size, &b);
				//TODO or return a AtomChunk?
				lua_createtable(L, size, 0);
				for(unsigned i=0; i<size; i++)
				{
					lua_pushinteger(L, b[i]);
					lua_rawseti(L, -2, i+1);
				}
				break;
			}
			
			case 'h':
			{
				int64_t h = 0;
				atom = osc_deforge_int64(oforge, forge, atom, &h);
				lua_pushinteger(L, h);
				break;
			}
			case 'd':
			{
				double d = 0.f;
				atom = osc_deforge_double(oforge, forge, atom, &d);
				lua_pushnumber(L, d);
				break;
			}
			case 't':
			{
				uint64_t t = 0;
				atom = osc_deforge_timestamp(oforge, forge, atom, &t);
				lua_pushinteger(L, t);
				break;
			}
			
			case 'c':
			{
				char c = '\0';
				atom = osc_deforge_char(oforge, forge, atom, &c);
				lua_pushinteger(L, c);
				break;
			}
			case 'm':
			{
				uint32_t size = 0;
				const uint8_t *m = NULL;
				atom = osc_deforge_midi(oforge, forge, atom, &size, &m);
				//TODO or return a MIDIEvent?
				lua_createtable(L, size, 0);
				for(unsigned i=0; i<size; i++)
				{
					lua_pushinteger(L, m[i]);
					lua_rawseti(L, -2, i+1);
				}
				break;
			}
			
			case 'T':
			case 'F':
			case 'N':
			case 'I':
			{
				break;
			}

			default: // unknown argument type
			{
				break;
			}
		}
	}

	lua_call(L, 4 + matching + lua_gettop(L) - oldtop, 0);
}

static int
_loscresponder__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 4); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: data
	// 4: atom
	
	lobj_t *lobj = NULL;
	if(luaL_testudata(L, 4, "latom"))
		lobj = lua_touserdata(L, 4);
	lua_pop(L, 1); // atom

	// check for valid atom and event type
	if(!lobj || !(osc_atom_is_bundle(&moony->oforge, lobj->obj)
		|| osc_atom_is_message(&moony->oforge, lobj->obj)))
	{
		lua_pushboolean(L, 0); // not handled
		return 1;
	}

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	osc_atom_event_unroll(&moony->oforge, lobj->obj, NULL, NULL, _loscresponder_msg, moony);

	lua_pushboolean(L, 1); // handled
	return 1;
}

static int
_loscresponder(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	// o = new 
	int32_t *dummy = lua_newuserdata(L, sizeof(int32_t));
	(void)dummy;

	// o.uservalue = uservalue
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "loscresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

static const luaL_Reg loscresponder_mt [] = {
	{"__call", _loscresponder__call},
	{NULL, NULL}
};

static void
_ltimeresponder_cb(timely_t *timely, int64_t frames, LV2_URID type,
	void *data)
{
	lua_State *L = data;

	if(lua_geti(L, 5, type) != LUA_TNIL) // uservalue[type]
	{
		lua_pushvalue(L, 5); // uservalue
		lua_pushinteger(L, frames); // frames
		lua_pushvalue(L, 4); // data

		if(type == TIMELY_URI_BAR_BEAT(timely))
		{
			lua_pushnumber(L, TIMELY_BAR_BEAT_RAW(timely));
		}
		else if(type == TIMELY_URI_BAR(timely))
		{
			lua_pushinteger(L, TIMELY_BAR(timely));
		}
		else if(type == TIMELY_URI_BEAT_UNIT(timely))
		{
			lua_pushinteger(L, TIMELY_BEAT_UNIT(timely));
		}
		else if(type == TIMELY_URI_BEATS_PER_BAR(timely))
		{
			lua_pushnumber(L, TIMELY_BEATS_PER_BAR(timely));
		}
		else if(type == TIMELY_URI_BEATS_PER_MINUTE(timely))
		{
			lua_pushnumber(L, TIMELY_BEATS_PER_MINUTE(timely));
		}
		else if(type == TIMELY_URI_FRAME(timely))
		{
			lua_pushinteger(L, TIMELY_FRAME(timely));
		}
		else if(type == TIMELY_URI_FRAMES_PER_SECOND(timely))
		{
			lua_pushnumber(L, TIMELY_FRAMES_PER_SECOND(timely));
		}
		else if(type == TIMELY_URI_SPEED(timely))
		{
			lua_pushnumber(L, TIMELY_SPEED(timely));
		}
		else
		{
			lua_pushnil(L);
		}

		lua_call(L, 4, 0);
	}
	else
		lua_pop(L, 1); // nil
}

static int
_ltimeresponder__call(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 5); // discard superfluous arguments
	// 1: self
	// 2: from
	// 3: to
	// 4: data aka forge
	// 5: atom || nil
	
	timely_t *timely = lua_touserdata(L, 1);
	int64_t from = luaL_checkinteger(L, 2);
	int64_t to = luaL_checkinteger(L, 3);
	lobj_t *lobj = NULL;
	if(luaL_testudata(L, 5, "latom"))
		lobj = lua_touserdata(L, 5);
	lua_pop(L, 1); // atom

	lua_getuservalue(L, 1); // 5: uservalue
	const int handled = timely_advance(timely, lobj ? lobj->obj : NULL, from, to);

	lua_pushboolean(L, handled); // handled vs not handled
	return 1;
}

static int
_ltimeresponder_apply(lua_State *L)
{
	// 1: self
	// 2: atom
	
	lua_pushinteger(L, 0);
	lua_insert(L, 2); // from

	lua_pushinteger(L, 0);
	lua_insert(L, 3); // to

	lua_pushnil(L);
	lua_insert(L, 4); // data aka forge

	return _ltimeresponder__call(L);
}

static int
_ltimeresponder_stash(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: lforge

	timely_t *timely = lua_touserdata(L, 1);
	lforge_t *lforge = luaL_checkudata(L, 2, "lforge");

	// serialize full time state to stash
	LV2_Atom_Forge_Frame frame;
	if(  !lv2_atom_forge_object(lforge->forge, &frame, 0, timely->urid.time_position)

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_barBeat)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_BAR_BEAT(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_bar)
		|| !lv2_atom_forge_long(lforge->forge, TIMELY_BAR(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_beatUnit)
		|| !lv2_atom_forge_int(lforge->forge, TIMELY_BEAT_UNIT(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_beatsPerBar)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_BEATS_PER_BAR(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_beatsPerMinute)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_BEATS_PER_MINUTE(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_frame)
		|| !lv2_atom_forge_long(lforge->forge, TIMELY_FRAME(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_framesPerSecond)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_FRAMES_PER_SECOND(timely))

		|| !lv2_atom_forge_key(lforge->forge, timely->urid.time_speed)
		|| !lv2_atom_forge_float(lforge->forge, TIMELY_SPEED(timely)) )
		luaL_error(L, forge_buffer_overflow);
	lv2_atom_forge_pop(lforge->forge, &frame);

	return 1; // forge
}

static int
_ltimeresponder__index(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: type
	
	timely_t *timely = lua_touserdata(L, 1);

	int ltype = lua_type(L, 2);
	if(ltype != LUA_TNUMBER)
	{
		if(ltype == LUA_TSTRING)
		{
			const char *key = lua_tostring(L, 2);
			if(!strcmp(key, "stash"))
			{
				lua_pushlightuserdata(L, moony);
				lua_pushcclosure(L, _ltimeresponder_stash, 1);
			}
			else if(!strcmp(key, "apply"))
			{
				lua_pushlightuserdata(L, moony);
				lua_pushcclosure(L, _ltimeresponder_apply, 1);
			}
			else
				lua_pushnil(L);
		}
		else
			lua_pushnil(L);

		return 1;
	}

	LV2_URID type = lua_tointeger(L, 2);

	if(type == TIMELY_URI_BAR_BEAT(timely))
	{
		lua_pushnumber(L, TIMELY_BAR_BEAT(timely));
	}
	else if(type == TIMELY_URI_BAR(timely))
	{
		lua_pushinteger(L, TIMELY_BAR(timely));
	}
	else if(type == TIMELY_URI_BEAT_UNIT(timely))
	{
		lua_pushinteger(L, TIMELY_BEAT_UNIT(timely));
	}
	else if(type == TIMELY_URI_BEATS_PER_BAR(timely))
	{
		lua_pushnumber(L, TIMELY_BEATS_PER_BAR(timely));
	}
	else if(type == TIMELY_URI_BEATS_PER_MINUTE(timely))
	{
		lua_pushnumber(L, TIMELY_BEATS_PER_MINUTE(timely));
	}
	else if(type == TIMELY_URI_FRAME(timely))
	{
		lua_pushinteger(L, TIMELY_FRAME(timely));
	}
	else if(type == TIMELY_URI_FRAMES_PER_SECOND(timely))
	{
		lua_pushnumber(L, TIMELY_FRAMES_PER_SECOND(timely));
	}
	else if(type == TIMELY_URI_SPEED(timely))
	{
		lua_pushnumber(L, TIMELY_SPEED(timely));
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int
_ltimeresponder(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	timely_mask_t mask = TIMELY_MASK_BAR_BEAT
		| TIMELY_MASK_BAR
		| TIMELY_MASK_BEAT_UNIT
		| TIMELY_MASK_BEATS_PER_BAR
		| TIMELY_MASK_BEATS_PER_MINUTE
		| TIMELY_MASK_FRAME
		| TIMELY_MASK_FRAMES_PER_SECOND
		| TIMELY_MASK_SPEED
		| TIMELY_MASK_BAR_BEAT_WHOLE
		| TIMELY_MASK_BAR_WHOLE;

	// o = o or {}
	if(lua_isnil(L, 1))
	{
		lua_pop(L, 1);
		lua_newtable(L);
	}

	// TODO do we want to cache/reuse this, too?
	timely_t *timely = lua_newuserdata(L, sizeof(timely_t)); // userdata
	timely_init(timely, moony->map, moony->opts.sample_rate, mask,
		_ltimeresponder_cb, L);

	// userdata.uservalue = o
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "ltimeresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

static const luaL_Reg ltimeresponder_mt [] = {
	{"__index", _ltimeresponder__index},
	{"__call", _ltimeresponder__call},
	{NULL, NULL}
};

static LV2_Atom_Forge_Ref
_lforge_explicit(lua_State *L, int idx, LV2_Atom_Forge *forge, LV2_URID range)
{
	if(range == forge->Int)
	{
		const int32_t value = luaL_checkinteger(L, idx);
		return lv2_atom_forge_int(forge, value);
	}
	else if(range == forge->URID)
	{
		const uint32_t value = luaL_checkinteger(L, idx);
		return lv2_atom_forge_urid(forge, value);
	}
	else if(range == forge->Long)
	{
		const int64_t value = luaL_checkinteger(L, idx);
		return lv2_atom_forge_long(forge, value);
	}
	else if(range == forge->Bool)
	{
		const int32_t value = lua_toboolean(L, idx);
		return lv2_atom_forge_bool(forge, value);
	}
	else if(range == forge->Float)
	{
		const float value = luaL_checknumber(L, idx);
		return lv2_atom_forge_float(forge, value);
	}
	else if(range == forge->Double)
	{
		const double value = luaL_checknumber(L, idx);
		return lv2_atom_forge_double(forge, value);
	}
	else if(range == forge->String)
	{
		size_t size;
		const char *value = luaL_checklstring(L, idx, &size);
		return lv2_atom_forge_string(forge, value, size);
	}
	else if(range == forge->URI)
	{
		size_t size;
		const char *value = luaL_checklstring(L, idx, &size);
		return lv2_atom_forge_uri(forge, value, size);
	}
	//TODO more types, e.g. Path , Literal

	return lv2_atom_forge_atom(forge, 0, 0); // nil
}

static inline void
_lstateresponder_register_access(lua_State *L, moony_t *moony, int64_t frames,
	lforge_t *lforge, const LV2_Atom_URID *subject, LV2_URID access)
{
	LV2_Atom_Forge_Frame obj_frame;
	LV2_Atom_Forge_Frame add_frame;
	LV2_Atom_Forge_Frame rem_frame;

	// iterate over properties
	lua_pushnil(L);  // first key 
	while(lua_next(L, -2))
	{
		// uses 'key' (at index -2) and 'value' (at index -1)
		const LV2_URID key = luaL_checkinteger(L, -2);

		const char *label = "undefined"; // fallback
		LV2_URID range = moony->forge.Int; // fallback

		if(lua_geti(L, -1, moony->uris.rdfs_label) == LUA_TSTRING)
			label = lua_tostring(L, -1);
		lua_pop(L, 1); // label

		if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER)
			range = lua_tointeger(L, -1);
		lua_pop(L, 1); // range

		if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		  || !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_patch) )
			luaL_error(L, forge_buffer_overflow);
		{
			if(subject)
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
					|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
					luaL_error(L, forge_buffer_overflow);
			}

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_remove)
				|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0)

				|| !lv2_atom_forge_key(lforge->forge, access)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);
			lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_add)
				|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0)

				|| !lv2_atom_forge_key(lforge->forge, access)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);
			lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
		}
		lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

		if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
			|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_patch) )
			luaL_error(L, forge_buffer_overflow);
		{
			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
				|| !lv2_atom_forge_urid(lforge->forge, key) )
				luaL_error(L, forge_buffer_overflow);

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_remove)
				|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0) )
				luaL_error(L, forge_buffer_overflow);
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_range)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_comment)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.core_minimum)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.core_maximum)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.units_unit)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.core_scale_point)
					|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard) )
					luaL_error(L, forge_buffer_overflow);
			}
			lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_add)
				|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0) )
				luaL_error(L, forge_buffer_overflow);
			{
				if(  !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
					|| !lv2_atom_forge_string(lforge->forge, label, strlen(label))

					|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_range)
					|| !lv2_atom_forge_urid(lforge->forge, range) )
					luaL_error(L, forge_buffer_overflow);

				if(lua_geti(L, -1, moony->uris.rdfs_comment) == LUA_TSTRING)
				{
					size_t comment_size;
					const char *comment = lua_tolstring(L, -1, &comment_size);
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_comment)
						|| !lv2_atom_forge_string(lforge->forge, comment, comment_size) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // comment

				if(lua_geti(L, -1, moony->uris.core_minimum) != LUA_TNIL)
				{
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.core_minimum)
						|| !_lforge_explicit(L, -1, lforge->forge, range) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // minimum

				if(lua_geti(L, -1, moony->uris.core_maximum) != LUA_TNIL)
				{
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.core_maximum)
						|| !_lforge_explicit(L, -1, lforge->forge, range) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // maximum

				if(lua_geti(L, -1, moony->uris.units_unit) == LUA_TNUMBER)
				{
					const LV2_URID unit = lua_tointeger(L, -1);
					if(  !lv2_atom_forge_key(lforge->forge, moony->uris.units_unit)
						|| !lv2_atom_forge_urid(lforge->forge, unit) )
						luaL_error(L, forge_buffer_overflow);
				}
				lua_pop(L, 1); // unit

				if(lua_geti(L, -1, moony->uris.core_scale_point) != LUA_TNIL)
				{
					// iterate over properties
					lua_pushnil(L);  // first key 
					while(lua_next(L, -2))
					{
						// uses 'key' (at index -2) and 'value' (at index -1)
						size_t point_size;
						const char *point = luaL_checklstring(L, -2, &point_size);
						LV2_Atom_Forge_Frame scale_point_frame;

						if(  !lv2_atom_forge_key(lforge->forge, moony->uris.core_scale_point)
							|| !lv2_atom_forge_object(lforge->forge, &scale_point_frame, 0, 0)

							|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdfs_label)
							|| !lv2_atom_forge_string(lforge->forge, point, point_size)

							|| !lv2_atom_forge_key(lforge->forge, moony->uris.rdf_value)
							|| !_lforge_explicit(L, -1, lforge->forge, range) )
							luaL_error(L, forge_buffer_overflow);
						
						lv2_atom_forge_pop(lforge->forge, &scale_point_frame); // core:scalePoint

						// removes 'value'; keeps 'key' for next iteration
						lua_pop(L, 1);
					}
				}
				lua_pop(L, 1); // scale_points
			}
			lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
		}
		lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

		// removes 'value'; keeps 'key' for next iteration
		lua_pop(L, 1);
	}
}

static inline void
_lstateresponder_register(lua_State *L, moony_t *moony, int64_t frames,
	lforge_t *lforge, const LV2_Atom_URID *subject)
{
	LV2_Atom_Forge_Frame obj_frame;
	LV2_Atom_Forge_Frame add_frame;
	LV2_Atom_Forge_Frame rem_frame;

	// clear all properties
	if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
		|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_patch) )
		luaL_error(L, forge_buffer_overflow);
	{
		if(subject)
		{
			if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
				|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
				luaL_error(L, forge_buffer_overflow);
		}

		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_remove)
			|| !lv2_atom_forge_object(lforge->forge, &rem_frame, 0, 0)

			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch_writable)
			|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard)

			|| !lv2_atom_forge_key(lforge->forge, moony->uris.patch_readable)
			|| !lv2_atom_forge_urid(lforge->forge, moony->uris.patch_wildcard) )
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pop(lforge->forge, &rem_frame); // patch:remove

		if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_add)
			|| !lv2_atom_forge_object(lforge->forge, &add_frame, 0, 0) )
			luaL_error(L, forge_buffer_overflow);
		lv2_atom_forge_pop(lforge->forge, &add_frame); // patch:add
	}
	lv2_atom_forge_pop(lforge->forge, &obj_frame); // patch:patch

	if(lua_geti(L, 1, moony->uris.patch_writable) != LUA_TNIL)
	{
		_lstateresponder_register_access(L, moony, frames, lforge, subject,
			moony->uris.patch_writable);
	}
	lua_pop(L, 1); // nil || table

	if(lua_geti(L, 1, moony->uris.patch_readable) != LUA_TNIL)
	{
		_lstateresponder_register_access(L, moony, frames, lforge, subject,
			moony->uris.patch_readable);
	}
	lua_pop(L, 1); // nil || table
}

static int
_lstateresponder__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 4); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: forge
	// 4: atom

	int64_t frames = luaL_checkinteger(L, 2);
	lforge_t *lforge = luaL_checkudata(L, 3, "lforge");
	lobj_t *lobj = NULL;
	if(luaL_testudata(L, 4, "latom"))
		lobj = lua_touserdata(L, 4);
	lua_pop(L, 1); // atom

	if(!lobj) // not a valid atom, abort
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	if(lobj->obj->body.otype == moony->uris.patch_get)
	{
		const LV2_Atom_URID *subject = NULL;
		const LV2_Atom_URID *property = NULL;

		LV2_Atom_Object_Query q [] = {
			{ moony->uris.patch_subject, (const LV2_Atom **)&subject },
			{ moony->uris.patch_property, (const LV2_Atom **)&property },
			{ 0, NULL }
		};
		lv2_atom_object_query(lobj->obj, q);

		if(!subject || ((subject->atom.type == moony->forge.URID) && (subject->body == moony->uris.subject)) )
		{
			if(!property)
			{
				// register state
				_lstateresponder_register(L, moony, frames, lforge, subject);

				lua_pushboolean(L, 1); // handled
				return 1;
			}
			else if(property->atom.type == moony->forge.URID)
			{
				bool found_it = false;

				if(lua_geti(L, 1, moony->uris.patch_writable) != LUA_TNIL)
				{
					if(lua_geti(L, -1, property->body) != LUA_TNIL)
						found_it = true;
					else
						lua_pop(L, 1); // nil
				}
				else
					lua_pop(L, 1); // nil

				if(!found_it)
				{
					if(lua_geti(L, 1, moony->uris.patch_readable) != LUA_TNIL)
					{
						if(lua_geti(L, -1, property->body) != LUA_TNIL)
							found_it = true;
						else
							lua_pop(L, 1); // readable, property nil
					}
					else
						lua_pop(L, 1); // nil
				}

				if(found_it)
				{
					LV2_URID range = moony->forge.Int; // fallback

					// get atom type
					if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER)
						range = lua_tointeger(L, -1);
					lua_pop(L, 1); // range

					LV2_Atom_Forge_Frame obj_frame;

					if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
						|| !lv2_atom_forge_object(lforge->forge, &obj_frame, 0, moony->uris.patch_set) )
						luaL_error(L, forge_buffer_overflow);
					{
						if(subject)
						{
							if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_subject)
								|| !lv2_atom_forge_urid(lforge->forge, subject->body) )
								luaL_error(L, forge_buffer_overflow);
						}

						if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_property)
							|| !lv2_atom_forge_urid(lforge->forge, property->body) )
							luaL_error(L, forge_buffer_overflow);

						if(lua_geti(L, -1, moony->uris.patch_get) != LUA_TNIL)
						{
							lua_pushvalue(L, -2); // self[property]
							lua_pushvalue(L, 2); // frames
							lua_pushvalue(L, 3); // forge
							lua_call(L, 3, 1);
						}
						else
						{
							lua_pop(L, 1); // nil
							lua_geti(L, -1, moony->uris.rdf_value);
						}

						if(!lua_isnil(L, -1))
						{
							if(  !lv2_atom_forge_key(lforge->forge, moony->uris.patch_value)
								|| !_lforge_explicit(L, -1, lforge->forge, range) )
								luaL_error(L, forge_buffer_overflow);
						}
					}
					lv2_atom_forge_pop(lforge->forge, &obj_frame);

					lua_pushboolean(L, 1); // handled
					return 1;
				}
			}
		}
	}
	else if(lobj->obj->body.otype == moony->uris.patch_set)
	{
		const LV2_Atom_URID *subject = NULL;
		const LV2_Atom_URID *property = NULL;
		const LV2_Atom *value = NULL;

		LV2_Atom_Object_Query q [] = {
			{ moony->uris.patch_subject, (const LV2_Atom **)&subject },
			{ moony->uris.patch_property, (const LV2_Atom **)&property },
			{ moony->uris.patch_value, &value },
			{ 0, NULL }
		};
		lv2_atom_object_query(lobj->obj, q);

		if(!subject || ((subject->atom.type == moony->forge.URID) && (subject->body == moony->uris.subject)) )
		{
			if(property && (property->atom.type == moony->forge.URID) && value)
			{
				if(  (lua_geti(L, 1, moony->uris.patch_writable) != LUA_TNIL)
					&& (lua_geti(L, -1, property->body) != LUA_TNIL) ) // self[property]
				{
					if(lua_geti(L, -1, moony->uris.patch_set) != LUA_TNIL)
					{
						lua_pushvalue(L, -2); // self[property]
						lua_pushvalue(L, 2); // frames
						lua_pushvalue(L, 3); // forge
						_latom_value(L, value);
						lua_call(L, 4, 0);
					}
					else
					{
						lua_pop(L, 1); // nil
						_latom_value(L, value);
						lua_seti(L, -2, moony->uris.rdf_value); // self[property].value = value
					}

					lua_pushboolean(L, 1); // handled
					return 1;
				}
			}
		}
	}

	lua_pushboolean(L, 0); // not handled
	return 1;
}

static int
_lstateresponder(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 1); // discard superfluous arguments

	// o = new 
	int32_t *dummy = lua_newuserdata(L, sizeof(int32_t));
	(void)dummy;

	// o.uservalue = uservalue
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "lstateresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

static int
_lstateresponder_reg(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 3); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: forge

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	int64_t frames = luaL_checkinteger(L, 2);
	lforge_t *lforge = luaL_checkudata(L, 3, "lforge");

	// fake subject
	const LV2_Atom_URID subject = {
		.atom = {
			.size = sizeof(uint32_t),
			.type = lforge->forge->URID
		},
		.body = moony->uris.subject
	};

	// register state
	_lstateresponder_register(L, moony, frames, lforge, &subject);

	return 0;
}

static int
_lstateresponder_stash(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: forge

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);

	lforge_t *lforge = luaL_checkudata(L, 2, "lforge");

	// ignore patch:readable's
	if(lua_geti(L, 1, moony->uris.patch_writable) == LUA_TNIL)
		return 0;

	LV2_Atom_Forge_Frame frame;
	if(!lv2_atom_forge_object(lforge->forge, &frame, 0, 0))
		luaL_error(L, forge_buffer_overflow);

	// iterate over writable properties
	lua_pushnil(L);  // first key 
	while(lua_next(L, -2))
	{
		// uses 'key' (at index -2) and 'value' (at index -1)
		const LV2_URID key = luaL_checkinteger(L, -2);
		LV2_URID range = lforge->forge->Int; // fallback

		if(lua_geti(L, -1, moony->uris.rdfs_range) == LUA_TNUMBER) // prop[RDFS.range]
			range = lua_tointeger(L, -1);
		lua_pop(L, 1); // range

		if(lua_geti(L, -1, moony->uris.rdf_value) != LUA_TNIL) // prop[RDF.value]
		{
			//TODO call prop[Patch.Get] ?
			if(  !lv2_atom_forge_key(lforge->forge, key)
				|| !_lforge_basic(L, -1, lforge->forge, range) )
				luaL_error(L, forge_buffer_overflow);
		}
		lua_pop(L, 1); // nil || prop[RDF.value]

		// removes 'value'; keeps 'key' for next iteration
		lua_pop(L, 1);
	}

	lv2_atom_forge_pop(lforge->forge, &frame);

	return 0;
}

static int
_lstateresponder_apply(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));

	lua_settop(L, 2); // discard superfluous arguments
	// 1: self
	// 2: obj

	// replace self with its uservalue
	lua_getuservalue(L, 1);
	lua_replace(L, 1);
	
	lobj_t *lobj = luaL_checkudata(L, 2, "latom");

	// ignore patch:readable's
	if(lua_geti(L, 1, moony->uris.patch_writable) == LUA_TNIL)
		return 0;

	LV2_ATOM_OBJECT_FOREACH(lobj->obj, prop)
	{
		if(lua_geti(L, -1, prop->key) != LUA_TNIL)
		{
			_latom_value(L, &prop->value);
			lua_seti(L, -2, moony->uris.rdf_value); // set prop[RDF.value]
			//TODO call prop[Patch.Set] ?
		}

		lua_pop(L, 1); // nil || prop
	}

	return 0;
}

static const luaL_Reg lstateresponder_mt [] = {
	{"__call", _lstateresponder__call},
	{"register", _lstateresponder_reg},
	{"stash", _lstateresponder_stash},
	{"apply", _lstateresponder_apply},
	{NULL, NULL}
};

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

	// Stash factory
	lua_pushlightuserdata(L, moony); // @ upvalueindex 1
	lua_pushcclosure(L, _lstash, 1);
	lua_setglobal(L, "Stash");

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
_restore(lua_State *L);

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
