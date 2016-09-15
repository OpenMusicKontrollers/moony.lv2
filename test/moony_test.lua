--[[
	Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
	
	This is free software: you can redistribute it and/or modify
	it under the terms of the Artistic License 2.0 as published by
	The Perl Foundation.
	
	This source is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	Artistic License 2.0 for more details.
	
	You should have received a copy of the Artistic License 2.0
	along the source as a COPYING file. If not, obtain it from
	http://www.perlfoundation.org/artistic_license_2_0.
--]]

-- Map/unmap
print('[test] Map/unmap')
do
	local uri = 'http://test.org#foo'

	assert(Map[uri] == Map[uri])
	assert(Map(uri) == Map(uri))

	local urid = Map[uri]
	assert(Unmap[urid] == Unmap[urid])
	assert(Unmap(urid) == Unmap(urid))
end

-- Int
print('[test] Int')
do
	local function producer(forge)
		forge:frame_time(0)
		forge:int(0x7fffffff)
		
		forge:frame_time(0):int(0xffffffff)
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == Atom.Int)
		assert(#atom == 4)
		assert(type(atom.body) == 'number')
		assert(atom.body == 0x7fffffff)
		
		local atom = seq[2]
		assert(atom.type == Atom.Int)
		assert(#atom == 4)
		assert(type(atom.body) == 'number')
		assert(atom.body ~= 0xffffffff)
	end

	test(producer, consumer)
end

-- frame mismatch
print('[test] Frame mismatch')
do
	local function producer(forge)
		forge:frame_time(0)
		forge:tuple() -- would need a pop()
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Tuple)
	end

	test(producer, consumer)
end

-- Long
print('[test] Long')
do
	local function producer(forge)
		forge:frame_time(0):long(0x100000000)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Long)
		assert(#atom == 8)
		assert(type(atom.body) == 'number')
		assert(atom.body == 0x100000000)
	end

	test(producer, consumer)
end

-- Float
print('[test] Float')
do
	local function producer(forge)
		forge:frame_time(0)
		forge:float(2.0)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Float)
		assert(#atom == 4)
		assert(type(atom.body) == 'number')
		assert(atom.body == 2.0)
	end

	test(producer, consumer)
end

-- Double
print('[test] Double')
do
	local function producer(forge)
		forge:frame_time(0):double(0.12)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Double)
		assert(#atom == 8)
		assert(type(atom.body) == 'number')
		assert(atom.body == 0.12)
	end

	test(producer, consumer)
end

-- Bool
print('[test] Bool')
do
	local function producer(forge)
		forge:beat_time(0.1):bool(true)
		forge:beat_time(0.2):bool(false)
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == Atom.Bool)
		assert(#atom == 4)
		assert(type(atom.body) == 'boolean')
		assert(atom.body == true)
		
		local atom = seq[2]
		assert(atom.type == Atom.Bool)
		assert(#atom == 4)
		assert(type(atom.body) == 'boolean')
		assert(type(atom.body) == 'boolean')
		assert(atom.body == false)
	end

	test(producer, consumer)
end

-- String
print('[test] String')
do
	local str = 'hello world'

	local function producer(forge)
		forge:frame_time(0)
		forge:string(str)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.String)
		assert(#atom == string.len(str) + 1)
		assert(type(atom.body) == 'string')
		assert(atom.body == str)
	end

	test(producer, consumer)
end

-- Literal
print('[test] Literal')
do
	local str = 'hello world'
	local datatype = Map['http://test.org#datatype']
	local lang = Map['http://test.org#lang']

	local function producer(forge)
		forge:frame_time(0)
		forge:literal(str, datatype, lang)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Literal)
		assert(type(atom.body) == 'string')
		assert(#atom == 8 + string.len(str) + 1)
		assert(atom.body == str)
		assert(atom.datatype == datatype)
		assert(atom.lang == lang)
		local str2, datatype2, lang2 = atom:unpack()
		assert(str2 == str)
		assert(datatype2 == datatype)
		assert(lang2 == lang)
	end

	test(producer, consumer)
end

-- URI
print('[test] URI')
do
	local uri = 'http://test.org'

	local function producer(forge)
		forge:frame_time(0)
		forge:uri(uri)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.URI)
		assert(#atom == string.len(uri) + 1)
		assert(type(atom.body) == 'string')
		assert(atom.body == uri)
	end

	test(producer, consumer)
end

-- Path
print('[test] Path')
do
	local path = '/tmp/lua.lua'

	local function producer(forge)
		forge:frame_time(0)
		forge:path(path)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Path)
		assert(#atom == string.len(path) + 1)
		assert(type(atom.body) == 'string')
		assert(atom.body == path)
	end

	test(producer, consumer)
end

-- URID
print('[test] URID')
do
	local uri = 'http://test.org#uri'
	local urid = Map[uri]

	local function producer(forge)
		forge:frame_time(0):urid(urid)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.URID)
		assert(#atom == 4)
		assert(type(atom.body) == 'number')
		assert(atom.body == urid)
		assert(Unmap[atom.body] == uri)
	end

	test(producer, consumer)
end

-- Midi
print('[test] Midi')
do
	local m = {0x90, 0x2a, 0x7f}

	local function producer(forge)
		forge:time(0.1):midi(m)

		forge:time(0.2)
		forge:midi(string.char(table.unpack(m)))

		forge:time(0.3)
		forge:midi(table.unpack(m))
	end

	local function consumer(seq)
		assert(#seq == 3)

		local atom = seq[1]
		assert(atom.type == MIDI.MidiEvent)
		assert(type(atom.body) == 'string')
		assert(#atom == #m)
		assert(#atom.body == #m)
		assert(atom.body:byte(0, 0) == nil)
		assert(atom.body:byte(1, 3) == 0x90, 0x2a, 0x7f)
		assert(atom.body:byte(4, 4) == nil)

		atom = seq[2]
		assert(atom.type == MIDI.MidiEvent)
		assert(#atom == #m)
		assert(atom[0] == nil)
		assert(atom[1] == 0x90)
		assert(atom[2] == 0x2a)
		assert(atom[3] == 0x7f)
		assert(atom[4] == nil)

		local status, note, vel = atom:unpack()
		assert(status == m[1])
		assert(note == m[2])
		assert(vel == m[3])
		
		status, note, vel = atom:unpack(1, 2)
		assert(status == m[1])
		assert(note == m[2])
		assert(vel == nil)
		
		note, vel = atom:unpack(2, 3)
		assert(note == m[2])
		assert(vel == m[3])
		
		status, note, vel = atom:unpack(-1, 5)
		assert(status == m[1])
		assert(note == m[2])
		assert(vel == m[3])

		assert(atom:unpack(-1) == atom:unpack(0))
		assert(atom:unpack(0) == atom:unpack(1))
		
		assert(#{atom:unpack()} == #{atom:unpack(-100, 100)})
		assert(#{atom:unpack(1, 2)} == #{atom:unpack(2, 3)})

		atom = seq[3]
		assert(atom.type == MIDI.MidiEvent)
		assert(atom.body == string.char(table.unpack(m)))
	end

	test(producer, consumer)
end

-- MIDIResponder
print('[test] MIDIResponder')
do
	local _chan = 0x01
	local _note = 0x2a
	local _vel = 0x7f
	local note_on = {MIDI.NoteOn | _chan, _note, _vel}
	local note_off = {MIDI.NoteOff | _chan, _note, _vel}
	local start = {MIDI.Start, 0x1}
	local stop = {MIDI.Stop, 0x2}

	local function producer(forge)
		forge:frame_time(0):midi(note_on)
		forge:frame_time(1):midi(note_off)
		forge:frame_time(2):midi(start)
		forge:frame_time(3):midi(stop)
	end

	local note_on_responder = false
	local note_off_responder = false
	local note_start_responder = false
	local note_stop_responder = false

	local midi_responder = MIDIResponder({
		[MIDI.NoteOn] = function(self, frames, forge, chan, note, vel)
			assert(frames == 0)
			assert(chan == _chan)
			assert(note == _note)
			assert(vel == _vel)
			note_on_responded = true
		end,
		[MIDI.NoteOff] = function(self, frames, forge, chan, note, vel)
			assert(frames == 1)
			assert(chan == _chan)
			assert(note == _note)
			assert(vel == _vel)
			note_off_responded = true
		end,
		[MIDI.Start] = function(self, frames, forge, chan, dat1)
			assert(frames == 2)
			assert(chan == nil)
			assert(dat1 == 0x1)
			note_start_responded = true
		end,
		[MIDI.Stop] = function(self, frames, forge, chan, dat1)
			assert(frames == 3)
			assert(chan == nil)
			assert(dat1 == 0x2)
			note_stop_responded = true
		end
	})

	local function consumer(seq)
		for frames, atom in seq:foreach() do
			assert(midi_responder(frames, nil, atom) == true)
		end

		assert(note_on_responded)
		assert(note_off_responded)
		assert(note_start_responded)
		assert(note_stop_responded)
	end

	test(producer, consumer)
end

local function consumer_chunk(seq, c, atype)
	assert(#seq == 3)

	local atom = seq[1]
	assert(atom.type == atype)
	assert(type(atom.body) == 'string')
	assert(#atom == #c)
	assert(#atom.body == #c)
	assert(atom.body:byte(0) == nil)
	assert(atom.body:byte(1, 6) == 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)
	assert(atom.body:byte(7) == nil)

	atom = seq[2]
	assert(atom.type == atype)
	assert(#atom == #c)
	assert(atom[0] == nil)
	assert(atom[1] == 0x01)
	assert(atom[2] == 0x02)
	assert(atom[3] == 0x03)
	assert(atom[4] == 0x04)
	assert(atom[5] == 0x05)
	assert(atom[6] == 0x06)
	assert(atom[7] == nil)

	local c1, c2, c3, c4, c5, c6 = atom:unpack()
	assert(c1 == c[1])
	assert(c2 == c[2])
	assert(c3 == c[3])
	assert(c4 == c[4])
	assert(c5 == c[5])
	assert(c6 == c[6])

	c1, c2 = atom:unpack(1, 2)
	c4, c5, c6 = atom:unpack(4, 6)
	assert(c1 == c[1])
	assert(c2 == c[2])
	assert(c3 == c[3])
	assert(c4 == c[4])
	assert(c5 == c[5])
	assert(c6 == c[6])

	assert(atom:unpack(-1) == atom:unpack(0))
	assert(atom:unpack(0) == atom:unpack(1))
	
	assert(#{atom:unpack()} == #{atom:unpack(-100, 100)})
	assert(#{atom:unpack(1, 2)} == #{atom:unpack(2, 3)})

	atom = seq[3]
	assert(atom.type == atype)
	assert(atom.body == string.char(table.unpack(c)))
end

-- Chunk
print('[test] Chunk')
do
	local c = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}

	local function producer(forge)
		forge:frame_time(0)
		forge:chunk(c)

		forge:frame_time(0)
		forge:chunk(string.char(table.unpack(c)))

		forge:frame_time(0)
		forge:chunk(table.unpack(c))
	end

	local function consumer(seq)
		consumer_chunk(seq, c, Atom.Chunk)
	end

	test(producer, consumer)
end

-- Raw 
print('[test] Raw')
do
	local u = Map['http://open-music-kontrollers.ch/lv2/moony#raw']
	local c = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}

	local function producer(forge)
		forge:frame_time(0)
		forge:raw(u, c)

		forge:frame_time(0)
		forge:raw(u, string.char(table.unpack(c)))

		forge:frame_time(0)
		forge:raw(u, table.unpack(c))
	end

	local function consumer(seq)
		consumer_chunk(seq, c, u)
	end

	test(producer, consumer)
end

-- Tuple
print('[test] Tuple')
do
	local function producer(forge)
		forge:frame_time(0)
		local tup = forge:tuple()
		assert(tup ~= forge)
		tup:int(1):float(2.0):long(3):double(4.0):pop()

		forge:frame_time(1):tuple():pop()
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == Atom.Tuple)
		assert(#atom == 4)

		local sub = atom[1]
		assert(sub.type == Atom.Int)
		assert(sub.body == 1)
		
		sub = atom[2]
		assert(sub.type == Atom.Float)
		assert(sub.body == 2.0)
		
		sub = atom[3]
		assert(sub.type == Atom.Long)
		assert(sub.body == 3)
		
		sub = atom[4]
		assert(sub.type == Atom.Double)
		assert(sub.body == 4.0)

		assert(atom[0] == nil)
		assert(atom[5] == nil)

		for i, sub in atom:foreach() do
			assert(atom[i].body == sub.body)
		end

		local subsub
		sub, subsub = atom:unpack(1, 1)
		assert(sub.type == Atom.Int)
		assert(subsub == nil)
		
		sub, subsub = atom:unpack(2)
		assert(sub.type == Atom.Float)
		assert(subsub.type == Atom.Long)
		
		assert(atom:unpack(-1).type == atom:unpack(0).type)
		assert(atom:unpack(0).type == atom:unpack(1).type)
		
		assert(#{atom:unpack()} == #{atom:unpack(-100, 100)})
		assert(#{atom:unpack(1, 2)} == #{atom:unpack(3, 4)})

		atom = seq[2]
		assert(atom.type == Atom.Tuple)
		assert(#atom == 0)
		assert(atom[0] == nil)
		assert(atom[1] == nil)
	end

	test(producer, consumer)
end

-- Object
print('[test] Object')
do
	local id = 0
	local otype = Map['http://test.org#type']
	local key1 = Map['http://test.org#key1']
	local ctx2 = Map['http://test.org#ctx2']
	local key2 = Map['http://test.org#key2']

	local function producer(forge)
		assert(forge:frame_time(0) == forge)
		local obj = forge:object(id, otype)
		assert(obj ~= forge)

		assert(obj:key(key1):int(12) == obj)
		
		obj:property(key2, ctx2):long(13)

		assert(obj:pop() == nil)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Object)
		assert(atom.id == 0)
		assert(atom.otype == otype)
		assert(#atom == 2)

		local sub = atom[key1]
		assert(sub.type == Atom.Int)
		
		sub = atom[key2]
		assert(sub.body == 13)

		for key, sub, ctx in atom:foreach() do
			assert(atom[key].body == sub.body)

			if(key == key2) then
				assert(ctx == ctx2)
			end
		end
	end

	test(producer, consumer)
end

-- Atom
print('[test] Atom')
do
	local forge0 = nil

	local function producer(forge)
		forge0 = forge

		forge:frame_time(0)
		forge:int(12)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Int)
		assert(atom.body == 12)

		forge0:frame_time(0):atom(atom)
	end

	test(producer, consumer)
end

-- OSC
print('[test] OSC')
do
	local function producer(forge)
		forge:frame_time(0)
		forge:message('/hello', 'sif', 'world', 12, 13.0)
		
		forge:frame_time(1)
		forge:message('/hallo', 'Shdt', 'velo', 12, 13.0, 1)
		
		forge:frame_time(2)
		forge:message('/yup', 'c', string.byte('a'))
		
		forge:frame_time(3)
		forge:message('/singletons', 'TFNI')
		
		forge:frame_time(4)
		forge:message('/chunky', 'mb', string.char(0x90, 0x20, 0x7f), string.char(0x01, 0x02, 0x03, 0x04))
		
		forge:frame_time(5)
		local bndl = forge:bundle(1)
		assert(bndl ~= forge)
		bndl:message('/one', 'i', 1)
		bndl:message('/two', 'i', 2)
		bndl:message('/three', 'i', 3)
		bndl:bundle(0.1):pop() -- nested
		assert(bndl:pop() == nil)
		
		forge:frame_time(6)
		forge:message('/color', 'r', 0xff00007f)
	end

	local function consumer(seq)
		assert(#seq == 7)

		local atom = seq[1]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		local path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.body == '/hello')
		local args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 3)
		assert(args[0] == nil)
		assert(args[1].type == Atom.String)
		assert(args[1].body == 'world')
		assert(args[2].type == Atom.Int)
		assert(args[2].body == 12)
		assert(args[3].type == Atom.Float)
		assert(args[3].body == 13.0)
		assert(args[4] == nil)
		
		atom = seq[2]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.body == '/hallo')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 4)
		assert(args[0] == nil)
		assert(args[1].type == Atom.URID)
		assert(args[1].body == Map['velo'])
		assert(args[2].type == Atom.Long)
		assert(args[2].body == 12)
		assert(args[3].type == Atom.Double)
		assert(args[3].body == 13.0)
		assert(args[4].type == Atom.Object)
		assert(args[4].otype == OSC.Timetag)
		assert(args[4][OSC.timetagIntegral].body == 0)
		assert(args[4][OSC.timetagFraction].body == 1)
		assert(args[5] == nil)
		
		atom = seq[3]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.body == '/yup')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 1)
		assert(args[0] == nil)
		assert(args[1].type == Atom.Literal)
		assert(args[1].datatype == OSC.Char)
		assert(args[1].body == 'a')
		assert(args[2] == nil)
		
		atom = seq[4]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.body == '/singletons')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 4)
		assert(args[0] == nil)
		assert(args[1].type == Atom.Bool)
		assert(args[1].body == true)
		assert(args[2].type == Atom.Bool)
		assert(args[2].body == false)
		assert(args[3].type == Atom.Literal)
		assert(args[3].datatype == OSC.Nil)
		assert(args[4].type == Atom.Literal)
		assert(args[4].datatype == OSC.Impulse)
		assert(args[4].body == '')
		assert(args[5] == nil)
		
		atom = seq[5]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.body == '/chunky')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 2)
		assert(args[0] == nil)
		assert(args[1].type == MIDI.MidiEvent)
		assert(#args[1] == 3)
		assert(args[1][1] == 0x90)
		assert(args[1][2] == 0x20)
		assert(args[1][3] == 0x7f)
		assert(args[2].type == Atom.Chunk)
		assert(#args[2] == 4)
		assert(args[2][1] == 0x01)
		assert(args[2][2] == 0x02)
		assert(args[2][3] == 0x03)
		assert(args[2][4] == 0x04)
		assert(args[5] == nil)
		
		atom = seq[6]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Bundle)
		local timetag = atom[OSC.bundleTimetag]
		assert(timetag.type == Atom.Object)
		assert(timetag.otype == OSC.Timetag)
		assert(timetag[OSC.timetagIntegral].body == 0)
		assert(timetag[OSC.timetagFraction].body == 1)
		local itms = atom[OSC.bundleItems]
		assert(itms.type == Atom.Tuple)
		assert(#itms == 4)
		assert(itms[1].otype == OSC.Message)
		assert(itms[2].otype == OSC.Message)
		assert(itms[3].otype == OSC.Message)
		assert(itms[4].otype == OSC.Bundle)
		assert(#itms[4][OSC.bundleItems] == 0)
		
		atom = seq[7]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args== 1)
		assert(args[1].type == Atom.Literal)
		assert(args[1].datatype == OSC.RGBA)
		assert(args[1].body == 'ff00007f')
	end

	test(producer, consumer)
end

-- OSCResponder
print('[test] OSCResponder')
do
	local function producer(forge)
		forge:frame_time(0):message('/ping', 'i', 13)
		forge:frame_time(1):bundle(0):message('/pong', 's', 'world'):pop()

		forge:frame_time(2):message('/one/two/three', 'd', 12.3)
		forge:frame_time(2):message('/one/*/three', 'd', 12.3)
		forge:frame_time(2):message('/?ne/*/three', 'd', 12.3)
		forge:frame_time(2):message('/?ne/*/{three,four}', 'd', 12.3)
		forge:frame_time(2):message('/?ne/*/[tT]hree', 'd', 12.3)
		forge:frame_time(2):message('/?ne/*/[!T]hree', 'd', 12.3)
		forge:frame_time(2):message('/{one,eins}/*/{three,drei}', 'd', 12.3)
	end

	local ping_responded = false
	local pong_responded = false
	local complex_responded = 0

	local osc_responder = OSCResponder({
		['/ping'] = function(self, frames, forge, fmt, i)
			assert(frames == 0)
			assert(fmt == 'i')
			assert(i == 13)
			ping_responded = true
		end,
		['/pong'] = function(self, frames, forge, fmt, s)
			assert(frames == 1)
			assert(fmt == 's')
			assert(s == 'world')
			pong_responded = true
		end,
		['/one/two/three'] = function(self, frames, forge, fmt, d)
			assert(frames == 2)
			assert(fmt == 'd')
			assert(d == 12.3)
			complex_responded = complex_responded + 1
		end
	})

	local function consumer(seq)
		for frames, atom in seq:foreach() do
			assert(osc_responder(frames, nil, atom) == true)
		end

		assert(ping_responded)
		assert(pong_responded)
		assert(complex_responded == 7)
	end

	test(producer, consumer)
end

-- TimeResponder
print('[test] TimeResponder')
do
	local rolling = 0
	local bar_beat_responded = 0
	local bar_responded = 0
	local beat_unit_responded = 0
	local beats_per_bar_responded = 0
	local beats_per_minute_responded = 0
	local frame_responded = 0
	local frames_per_second_responded = 0
	local speed_responded = 0

	local time_cb = {
		[Time.barBeat] = function(self, frames, forge, bar_beat)
			if bar_beat_responded == 0 then
				assert(bar_beat == 0.0)
				bar_beat_responded = 1
			else
				assert(frames == 0)
				assert(bar_beat == 0.5)
				bar_beat_responded = 2
			end
		end,
		[Time.bar] = function(self, frames, forge, bar)
			if bar_responded == 0 then
				assert(bar == 0)
				bar_responded = 1
			else
				assert(frames == 0)
				assert(bar == 34)
				bar_responded = 2
			end
		end,
		[Time.beatUnit] = function(self, frames, forge, beat_unit)
			if beat_unit_responded == 0 then
				assert(beat_unit == 4)
				beat_unit_responded = 1
			else
				assert(frames == 0)
				assert(beat_unit == 8)
				beat_unit_responded = 2
			end
		end,
		[Time.beatsPerBar] = function(self, frames, forge, beats_per_bar)
			if beats_per_bar_responded == 0 then
				assert(beats_per_bar == 4.0)
				beats_per_bar_responded = 1
			else
				assert(frames == 0)
				assert(beats_per_bar == 6.0)
				beats_per_bar_responded = 2
			end
		end,
		[Time.beatsPerMinute] = function(self, frames, forge, beats_per_minute)
			if beats_per_minute_responded == 0 then
				assert(beats_per_minute == 120.0)
				beats_per_minute_responded = 1
			else
				assert(frames == 0)
				assert(beats_per_minute == 200.0)
				beats_per_minute_responded = 2
			end
		end,
		[Time.frame] = function(self, frames, forge, frame)
			if frame_responded == 0 then
				frame_responded = 1
			else
				frame_responded = 2
			end
		end,
		[Time.framesPerSecond] = function(self, frames, forge, frames_per_second)
			if frames_per_second_responded == 0 then
				assert(frames_per_second == 48000.0)
				frames_per_second_responded = 1
			else
				assert(frames == 0)
				assert(frames_per_second == 44100.0)
				frames_per_second_responded = 2
			end
		end,
		[Time.speed] = function(self, frames, forge, speed)
			rolling = speed > 0.0 and true or false
			if speed_responded == 0 then
				assert(speed == 0.0)
				speed_responded = 1
			else
				assert(frames == 0)
				assert(speed == 1.0)
				speed_responded = 2
			end
		end,
	}
	local time_responder = TimeResponder(time_cb)

	local function producer(forge)
		local obj = forge:frame_time(0):object(0, Time.Position)
		obj:key(Time.barBeat):float(0.5)
		obj:key(Time.bar):long(34)
		obj:key(Time.beatUnit):int(8)
		obj:key(Time.beatsPerBar):float(6.0)
		obj:key(Time.beatsPerMinute):float(200.0)
		obj:key(Time.frame):long(23000)
		obj:key(Time.framesPerSecond):float(44100.0)
		obj:key(Time.speed):float(1.0)
		obj:pop()

		-- time:stash
		forge:frame_time(1)
		time_responder:stash(forge)
	end

	local function consumer(seq)
		assert(#seq == 2)

		local from = 0
		for frames, atom in seq:foreach() do
			if frames == 0 then
				assert(atom.type == Atom.Object)
				assert(atom.otype == Time.Position)
				time_responder(from, frames, nil, atom)
				from = frames
			end
		end
		time_responder(from, 256, nil, nil)

		assert(bar_beat_responded == 2)
		assert(bar_responded == 2)
		assert(beat_unit_responded == 2)
		assert(beats_per_bar_responded == 2)
		assert(beats_per_minute_responded == 2)
		assert(frame_responded == 2)
		assert(frames_per_second_responded == 2)
		assert(speed_responded == 2)

		assert(time_responder[Time.barBeat] > 0.5) --!
		assert(time_responder[Time.bar] == 34)
		assert(time_responder[Time.beatUnit] == 8)
		assert(time_responder[Time.beatsPerBar] == 6.0)
		assert(time_responder[Time.beatsPerMinute] == 200.0)
		assert(time_responder[Time.frame] == 23000 + 256)
		assert(time_responder[Time.framesPerSecond] == 44100.0)
		assert(time_responder[Time.speed] == 1.0)

		-- test time:stash
		local atom = seq[1]
		assert(atom.type == Atom.Object)
		assert(atom.otype == Time.Position)
		assert(atom[Time.barBeat].body == 0.5)
		assert(atom[Time.bar].body == 34)
		assert(atom[Time.beatUnit].body == 8)
		assert(atom[Time.beatsPerBar].body == 6.0)
		assert(atom[Time.beatsPerMinute].body == 200.0)
		assert(atom[Time.frame].body == 23000)
		assert(atom[Time.framesPerSecond].body == 44100.0)
		assert(atom[Time.speed].body == 1.0)

		-- test time:apply
		--time_responder(0, 0, nil, atom)
		time_responder:apply(atom)
		assert(time_responder[Time.barBeat] == 0.5)
		assert(time_responder[Time.bar] == 34)
		assert(time_responder[Time.beatUnit] == 8)
		assert(time_responder[Time.beatsPerBar] == 6.0)
		assert(time_responder[Time.beatsPerMinute] == 200.0)
		assert(time_responder[Time.frame] == 23000)
		assert(time_responder[Time.framesPerSecond] == 44100.0)
		assert(time_responder[Time.speed] == 1.0)
	end

	test(producer, consumer)
end

-- Sequence
print('[test] Sequence')
do

	local function producer(forge)
		forge:frame_time(0)
		local subseq = forge:sequence(Atom.beatTime)
		assert(subseq ~= forge)

		subseq:time(1.1):int(1)
		subseq:beat_time(2.2):int(1)

		assert(subseq:pop() == nil)

		forge:frame_time(1):sequence():pop()
	end

	local function consumer(seq)
		assert(#seq == 2)
		local subseq = seq[1]
		assert(subseq.type == Atom.Sequence)
		assert(subseq.unit == Atom.beatTime)
		assert(#subseq == 2)

		for frames, atom in subseq:foreach() do
			assert(not math.tointeger(frames)) -- beatTime is double
			assert(atom.type == Atom.Int)
			assert(atom.body == 1)
		end

		subseq = seq[2]
		assert(subseq.type == Atom.Sequence)
		assert(subseq.unit == 0)
		assert(#subseq == 0)
		assert(subseq[0] == nil)
		assert(subseq[1] == nil)
	end

	test(producer, consumer)
end

-- Vector
print('[test] Vector')
do

	local function producer(forge)
		forge:time(0)
		forge:vector(Atom.Int, {1, 2, 3, 4})

		forge:time(1)
		forge:vector(Atom.Long, 5, 6, 7, 8)
		
		forge:time(2)
		forge:vector(Atom.Bool, {true, false})
		
		forge:time(3)
		forge:vector(Atom.Float, 1.0, 2.0)
		
		forge:time(4)
		forge:vector(Atom.Double, {3.3, 4.4})
		
		forge:time(5)
		forge:vector(Atom.URID, Atom.Int, Atom.Long)
	end

	local function consumer(seq)
		assert(#seq == 6)

		local vec = seq[1]
		assert(vec.type == Atom.Vector)
		assert(#vec == 4)
		assert(vec.child_type == Atom.Int)
		assert(vec.child_size == 4)

		for i, atom in vec:foreach() do
			assert(atom.type == Atom.Int)
			assert(atom.body == i)
		end
		
		vec = seq[2]
		assert(vec.type == Atom.Vector)
		assert(#vec == 4)
		assert(vec.child_type == Atom.Long)
		assert(vec.child_size == 8)

		for i, atom in vec:foreach() do
			assert(atom.type == Atom.Long)
			assert(atom.body == i + 4)
		end

		vec = seq[3]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.Bool)
		assert(vec.child_size == 4)
		assert(vec[0] == nil)
		assert(vec[1].body == true)
		assert(vec[2].body == false)
		assert(vec[3] == nil)

		vec = seq[4]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.Float)
		assert(vec.child_size == 4)
		local a, b = vec:unpack()
		assert(a.body == 1.0)
		assert(b.body == 2.0)
		
		vec = seq[5]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.Double)
		assert(vec.child_size == 8)
		local a, b = vec:unpack()
		assert(a.body == 3.3)
		assert(b.body == 4.4)
		
		vec = seq[6]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.URID)
		assert(vec.child_size == 4)
		local a, b = vec:unpack(1, 1)
		assert(a.body == Atom.Int)
		assert(b == nil)
		local a, b = vec:unpack(2, 2)
		assert(a.body == Atom.Long)
		assert(b == nil)
	end

	test(producer, consumer)
end

-- Options
print('[test] Options')
do

	assert(Core.sampleRate == Map['http://lv2plug.in/ns/lv2core#sampleRate'])
	assert(Options[Core.sampleRate] == 48000)
	assert(Options[Buf_Size.minBlockLength] == nil)
	assert(Options[Buf_Size.maxBlockLength] == nil)
	assert(Options[Buf_Size.sequenceSize] == nil)
end

-- Patch
print('[test] Patch')
do
	local subject = Map['http://open-music-kontrollers.ch/lv2/moony#subject']
	local property = Map['http://open-music-kontrollers.ch/lv2/moony#property']
	local access = Patch.writable
	local rtid = VoiceMap() & 0x7fffffff

	local function producer(forge)
		forge:time(0):get(subject, property, rtid)

		forge:time(1):get(nil, property, rtid)

		local set = forge:time(2):set(subject, property, rtid)
		set:string('hello world'):pop()
		
		local set = forge:time(3):set(nil, property, rtid)
		set:string('hello world'):pop()

		local patch = forge:time(4):patch(subject, rtid)
		patch:remove():key(access):urid(Patch.wildcard):pop()
		patch:add():key(access):urid(property):pop()
		patch:pop()

		patch = forge:time(5):patch(property, rtid)
		local remove = patch:remove()
		remove:key(RDFS.label):urid(Patch.wildcard)
		remove:key(RDFS.range):urid(Patch.wildcard)
		remove:key(Core.minimum):urid(Patch.wildcard)
		remove:key(Core.maximum):urid(Patch.wildcard)
		remove:pop()
		local add = patch:add()
		add:key(RDFS.label):string('A dummy label')
		add:key(RDFS.range):urid(Atom.Int)
		add:key(Core.minimum):int(0)
		add:key(Core.maximum):int(10)
		add:pop()
		patch:pop()

		local put = forge:time(6):put(subject, rtid)
		put:key(property):string('hello world')
		put:pop()
	end

	local function consumer(seq)
		assert(#seq == 7)

		local get = seq[1]
		assert(get.type == Atom.Object)
		assert(get.otype == Patch.Get)
		assert(#get == 3)
		assert(get[Patch.subject].body == subject)
		assert(get[Patch.property].body == property)
		assert(get[Patch.sequenceNumber].body == rtid)

		get = seq[2]
		assert(get.type == Atom.Object)
		assert(get.otype == Patch.Get)
		assert(#get == 2)
		assert(get[Patch.subject] == nil)
		assert(get[Patch.property].body == property)
		assert(get[Patch.sequenceNumber].body == rtid)

		local set = seq[3]
		assert(set.type == Atom.Object)
		assert(set.otype == Patch.Set)
		assert(#set == 4)
		assert(set[Patch.subject].body == subject)
		assert(set[Patch.property].body == property)
		assert(set[Patch.value].body == 'hello world')
		assert(set[Patch.sequenceNumber].body == rtid)

		set = seq[4]
		assert(set.type == Atom.Object)
		assert(set.otype == Patch.Set)
		assert(#set == 3)
		assert(set[Patch.subject] == nil)
		assert(set[Patch.property].body == property)
		assert(set[Patch.value].body == 'hello world')
		assert(set[Patch.sequenceNumber].body == rtid)

		local patch = seq[5]
		assert(patch.type == Atom.Object)
		assert(patch.otype == Patch.Patch)
		assert(#patch == 4)
		assert(patch[Patch.subject].body == subject)
		assert(patch[Patch.sequenceNumber].body == rtid)
		assert(patch[Patch.remove][access].body == Patch.wildcard) 
		assert(patch[Patch.add][access].body == property) 

		patch = seq[6]
		assert(patch.type == Atom.Object)
		assert(patch.otype == Patch.Patch)
		assert(#patch == 4)
		assert(patch[Patch.subject].body == property)
		assert(patch[Patch.sequenceNumber].body == rtid)
		assert(patch[Patch.remove][RDFS.label].body == Patch.wildcard)
		assert(patch[Patch.remove][RDFS.range].body == Patch.wildcard) 
		assert(patch[Patch.remove][Core.minimum].body == Patch.wildcard) 
		assert(patch[Patch.remove][Core.maximum].body == Patch.wildcard) 
		assert(patch[Patch.add][RDFS.label].body == 'A dummy label') 
		assert(patch[Patch.add][RDFS.range].body == Atom.Int) 
		assert(patch[Patch.add][Core.minimum].body == 0) 
		assert(patch[Patch.add][Core.maximum].body == 10) 

		local put = seq[7]
		assert(put.type == Atom.Object)
		assert(put.otype == Patch.Put)
		assert(#put == 3)
		assert(put[Patch.subject].body == subject)
		assert(put[Patch.sequenceNumber].body == rtid)
		local body = put[Patch.body]
		assert(body)
		assert(#body == 1)
		assert(body[property].body == 'hello world')
	end

	test(producer, consumer)
end

-- StateResponder
print('[test] StateResponder')
do
	prefix = 'http://open-music-kontrollers.ch/lv2/moony#test'
	local urid = {
		subject = Map[prefix],
		int = Map[prefix .. '#int'],
		flt = Map[prefix .. '#flt']
	}

	local flt_get_responded = false
	local flt_set_responded = false

	local state_int = {
		[RDFS.label] = 'Int',
		[RDFS.range] = Atom.Int,
		[Core.minimum] = 0,
		[Core.maximum] = 10,
		[RDF.value] = 1
	}

	local state_flt = {
		[RDFS.label] = 'Flt',
		[RDFS.range] = Atom.Float,
		[Core.minimum] = -0.5,
		[Core.maximum] = 10.0,
		[RDF.value] = 1.0,
		[Patch.Get] = function(self, frames, forge)
			flt_get_responded = true
			return self[RDF.value]
		end,
		[Patch.Set] = function(self, frames, forge, value)
			flt_set_responded = true
			self[RDF.value] = value
		end
	}

	local state = StateResponder({
		[Patch.writable] = {
			[urid.int] = state_int,
			[urid.flt] = state_flt
		}
	})

	local function producer(forge)
		forge:frame_time(0):get(urid.subject, urid.int)
		forge:frame_time(1):set(urid.subject, urid.int):typed(Atom.Int, 2):pop()

		forge:frame_time(2):get(urid.subject, urid.flt)
		forge:frame_time(3):set(urid.subject, urid.flt):typed(Atom.Float, 2.0):pop()

		-- state:stash
		forge:frame_time(4)
		state:stash(forge)
	end

	local function consumer(seq, forge)
		for frames, atom in seq:foreach() do
			if frames < 4 then
				assert(state(frames, forge, atom) == true)
			end
		end

		assert(state_int[RDF.value] == 2)
		assert(state_flt[RDF.value] == 2.0)
		assert(flt_get_responded)
		assert(flt_set_responded)

		--test state:stash
		local atom = seq[5]
		assert(atom.type == Atom.Object)
		assert(#atom == 2)
		assert(atom[urid.int].type == Atom.Int)
		assert(atom[urid.int].body == 1)
		assert(atom[urid.flt].type == Atom.Float)
		assert(atom[urid.flt].body == 1.0)

		-- test state:apply
		state:apply(atom)
		assert(state_int[RDF.value] == 1)
		assert(state_flt[RDF.value] == 1.0)
	end

	test(producer, consumer)
end

-- Note 
print('[test] Note')
do
	assert(Note[0] == 'C-0')
	assert(Note[1] == 'C#-0')
	assert(Note(12) == 'C-1')
	assert(Note(13) == 'C#-1')

	assert(Note['C-0'] == 0)
	assert(Note['C#-0'] == 1)
	assert(Note['C-1'] == 12)
	assert(Note('C#-1') == 13)

	assert(Note['C-5'] == 60)
	assert(Note[60] == 'C-5')

	assert(Note[-1] == nil)
	assert(Note(0x80) == nil)
	assert(Note.foo == nil)
	assert(Note.bar == nil)

	local keys = {'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'H'}

	local function itr()
		return coroutine.wrap(function()
			for octave = 0, 9 do
				for i, key in ipairs(keys) do
					coroutine.yield(octave * #keys + i - 1, key .. '-' .. octave)
				end
			end
		end)
	end

	for i, note_name in itr() do
		assert(Note[i] == note_name)
		assert(Note[note_name] == i)
	end
end

-- Clone
print('[test] Clone')
do
	local function producer(forge)
		forge:time(1):int(1)
		forge:time(2):long(2)
	end

	local references = {}
	local clones = {}

	local function consumer(seq)
		for frames, atom in seq:foreach() do
			local clone = atom:clone()

			assert(#atom == #clone)
			assert(atom.type == clone.type)
			assert(atom.body == clone.body)

			clones[frames] = clone
			references[frames] = atom
		end
	end

	test(producer, consumer)

	assert(#clones[1] == 4)
	assert(clones[1].type == Atom.Int)
	assert(clones[1].body == 1)

	assert(#clones[2] == 8)
	assert(clones[2].type == Atom.Long)
	assert(clones[2].body == 2)

	assert(#references[1] == 8) -- latoms in :foreach are recycled
	assert(references[1].type == Atom.Long)
	assert(references[1].body == 2)

	assert(#references[2] == 8)
	assert(references[2].type == Atom.Long)
	assert(references[2].body == 2)
end

-- Stash
print('[test] Stash')
do
	local stash = Stash()

	stash:int(12)

	stash:read()
	assert(#stash == 4)
	assert(stash.type == Atom.Int)
	assert(stash.body == 12)

	stash:write()
	stash:read()
	assert(#stash == 0)
	assert(stash.type == 0)
end

-- Equal
print('[test] Equal')
do
	local stash1 = Stash()
	local stash2 = Stash()
	local stash3 = Stash()
	local stash4 = Stash()

	stash1:int(11)
	stash2:int(11)
	stash3:int(12)
	stash4:long(11)

	local atom1 = stash1:read()
	local atom2 = stash2:read()
	local atom3 = stash3:read()
	local atom4 = stash4:read()

	assert(atom1 == atom2)
	assert(atom1 ~= atom3)
	assert(atom1 ~= atom4)
	assert(atom2 ~= atom3)
	assert(atom2 ~= atom4)
	assert(atom3 ~= atom4)
end

-- Byte Code
print('[test] Byte Code')
do
	local env = {
		foo = 'foo'
	}

	local function bar()
		foo = 'bar'
	end

	local function producer(forge)
		local chunk = string.dump(bar) -- dump byte code of function bar
		forge:time(1):chunk(chunk)
	end

	local function consumer(seq)
		for frames, atom in seq:foreach() do
			if atom.type == Atom.Chunk then
				local f = load(atom.body, 'rt-chunk', 'b', env)
				if f then
					foo = f()
				end
			end
		end
	end

	assert(env.foo == 'foo')

	test(producer, consumer)

	assert(env.foo == 'bar')
end

-- voiceMap
print('[test] VoiceMap')
do
	local ids = {}
	for i=1, 1000 do
		local id = VoiceMap()
		assert(ids[id] == nil)
		ids[id] = true
	end
end

-- midi2cps/cps2midi
print('[test] midi2cps/cps2midi')
do
	for note = 0.0, 127.0, 0.1 do
		local cps = midi2cps(note)
		assert(note - cps2midi(cps) < 1e-4)
	end
	assert(69.0 == cps2midi(440.0))
	assert(440.0 == midi2cps(69.0))
end

-- midi2cps/cps2midi
print('[test] HashMap')
do
	local prefix = 'http://open-music-kontrollers.ch/lv2/synthpod#'
	local hashmap = HashMap(prefix)
	assert(Map[prefix .. 'addModule'] == hashmap.addModule)
	assert(Map[prefix .. 'addModule'] == hashmap.addModule)
	assert(Map[prefix .. 'connectPort'] == hashmap.connectPort)
	assert(Map[prefix .. 'connectPort'] == hashmap.connectPort)
end

-- multiplex
print('[test] multiplex')
do
	local stash1 = Stash()
	local stash2 = Stash()

	local seq1 = stash1:sequence()
	seq1:time(0):int(0)
	seq1:time(2):int(2)
	seq1:time(4):int(4)
	seq1:pop()

	local seq2 = stash2:sequence()
	seq2:time(1):int(1)
	seq2:time(3):int(3)
	seq2:time(5):int(5)
	seq2:pop()

	stash1:read()
	stash2:read()

	assert(#stash1 == 3)
	assert(#stash2 == 3)

	local i = 0
	for frames, atom, seq in stash1:foreach(stash2) do
		assert(frames == i)
		assert(atom.type == Atom.Int)
		assert(atom.body == i)
		if i % 2 == 0 then
			assert(seq == stash1)
		else
			assert(seq == stash2)
		end

		i = i + 1
	end
end

print('[test] encrypt')
do
	local key = string.char(
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c)
  local inp = string.char(
		0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
		0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a)
	local out = string.char(
		0x00, 0x00, 0x00, 0x10, -- size = 16
		0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
		0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97)

	local sec = encrypt(inp, key)
	assert(#sec - 4 == #inp)
	assert(#sec == #out)
	assert(sec == out)
end

print('[test] decrypt')
do
	local key = '2b7e151628aed2a6abf7158809cf4f3c'
  local inp = string.char(
		0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
		0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a)
	local out = string.char(
		0x00, 0x00, 0x00, 0x10, -- size = 16
		0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
		0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97)

	local dec = decrypt(out, key)
	assert(dec == inp)
end

print('[test] obfuscate')
do
	local key = string.char(
		0x43, 0x4e, 0x5e, 0x42, 0x1a, 0x9e, 0x4f, 0xda,
		0xb0, 0x83, 0xa2, 0xeb, 0xf2, 0xd9, 0x78, 0x07)
  local chunk_1 = 'return "hello world"'
	local chunk_2 = 'hello world'

	local sec = encrypt(chunk_1, key)
	local dec = decrypt(sec, key)
	assert(type(dec) == 'function')
	assert(dec() == 'hello world')

	sec = encrypt(chunk_2, key)
	dec = decrypt(sec, key)
	assert(type(dec) == 'string')
	assert(#dec == #chunk_2)
	assert(dec == chunk_2)
end
