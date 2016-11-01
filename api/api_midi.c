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

#include <api_midi.h>
#include <api_atom.h>
#include <api_forge.h>

static int
_lmidiresponder__call(lua_State *L)
{
	moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	const bool *through = lua_touserdata(L, 1);

	lua_settop(L, 4); // discard superfluous arguments
	// 1: self
	// 2: frames
	// 3: forge
	// 4: atom
	
	latom_t *lchunk = luaL_checkudata(L, 4, "latom");
	lua_pop(L, 1); // atom

	// check for valid atom and event type
	if(lchunk->atom->type != moony->uris.midi_event)
	{
		lua_pushboolean(L, 0); // not handled
		return 1;
	}

	const uint8_t *midi = lchunk->body.raw;
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
	else if(*through) // through
	{
		const int64_t frames = luaL_checkinteger(L, 2);
		lforge_t *lforge = luaL_checkudata(L, 3, "lforge");

		if(frames < lforge->last.frames)
			luaL_error(L, "invalid frame time, must not decrease");
		lforge->last.frames = frames;

		if(  !lv2_atom_forge_frame_time(lforge->forge, frames)
			|| !lv2_atom_forge_atom(lforge->forge, lchunk->atom->size, lchunk->atom->type)
			|| !lv2_atom_forge_write(lforge->forge, lchunk->body.raw, lchunk->atom->size) )
			luaL_error(L, forge_buffer_overflow);
	}

	lua_pushboolean(L, 1); // handled
	return 1;
}

int
_lmidiresponder(lua_State *L)
{
	//moony_t *moony = lua_touserdata(L, lua_upvalueindex(1));
	
	lua_settop(L, 2); // discard superfluous arguments

	const bool _through = lua_toboolean(L, 2);
	lua_pop(L, 1); // bool

	// o = new
	bool *through = lua_newuserdata(L, sizeof(bool));
	*through= _through;

	// o.uservalue = uservalue
	lua_insert(L, 1);
	lua_setuservalue(L, -2);

	// setmetatable(o, self)
	luaL_getmetatable(L, "lmidiresponder");
	lua_setmetatable(L, -2);

	// return o
	return 1;
}

const luaL_Reg lmidiresponder_mt [] = {
	{"__call", _lmidiresponder__call},
	{NULL, NULL}
};

const midi_msg_t midi_msgs [] = {
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
	{ 0xf7															, "EndOfExclusive" },

	{ 0, NULL} // sentinel
};

const midi_msg_t controllers [] = {
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
