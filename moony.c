/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <moony.h>

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &c1xc1;
		case 1:
			return &c2xc2;
		case 2:
			return &c4xc4;

		case 3:
			return &a1xa1;
		case 4:
			return &a2xa2;
		case 5:
			return &a4xa4;
		
		case 6:
			return &c1a1xc1a1;
		case 7:
			return &c2a1xc2a1;
		case 8:
			return &c4a1xc4a1;

		default:
			return NULL;
	}
}
