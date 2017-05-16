# Moony.lv2

## Realtime Lua as programmable glue in LV2

### Webpage 

Get more information at: [http://open-music-kontrollers.ch/lv2/moony](http://open-music-kontrollers.ch/lv2/moony)

### Build status

[![build status](https://gitlab.com/OpenMusicKontrollers/moony.lv2/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/moony.lv2/commits/master)

### Plugins

#### Control port input to control port output

* **C1 x C1** 1x control input to 1x control output
* **C2 x C2** 2x control input to 2x control output
* **C4 x C4** 4x control input to 4x control output

#### Atom port input to atom port output

* **A1 x A1** 1x atom input to 1x atom output
* **A2 x A2** 2x atom input to 2x atom output
* **A4 x A4** 4x atom input to 4x atom output

#### Control+atom port input to control+atom port output

* **C1+A1 x C1+A1** 1x control + 1x atom input to 1x control + 1x atom output
* **C2+A1 x C2+A1** 2x control + 1x atom input to 2x control + 1x atom output
* **C4+A1 x C4+A1** 4x control + 1x atom input to 4x control + 1x atom output

### Dependencies

* [LV2](http://lv2plug.in) (LV2 Plugin Standard)

### Build / install

	git clone https://gitlab.com/OpenMusicKontrollers/moony.lv2.git
	cd moony.lv2
	mkdir build
	cd build
	cmake -DCMAKE_BUILD_TYPE="Release" ..
	make
	sudo make install

If you want to run the unit test, do instead:
	
	.
	.
	cmake -DCMAKE_BUILD_TYPE="Debug" -DBUILD_TESTING=1 ..
	make
	ARGS="-VV" make test
	.
	.

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
