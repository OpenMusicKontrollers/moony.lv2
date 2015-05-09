--[[
	Lua LV2 API v.0.1
	=============================================================================
	
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

--[[
	General usage

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

--[[
	LV2 module
--]]

-- Functions
urid = lv2.map('http://foo.org#bar') -- map URI to URID
uri = lv2.unmap(urid) -- unmap URID to URI

-- Constants
urid = lv2.Bool -- URID of LV2 Bool
urid = lv2.Chunk -- URID of LV2 Chunk
urid = lv2.Double -- URID of LV2 Double
urid = lv2.Float -- URID of LV2 Float
urid = lv2.Int -- URID of LV2 Integer (32bit integer)
urid = lv2.Long -- URID of LV2 Long (64bit integer)
urid = lv2.Literal -- URID of LV2 Literal
urid = lv2.Object -- URID of LV2 Object
urid = lv2.Path -- URID of LV2 Path
urid = lv2.Property -- URID of LV2 Property
urid = lv2.Sequence -- URID of LV2 Sequence
urid = lv2.String -- URID of LV2 String
urid = lv2.Tuple -- URID of LV2 Tuple
urid = lv2.URI -- URID of LV2 URI
urid = lv2.URID -- URID of LV2 URID
urid = lv2.Vector -- URID of LV2 Vector
urid = lv2.Midi -- URID of LV2 MIDI

--[[
	Atom Sequence
--]]
function _(seq)
	-- length operator
	n = #seq -- number of events in sequence

	-- indexing by string key
	t = seq.type -- type of atom, e.g. lv2.Sequence

	-- indexing by number key
	frames, atom = seq[1] -- get first event (frame time + atom)
	frames, atom = seq[#seq] -- get last event

	-- iteration
	for frames, atom in seq:foreach() do -- iterate over all events in sequence
	end
end

--[[
	Atom Forge
--]]
function _(forge)
	-- functions
	forge:frame_time(frames) -- start a new event at frames (Lua integer)
	forge:beat_time(beats) -- start a new event at beats (Lua number)

	forge:atom(atom) -- push a whole atom (atom, object, tuple, vector, ...)
	forge:int(1) -- push a Lua integer as 32bit integer
	forge:long(2) -- push a Lua integer as 64bit integer
	forge:float(0.1) -- push a Lua number as floating point
	forge:double(0.4) -- push a Lua number as double-precision floating point
	forge:bool(true) -- push Lua boolean
	
	forge:urid(URID) -- push a Lua number as URID
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
end

--[[
	Atom Object
--]]
function _(obj)
	local foo = {
		bar = lv2.map('http://foo.com#bar')
	}

	-- length operator
	n = #obj -- number of properties in object 

	-- indexing by string key
	t = obj.type -- type of atom, e.g. lv2.Object
	t = obj.id -- object id, e.g. 0
	t = obj.otype -- object type , e.g. foo_bar

	-- indexing by property key (number)
	atom = obj[foo.bar] -- get property value for key foo.bar

	-- iteration
	for key, context, atom in obj:foreach() do -- iterate over all elements in object
	end
end

--[[
	Atom Tuple
--]]
function _(tup)
	-- length operator
	n = #tup -- number of elements in tuple 

	-- indexing by string key
	t = tup.type -- type of atom, e.g. lv2.Tuple

	-- indexing by number key
	atom = tup[1] -- get first atom element
	atom = tup[#tup] -- get last atom element

	-- iteration
	for i, atom in tup:foreach() do -- iterate over all elements in tuple
	end

	-- unpacking
	atom1, atom2, atom3 = tup:unpack() -- unpack all elements in tuple
end

--[[
	Atom Vector
--]]
function _(vec)
	-- length operator
	n = #vec -- number of elements in vector

	-- indexing by string key
	t = vec.type -- type of atom, e.g. lv2.Vector
	t = vec.child_type -- type of child atom, e.g. lv2.Float
	t = vec.child_size -- byte size of child atom, e.g. 4

	-- indexing by number key
	atom = vec[1] -- get first atom element
	atom = vec[#vec] -- get last atom element

	-- iteration
	for i, atom in vec:foreach() do -- iterate over all elements in vector
	end

	-- unpacking
	atom1, atom2, atom3 = vec:unpack() -- unpack all elements in vector
end

--[[
	First-class Atom
--]]
function _(atom)
	-- length operator
	n = #atom -- byte size of atom

	-- indexing by string key
	t = atom.type -- type of atom, e.g. lv2.Int, lv2.Double, lv2.String
	v = atom.value -- native Lua value for the corresponding atom

	-- lv2.Bool
	bool = atom.value -- Lua boolean, e.g. true or false

	-- lv2.Int, lv2.Long
	int = atom.value -- Lua integer

	-- lv2.Float, lv2.Double
	float = atom.value -- Lua number

	-- lv2.String
	str = atom.value -- Lua string

	-- lv2.Literal
	literal = atom.value -- Lua string

	-- lv2.URID
	urid = atom.value -- Lua integer

	-- lv2.URI
	uri = atom.value -- Lua string

	-- lv2.Path
	path = atom.value -- Lua string

	-- lv2.Midi
	midi = atom.value -- Lua table with single raw Midi bytes
	m = #midi -- number of Midi bytes
	status = m[1] -- Midi status byte
end
