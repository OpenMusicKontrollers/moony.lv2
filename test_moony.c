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

typedef struct _Handle Handle;

struct _Handle {
	moony_t moony;

	int max_val;

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
	const float *val_in [4];
	float *val_out [4];

	const LV2_Atom_Sequence *control;
	LV2_Atom_Sequence *notify;
	
	LV2_Atom_Forge forge;
};

int
main(int argc, char **argv)
{
	Handle *producer = calloc(1, sizeof(Handle));
	Handle *consumer = calloc(1, sizeof(Handle));

	if(!producer || !consumer)
	{
		if(producer)
			free(producer);
		if(consumer)
			free(consumer);

		return -1;
	}

	//TODO write test cases

	return 0;
}
