# Moony.lv2

## Lua API (0.1.1)

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

#### URIDs of Atom extension

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

#### URIDs of LV2 MIDI extension

``` lua
urid = MIDI.MidiEvent
```

#### URIDs of Time extension

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

#### URIDs of OSC extension

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

#### URIDs of LV2 Core

``` lua
urid = Core.sampleRate
```

#### URIDs of Buf\_Size extension

``` lua
urid = Buf_Size.minBlockLength -- only defined if exported by host
urid = Buf_Size.maxBlockLength -- only defined if exported by host
urid = Buf_Size.sequenceSize -- only defined if exported by host
```

#### URIDs of Patch extension

``` lua
urid = Patch.Get
urid = Patch.Set
urid = Patch.subject
urid = Patch.property
urid = Patch.value
```

#### Options Table

``` lua
sampleRate = Options[Core.sampleRate]
sequenceSize = Options[Buf_Size.sequenceSize]
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

forge:vector(URID, {a, b, c, d}) -- push a vector of fixed-sized type URID
forge:vector(URID, a, b, c, d) -- push a vector of fixed-sized type URID

bndl = forge:bundle() -- start a new OSC bundle (returns a derived forge container)
bndl:message('/hello', 'si', 'world', 2015) -- push a complete OSC message
bndl:pop() -- finalize derived forge container

forge:bundle() -- start bundle with immediate timestamp (nil)
forge:bundle(1) -- start bundle with immediate timestamp (Lua integer = 1)
forge:bundle(0xd97332a2998a7199) -- start bundle with absolute timestamp (Lua integer = ?)
forge:bundle(0.1) -- start bundle with relative timestamp offset (Lua number = +0.1s)

tup = forge:tuple() -- start a new tuple (returns a derived forge container)
tup:pop() -- finalize derived forge container

obj = forge:object(id, otype) -- start a new object (returns a derived forge container)
obj:key(key) -- push a new object property with key (Lua integer)
obj:property(key, context) -- push a new object property with key, context (Lua integer)
obj:pop() -- finalize derived forge container

seq = forge:sequence(unit) -- start a nested sequence (returns a derived forge container)
seq:pop()
```

Vector forge only supports fixed-sized Atom types: e.g. Int, Long, Float, Double,
URID and Bool.

All forge functions but forge:pop return a forge; either itself or a
derived one, depending on context. One can thus fill values in sequence, e.g:

``` lua
forge:frame_time(0):midi(0x90, 0x20, 0x7f):frame_time(1):midi(0x80, 0x20, 0x00)
forge:frame_time(2):object(id, otype):key(key):int(1):pop()
```

### Atom Object

``` lua
local foo = {
	bar = Map['http://foo.com#bar']
}
local bar = {
	foo = Map['http://bar.com#foo']
}
```

#### length operator

``` lua
n = #obj -- number of properties in object 
```

#### indexing by string key

``` lua
t = obj.type -- type of atom, e.g. Atom.Object
t = obj.id -- object id, e.g. foo.bar
t = obj.otype -- object type , e.g. bar.foo
```

#### indexing by property key (URID as Lua integer)

``` lua
atom = obj[foo.bar] -- get property value for key foo.bar
```

#### iteration

``` lua
for key, context, atom in obj:foreach() do -- iterate over all elements in object
end
```

### Atom Tuple

#### length operator

``` lua
n = #tup -- number of elements in tuple 
```

#### indexing by string key

``` lua
t = tup.type -- type of atom, e.g. Atom.Tuple
```

#### indexing by number key

``` lua
atom = tup[1] -- get first atom element
atom = tup[#tup] -- get last atom element
```

#### iteration

``` lua
for i, atom in tup:foreach() do -- iterate over all elements in tuple
end
```

#### unpacking

``` lua
atom1, atom2, atom3 = tup:unpack() -- unpack all elements in tuple
atom2, atom3 = tup:unpack(2, 3) -- unpack given range of elements 
atom2, atom3 = tup:unpack(2) -- unpack starting from given index
```

