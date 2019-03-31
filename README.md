## Moony

### Realtime Lua as programmable glue in LV2

#### Build status

[![build status](https://gitlab.com/OpenMusicKontrollers/moony.lv2/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/moony.lv2/commits/master)

### Binaries

For GNU/Linux (64-bit, 32-bit, armv7), Windows (64-bit, 32-bit) and MacOS
(64/32-bit univeral).

To install the plugin bundle on your system, simply copy the __moony.lv2__
folder out of the platform folder of the downloaded package into your
[LV2 path](http://lv2plug.in/pages/filesystem-hierarchy-standard.html).

#### Stable release

* [moony.lv2-0.22.0.zip](https://dl.open-music-kontrollers.ch/moony.lv2/stable/moony.lv2-0.22.0.zip) ([sig](https://dl.open-music-kontrollers.ch/moony.lv2/stable/moony.lv2-0.22.0.zip.sig))

#### Unstable (nightly) release

* [moony.lv2-latest-unstable.zip](https://dl.open-music-kontrollers.ch/moony.lv2/unstable/moony.lv2-latest-unstable.zip) ([sig](https://dl.open-music-kontrollers.ch/moony.lv2/unstable/moony.lv2-latest-unstable.zip.sig))

### Sources

#### Stable release

* [moony.lv2-0.22.0.tar.xz](https://git.open-music-kontrollers.ch/lv2/moony.lv2/snapshot/moony.lv2-0.22.0.tar.xz)

#### Git repository

* <https://git.open-music-kontrollers.ch/lv2/moony.lv2>

### Packages

* [ArchLinux](https://www.archlinux.org/packages/community/x86_64/moony.lv2/)

### Bugs and feature requests

* [Gitlab](https://gitlab.com/OpenMusicKontrollers/moony.lv2)
* [Github](https://github.com/OpenMusicKontrollers/moony.lv2)

### General Overview

The Moony plugins come in three flavours, whereby some of them are more and
others less suitable for linear plugin hosts (e.g. DAWs). All of them are
suitable for non-linear hosts (NLH), e.g. [Ingen](https://drobilla.net/software/ingen/)
or [Synthpod](/lv2/synthpod/#).

* Control to control port conversion (NLH)
* Atom to atom port conversion (DAW, NLH)
* Control+atom to control+atom port conversion (DAW, NLH)

The design goal of the plugin bundle was to create a tool to easily add
realtime programmable logic glue in LV2 plugin graphs.

To have plugins which do a specific task efficiently is great, especially
for audio plugins. LV2 stands apart from other audio plugin specifications
with its extentable event system based on Atoms. As events can be much more
varied in nature and represent pretty much anything (NOT ONLY MIDI), it would
be useful to have a tool to create arbitrary event filters for a given setup
on-the-fly.

For a given setup, one may need a special event filter only once and it
seems to be overkill to write a native LV2 event filter in C/C++ just for
that. It would also be nice to have a tool for fast prototyping
of new event filters.

A scripting language seems to be ideal for these cases, where the user can
write an event filter on a higher level of abstraction on-the-fly.
The scripting language needs to be realtime safe, though, which restricts
the choices dramatically.

One such scripting language is [Lua](https://www.lua.org). It is
small, fast, easily embeddable and realtime-safe if coupled to a realtime-safe
memory allocator like [TLSF](http://www.gii.upv.es/tlsf/).

The Moony plugins can handle LV2 control and atom event ports, only. They do not
handle LV2 audio ports. They may eventually handle LV2 control-voltage ports
in the future, though. Control port values are internally
handled as simple floating point numbers, whereas the atom event ports
build on top of the LV2 atom and atom forge C headers.

The control port plugins are simple to script and need only low level
programming skills.

The atom event port plugins are more complex. You may want to first understand
the underlying concepts of LV2 atom and atom forge in the official resources:

* <http://lv2plug.in/ns/ext/atom/>
* <http://lv2plug.in/doc/html/group__atom.html>
* <http://lv2plug.in/doc/html/group__forge.html>

### API

_Note: The Moony Lua API is not stable yet, expect changes to happen until
an official release._ Preview the manual here:

<https://openmusickontrollers.gitlab.io/moony.lv2>

### Plugins

![Screenshot](https://git.open-music-kontrollers.ch/lv2/moony.lv2/plain/screenshots/screenshot_1.png)

#### Control to control port

There are different plugins which feature different numbers of input and
output ports. These plugins are simple to use: Define a __run__ function
which receives the right number of input values and returns the desired
output values. Missing output values will be set to 0.0 automatically.

##### C1 x C1

1x control input to 1x control output.

##### C2 x C2

2x control input to 2x control output.

##### C4 x C4
	
4x control input to 4x control output.

#### Atom to atom port

All atom containers (sequence, object, tuple, vector) implement a
__foreach__ method to be iterated over with Lua's __for__. The number
of children in each container is returned with Lua's length operator __#__.
Child elements can also be queried individually with an integer key
representing position (sequence, tuple, vector) or URID (object).

With an atom sequence as output, the plugins use the atom forge infrastructure
underneath. Each event added to the sequence consists of a frame time and
and a given atom type. The Lua atom forge API closely follows the C API.

##### A1 x A1
	
1x atom input to 1x atom output.

##### A2 x A2

2x atom input to 2x atom output.

##### A4 x A4

4x atom input to 4x atom output.

#### Control+atom to control+atom port

And if you need both atom input/output and control input/output, then there
is this here:

##### C1+A1 x C1+A1

1x control + 1x atom input to 1x control + 1x atom output.

##### C2+A1 x C2+A1

2x control + 1x atom input to 2x control + 1x atom output.

##### C4+A1 x C4+A1

4x control + 1x atom input to 4x control + 1x atom output.

### License

Copyright (c) 2015-2017 Hanspeter Portner (dev@open-music-kontrollers.ch)

This is free software: you can redistribute it and/or modify
it under the terms of the Artistic License 2.0 as published by
The Perl Foundation.

This source is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
Artistic License 2.0 for more details.

You should have received a copy of the Artistic License 2.0
along the source as a COPYING file. If not, obtain it from
<http://www.perlfoundation.org/artistic_license_2_0>.
