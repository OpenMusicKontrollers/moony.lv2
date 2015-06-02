# Moony.lv2

## Lua API v.0.1

### General usage
	
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

``` lua
-- code example for plugin 'C4+A1xC4+A1' in a clone configuration
function run(nsamples, seq, forge, a, b, c, d)
	for frames, atom in seq:foreach() do
		forge:frame_time(frames)
		forge:atom(atom)
	end

	return a, b, c, d
end
```

### Logging

Moony overwrites Lua's 'print' function to use the LV2 log extension.
This may be useful for debugging purposes

``` lua
print('hello', 'world', 2015, true)
```
	
### URI/URID (un)Mapping

#### Tables

``` lua
urid = Map['http://foo.org#bar'] -- Map URI to URID
uri = Unmap[urid] -- Unmap URID to URI
```

#### Constants

``` lua
urid = Atom.Bool -- URID of LV2 Bool
urid = Atom.Chunk -- URID of LV2 Chunk
urid = Atom.Double -- URID of LV2 Double
urid = Atom.Float -- URID of LV2 Float
urid = Atom.Int -- URID of LV2 Integer (32bit integer)
urid = Atom.Long -- URID of LV2 Long (64bit integer)
urid = Atom.Literal -- URID of LV2 Literal
urid = Atom.Object -- URID of LV2 Object
urid = Atom.Path -- URID of LV2 Path
urid = Atom.Property -- URID of LV2 Property
urid = Atom.Sequence -- URID of LV2 Sequence
urid = Atom.String -- URID of LV2 String
urid = Atom.Tuple -- URID of LV2 Tuple
urid = Atom.URI -- URID of LV2 URI
urid = Atom.URID -- URID of LV2 URID
urid = Atom.Vector -- URID of LV2 Vector
```

#### URID of LV2 MIDI Event

``` lua
urid = MIDI.Event
```

#### URIDs of LV2 Time Position object

``` lua
urid = Time.Position
urid = Time.barBeat
urid = Time.bar
urid = Time.beat
urid = Time.beatUnit
urid = Time.beatsPerBar
urid = Time.beatsPerMinute
urid = Time.frame
urid = Time.framesPerSecond
urid = Time.speed
```

#### URIDs of OSC objects

``` lua
urid = OSC.Event
urid = OSC.Bundle
urid = OSC.Message
urid = OSC.bundleTimestamp
urid = OSC.bundleItems
urid = OSC.messagePath
urid = OSC.messageFormat
urid = OSC.messageArguments
```
	
### Atom Sequence

#### length operator

``` lua
n = #seq -- number of events in sequence
```

#### indexing by string key

``` lua
t = seq.type -- type of atom, e.g. Atom.Sequence
```

#### indexing by number key

``` lua
atom = seq[1] -- get first event atom
atom = seq[#seq] -- get last event atom
```

#### iteration

``` lua
for frames, atom in seq:foreach() do -- iterate over all events in sequence
end
```
	
### Atom Forge

#### functions

``` lua
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

forge:midi({0x90, 0x4a, 0x7f}) -- push a Lua table as MIDI message
forge:midi(0x90, 0x4a, 0x7f) -- push individual Lua integers as MIDI message
forge:chunk({0x01, 0x02, 0x03, 0x04}) -- push a Lua table as atom chunk
forge:chunk(0x01, 0x02, 0x03, 0x04) -- push individual Lua integers as atom chunk

bndl = forge:bundle(1) -- start a new OSC bundle with timestamp (returns a derived forge container)
bndl:message('/hello', 'si', 'world', 2015) -- push a complete OSC message
bndl:pop() -- finalize derived forge container

tup = forge:tuple() -- start a new tuple (returns a derived forge container)
tup:pop() -- finalize derived forge container

obj = forge:object(id, otype) -- start a new object (returns a derived forge container)
obj:key(key) -- push a new object property with key (Lua integer)
obj:property(key, context) -- push a new object property with key, context (Lua integer)
obj:pop() -- finalize derived forge container

seq = forge:sequence(unit) -- start a nested sequence (returns a derived forge container)
seq:pop()
```

All forge functions but forge:pop return a forge; either itself or a
derived one, depending on context. One can thus fill values in sequence, e.g:

``` lua
forge:frame_time(0):midi(0x90, 0x20, 0x7f):frame_time(1):midi(0x80, 0x20, 0x00)
forge:frame_time(2):object(id, otype):key(key):int(1):pop()
```