### Atom Vector

#### length operator

``` lua
n = #vec -- number of elements in vector
```

#### indexing by string key

``` lua
t = vec.type -- type of atom, e.g. Atom.Vector
t = vec.child_type -- type of child atom, e.g. Atom.Float
t = vec.child_size -- byte size of child atom, e.g. 4
```

#### indexing by number key

``` lua
atom = vec[1] -- get first atom element
atom = vec[#vec] -- get last atom element
```

#### iteration

``` lua
for i, atom in vec:foreach() do -- iterate over all elements in vector
end
```

#### unpacking

``` lua
atom1, atom2, atom3 = vec:unpack() -- unpack all elements in vector
atom2, atom3 = vec:unpack(2, 3) -- unpack given range of elements
atom2, atom3 = vec:unpack(2) -- unpack starting from given index
```

### Atom MIDI

#### length operator

``` lua
bytes = #midi -- number of MIDI bytes
```

#### indexing by string key

``` lua
t = midi.type -- type of atom, e.g. MIDI.MidiEvent
m = midi.value -- Lua table with single raw MIDI bytes
status = m[1] -- MIDI status byte as Lua integer
```

#### indexing by number key

``` lua
status = midi[1] -- direct access of MIDI status byte
```

#### unpacking

``` lua
status, note, vel = midi:unpack() -- unpack all raw MIDI bytes to stack
status, note = midi:unpack(1, 2) -- unpack given range of bytes
note, vel = midi:unpack(2) -- unpack starting from given index
```

#### Constants

``` lua
-- System messages
MIDI.NoteOff
MIDI.NoteOn
MIDI.NotePressure
MIDI.Controller
MIDI.ProgramChange
MIDI.ChannelPressure
MIDI.Bender
MIDI.SystemExclusive
MIDI.QuarterFrame
MIDI.SongPosition
MIDI.SongSelect
MIDI.TuneRequest
MIDI.Clock
MIDI.Start
MIDI.Continue
MIDI.Stop
MIDI.ActiveSense
MIDI.Reset

-- Controllers
MIDI.BankSelection_MSB
MIDI.Modulation_MSB
MIDI.Breath_MSB
MIDI.Foot_MSB
MIDI.PortamentoTime_MSB
MIDI.DataEntry_MSB
MIDI.MainVolume_MSB
MIDI.Balance_MSB
MIDI.Panpot_MSB
MIDI.Expression_MSB
MIDI.Effect1_MSB
MIDI.Effect2_MSB
MIDI.GeneralPurpose1_MSB
MIDI.GeneralPurpose2_MSB
MIDI.GeneralPurpose3_MSB
MIDI.GeneralPurpose4_MSB

MIDI.BankSelection_LSB
MIDI.Modulation_LSB
MIDI.Breath_LSB
MIDI.Foot_LSB
MIDI.PortamentoTime_LSB
MIDI.DataEntry_LSB
MIDI.MainVolume_LSB
MIDI.Balance_LSB
MIDI.Panpot_LSB
MIDI.Expression_LSB
MIDI.Effect1_LSB
MIDI.Effect2_LSB
MIDI.GeneralPurpose1_LSB
MIDI.GeneralPurpose2_LSB
MIDI.GeneralPurpose3_LSB
MIDI.GeneralPurpose4_LSB

MIDI.SustainPedal
MIDI.Portamento
MIDI.Sostenuto
MIDI.SoftPedal
MIDI.LegatoFootSwitch
MIDI.Hold2

MIDI.SoundVariation
MIDI.ReleaseTime
MIDI.Timbre
MIDI.AttackTime
MIDI.Brightness
MIDI.SC1
MIDI.SC2
MIDI.SC3
MIDI.SC4
MIDI.SC5
MIDI.SC6
MIDI.SC7
MIDI.SC8
MIDI.SC9
MIDI.SC10

MIDI.GeneralPurpose5
MIDI.GeneralPurpose6
MIDI.GeneralPurpose7
MIDI.GeneralPurpose8
MIDI.PortamentoControl

MIDI.ReverbDepth
MIDI.TremoloDepth
MIDI.ChorusDepth
MIDI.DetuneDepth
MIDI.PhaserDepth
MIDI.E1
MIDI.E2
MIDI.E3
MIDI.E4
MIDI.E5

MIDI.DataIncrement
MIDI.DataDecrement

MIDI.NRPN_LSB
MIDI.NRPN_MSB

MIDI.RPN_LSB
MIDI.RPN_MSB

MIDI.AllSoundsOff
MIDI.ResetControllers
MIDI.LocalControlSwitch
MIDI.AllNotesOff
MIDI.OmniOff
MIDI.OmniOn
MIDI.Mono1
MIDI.Mono2
```

