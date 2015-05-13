--[[===========================================================================
	Moony LV2: Lua API v.0.1
===============================================================================
	
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

--[[---------------------------------------------------------------------------
	General usage
-------------------------------------------------------------------------------

	Plugin scripts need to define a 'run' function which gets called at each
	plugin update period. The input and output arguments of the 'run' function
	may differ from plugin to plugin, depending on number and types of input
	and output ports.

	If a plugin has input event ports, sequence objects (Lua userdata) will be
	given to the 'run' function whose events can be iterated over.

	If a plugin has output event ports, forge objects (Lua userdata) will be
	given to the 'run' function which can be filled with arbitrary atom event
	structures.

	If a plugin has input control ports, those values will be given to the 'run'
	function as native Lua numbers.

	If a plugin has output control ports, the 'run' function is expected to 
	return native Lua numbers, missing return values will be set to 0.0. 
--]]

-- code example for plugin 'C4+A1xC4+A1' in a clone configuration
function run(seq, forge, a, b, c, d)
	for frames, atom in seq:foreach() do
		forge:frame_time(frames)
		forge:atom(atom)
	end

	return a, b, c, d
end

--[[---------------------------------------------------------------------------
	Logging	
-----------------------------------------------------------------------------]]

-- Moony overwrites Lua's 'print' function to use the LV2 log extension.
print('hello', 'world', 2015, true)

--[[---------------------------------------------------------------------------
	URI/URID (un)mapping
-----------------------------------------------------------------------------]]

-- Tables
urid = map['http://foo.org#bar'] -- map URI to URID
uri = unmap[urid] -- unmap URID to URI

-- Constants
urid = map.Bool -- URID of LV2 Bool
urid = map.Chunk -- URID of LV2 Chunk
urid = map.Double -- URID of LV2 Double
urid = map.Float -- URID of LV2 Float
urid = map.Int -- URID of LV2 Integer (32bit integer)
urid = map.Long -- URID of LV2 Long (64bit integer)
urid = map.Literal -- URID of LV2 Literal
urid = map.Object -- URID of LV2 Object
urid = map.Path -- URID of LV2 Path
urid = map.Property -- URID of LV2 Property
urid = map.Sequence -- URID of LV2 Sequence
urid = map.String -- URID of LV2 String
urid = map.Tuple -- URID of LV2 Tuple
urid = map.URI -- URID of LV2 URI
urid = map.URID -- URID of LV2 URID
urid = map.Vector -- URID of LV2 Vector
urid = map.Midi -- URID of LV2 MIDI

--[[---------------------------------------------------------------------------
	Atom Sequence
-----------------------------------------------------------------------------]]

-- length operator
n = #seq -- number of events in sequence

-- indexing by string key
t = seq.type -- type of atom, e.g. map.Sequence

-- indexing by number key
atom = seq[1] -- get first event atom
atom = seq[#seq] -- get last event atom

-- iteration
for frames, atom in seq:foreach() do -- iterate over all events in sequence
end

--[[---------------------------------------------------------------------------
	Atom Forge
-----------------------------------------------------------------------------]]

-- functions
forge:frame_time(frames) -- start a new event at frames (Lua integer)
forge:beat_time(beats) -- start a new event at beats (Lua number)

forge:atom(atom) -- push a whole atom (atom, object, tuple, vector, ...)
forge:int(1) -- push a Lua integer as 32bit integer
forge:long(2) -- push a Lua integer as 64bit integer
forge:float(0.1) -- push a Lua number as floating point
forge:double(0.4) -- push a Lua number as double-precision floating point
forge:bool(true) -- push Lua boolean

forge:urid(URID) -- push a Lua integer as URID
forge:string('hello') -- push a Lua string
forge:literal('world', datatype, lang) -- push a Lua string, integer, integer
forge:uri('http://foo.org#bar') -- push a Lua string as URI
forge:path('/tmp/test.lua') -- push a Lua string as path

forge:midi({0x90, 0x4a, 0x7f}) -- push a Lua table as Midi message
tup = forge:tuple() -- start a new tuple (returns a frame for forge:pop)
obj = forge:object(id, otype) -- start a new object (returns a frame for forge:pop)
forge:key(key) -- push a new object property with key (Lua integer)
forge:property(context, key) -- push a new object property with context, key 9Lua integer)
forge:sequence(seq) -- append all events in seq to forge

forge:pop(frame) -- finalize tuple or object frame

--[[---------------------------------------------------------------------------
	Atom Object
-----------------------------------------------------------------------------]]

local foo = {
	bar = map['http://foo.com#bar']
}
local bar = {
	foo = map['http://bar.com#foo']
}

-- length operator
n = #obj -- number of properties in object 

-- indexing by string key
t = obj.type -- type of atom, e.g. map.Object
t = obj.id -- object id, e.g. foo.bar
t = obj.otype -- object type , e.g. bar.foo

-- indexing by property key (URID as Lua integer)
atom = obj[foo.bar] -- get property value for key foo.bar

-- iteration
for key, context, atom in obj:foreach() do -- iterate over all elements in object
end

--[[---------------------------------------------------------------------------
	Atom Tuple
-----------------------------------------------------------------------------]]

-- length operator
n = #tup -- number of elements in tuple 

-- indexing by string key
t = tup.type -- type of atom, e.g. map.Tuple

-- indexing by number key
atom = tup[1] -- get first atom element
atom = tup[#tup] -- get last atom element

-- iteration
for i, atom in tup:foreach() do -- iterate over all elements in tuple
end

-- unpacking
atom1, atom2, atom3 = tup:unpack() -- unpack all elements in tuple

--[[---------------------------------------------------------------------------
	Atom Vector
-----------------------------------------------------------------------------]]

-- length operator
n = #vec -- number of elements in vector

-- indexing by string key
t = vec.type -- type of atom, e.g. map.Vector
t = vec.child_type -- type of child atom, e.g. map.Float
t = vec.child_size -- byte size of child atom, e.g. 4

-- indexing by number key
atom = vec[1] -- get first atom element
atom = vec[#vec] -- get last atom element

-- iteration
for i, atom in vec:foreach() do -- iterate over all elements in vector
end

-- unpacking
atom1, atom2, atom3 = vec:unpack() -- unpack all elements in vector

--[[---------------------------------------------------------------------------
	First-class Atom
-----------------------------------------------------------------------------]]

-- length operator
n = #atom -- byte size of atom

-- indexing by string key
t = atom.type -- type of atom, e.g. map.Int, map.Double, map.String
v = atom.value -- native Lua value for the corresponding atom

-- map.Bool
bool = atom.value -- Lua boolean, e.g. true or false

-- map.Int, map.Long
int = atom.value -- Lua integer

-- map.Float, map.Double
float = atom.value -- Lua number

-- map.String
str = atom.value -- Lua string

-- map.Literal
literal = atom.value -- Lua string
datatype = atom.datatype -- URID as Lua integer
lang = atom.lang -- URID as Lua integer

-- map.URID
urid = atom.value -- Lua integer

-- map.URI
uri = atom.value -- Lua string

-- map.Path
path = atom.value -- Lua string

-- map.Midi
midi = atom.value -- Lua table with single raw Midi bytes
m = #midi -- number of Midi bytes
status = m[1] -- Midi status byte
