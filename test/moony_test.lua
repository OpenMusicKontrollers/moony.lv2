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
		print(forge)

		forge:frameTime(0)
		forge:int(0x7fffffff)

		forge:frameTime(0):int(0xffffffff)
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
		forge:frameTime(0)
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
		forge:frameTime(0):long(0x100000000)
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
		forge:frameTime(0)
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
		forge:frameTime(0):double(0.12)
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
		forge:beatTime(0.1):bool(true)
		forge:beatTime(0.2):bool(false)
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		print(tostring(atom))
		assert(atom.type == Atom.Bool)
		assert(#atom == 4)
		assert(type(atom.body) == 'boolean')
		assert(atom.body == true)

		local atom = seq[2]
		print(tostring(atom))
		assert(atom.type == Atom.Bool)
		assert(#atom == 4)
		assert(type(atom.body) == 'boolean')
		assert(type(atom.body) == 'boolean')
		assert(atom.body == false)

		assert(atom.foo == nil)
	end

	test(producer, consumer)
end

-- String
print('[test] String')
do
	local str = 'hello world'

	local function producer(forge)
		forge:frameTime(0)
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
		forge:frameTime(0)
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

		assert(atom.foo == nil)
	end

	test(producer, consumer)
end

-- URI
print('[test] URI')
do
	local uri = 'http://test.org'

	local function producer(forge)
		forge:frameTime(0)
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
		forge:frameTime(0)
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
		forge:frameTime(0):urid(urid)
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

-- Typed
print('[test] Typed')
do
	local path = '/tmp'
	local uri = 'http://test.org#uri'
	local urid = Map[uri]

	local function producer(forge)
		forge:time(0):typed(Atom.Bool, true)
		forge:time(0):typed(Atom.Int, 1)
		forge:time(0):typed(Atom.Float, 2)
		forge:time(0):typed(Atom.Long, 3)
		forge:time(0):typed(Atom.Double, 4)
		forge:time(0):typed(Atom.URI, uri)
		forge:time(0):typed(Atom.URID, urid)
		forge:time(0):typed(Atom.Path, path)
		forge:time(0):typed(Atom.String, 'hello')
		forge:time(0):typed(Atom.Literal, 'hello')
		forge:time(0):typed(Atom.Chunk, string.char(0x1))
		forge:time(0):typed(MIDI.MidiEvent, string.char(0x1))
		forge:time(0):typed(Atom.Tuple):pop()
		forge:time(0):typed(Atom.Object):pop()
		forge:time(0):typed(Atom.Property, Param.sampleRate):pop()
		forge:time(0):typed(Atom.Vector, Atom.Int):pop()
		forge:time(0):typed(Atom.Sequence):pop()
		forge:time(0):typed(OSC.Message, '/ping', '')
		forge:time(0):typed(OSC.Bundle):pop()
	end

	local function consumer(seq)
		--FIXME
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
	local ctrl  = {MIDI.Controller, MIDI.SustainPedal, 0x7f}

	local function producer(forge)
		forge:frameTime(0):midi(note_on)
		forge:frameTime(1):midi(note_off)
		forge:frameTime(2):midi(start)
		forge:frameTime(3):midi(stop)
		forge:frameTime(4):string('fooling you')
		forge:frameTime(5):midi(ctrl)
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
	}, true)

	local function consumer(seq, forge)
		for frames, atom in seq:foreach() do
			local handled = midi_responder(frames, forge, atom)
			if frames == 4 then
				assert(handled == false)
			else
				assert(handled == true)
			end
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
	assert(atom.raw == string.char(table.unpack(c)))
	assert(type(atom.body) == 'string')
	assert(#atom == #c)
	assert(#atom.body == #c)
	assert(atom.body:byte(0) == nil)
	assert(atom.body:byte(1, 6) == 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)
	assert(atom.body:byte(7) == nil)

	atom = seq[2]
	assert(atom.type == atype)
	assert(atom.raw == string.char(table.unpack(c)))
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
	assert(atom.raw == string.char(table.unpack(c)))
	assert(atom.body == string.char(table.unpack(c)))

	c7, c8 = atom:unpack(7, 8)
	assert(c7 == c[6])
	assert(c8 == nil)
	cm1, c0 = atom:unpack(-2, -1)
	assert(cm1 == c[1])
	assert(c0 == nil)
end

-- Chunk
print('[test] Chunk')
do
	local c = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}

	local function producer(forge)
		forge:frameTime(0)
		forge:chunk(c)

		forge:frameTime(0)
		forge:chunk(string.char(table.unpack(c)))

		forge:frameTime(0)
		forge:chunk(table.unpack(c))
	end

	local function consumer(seq, forge)
		consumer_chunk(seq, c, Atom.Chunk)

		forge:chunk(seq[1])
	end

	test(producer, consumer)
end

-- Raw
print('[test] Raw')
do
	local u = Map['http://open-music-kontrollers.ch/lv2/moony#raw']
	local c = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}

	local function producer(forge)
		forge:frameTime(0)
		forge:raw(u, c)

		forge:frameTime(0)
		forge:raw(u, string.char(table.unpack(c)))

		forge:frameTime(0)
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
		forge:frameTime(0)
		local tup = forge:tuple()
		assert(tup ~= forge)
		tup:int(1):float(2.0):long(3):double(4.0):pop()

		-- test autopop
		for tup in forge:frameTime(1):tuple():autopop() do
			tup:bool(true)
		end
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

		-- consume autopop
		atom = seq[2]
		assert(atom.type == Atom.Tuple)
		assert(#atom == 1)

		sub = atom[1]
		assert(sub.type == Atom.Bool)
		assert(sub.body == true)

		assert(atom[0] == nil)
		assert(atom[2] == nil)
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
		assert(forge:frameTime(0) == forge)
		local obj = forge:object(otype, id)
		assert(obj ~= forge)

		assert(obj:key(key1):int(12) == obj)

		obj:key(key2, ctx2):long(13)

		assert(obj:pop() == forge)

		-- test autopop
		for obj in forge:frameTime(0):object(otype, id):autopop() do
			obj:key(key1):int(14)
		end
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == Atom.Object)
		assert(atom.id == id)
		assert(atom.otype == otype)
		assert(atom.foo == nil)
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

		-- consume autopop
		atom = seq[2]
		assert(atom.type == Atom.Object)
		assert(atom.id == id)
		assert(atom.otype == otype)
		assert(#atom == 1)

		sub = atom[key1]
		assert(sub.type == Atom.Int)
		assert(sub.body == 14)
	end

	test(producer, consumer)
end

-- Atom
print('[test] Atom')
do
	local forge0 = nil

	local function producer(forge)
		forge0 = forge

		forge:frameTime(0)
		forge:int(12)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == Atom.Int)
		assert(atom.body == 12)

		forge0:frameTime(0):atom(atom)
	end

	test(producer, consumer)