### Atom Chunk

#### length operator

``` lua
bytes = #chunk -- number of chunk bytes
```

#### indexing by string key

``` lua
t = chunk.type -- type of atom, e.g. Atom.Chunk
c = chunk.value -- Lua table with single raw chunk bytes
c1 = c[1] -- first byte as Lua integer
```

#### indexing by number key

``` lua
c1 = chunk[1] -- direct access of individual chunk bytes
```

#### unpacking

``` lua
c1, c2, c3, c4 = chunk:unpack() -- unpack all raw chunk bytes to stack
c2, c3, c4 = chunk:unpack(2, 4) -- unpack given range of elements
c3, c4 = chunk:unpack(3) -- unpack starting from given index
```

### First-class Atom

#### length operator

``` lua
n = #atom -- byte size of atom
```

#### indexing by string key

``` lua
t = atom.type -- type of atom, e.g. Atom.Int, Atom.Double, Atom.String
v = atom.value -- native Lua value for the corresponding atom
```

#### Atom.Bool

``` lua
bool = atom.value -- Lua boolean, e.g. true or false
```

#### Atom.Int, Atom.Long

``` lua
int = atom.value -- Lua integer
```

#### Atom.Float, Atom.Double

``` lua
float = atom.value -- Lua number
```

#### Atom.String

``` lua
str = atom.value -- Lua string
```

#### Atom.Literal

``` lua
literal = atom.value -- Lua string
datatype = atom.datatype -- URID as Lua integer
lang = atom.lang -- URID as Lua integer
```

#### Atom.URID

``` lua
urid = atom.value -- Lua integer
```

#### Atom.URI

``` lua
uri = atom.value -- Lua string
```

#### Atom.Path

``` lua
path = atom.value -- Lua string
```

### Responders

#### MIDIResponder

``` lua
midi_responder = MIDIResponder:new({
	[MIDI.NoteOn] = function(self, frames, forge, chan, note, vel)
		--
		return true -- handled
	end
})

function run(n, seq, forge)
	for frames, atom in seq:foreach() do
		local handled = midi_responder(frames, forge, atom)
	end
end
```

#### OSCResponder

``` lua
osc_responder = OSCResponder:new({
	['/ping'] = function(self, frames, forge, fmt, ...)
		--
		return true -- handled
	end
})

function run(n, seq, forge)
	for frames, atom in seq:foreach() do
		local handled = osc_responder(frames, forge, atom)
	end
end
```

#### TimeResponder

``` lua
time_responder = OSCResponder:new({
	[Time.barBeat] = function(self, frames, forge, bar_beat)
		--
	end,
	[Time.bar] = function(self, frames, forge, bar)
		--
	end,
	[Time.speed] = function(self, frames, forge, speed)
		--
	end
})

function run(n, seq, forge)
	local from = 0
	for frames, atom in seq:foreach() do
		time_responder(from, frames, forge, atom)
		from = frames
	end
	time_responder(from, n, forge)
end
```
