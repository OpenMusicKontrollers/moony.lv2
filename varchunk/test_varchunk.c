/*
 * Copyright (c) 2015-2017 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>

#include <varchunk.h>

#define ITERATIONS 10000000
#define THRESHOLD (RAND_MAX / 256)

static const struct timespec req = {
	.tv_sec = 0,
	.tv_nsec = 1
};

static void *
producer_main(void *arg)
{
	varchunk_t *varchunk = arg;
	void *ptr;
	const void *end;
	size_t written;
	uint64_t cnt = 0;

	while(cnt < ITERATIONS)
	{
		if(rand() < THRESHOLD)
			nanosleep(&req, NULL);

		written = rand() * 1024.f / RAND_MAX;

		size_t maximum;
		if( (ptr = varchunk_write_request_max(varchunk, written, &maximum)) )
		{
			if(maximum < written)
				exit(-1);
			end = ptr + written;
			for(void *src=ptr; src<end; src+=sizeof(uint64_t))
				*(uint64_t *)src = cnt;
			varchunk_write_advance(varchunk, written);
			//fprintf(stdout, "P %"PRIu64" %zu %zu\n", cnt, written, maximum);
			cnt++;
		}
		else
		{
			// buffer full
		}
	}

	return NULL;
}

static void *
consumer_main(void *arg)
{
	varchunk_t *varchunk = arg;
	const void *ptr;
	const void *end;
	size_t toread;
	uint64_t cnt = 0;

	while(cnt < ITERATIONS)
	{
		if(rand() < THRESHOLD)
			nanosleep(&req, NULL);

		if( (ptr = varchunk_read_request(varchunk, &toread)) )
		{
			end = ptr + toread;
			for(const void *src=ptr; src<end; src+=sizeof(uint64_t))
				if(*(const uint64_t *)src != cnt)
					exit(-1); // TEST FAILED
			varchunk_read_advance(varchunk);
			//fprintf(stdout, "C %"PRIu64" %zu\n", cnt, toread);
			cnt++;
		}
		else
		{
			// buffer empty
		}
	}

	return NULL;
}

int
main(int argc, char **argv)
{
	const int seed = time(NULL);
	srand(seed);

	if(!varchunk_is_lock_free())
		return -1; // TEST FAILED

	pthread_t producer;
	pthread_t consumer;
	varchunk_t *varchunk = varchunk_new(8192, true);
	if(!varchunk)
		return -1; // TEST FAILED

	pthread_create(&consumer, NULL, consumer_main, varchunk);
	pthread_create(&producer, NULL, producer_main, varchunk);

	pthread_join(producer, NULL);
	pthread_join(consumer, NULL);

	varchunk_free(varchunk);

	return 0;
}
