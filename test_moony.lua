--[[
	Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
	
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
		assert(type(atom.value) == 'number')
		assert(atom.value == 0x7fffffff)
		
		local atom = seq[2]
		assert(atom.type == Atom.Int)
		assert(#atom == 4)
		assert(type(atom.value) == 'number')
		assert(atom.value ~= 0xffffffff)
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
		assert(type(atom.value) == 'number')
		assert(atom.value == 0x100000000)
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
		assert(type(atom.value) == 'number')
		assert(atom.value == 2.0)
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
		assert(type(atom.value) == 'number')
		assert(atom.value == 0.12)
	end

	test(producer, consumer)
end

-- Bool
print('[test] Bool')
do
	local function producer(forge)
		forge:frame_time(0):bool(true)
		forge:frame_time(0):bool(false)
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == Atom.Bool)
		assert(#atom == 4)
		assert(type(atom.value) == 'boolean')
		assert(atom.value == true)
		
		local atom = seq[2]
		assert(atom.type == Atom.Bool)
		assert(#atom == 4)
		assert(type(atom.value) == 'boolean')
		assert(type(atom.value) == 'boolean')
		assert(atom.value == false)
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
		assert(type(atom.value) == 'string')
		assert(atom.value == str)
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
		assert(type(atom.value) == 'string')
		assert(#atom == string.len(str) + 1)
		assert(atom.value == str)
		assert(atom.datatype == datatype)
		assert(atom.lang == lang)
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
		assert(type(atom.value) == 'string')
		assert(atom.value == uri)
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
		assert(type(atom.value) == 'string')
		assert(atom.value == path)
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
		assert(type(atom.value) == 'number')
		assert(atom.value == urid)
		assert(Unmap[atom.value] == uri)
	end

	test(producer, consumer)
end

-- Midi
print('[test] Midi')
do
	local m = {0x90, 0x2a, 0x7f}

	local function producer(forge)
		forge:frame_time(0):midi(m)

		forge:frame_time(0)
		forge:midi(table.unpack(m))
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == MIDI.Event)
		assert(type(atom.value) == 'table')
		assert(#atom == #m)
		assert(#atom.value == #m)
		assert(atom.value[0] == nil)
		assert(atom.value[1] == 0x90)
		assert(atom.value[2] == 0x2a)
		assert(atom.value[3] == 0x7f)
		assert(atom.value[4] == nil)

		atom = seq[2]
		assert(atom.type == MIDI.Event)
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
	end

	test(producer, consumer)
end

-- Chunk
print('[test] Chunk')
do
	local c = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}

	local function producer(forge)
		forge:frame_time(0)
		forge:chunk(c)

		forge:frame_time(0)
		forge:chunk(table.unpack(c))
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == Atom.Chunk)
		assert(type(atom.value) == 'table')
		assert(#atom == #c)
		assert(#atom.value == #c)
		assert(atom.value[0] == nil)
		assert(atom.value[1] == 0x01)
		assert(atom.value[2] == 0x02)
		assert(atom.value[3] == 0x03)
		assert(atom.value[4] == 0x04)
		assert(atom.value[5] == 0x05)
		assert(atom.value[6] == 0x06)
		assert(atom.value[7] == nil)

		atom = seq[2]
		assert(atom.type == Atom.Chunk)
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
		assert(sub.value == 1)
		
		sub = atom[2]
		assert(sub.type == Atom.Float)
		assert(sub.value == 2.0)
		
		sub = atom[3]
		assert(sub.type == Atom.Long)
		assert(sub.value == 3)
		
		sub = atom[4]
		assert(sub.type == Atom.Double)
		assert(sub.value == 4.0)

		assert(atom[0] == nil)
		assert(atom[5] == nil)

		for i, sub in atom:foreach() do
			assert(atom[i].value == sub.value)
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
	local context2 = Map['http://test.org#context2']
	local key2 = Map['http://test.org#key2']

	local function producer(forge)
		assert(forge:frame_time(0) == forge)
		local obj = forge:object(id, otype)
		assert(obj ~= forge)

		assert(obj:key(key1):int(12) == obj)
		
		obj:property(key2, context2):long(13)

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
		assert(sub.value == 13)

		for key, context, sub in atom:foreach() do
			assert(atom[key].value == sub.value)

			if(key == key2) then
				assert(context == context2)
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
		assert(atom.value == 12)

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
		forge:message('/chunky', 'mb', {0x90, 0x20, 0x7f}, {0x01, 0x02, 0x03, 0x04})
		
		forge:frame_time(5)
		local bndl = forge:bundle(1)
		assert(bndl ~= forge)
		bndl:message('/one', 'i', 1)
		bndl:message('/two', 'i', 2)
		bndl:message('/three', 'i', 3)
		bndl:bundle(1):pop() -- nested
		assert(bndl:pop() == nil)
	end

	local function consumer(seq)
		assert(#seq == 6)

		local atom = seq[1]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		local path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.value == '/hello')
		local fmt = atom[OSC.messageFormat]
		assert(fmt.type == Atom.String)
		assert(fmt.value == 'sif')
		local args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 3)
		assert(args[0] == nil)
		assert(args[1].type == Atom.String)
		assert(args[1].value == 'world')
		assert(args[2].type == Atom.Int)
		assert(args[2].value == 12)
		assert(args[3].type == Atom.Float)
		assert(args[3].value == 13.0)
		assert(args[4] == nil)
		
		atom = seq[2]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.value == '/hallo')
		fmt = atom[OSC.messageFormat]
		assert(fmt.type == Atom.String)
		assert(fmt.value == 'Shdt')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 4)
		assert(args[0] == nil)
		assert(args[1].type == Atom.String)
		assert(args[1].value == 'velo')
		assert(args[2].type == Atom.Long)
		assert(args[2].value == 12)
		assert(args[3].type == Atom.Double)
		assert(args[3].value == 13.0)
		assert(args[4].type == Atom.Long)
		assert(args[4].value == 1)
		assert(args[5] == nil)
		
		atom = seq[3]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.value == '/yup')
		fmt = atom[OSC.messageFormat]
		assert(fmt.type == Atom.String)
		assert(fmt.value == 'c')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 1)
		assert(args[0] == nil)
		assert(args[1].type == Atom.Int)
		assert(string.char(args[1].value) == 'a')
		assert(args[2] == nil)
		
		atom = seq[4]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.value == '/singletons')
		fmt = atom[OSC.messageFormat]
		assert(fmt.type == Atom.String)
		assert(fmt.value == 'TFNI')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 0)
		assert(args[0] == nil)
		assert(args[1] == nil)
		assert(args[2] == nil)
		assert(args[3] == nil)
		assert(args[4] == nil)
		assert(args[5] == nil)
		
		atom = seq[5]
		assert(atom.type == Atom.Object)
		assert(atom.otype == OSC.Message)
		path = atom[OSC.messagePath]
		assert(path.type == Atom.String)
		assert(path.value == '/chunky')
		fmt = atom[OSC.messageFormat]
		assert(fmt.type == Atom.String)
		assert(fmt.value == 'mb')
		args = atom[OSC.messageArguments]
		assert(args.type == Atom.Tuple)
		assert(#args == 2)
		assert(args[0] == nil)
		assert(args[1].type == MIDI.Event)
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
		local timestamp = atom[OSC.bundleTimestamp]
		assert(timestamp.type == Atom.Long)
		assert(timestamp.value == 1)
		local itms = atom[OSC.bundleItems]
		assert(itms.type == Atom.Tuple)
		assert(#itms == 4)
		assert(itms[1].otype == OSC.Message)
		assert(itms[2].otype == OSC.Message)
		assert(itms[3].otype == OSC.Message)
		assert(itms[4].otype == OSC.Bundle)
		assert(#itms[4][OSC.bundleItems] == 0)
	end

	test(producer, consumer)
end

-- Sequence
print('[test] Sequence')
do

	local function producer(forge)
		forge:frame_time(0)
		local subseq = forge:sequence()
		assert(subseq ~= forge)

		subseq:frame_time(1):int(1)
		subseq:frame_time(2):int(2)

		assert(subseq:pop() == nil)

		forge:frame_time(1):sequence():pop()
	end

	local function consumer(seq)
		assert(#seq == 2)
		local subseq = seq[1]
		assert(subseq.type == Atom.Sequence)
		assert(#subseq == 2)

		for frames, atom in subseq:foreach() do
			assert(atom.type == Atom.Int)
			assert(atom.value == frames)
		end

		subseq = seq[2]
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
		forge:frame_time(0)
		forge:vector(Atom.Int, {1, 2, 3, 4})

		forge:frame_time(1)
		forge:vector(Atom.Long, 5, 6, 7, 8)
		
		forge:frame_time(2)
		forge:vector(Atom.Bool, {true, false})
		
		forge:frame_time(3)
		forge:vector(Atom.Float, 1.0, 2.0)
		
		forge:frame_time(4)
		forge:vector(Atom.Double, {3.3, 4.4})
		
		forge:frame_time(5)
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
			assert(atom.value == i)
		end
		
		vec = seq[2]
		assert(vec.type == Atom.Vector)
		assert(#vec == 4)
		assert(vec.child_type == Atom.Long)
		assert(vec.child_size == 8)

		for i, atom in vec:foreach() do
			assert(atom.type == Atom.Long)
			assert(atom.value == i + 4)
		end

		vec = seq[3]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.Bool)
		assert(vec.child_size == 4)
		assert(vec[0] == nil)
		assert(vec[1].value == true)
		assert(vec[2].value == false)
		assert(vec[3] == nil)

		vec = seq[4]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.Float)
		assert(vec.child_size == 4)
		local a, b = vec:unpack()
		assert(a.value == 1.0)
		assert(b.value == 2.0)
		
		vec = seq[5]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.Double)
		assert(vec.child_size == 8)
		local a, b = vec:unpack()
		assert(a.value == 3.3)
		assert(b.value == 4.4)
		
		vec = seq[6]
		assert(vec.type == Atom.Vector)
		assert(#vec == 2)
		assert(vec.child_type == Atom.URID)
		assert(vec.child_size == 4)
		local a, b = vec:unpack(1, 1)
		assert(a.value == Atom.Int)
		assert(b == nil)
		local a, b = vec:unpack(2, 2)
		assert(a.value == Atom.Long)
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
