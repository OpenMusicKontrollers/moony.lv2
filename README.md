# Lua.lv2

## Realtime Lua as programmable glue in LV2

### Webpage 

Get more information at: [http://open-music-kontrollers.ch/lv2/lua](http://open-music-kontrollers.ch/lv2/lua)

### Build status

[![Build Status](https://travis-ci.org/OpenMusicKontrollers/lua.lv2.svg)](https://travis-ci.org/OpenMusicKontrollers/lua.lv2)

### Plugins

#### Lua for Control Ports

#### Lua for Atom Ports

### Dependencies

* [LV2](http://lv2plug.in) (LV2 Plugin Standard)
* [Lua](http://lua.org) (Lightweight embeddable language)
* [EFL](http://docs.enlightenment.org/stable/elementary/) (Enlightenment Foundation Libraries)
* [Elementary](http://docs.enlightenment.org/stable/efl/) (Lightweight GUI Toolkit)

### Build / install

	git clone https://github.com/OpenMusicKontrollers/lua.lv2.git
	cd lua.lv2
	mkdir build
	cd build
	cmake -DCMAKE_C_FLAGS="-std=gnu99" ..
	make
	sudo make install

### License

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
<http://www.perlfoundation.org/artistic_license_2_0>.
