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

%{
#include <stdio.h>
#include <string.h>

#include <common_ui.h>

typedef enum _markup_type_t markup_type_t;
typedef struct _markup_item_t markup_item_t;

enum _markup_type_t {
	MARKUP_CODE,
	MARKUP_FUNCTION,
	MARKUP_TYPE,
	MARKUP_KEYWORD,
	MARKUP_NUMBER,
	MARKUP_STRING,
	MARKUP_BRACE,
	MARKUP_COMMENT
};

struct _markup_item_t {
	const char *begin;
	const char *end;
};

#if defined(ELM_1_9)
static const markup_item_t markup_items [] = {
	[MARKUP_CODE] = {"code", "code"},
	[MARKUP_FUNCTION] = {"function", "function"},
	[MARKUP_TYPE] = {"type", "type"},
	[MARKUP_KEYWORD] = {"keyword", "keyword"},
	[MARKUP_NUMBER] = {"number", "number"},
	[MARKUP_STRING] = {"string", "string"},
	[MARKUP_BRACE] = {"comment", "comment"},
	[MARKUP_COMMENT] = {"brace", "brace"}
};
#else
// elementary <1.9 does not define the needed markup styles, fall-back to these
static const markup_item_t markup_items [] = {
	[MARKUP_CODE] = {"font=Mono style=Plain color=#FFFFFF", "font"},
  [MARKUP_FUNCTION] = {"color=#72AAD4 font_weight=Bold", "color"},
	[MARKUP_TYPE] = {"color=#3399ff", "color"},
	[MARKUP_KEYWORD] = {"color=#ff9900 font_weight=Bold", "color"},
	[MARKUP_NUMBER] = {"color=#D4D42A font_weight=Bold", "color"},
	[MARKUP_STRING] = {"color=#ff3a35", "color"},
	[MARKUP_BRACE] = {"color=#00B000", "color"},
	[MARKUP_COMMENT] = {"color=#656565", "color"}
};
#endif

static void
_add_plain(const char *content)
{
	encoder->append(content, encoder->data);
}

static void 
_add_singleton(const char *key)
{
	char buf [64];
	sprintf(buf, "<%s/>", key);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup_begin(markup_type_t type)
{
	char buf [64];
	sprintf(buf, "<%s>", markup_items[type].begin);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup_end(markup_type_t type)
{
	char buf [64];
	sprintf(buf, "</%s>", markup_items[type].end);
	encoder->append(buf, encoder->data);
}

static void 
_add_markup(markup_type_t type, const char *content)
{
	char buf [64];
	sprintf(buf, "<%s>", markup_items[type].begin);
	encoder->append(buf, encoder->data);
	encoder->append(content, encoder->data);
	sprintf(buf, "</%s>", markup_items[type].end);
	encoder->append(buf, encoder->data);
}

enum {
	TK_CALLER = 0x7f,
	TK_AND,
	TK_BREAK,
	TK_DO,
	TK_ELSE,
	TK_ELSEIF,
	TK_END,
	TK_FALSE,
	TK_FOR,
	TK_FUNCTION,
	TK_IF,
	TK_IN,
	TK_LOCAL,
	TK_NIL,
	TK_NOT,
	TK_OR,
	TK_REPEAT,
	TK_RETURN,
	TK_THEN,
	TK_TRUE,
	TK_UNTIL,
	TK_WHILE,
	TK_CONCAT,
	TK_DOTS,
	TK_LABEL,
	TK_NUMBER,
	TK_NAME,
	TK_SINGLE_QUOTE_IN,
	TK_SINGLE_QUOTE_OUT,
	TK_SINGLE_QUOTE_ERR,
	TK_DOUBLE_QUOTE_IN,
	TK_DOUBLE_QUOTE_OUT,
	TK_DOUBLE_QUOTE_ERR,
	TK_LONG_STRING_IN,
	TK_LONG_STRING_OUT,
	TK_SHORT_COMMENT_IN,
	TK_SHORT_COMMENT_OUT,
	TK_LONG_COMMENT_IN,
	TK_LONG_COMMENT_OUT,
	TK_WHITESPACE,
	TK_TAB,
	TK_NEWLINE,
	TK_BRACE,
	TK_RAW,
	TK_AMP,
	TK_LT,
	TK_GT,
	TK_BADCHAR
};

static int firstline=1;

static int
checkKw(char*s)
{
	#define CHECK(S,T) \
		if(!strcmp(#S,s)) \
			return TK_##T;
	
	switch(*s)
	{
		case 'a':
			CHECK(and, AND);
			CHECK(assert, CALLER);

			CHECK(apply, CALLER); // lua.lv2 specific
			break;
		case 'A':
			CHECK(Atom.Bool, NUMBER);
			CHECK(Atom.Chunk, NUMBER);
			CHECK(Atom.Double, NUMBER);
			CHECK(Atom.Float, NUMBER);
			CHECK(Atom.Int, NUMBER);
			CHECK(Atom.Long, NUMBER);
			CHECK(Atom.Literal, NUMBER);
			CHECK(Atom.Object, NUMBER);
			CHECK(Atom.Path, NUMBER);
			CHECK(Atom.Property, NUMBER);
			CHECK(Atom.Sequence, NUMBER);
			CHECK(Atom.String, NUMBER);
			CHECK(Atom.Tuple, NUMBER);
			CHECK(Atom.URI, NUMBER);
			CHECK(Atom.URID, NUMBER);
			CHECK(Atom.Vector, NUMBER);
			CHECK(Atom.frameTime, NUMBER);
			CHECK(Atom.beatTime, NUMBER);
			break;
		case 'b':
			CHECK(break, BREAK);
			break;
		case 'B':
			CHECK(Buf_Size.minBlockLength, NUMBER);
			CHECK(Buf_Size.maxBlockLength, NUMBER);
			CHECK(Buf_Size.sequenceSize, NUMBER);
			break;
		case 'c':
			CHECK(coroutine.running, CALLER);
			CHECK(coroutine.create, CALLER);
			CHECK(coroutine.yield, CALLER);
			CHECK(coroutine.isyieldable, CALLER);
			CHECK(coroutine.wrap, CALLER);
			CHECK(coroutine.resume, CALLER);
			CHECK(coroutine.status, CALLER);

			CHECK(collectgarbage, CALLER);
			break;
		case 'C':
			CHECK(Core.sampleRate, NUMBER);
			CHECK(Core.minimum, NUMBER);
			CHECK(Core.maximum, NUMBER);
			CHECK(Core.scalePoint, NUMBER);
			break;
		case 'd':
			CHECK(dofile, CALLER);
			CHECK(do, DO);
			//TODO debug
			break;
		case 'e':
			CHECK(elseif, ELSEIF);
			CHECK(else, ELSE);
			CHECK(end, END);
			CHECK(error, CALLER);
			break;
		case 'f':
			CHECK(false, FALSE);
			CHECK(for, FOR);
			CHECK(function, FUNCTION);
			break;
		case 'g':
			CHECK(getmetatable, CALLER);
			break;
		case 'i':
			CHECK(if, IF);
			CHECK(in, IN);
			CHECK(ipairs, CALLER);
			//TODO io
			break;
		case 'l':
			CHECK(local, LOCAL);
			CHECK(load, CALLER);
			CHECK(loadfile, CALLER);
			break;
		case 'm':
			CHECK(math.sqrt, CALLER)
			CHECK(math.ult, CALLER)
			CHECK(math.deg, CALLER)
			CHECK(math.tointeger, CALLER)
			CHECK(math.asin, CALLER)
			CHECK(math.randomseed, CALLER)
			CHECK(math.atan2, CALLER)
			CHECK(math.floor, CALLER)
			CHECK(math.max, CALLER)
			CHECK(math.acos, CALLER)
			CHECK(math.ceil, CALLER)
			CHECK(math.modf, CALLER)
			CHECK(math.huge, CALLER)
			CHECK(math.min, CALLER)
			CHECK(math.rad, CALLER)
			CHECK(math.exp, CALLER)
			CHECK(math.fmod, CALLER)
			CHECK(math.type, CALLER)
			CHECK(math.mininteger, CALLER)
			CHECK(math.maxinteger, CALLER)
			CHECK(math.random, CALLER)
			CHECK(math.pi, CALLER)
			CHECK(math.cos, CALLER)
			CHECK(math.abs, CALLER)
			CHECK(math.tan, CALLER)
			CHECK(math.sin, CALLER)
			CHECK(math.log, CALLER)
			CHECK(math.atan, CALLER)
			break;
		case 'M':
			CHECK(Map, CALLER);

			CHECK(MIDIResponder, CALLER);

			CHECK(MIDI.MidiEvent, NUMBER);

			CHECK(MIDI.NoteOff, NUMBER);
			CHECK(MIDI.NoteOn, NUMBER);
			CHECK(MIDI.NotePressure, NUMBER);
			CHECK(MIDI.Controller, NUMBER);
			CHECK(MIDI.ProgramChange, NUMBER);
			CHECK(MIDI.ChannelPressure, NUMBER);
			CHECK(MIDI.Bender, NUMBER);
			CHECK(MIDI.SystemExclusive, NUMBER);
			CHECK(MIDI.QuarterFrame, NUMBER);
			CHECK(MIDI.SongPosition, NUMBER);
			CHECK(MIDI.SongSelect, NUMBER);
			CHECK(MIDI.TuneRequest, NUMBER);
			CHECK(MIDI.Clock, NUMBER);
			CHECK(MIDI.Start, NUMBER);
			CHECK(MIDI.Continue, NUMBER);
			CHECK(MIDI.Stop, NUMBER);
			CHECK(MIDI.ActiveSense, NUMBER);
			CHECK(MIDI.Reset, NUMBER);
	
			CHECK(MIDI.BankSelection_MSB, NUMBER);
			CHECK(MIDI.Modulation_MSB, NUMBER);
			CHECK(MIDI.Breath_MSB, NUMBER);
			CHECK(MIDI.Foot_MSB, NUMBER);
			CHECK(MIDI.PortamentoTime_MSB, NUMBER);
			CHECK(MIDI.DataEntry_MSB, NUMBER);
			CHECK(MIDI.MainVolume_MSB, NUMBER);
			CHECK(MIDI.Balance_MSB, NUMBER);
			CHECK(MIDI.Panpot_MSB, NUMBER);
			CHECK(MIDI.Expression_MSB, NUMBER);
			CHECK(MIDI.Effect1_MSB, NUMBER);
			CHECK(MIDI.Effect2_MSB, NUMBER);
			CHECK(MIDI.GeneralPurpose1_MSB, NUMBER);
			CHECK(MIDI.GeneralPurpose2_MSB, NUMBER);
			CHECK(MIDI.GeneralPurpose3_MSB, NUMBER);
			CHECK(MIDI.GeneralPurpose4_MSB, NUMBER);

			CHECK(MIDI.BankSelection_LSB, NUMBER);
			CHECK(MIDI.Modulation_LSB, NUMBER);
			CHECK(MIDI.Breath_LSB, NUMBER);
			CHECK(MIDI.Foot_LSB, NUMBER);
			CHECK(MIDI.PortamentoTime_LSB, NUMBER);
			CHECK(MIDI.DataEntry_LSB, NUMBER);
			CHECK(MIDI.MainVolume_LSB, NUMBER);
			CHECK(MIDI.Balance_LSB, NUMBER);
			CHECK(MIDI.Panpot_LSB, NUMBER);
			CHECK(MIDI.Expression_LSB, NUMBER);
			CHECK(MIDI.Effect1_LSB, NUMBER);
			CHECK(MIDI.Effect2_LSB, NUMBER);
			CHECK(MIDI.GeneralPurpose1_LSB, NUMBER);
			CHECK(MIDI.GeneralPurpose2_LSB, NUMBER);
			CHECK(MIDI.GeneralPurpose3_LSB, NUMBER);
			CHECK(MIDI.GeneralPurpose4_LSB, NUMBER);

			CHECK(MIDI.SustainPedal, NUMBER);
			CHECK(MIDI.Portamento, NUMBER);
			CHECK(MIDI.Sostenuto, NUMBER);
			CHECK(MIDI.SoftPedal, NUMBER);
			CHECK(MIDI.LegatoFootSwitch, NUMBER);
			CHECK(MIDI.Hold2, NUMBER);

			CHECK(MIDI.SoundVariation, NUMBER);
			CHECK(MIDI.ReleaseTime, NUMBER);
			CHECK(MIDI.Timbre, NUMBER);
			CHECK(MIDI.AttackTime, NUMBER);
			CHECK(MIDI.Brightness, NUMBER);
			CHECK(MIDI.SC1, NUMBER);
			CHECK(MIDI.SC2, NUMBER);
			CHECK(MIDI.SC3, NUMBER);
			CHECK(MIDI.SC4, NUMBER);
			CHECK(MIDI.SC5, NUMBER);
			CHECK(MIDI.SC6, NUMBER);
			CHECK(MIDI.SC7, NUMBER);
			CHECK(MIDI.SC8, NUMBER);
			CHECK(MIDI.SC9, NUMBER);
			CHECK(MIDI.SC10, NUMBER);

			CHECK(MIDI.GeneralPurpose5, NUMBER);
			CHECK(MIDI.GeneralPurpose6, NUMBER);
			CHECK(MIDI.GeneralPurpose7, NUMBER);
			CHECK(MIDI.GeneralPurpose8, NUMBER);
			CHECK(MIDI.PortamentoControl, NUMBER);

			CHECK(MIDI.ReverbDepth, NUMBER);
			CHECK(MIDI.TremoloDepth, NUMBER);
			CHECK(MIDI.ChorusDepth, NUMBER);
			CHECK(MIDI.DetuneDepth, NUMBER);
			CHECK(MIDI.PhaserDepth, NUMBER);
			CHECK(MIDI.E1, NUMBER);
			CHECK(MIDI.E2, NUMBER);
			CHECK(MIDI.E3, NUMBER);
			CHECK(MIDI.E4, NUMBER);
			CHECK(MIDI.E5, NUMBER);

			CHECK(MIDI.DataIncrement, NUMBER);
			CHECK(MIDI.DataDecrement, NUMBER);

			CHECK(MIDI.NRPN_LSB, NUMBER);
			CHECK(MIDI.NRPN_MSB, NUMBER);

			CHECK(MIDI.RPN_LSB, NUMBER);
			CHECK(MIDI.RPN_MSB, NUMBER);

			CHECK(MIDI.AllSoundsOff, NUMBER);
			CHECK(MIDI.ResetControllers, NUMBER);
			CHECK(MIDI.LocalControlSwitch, NUMBER);
			CHECK(MIDI.AllNotesOff, NUMBER);
			CHECK(MIDI.OmniOff, NUMBER);
			CHECK(MIDI.OmniOn, NUMBER);
			CHECK(MIDI.Mono1, NUMBER);
			CHECK(MIDI.Mono2, NUMBER);

			break;
		case 'n':
			CHECK(nil, NIL);
			CHECK(not, NOT);
			CHECK(next, CALLER);
			break;
		case 'o':
			CHECK(or, OR);
			//TODO os
			break;
		case 'O':
			CHECK(OSCResponder, CALLER);

			CHECK(OSC.Event, NUMBER);
			CHECK(OSC.Bundle, NUMBER);
			CHECK(OSC.Message, NUMBER);
			CHECK(OSC.bundleTimestamp, NUMBER);
			CHECK(OSC.bundleItems, NUMBER);
			CHECK(OSC.messagePath, NUMBER);
			CHECK(OSC.messageFormat, NUMBER);
			CHECK(OSC.messageArguments, NUMBER);
			
			CHECK(Options, CALLER);
			break;
		case 'p':
			CHECK(pairs, CALLER);
			CHECK(print, CALLER);
			CHECK(pcall, CALLER);
			//TODO package
			break;
		case 'P':
			CHECK(Patch.Ack, NUMBER);
			CHECK(Patch.Delete, NUMBER);
			CHECK(Patch.Copy, NUMBER);
			CHECK(Patch.Error, NUMBER);
			CHECK(Patch.Get, NUMBER);
			CHECK(Patch.Message, NUMBER);
			CHECK(Patch.Move, NUMBER);
			CHECK(Patch.Patch, NUMBER);
			CHECK(Patch.Post, NUMBER);
			CHECK(Patch.Put, NUMBER);
			CHECK(Patch.Request, NUMBER);
			CHECK(Patch.Response, NUMBER);
			CHECK(Patch.Set, NUMBER);
			CHECK(Patch.add, NUMBER);
			CHECK(Patch.body, NUMBER);
			CHECK(Patch.destination, NUMBER);
			CHECK(Patch.property, NUMBER);
			CHECK(Patch.readable, NUMBER);
			CHECK(Patch.remove, NUMBER);
			CHECK(Patch.request, NUMBER);
			CHECK(Patch.subject, NUMBER);
			CHECK(Patch.sequenceNumber, NUMBER);
			CHECK(Patch.value, NUMBER);
			CHECK(Patch.wildcard, NUMBER);
			CHECK(Patch.writable, NUMBER);
			break;
		case 'r':
			CHECK(repeat, REPEAT);
			CHECK(return, RETURN);
			CHECK(rawget, CALLER);
			CHECK(rawset, CALLER);
			CHECK(rawlen, CALLER);
			CHECK(rawequal, CALLER);
			CHECK(require, CALLER);
			
			CHECK(read, CALLER); // lua.lv2 specific
			CHECK(run, CALLER); // lua.lv2 specific
			CHECK(restore, CALLER); // lua.lv2 specific
			break;
		case 'R':
			CHECK(RDF.value, NUMBER);
			CHECK(RDFS.label, NUMBER);
			CHECK(RDFS.range, NUMBER);
			CHECK(RDFS.comment, NUMBER);
			break;
		case 's':
			CHECK(string.char, CALLER);
			CHECK(string.gsub, CALLER);
			CHECK(string.len, CALLER);
			CHECK(string.byte, CALLER);
			CHECK(string.format, CALLER);
			CHECK(string.lower, CALLER);
			CHECK(string.gmatch, CALLER);
			CHECK(string.dump, CALLER);
			CHECK(string.packsize, CALLER);
			CHECK(string.pack, CALLER);
			CHECK(string.unpack, CALLER);
			CHECK(string.upper, CALLER);
			CHECK(string.rep, CALLER);
			CHECK(string.reverse, CALLER);
			CHECK(string.sub, CALLER);
			CHECK(string.match, CALLER);
			CHECK(string.find, CALLER);

			CHECK(select, CALLER);
			CHECK(setmetatable, CALLER);
			CHECK(save, CALLER); // lua.lv2 specific
			CHECK(stash, CALLER); // lua.lv2 specific
			break;
		case 'S':
			CHECK(StateResponder, CALLER);
			CHECK(Stash, CALLER);
			break;
		case 't':
			CHECK(then, THEN);
			CHECK(true, TRUE);
			CHECK(type, CALLER);
			CHECK(tonumber, CALLER);
			CHECK(tostring, CALLER);

			CHECK(table.insert, CALLER);
			CHECK(table.unpack, CALLER);
			CHECK(table.concat, CALLER);
			CHECK(table.move, CALLER);
			CHECK(table.sort, CALLER);
			CHECK(table.remove, CALLER);
			CHECK(table.pack, CALLER);
			break;
		case 'T':
			CHECK(TimeResponder, CALLER);

			CHECK(Time.Position, NUMBER);
			CHECK(Time.barBeat, NUMBER);
			CHECK(Time.bar, NUMBER);
			CHECK(Time.beat, NUMBER);
			CHECK(Time.beatUnit, NUMBER);
			CHECK(Time.beatsPerBar, NUMBER);
			CHECK(Time.beatsPerMinute, NUMBER);
			CHECK(Time.frame, NUMBER);
			CHECK(Time.framesPerSecond, NUMBER);
			CHECK(Time.speed, NUMBER);
			break;
		case 'u':
			CHECK(until, UNTIL);

			CHECK(utf8.offset, CALLER);
			CHECK(utf8.charpattern, CALLER);
			CHECK(utf8.codes, CALLER);
			CHECK(utf8.len, CALLER);
			CHECK(utf8.codepoint, CALLER);
			CHECK(utf8.char, CALLER);
			break;
		case 'U':	
			CHECK(Unmap, CALLER);

			CHECK(Units.Conversion, NUMBER);
			CHECK(Units.Unit, NUMBER);
			CHECK(Units.bar, NUMBER);
			CHECK(Units.beat, NUMBER);
			CHECK(Units.bpm, NUMBER);
			CHECK(Units.cent, NUMBER);
			CHECK(Units.cm, NUMBER);
			CHECK(Units.coef, NUMBER);
			CHECK(Units.conversion, NUMBER);
			CHECK(Units.db, NUMBER);
			CHECK(Units.degree, NUMBER);
			CHECK(Units.frame, NUMBER);
			CHECK(Units.hz, NUMBER);
			CHECK(Units.inch, NUMBER);
			CHECK(Units.khz, NUMBER);
			CHECK(Units.km, NUMBER);
			CHECK(Units.m, NUMBER);
			CHECK(Units.mhz, NUMBER);
			CHECK(Units.midiNote, NUMBER);
			CHECK(Units.mile, NUMBER);
			CHECK(Units.min, NUMBER);
			CHECK(Units.mm, NUMBER);
			CHECK(Units.ms, NUMBER);
			CHECK(Units.name, NUMBER);
			CHECK(Units.oct, NUMBER);
			CHECK(Units.pc, NUMBER);
			CHECK(Units.prefixConversion, NUMBER);
			CHECK(Units.render, NUMBER);
			CHECK(Units.s, NUMBER);
			CHECK(Units.semitone12TET, NUMBER);
			CHECK(Units.symbol, NUMBER);
			CHECK(Units.unit, NUMBER);
			break;
		case 'V':
			CHECK(VoiceMap, CALLER);
			break;
		case 'w':
			CHECK(write, CALLER); // lua.lv2 specific
			CHECK(while, WHILE);
			break;
		case 'x':
			CHECK(xpcall, CALLER);
			break;
	}
	
	return TK_NAME;
	#undef CHECK
}

%}

%option reentrant noyywrap

w              [ \v\a]+
name           [_a-zA-Z][_a-zA-Z0-9\.]*
n              [0-9]+
exp            [Ee][+-]?{n}
number         ({n}|{n}[.]{n}){exp}?
hexnumber			 0x[a-f0-9]+
eol            [\n\r]
sep            =*
brace					 [\(\)\[\]\{\}]

%x XLONGSTRING
%x XSHORTCOMMENT
%x XLONGCOMMENT
%x XSTRINGQ
%x XSTRINGA

%%

^#!.*          {if(firstline) firstline=0;}
"::"{name}"::" return TK_LABEL;
{name}         return checkKw(yytext);
{hexnumber}    return TK_NUMBER;
{number}       return TK_NUMBER;
"--[["  			 BEGIN(XLONGCOMMENT); return TK_LONG_COMMENT_IN;
"[["    			 BEGIN(XLONGSTRING); return TK_LONG_STRING_IN;
"--"           BEGIN( XSHORTCOMMENT); return TK_SHORT_COMMENT_IN;
{w}            return TK_WHITESPACE;
"\t"           return TK_TAB;
\"             BEGIN(XSTRINGQ); return TK_DOUBLE_QUOTE_IN;
'              BEGIN(XSTRINGA); return TK_SINGLE_QUOTE_IN;
"..."          return TK_DOTS;
".."           return TK_CONCAT;
{brace}				 return TK_BRACE;
{eol}          return TK_NEWLINE;
"&"            return TK_AMP;
"<"            return TK_LT;
">"            return TK_GT;
.              return TK_RAW;

<XSTRINGQ>
{
   \\\"	       return TK_RAW;
   \"          BEGIN(0); return TK_DOUBLE_QUOTE_OUT;
   {eol}	     BEGIN(0); return TK_DOUBLE_QUOTE_ERR;
   "&"         return TK_AMP;
   "<"         return TK_LT;
   ">"         return TK_GT;
   .           return TK_RAW;
}

<XSTRINGA>
{
   \\'	       return TK_RAW;
   '           BEGIN(0); return TK_SINGLE_QUOTE_OUT;
   {eol}	     BEGIN(0); return TK_SINGLE_QUOTE_ERR;
   "&"         return TK_AMP;
   "<"         return TK_LT;
   ">"         return TK_GT;
   .           return TK_RAW;
}

<XSHORTCOMMENT>
{
   {eol}       BEGIN(0); return TK_SHORT_COMMENT_OUT;
   "&"         return TK_AMP;
   "<"         return TK_LT;
   ">"         return TK_GT;
   .           return TK_RAW;
}

<XLONGCOMMENT>
{
   "]]"        BEGIN(0); return TK_LONG_COMMENT_OUT;
   {eol}       return TK_NEWLINE;
   "&"         return TK_AMP;
   "<"         return TK_LT;
   ">"         return TK_GT;
   .           return TK_RAW;
}

<XLONGSTRING>
{
   "]]"				 BEGIN(0); return TK_LONG_STRING_OUT;
   {eol}       return TK_NEWLINE;
   "&"         return TK_AMP;
   "<"         return TK_LT;
   ">"         return TK_GT;
   .           return TK_RAW;
}

%%

void
lua_to_markup(const char *utf8, FILE *f)
{
	yyscan_t scanner;
	YY_BUFFER_STATE buf;

	enclex_init(&scanner);
	if(utf8)
	{
		buf = enc_scan_string(utf8, scanner);
	}
	else if(f)
	{
		encset_in(f, scanner);
		buf = enc_create_buffer(NULL, YY_BUF_SIZE, scanner);
	}
	else
	{
		enclex_destroy(scanner);
		return;
	}
				
	encoder->begin(encoder->data);
	_add_markup_begin(MARKUP_CODE);

	for(int tok=enclex(scanner); tok; tok=enclex(scanner))
	{
		const char *txt = encget_text(scanner);
		switch(tok)
		{
			case TK_CALLER:
				_add_markup(MARKUP_FUNCTION, txt);
				break;

			case TK_LOCAL:
				_add_markup(MARKUP_TYPE, txt);
				break;

			case TK_FUNCTION:
			case TK_AND:
			case TK_BREAK:
			case TK_DO:
			case TK_ELSE:
			case TK_ELSEIF:
			case TK_END:
			case TK_FOR:
			case TK_IF:
			case TK_IN:
			case TK_NOT:
			case TK_OR:
			case TK_REPEAT:
			case TK_RETURN:
			case TK_THEN:
			case TK_UNTIL:
			case TK_WHILE:
			case TK_CONCAT:
				_add_markup(MARKUP_KEYWORD, txt);
				break;
			
			case TK_LABEL:
				_add_markup(MARKUP_TYPE, txt);
				break;

			case TK_NUMBER:
			case TK_NIL:
			case TK_TRUE:
			case TK_FALSE:
			case TK_DOTS:
				_add_markup(MARKUP_NUMBER, txt);
				break;

			case TK_SINGLE_QUOTE_IN:
				_add_markup_begin(MARKUP_STRING);
				_add_plain("'");
				break;
			case TK_SINGLE_QUOTE_OUT:
				_add_plain("'");
				_add_markup_end(MARKUP_STRING);
				break;
			case TK_SINGLE_QUOTE_ERR:
				_add_markup_end(MARKUP_STRING);
				_add_singleton("br");
				break;
			
			case TK_DOUBLE_QUOTE_IN:
				_add_markup_begin(MARKUP_STRING);
				_add_plain("\"");
				break;
			case TK_DOUBLE_QUOTE_OUT:
				_add_plain("\"");
				_add_markup_end(MARKUP_STRING);
				break;
			case TK_DOUBLE_QUOTE_ERR:
				_add_markup_end(MARKUP_STRING);
				_add_singleton("br");
				break;

			case TK_LONG_STRING_IN:
				_add_markup_begin(MARKUP_STRING);
				_add_plain("[[");
				break;
			case TK_LONG_STRING_OUT:
				_add_plain("]]");
				_add_markup_end(MARKUP_STRING);
				break;

			case TK_SHORT_COMMENT_IN:
				_add_markup_begin(MARKUP_COMMENT);
				_add_plain("--");
				break;
			case TK_SHORT_COMMENT_OUT:
				_add_markup_end(MARKUP_COMMENT);
				_add_singleton("br");
				break;
			
			case TK_LONG_COMMENT_IN:
				_add_markup_begin(MARKUP_COMMENT);
				_add_plain("--[[");
				break;
			case TK_LONG_COMMENT_OUT:
				_add_plain("]]");
				_add_markup_end(MARKUP_COMMENT);
				break;

			case TK_NEWLINE:
				_add_singleton("br");
				break;
			
			case TK_BRACE:
				_add_markup(MARKUP_BRACE, txt);
				break;

			case TK_BADCHAR:
				break;
			
			case TK_AMP:
				_add_plain("&amp;");
				break;
			case TK_LT:
				_add_plain("&lt;");
				break;
			case TK_GT:
				_add_plain("&gt;");
				break;

			case TK_TAB:
				_add_plain("  ");
				break;

			case TK_NAME:
			case TK_WHITESPACE:
			case TK_RAW:
			default:
				_add_plain(txt);
				break;
		}
	}
				
	_add_markup_end(MARKUP_CODE);
	encoder->end(encoder->data);

	enc_delete_buffer(buf, scanner);
	enclex_destroy(scanner);
}
