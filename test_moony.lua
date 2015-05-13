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

-- map/unmap
print('[test] map/unmap')
do
	local uri = 'http://test.org#foo'

	assert(map[uri] == map[uri])
	assert(map(uri) == map(uri))

	local urid = map[uri]
	assert(unmap[urid] == unmap[urid])
	assert(unmap(urid) == unmap(urid))
end

-- Int
print('[test] Int')
do
	local function producer(forge)
		forge:frame_time(0)
		forge:int(0x7fffffff)
		
		forge:frame_time(0)
		forge:int(0xffffffff)
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == map.Int)
		assert(#atom == 4)
		assert(type(atom.value) == 'number')
		assert(atom.value == 0x7fffffff)
		
		local atom = seq[2]
		assert(atom.type == map.Int)
		assert(#atom == 4)
		assert(type(atom.value) == 'number')
		assert(atom.value ~= 0xffffffff)
	end

	test(producer, consumer)
end

-- Long
print('[test] Long')
do
	local function producer(forge)
		forge:frame_time(0)
		forge:long(0x100000000)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == map.Long)
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
		assert(atom.type == map.Float)
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
		forge:frame_time(0)
		forge:double(0.12)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == map.Double)
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
		forge:frame_time(0)
		forge:bool(true)

		forge:frame_time(0)
		forge:bool(false)
	end

	local function consumer(seq)
		assert(#seq == 2)
		local atom = seq[1]
		assert(atom.type == map.Bool)
		assert(#atom == 4)
		assert(type(atom.value) == 'boolean')
		assert(atom.value == true)
		
		local atom = seq[2]
		assert(atom.type == map.Bool)
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
		assert(atom.type == map.String)
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
	local datatype = map['http://test.org#datatype']
	local lang = map['http://test.org#lang']

	local function producer(forge)
		forge:frame_time(0)
		forge:literal(str, datatype, lang)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == map.Literal)
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
		assert(atom.type == map.URI)
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
		assert(atom.type == map.Path)
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
	local urid = map[uri]

	local function producer(forge)
		forge:frame_time(0)
		forge:urid(urid)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == map.URID)
		assert(#atom == 4)
		assert(type(atom.value) == 'number')
		assert(atom.value == urid)
		assert(unmap[atom.value] == uri)
	end

	test(producer, consumer)
end

-- Midi
print('[test] Midi')
do
	local m = {0x90, 0x2a, 0x7f}

	local function producer(forge)
		forge:frame_time(0)
		forge:midi(m)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == map.Midi)
		assert(type(atom.value) == 'table')
		assert(#atom == #m)
		assert(atom.value[0] == nil)
		assert(atom.value[1] == 0x90)
		assert(atom.value[2] == 0x2a)
		assert(atom.value[3] == 0x7f)
		assert(atom.value[4] == nil)
	end

	test(producer, consumer)
end

-- Tuple
print('[test] Tuple')
do
	local function producer(forge)
		forge:frame_time(0)
		local tup = forge:tuple()
		forge:int(1)
		forge:float(2.0)
		forge:long(3)
		forge:double(4.0)
		forge:pop(tup)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == map.Tuple)
		assert(#atom == 4)

		local sub = atom[1]
		assert(sub.type == map.Int)
		assert(sub.value == 1)
		
		sub = atom[2]
		assert(sub.type == map.Float)
		assert(sub.value == 2.0)
		
		sub = atom[3]
		assert(sub.type == map.Long)
		assert(sub.value == 3)
		
		sub = atom[4]
		assert(sub.type == map.Double)
		assert(sub.value == 4.0)

		assert(atom[0] == nil)
		assert(atom[5] == nil)

		for i, sub in atom:foreach() do
			assert(atom[i].value == sub.value)
		end
	end

	test(producer, consumer)
end

-- Object
print('[test] Object')
do
	local id = 0
	local otype = map['http://test.org#type']
	local key1 = map['http://test.org#key1']
	local context2 = map['http://test.org#context2']
	local key2 = map['http://test.org#key2']

	local function producer(forge)
		forge:frame_time(0)
		local obj = forge:object(id, otype)

		forge:key(key1)
		forge:int(12)
		
		forge:property(key2, context2)
		forge:long(13)

		forge:pop(obj)
	end

	local function consumer(seq)
		assert(#seq == 1)
		local atom = seq[1]
		assert(atom.type == map.Object)
		assert(atom.id == 0)
		assert(atom.otype == otype)
		assert(#atom == 2)

		local sub = atom[key1]
		assert(sub.type == map.Int)
		
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