end

-- OSC
print('[test] OSC')
do
	local function producer(forge)
		forge:frameTime(0)
		forge:message('/hello', 'sif', 'world', 12, 13.0)

		forge:frameTime(1)
		forge:message('/hallo', 'Shdt', Atom.Int, 12, 13.0, 1)

		forge:frameTime(2)
		forge:message('/yup', 'c', string.byte('a'))

		forge:frameTime(3)
		forge:message('/singletons', 'TFNI')

		forge:frameTime(4)
		forge:message('/chunky', 'mb', string.char(0x90, 0x20, 0x7f), string.char(0x01, 0x02, 0x03, 0x04))

		forge:frameTime(5)
		local bndl = forge:bundle(1)
		assert(bndl ~= forge)
		bndl:message('/one', 'i', 1)
		bndl:message('/two', 'i', 2)
		bndl:message('/three', 'i', 3)
		bndl:bundle(0.1):pop() -- nested
		assert(bndl:pop() == forge)

		forge:frameTime(6)
		forge:message('/color', 'r', 0xff00007f)

		-- create OSC message from scratch
		for obj in forge:frameTime(6):object(OSC.Message):autopop() do
			obj:key(OSC.messagePath):string('/custom')
			for tup in obj:key(OSC.messageArguments):tuple():autopop() do
				tup:raw(0) -- nil
				tup:impulse()
				tup:char(string.byte('c'))
				tup:rgba(0xbb0000ff)
				tup:timetag(0xffeeddccaa998877)
				tup:timetag(0.1)
			end
		end
	end

	local function consumer(seq)
		assert(#seq == 8)

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
		assert(args[1].body == Atom.Int)
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

		atom = seq[8]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 6)
		assert(args[1].type == 0)
		assert(args[2].type == Atom.Literal)
		assert(args[2].datatype == OSC.Impulse)
		assert(args[3].type == Atom.Literal)
		assert(args[3].datatype == OSC.Char)
		assert(args[3].body == 'c')
		assert(args[4].type == Atom.Literal)
		assert(args[4].datatype == OSC.RGBA)
		assert(args[4].body == 'bb0000ff')
		assert(args[5].type == Atom.Object)
		assert(args[5].otype == OSC.Timetag)
		assert(args[5][OSC.timetagIntegral].body == 0xffeeddcc)
		assert(args[5][OSC.timetagFraction].body == 0xaa998877)
		assert(args[6].type == Atom.Object)
		assert(args[6].otype == OSC.Timetag)
	end

	test(producer, consumer)
end

-- OSCResponder
print('[test] OSCResponder')
do
	local vals = {
		i = 13,
		f = 0.5,
		h = 14,
		d = 0.1,
		c = string.byte('c'),
		r = 0xff0000ff,
		b = string.char(0x1, 0x2, 0x3),
		m = string.char(MIDI.NoteOn, 0x20, 0x7f),
		S = Atom.Int,
		s = 'hello world',
		t = 0x0111111122222222,
		T = true,
		F = false,
		I = math.huge
	}
	local nvals = 15

	local function producer(forge)
		for k, v in pairs(vals) do
			forge:frameTime(0):message('/types', k, v)
		end
		forge:frameTime(0):message('/types', 'N')

		forge:frameTime(1):bundle(0):message('/pong', 's', 'world'):pop()

		forge:frameTime(2):message('/one/two/three', 'd', 12.3)
		forge:frameTime(2):message('/one/*/three', 'd', 12.3)
		forge:frameTime(2):message('/?ne/*/three', 'd', 12.3)
		forge:frameTime(2):message('/?ne/*/{three,four}', 'd', 12.3)
		forge:frameTime(2):message('/?ne/*/[tT]hree', 'd', 12.3)
		forge:frameTime(2):message('/?ne/*/[!T]hree', 'd', 12.3)
		forge:frameTime(2):message('/{one,eins}/*/{three,drei}', 'd', 12.3)

		forge:frameTime(3):message('/foo', 'd', 12.3)
		forge:frameTime(4):string('just fooling you')
	end

	local types_responded = 0
	local pong_responded = false
	local complex_responded = 0

	local osc_responder = OSCResponder({
		['/types'] = function(self, frames, forge, fmt, val)
			assert(frames == 0)
			if fmt == 'N' then
				assert(vals[fmt] == nil)
			else
				assert(vals[fmt] ~= nil)
			end
			assert(vals[fmt] == val)
			types_responded = types_responded + 1
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
	}, true)

	local function consumer(seq, forge)
		for frames, atom in seq:foreach() do
			local handled, matched = osc_responder(frames, forge, atom)

			if frames == 4 then
				assert(handled == false)
			else
				assert(handled == true)

				if frames == 3 then
					assert(matched == false)
				else
					assert(matched == true)
				end
			end
		end

		assert(types_responded == nvals)
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
	local time_responder = TimeResponder(time_cb, 1.0)
	local dummy = TimeResponder()
	assert(type(dummy) == 'userdata')

	local function producer(forge)
		local obj = forge:frameTime(0):object(Time.Position)
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
		forge:frameTime(1)
		time_responder:stash(forge)

		assert(time_responder.multiplier == 1.0)
		time_responder.multiplier = 2.0
		assert(time_responder.multiplier == 2.0)
		time_responder.multiplier = 1.0
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
		assert(time_responder[Param.sampleRate] == nil)
		assert(time_responder.foo == nil)
		assert(time_responder[true] == nil)

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
		forge:frameTime(0)
		local subseq = forge:sequence(Atom.beatTime)
		assert(subseq ~= forge)

		subseq:time(1.1):int(1)
		subseq:beatTime(2.2):int(1)

		assert(subseq:pop() == forge)

		-- test autopop
		for seq in forge:frameTime(1):sequence():autopop() do
			seq:time(10):bool(true)
		end
	end

	local function consumer(seq)
		assert(#seq == 2)
		local subseq1 = seq[1]
		assert(subseq1.type == Atom.Sequence)
		assert(subseq1.unit == Atom.beatTime)
		assert(subseq1.foo == nil)
		assert(#subseq1 == 2)

		for frames, atom in subseq1:foreach() do
			assert(not math.tointeger(frames)) -- beatTime is double
			assert(atom.type == Atom.Int)
			assert(atom.body == 1)
		end

		-- consume autopop
		local subseq2 = seq[2]
		assert(subseq2.type == Atom.Sequence)
		assert(subseq2.unit == 0)
		assert(#subseq2 == 1)

		for frames, atom in subseq2:foreach() do
			assert(frames == 10)
			assert(atom.type == Atom.Bool)
			assert(atom.body == true)
		end

		assert(subseq2[0] == nil)
		assert(subseq2[2] == nil)

		for time, atom in subseq1:foreach(subseq1) do
			assert(not math.tointeger(frames)) -- beatTime is double
			assert(atom.body == 1)
		end
	end

	test(producer, consumer)
end

-- Vector
print('[test] Vector')
do

	local function vectorize(forge, t, o)
		local vec = forge:vector(t)
		for i, v in ipairs(o) do
			vec:typed(t, v)
		end
		vec:pop()
	end

	local function producer(forge)
		forge:time(0)
		vectorize(forge, Atom.Int, {1, 2, 3, 4})
		forge:time(0):vector(Atom.Int, 1, 2, 3, 4)
		forge:time(0):vector(Atom.Int, {1, 2, 3, 4})

		forge:time(1)
		vectorize(forge, Atom.Long, {5, 6, 7, 8})
		forge:time(1):vector(Atom.Long, 5, 6, 7, 8)
		forge:time(1):vector(Atom.Long, {5, 6, 7, 8})

		forge:time(2)
		vectorize(forge, Atom.Bool, {true, false})
		forge:time(2):vector(Atom.Bool, true, false)
		forge:time(2):vector(Atom.Bool, {true, false})

		forge:time(3)
		vectorize(forge, Atom.Float, {1.0, 2.0})
		forge:time(3):vector(Atom.Float, 1.0, 2.0)
		forge:time(3):vector(Atom.Float, {1.0, 2.0})

		forge:time(4)
		vectorize(forge, Atom.Double, {3.3, 4.4})
		forge:time(4):vector(Atom.Double, 3.3, 4.4)
		forge:time(4):vector(Atom.Double, {3.3, 4.4})

		forge:time(5)
		vectorize(forge, Atom.URID, {Atom.Int, Atom.Long})
		forge:time(5):vector(Atom.URID, Atom.Int, Atom.Long)
		forge:time(5):vector(Atom.URID, {Atom.Int, Atom.Long})

		forge:time(6):vector(Atom.Float, { a = 1.0, b = 2.0 })
	end

	local function consumer(seq)
		assert(#seq == 19)

		for _, vec in ipairs{seq[1], seq[2], seq[3]} do
			assert(vec.type == Atom.Vector)
			assert(#vec == 4)
			assert(vec.childType == Atom.Int)
			assert(vec.childSize == 4)
			assert(vec.foo == nil)

			for i, atom in vec:foreach() do
				assert(atom.type == Atom.Int)
				assert(atom.body == i)
			end

			local c = vec.body
			for i, v in ipairs(c) do
				assert(i == v)
			end
		end

		for _, vec in ipairs{seq[4], seq[5], seq[6]} do
			assert(vec.type == Atom.Vector)
			assert(#vec == 4)
			assert(vec.childType == Atom.Long)
			assert(vec.childSize == 8)

			for i, atom in vec:foreach() do
				assert(atom.type == Atom.Long)
				assert(atom.body == i + 4)
			end

			local c = vec.body
			for i, v in ipairs(c) do
				assert(i + 4 == v)
			end
		end

		for _, vec in ipairs{seq[7], seq[8], seq[9]} do
			assert(vec.type == Atom.Vector)
			assert(#vec == 2)
			assert(vec.childType == Atom.Bool)
			assert(vec.childSize == 4)
			assert(vec[0] == nil)
			assert(vec[1].body == true)
			assert(vec[2].body == false)
			assert(vec[3] == nil)

			local a, b = vec:unpack(0, 0)
			assert(a.body == true)
			assert(b == nil)
			a, b = vec:unpack(3, 3)
			assert(a.body == false)
			assert(b == nil)

			local c = vec.body
			assert(c[1] == true)
			assert(c[2] == false)
		end

		for _, vec in ipairs{seq[10], seq[11], seq[12]} do
			assert(vec.type == Atom.Vector)
			assert(#vec == 2)
			assert(vec.childType == Atom.Float)
			assert(vec.childSize == 4)
			local a, b = vec:unpack()
			assert(a.body == 1.0)
			assert(b.body == 2.0)

			local c = vec.body
			for i, v in ipairs(c) do
				assert(i == v)
			end
		end

		for _, vec in ipairs{seq[13], seq[14], seq[15]} do
			assert(vec.type == Atom.Vector)
			assert(#vec == 2)
			assert(vec.childType == Atom.Double)
			assert(vec.childSize == 8)
			local a, b = vec:unpack()
			assert(a.body == 3.3)
			assert(b.body == 4.4)

			local c = vec.body
			assert(c[1] == 3.3)
			assert(c[2] == 4.4)
		end

		for _, vec in ipairs{seq[16], seq[17], seq[18]} do
			assert(vec.type == Atom.Vector)
			assert(#vec == 2)
			assert(vec.childType == Atom.URID)
			assert(vec.childSize == 4)
			local a, b = vec:unpack(1, 1)
			assert(a.body == Atom.Int)
			assert(b == nil)
			local a, b = vec:unpack(2, 2)
			assert(a.body == Atom.Long)
			assert(b == nil)

			local c = vec.body
			assert(c[1] == Atom.Int)
			assert(c[2] == Atom.Long)
		end

		for _, vec in ipairs{seq[19]} do
			assert(vec.type == Atom.Vector)
			assert(#vec == 2)
			assert(vec.childType == Atom.Float)
			assert(vec.childSize == 4)
			local a, b = vec:unpack(1, 1)
			assert( (a.body == 1.0) or (a.body == 2.0) )
			assert(b == nil)
			local a, b = vec:unpack(2, 2)
			assert( (a.body == 1.0) or (a.body == 2.0) )
			assert(b == nil)

			local c = vec.body
			assert( (c[1] == 1.0) or (c[1] == 2.0) )
			assert( (c[2] == 1.0) or (c[2] == 2.0) )
		end
	end

	test(producer, consumer)
end

-- Options
print('[test] Options')
do
	assert(Options[Param.sampleRate].body == 48000)
	assert(Options[Buf_Size.minBlockLength] == nil)
	assert(Options[Buf_Size.maxBlockLength] == nil)
	assert(Options(Buf_Size.sequenceSize) == nil)

	for k, v in pairs(Options) do
		assert(k == Param.sampleRate)
		assert(v.body == 48000)
	end
end

-- Patch
print('[test] Patch')
do
	local subject = Map['http://open-music-kontrollers.ch/lv2/moony#subject']
	local destination = Map['http://open-music-kontrollers.ch/lv2/moony#destination']
	local property = Map['http://open-music-kontrollers.ch/lv2/moony#property']
	local access = Patch.writable
	local rtid = Blank() & 0x7fffffff

	local function producer(forge)
		forge:time(0):get(property, subject, rtid)

		forge:time(1):get(property, nil, rtid)

		local set = forge:time(2):set(property, subject, rtid)
		set:string('hello world'):pop()

		local set = forge:time(3):set(property, nil, rtid)
		set:string('hello world'):pop()

		local patch = forge:time(4):patch(subject, rtid)
		patch:remove():key(access):urid(Patch.wildcard):pop()
		patch:add():key(access):urid(property):pop()
		patch:pop()

		patch = forge:time(5):patch(property, rtid)
		local remove = patch:remove()
		remove:key(RDFS.label):urid(Patch.wildcard)
		remove:key(RDFS.range):urid(Patch.wildcard)
		remove:key(LV2.minimum):urid(Patch.wildcard)
		remove:key(LV2.maximum):urid(Patch.wildcard)
		remove:pop()
		local add = patch:add()
		add:key(RDFS.label):string('A dummy label')
		add:key(RDFS.range):urid(Atom.Int)
		add:key(LV2.minimum):int(0)
		add:key(LV2.maximum):int(10)
		add:pop()
		patch:pop()

		local put = forge:time(6):put(subject, rtid)
		put:key(property):string('hello world')
		put:pop()

		forge:time(7):ack(subject, rtid)

		forge:time(8):error(subject, rtid)

		forge:time(9):delete(subject, rtid)

		forge:time(10):copy(subject, destination, rtid)

		forge:time(11):move(subject, destination, rtid)

		local insert = forge:time(12):insert(subject, rtid)
		insert:key(property):string('insertion')
		insert:pop()
	end

	local function consumer(seq)
		assert(#seq == 13)

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
		assert(patch[Patch.remove][LV2.minimum].body == Patch.wildcard)
		assert(patch[Patch.remove][LV2.maximum].body == Patch.wildcard)
		assert(patch[Patch.add][RDFS.label].body == 'A dummy label')
		assert(patch[Patch.add][RDFS.range].body == Atom.Int)
		assert(patch[Patch.add][LV2.minimum].body == 0)
		assert(patch[Patch.add][LV2.maximum].body == 10)

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

		local ack = seq[8]
		assert(ack.type == Atom.Object)
		assert(ack.otype == Patch.Ack)
		assert(#ack == 2)
		assert(ack[Patch.subject].body == subject)
		assert(ack[Patch.sequenceNumber].body == rtid)

		local err = seq[9]
		assert(err.type == Atom.Object)
		assert(err.otype == Patch.Error)
		assert(#err == 2)
		assert(err[Patch.subject].body == subject)
		assert(err[Patch.sequenceNumber].body == rtid)

		local err = seq[10]
		assert(err.type == Atom.Object)
		assert(err.otype == Patch.Delete)
		assert(#err == 2)
		assert(err[Patch.subject].body == subject)
		assert(err[Patch.sequenceNumber].body == rtid)

		local err = seq[11]
		assert(err.type == Atom.Object)
		assert(err.otype == Patch.Copy)
		assert(#err == 3)
		assert(err[Patch.subject].body == subject)
		assert(err[Patch.destination].body == destination)
		assert(err[Patch.sequenceNumber].body == rtid)

		local err = seq[12]
		assert(err.type == Atom.Object)
		assert(err.otype == Patch.Move)
		assert(#err == 3)
		assert(err[Patch.subject].body == subject)
		assert(err[Patch.destination].body == destination)
		assert(err[Patch.sequenceNumber].body == rtid)

		local put = seq[13]
		assert(put.type == Atom.Object)
		assert(put.otype == Patch.Insert)
		assert(#put == 3)
		assert(put[Patch.subject].body == subject)
		assert(put[Patch.sequenceNumber].body == rtid)
		local body = put[Patch.body]
		assert(body)
		assert(#body == 1)
		assert(body[property].body == 'insertion')
	end

	test(producer, consumer)
end

-- StateResponder
print('[test] StateResponder')
do
	prefix = 'http://open-music-kontrollers.ch/lv2/moony#test#'
	local urid = Mapper(prefix)
	local subj = Map('http://open-music-kontrollers.ch/lv2/moony#test')

	local flt_get_responded = 0
	local flt_set_responded = 0

	local state_int = Parameter{
		[RDFS.label] = 'Int',
		[RDFS.range] = Atom.Int,
		[Atom.childType] = Atom.Bool, -- only for unit testing
		[LV2.minimum] = 0,
		[LV2.maximum] = 10,
		[Units.unit] = Units.ms,
		[RDF.value] = 2
	}

	assert(state_int() == state_int[RDF.value])
	assert(state_int() == 2)
	assert(state_int(1) == 2)
	assert(state_int() == 1)

	local state_flt = Parameter{
		[RDFS.label] = 'Flt',
		[RDFS.range] = Atom.Float,
		[LV2.minimum] = -0.5,
		[LV2.maximum] = 10.0,
		[Units.symbol] = 'flt',
		_value = 1.0,
		[Patch.Get] = function(self)
			flt_get_responded = flt_get_responded + 1
			return self._value
		end,
		[Patch.Set] = function(self, value)
			self._value = value
			flt_set_responded = flt_set_responded + 1
		end
	}

	local state_bool = Parameter{
		[RDFS.label] = 'Bool',
		[RDFS.range] = Atom.Bool,
		[Atom.childType] = Atom.Bool, -- only for unit testing
		[Moony.color] = 0xff0000ff,
		[Moony.syntax] = Lua.lang,
		[LV2.scalePoint] = {},
		[RDF.value] = false,
	}

	local state_dummy = Parameter()
	assert(type(state_dummy) == 'table')

	local state = StateResponder({
		[Patch.writable] = {
			[urid.int] = state_int,
			[urid.flt] = state_flt
		},
		[Patch.readable] = {
			[urid.bool] = state_bool
		}
	})

	local function producer(forge)
		forge:frameTime(0):get(urid.int, subj, Blank())
		forge:frameTime(1):set(urid.int, subj, Blank()):typed(Atom.Int, 2):pop()

		forge:frameTime(2):get(urid.flt, subj, Blank())
		forge:frameTime(3):set(urid.flt, subj, Blank()):typed(Atom.Float, 2.0):pop()

		forge:frameTime(4):get(nil, nil, Blank())

		-- state:stash
		forge:frameTime(5)
		state:stash(forge)

		-- state:sync
		state:sync(6, forge)

		--FIXME state:register()
	end

	local function consumer(seq, forge)
		assert(#seq == 7)

		-- test up to sync
		for frames, atom in seq:foreach() do
			if frames < 6 then
				local handled = state(frames, forge, atom)
				assert(handled == (frames ~= 5))
			end
		end

		assert(state_int() == state_int[RDF.value])
		assert(state_int() == 2)
		assert(state_flt() == state_flt[RDF.value])
		assert(state_flt() == 2.0)
		assert(flt_get_responded == 6)
		assert(flt_set_responded == 1)

		assert(#seq == 7)

		local atom = seq[4]
		assert(atom.type == Atom.Object)
		assert(atom.otype == Patch.Set)
		assert(#atom == 4)
		assert(atom[Patch.subject].body == subj)
		assert(atom[Patch.property].body == urid.flt)
		assert(atom[Patch.value].body == 2.0)
		assert(atom[Patch.sequenceNumber].body ~= 0)

		local atom = seq[5]
		assert(atom.type == Atom.Object)
		assert(atom.otype == Patch.Get)
		assert(atom[Patch.sequenceNumber].body ~= 0)

		--test state:stash
		local atom = seq[6]
		assert(atom.type == Atom.Object)
		assert(#atom == 2)
		assert(atom[urid.int].type == Atom.Int)
		assert(atom[urid.int].body == 1)
		assert(atom[urid.flt].type == Atom.Float)
		assert(atom[urid.flt].body == 1.0)

		-- test state:apply
		state:apply(atom)
		assert(state_int() == 1)
		assert(state_flt() == 1.0)

		assert(state:apply(Stash():int(1):read()) == false)

		-- test state:sync
		atom = seq[7]
		assert(atom.type == Atom.Object)
		assert(#atom == 3)
		assert(atom[Patch.subject].type == Atom.URID)
		assert(atom[Patch.sequenceNumber].type == Atom.Int)
		assert(atom[Patch.sequenceNumber].body == 0)
		local body = atom[Patch.body]
		assert(body.type == Atom.Object)
		assert(#body == 3)
		assert(body[urid.int].type == Atom.Int)
		assert(body[urid.int].body == 1)
		assert(body[urid.flt].type == Atom.Float)
		assert(body[urid.flt].body == 1.0)
		assert(body[urid.bool].type == Atom.Bool)
		assert(body[urid.bool].body == false)

		-- test from sync
		for frames, atom in seq:foreach() do
			if frames == 6 then
				local handled = state(frames, forge, atom)
				assert(handled)
			end
		end

		assert(state_int() == state_int[RDF.value])
		assert(state_int() == 1)
		assert(state_flt() == state_flt[RDF.value])
		assert(state_flt() == 1.0)
		assert(flt_get_responded == 10)
		assert(flt_set_responded == 3)
	end

	test(producer, consumer)
end

-- Note
print('[test] Note')
do
	assert(Note[0] == 'C-1')
	assert(Note[1] == 'C#-1')
	assert(Note(12) == 'C+0')
	assert(Note(13) == 'C#+0')

	assert(Note['C-1'] == 0)
	assert(Note['C#-1'] == 1)
	assert(Note['C+0'] == 12)
	assert(Note('C#+0') == 13)

	assert(Note['C+4'] == 60)
	assert(Note[60] == 'C+4')

	assert(Note[-1] == nil)
	assert(Note(0x80) == nil)
	assert(Note.foo == nil)
	assert(Note.bar == nil)

	local keys = {'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'}

	local function itr()
		return coroutine.wrap(function()
			for octave = -1, 8 do
				for i, key in ipairs(keys) do
					coroutine.yield( (octave+1) * #keys + i - 1, string.format('%s%+i', key, octave) )
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

	assert(#references[1] == 0) -- latoms in :foreach are recycled, and reset
	assert(references[1].type == 0)

	assert(#references[2] == 0)
	assert(references[2].type == 0)
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

	-- test superfluous :read()
	assert(stash:read() == stash:read())

	stash:write()
	stash:read()
	assert(#stash == 0)
	assert(stash.type == 0)

	-- test superfluous :write()
	assert(stash:write() == stash:write())
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
print('[test] Blank')
do
	local ids = {}
	for i=1, 1000 do
		local id = Blank()
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

	local base = 60.0
	local noct = 32.0
	local fref = 400.0
	for note = 0.0, 127.0, 0.1 do
		local cps = midi2cps(note, base, noct, fref)
		assert(note - cps2midi(cps, base, noct, fref) < 1e-4)
	end
	assert(60.0 == cps2midi(400.0, base, noct, fref))
	assert(400.0 == midi2cps(60.0, base, noct, fref))
end

-- Mapper
print('[test] Mapper')
do
	local prefix = 'http://open-music-kontrollers.ch/lv2/synthpod#'
	local mapper = Mapper(prefix)
	assert(Map[prefix .. 'addModule'] == mapper.addModule)
	assert(Map[prefix .. 'addModule'] == mapper.addModule)
	assert(Map[prefix .. 'connectPort'] == mapper.connectPort)
	assert(Map[prefix .. 'connectPort'] == mapper.connectPort)
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

print('[test] aes128')
do
	local key1 = '2b7e151628aed2a6abf7158809cf4f3c'
	local key2 = string.char(
		0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c)

  local inp = 'function run(n, control, notify) end'

	local sec1, len1 = aes128.encode(inp, key1)
	local sec2, len2 = aes128.encode(inp, key2)

	assert(sec1 == sec2)
	assert(#sec1 == 48)
	assert(#sec2 == 48)
	assert(len1 == len2)
	assert(len1 == #inp)

	local out1 = aes128.decode(sec1, key1, len1)
	local out2 = aes128.decode(sec2, key2, len2)

	assert(out1 == inp)
	assert(out2 == inp)

	assert(#out1 == len1)
	assert(#out2 == len2)
end

print('[test] base64')
do
	local R = random.new(12345678)
  local inp = ''
	for i = 1, 64 do
		inp = inp .. string.char(R(255))
	end

	local enc = base64.encode(inp)
	local out = base64.decode(enc)

	assert(#inp == #out)
	assert(inp == out)
end

print('[test] ascii85')
do
	local R = random.new(12345678)
  local inp = ''
	for i = 1, 64 do
		inp = inp .. string.char(R(255))
	end

	local enc = ascii85.encode(inp)
	local out = ascii85.decode(enc)

	assert(#inp == #out)
	assert(inp == out)
end

-- Canvas
print('[test] Canvas')
do
	local subj = Map['http://open-music-kontrollers.ch/lv2/moony#subject']
	local seqn = 13

	local n = {0.125, 0.25, 0.375, 0.5, 0.625, 0.75}

	local function producer(forge)
		for ctx in forge:time(0):tuple():autopop() do
			ctx:beginPath()
			ctx:closePath()
			ctx:arc(table.unpack(n, 1, 5))
			ctx:curveTo(table.unpack(n, 1, 6))
			ctx:lineTo(table.unpack(n, 1, 2))
			ctx:moveTo(table.unpack(n, 1, 2))
			ctx:rectangle(table.unpack(n, 1, 4))
			ctx:style(0x12345678)
			ctx:lineWidth(table.unpack(n, 1, 1))
			ctx:lineDash(table.unpack(n, 1, 2))
			ctx:lineCap(Canvas.lineCapRound)
			ctx:lineJoin(Canvas.lineJoinRound)
			ctx:miterLimit(table.unpack(n, 1, 1))
			ctx:stroke()
			ctx:fill()
			ctx:clip()
			ctx:save()
			ctx:restore()
			ctx:translate(table.unpack(n, 1, 2))
			ctx:scale(table.unpack(n, 1, 2))
			ctx:rotate(table.unpack(n, 1, 1))
			ctx:reset()
			ctx:fontSize(table.unpack(n, 1, 1))
			ctx:fillText('hello')
			ctx:polyLine(table.unpack(n, 1, 4))
			ctx:transform(table.unpack(n, 1, 6))
		end
	end

	local function check_canvas_object(obj, otype)
		assert(obj.type == Atom.Object)
		assert(obj.otype == otype)
	end

	local function check_canvas_vector(body, num)
		assert(body.type == Atom.Vector)
		assert(#body == num)

		for i = 1, #body do
			assert(body[i].body == n[i])
		end
	end

	local function consumer(seq)
		assert(#seq == 1)

		local graph = seq[1]
		assert(graph)
		assert(graph.type == Atom.Tuple)
		assert(#graph == 26)

		local beginPath = graph[1]
		check_canvas_object(beginPath, Canvas.BeginPath)

		local closePath = graph[2]
		check_canvas_object(closePath, Canvas.ClosePath)

		local itm, body

		itm = graph[3]
		check_canvas_object(itm, Canvas.Arc)
		body = itm[Canvas.body]
		check_canvas_vector(body, 5)

		itm = graph[4]
		check_canvas_object(itm, Canvas.CurveTo)
		body = itm[Canvas.body]
		check_canvas_vector(body, 6)

		itm = graph[5]
		check_canvas_object(itm, Canvas.LineTo)
		body = itm[Canvas.body]
		check_canvas_vector(body, 2)

		itm = graph[6]
		check_canvas_object(itm, Canvas.MoveTo)
		body = itm[Canvas.body]
		check_canvas_vector(body, 2)

		itm = graph[7]
		check_canvas_object(itm, Canvas.Rectangle)
		body = itm[Canvas.body]
		check_canvas_vector(body, 4)

		itm = graph[8]
		check_canvas_object(itm, Canvas.Style)
		body = itm[Canvas.body]
		assert(body.type == Atom.Long)
		assert(body.body == 0x12345678)

		itm = graph[9]
		check_canvas_object(itm, Canvas.LineWidth)
		body = itm[Canvas.body]
		assert(body.type == Atom.Float)
		assert(body.body == n[1])

		itm = graph[10]
		check_canvas_object(itm, Canvas.LineDash)
		body = itm[Canvas.body]
		check_canvas_vector(body, 2)

		itm = graph[11]
		check_canvas_object(itm, Canvas.LineCap)
		body = itm[Canvas.body]
		assert(body.type == Atom.URID)
		assert(body.body == Canvas.lineCapRound)

		itm = graph[12]
		check_canvas_object(itm, Canvas.LineJoin)
		body = itm[Canvas.body]
		assert(body.type == Atom.URID)
		assert(body.body == Canvas.lineJoinRound)

		itm = graph[13]
		check_canvas_object(itm, Canvas.MiterLimit)
		body = itm[Canvas.body]
		assert(body.type == Atom.Float)
		assert(body.body == n[1])

		itm = graph[14]
		check_canvas_object(itm, Canvas.Stroke)
		assert(#itm == 0)

		itm = graph[15]
		check_canvas_object(itm, Canvas.Fill)
		assert(#itm == 0)

		itm = graph[16]
		check_canvas_object(itm, Canvas.Clip)
		assert(#itm == 0)

		itm = graph[17]
		check_canvas_object(itm, Canvas.Save)
		assert(#itm == 0)

		itm = graph[18]
		check_canvas_object(itm, Canvas.Restore)
		assert(#itm == 0)

		itm = graph[19]
		check_canvas_object(itm, Canvas.Translate)
		body = itm[Canvas.body]
		check_canvas_vector(body, 2)

		itm = graph[20]
		check_canvas_object(itm, Canvas.Scale)
		body = itm[Canvas.body]
		check_canvas_vector(body, 2)

		itm = graph[21]
		check_canvas_object(itm, Canvas.Rotate)
		body = itm[Canvas.body]
		assert(body.type == Atom.Float)
		assert(body.body == n[1])

		itm = graph[22]
		check_canvas_object(itm, Canvas.Reset)
		assert(#itm == 0)

		itm = graph[23]
		check_canvas_object(itm, Canvas.FontSize)
		body = itm[Canvas.body]
		assert(body.type == Atom.Float)
		assert(body.body == n[1])

		itm = graph[24]
		check_canvas_object(itm, Canvas.FillText)
		body = itm[Canvas.body]
		assert(body.type == Atom.String)
		assert(body.body == 'hello')

		itm = graph[25]
		check_canvas_object(itm, Canvas.PolyLine)
		body = itm[Canvas.body]
		check_canvas_vector(body, 4)

		itm = graph[26]
		check_canvas_object(itm, Canvas.Transform)
		body = itm[Canvas.body]
		check_canvas_vector(body, 6)
	end

	test(producer, consumer)
end

-- disabled routines
print('[test] Disabled routines')
do
	assert(os == nil)
	assert(bit32 == nil)

	assert(math.random == nil)
	assert(math.randomseed == nil)
end
