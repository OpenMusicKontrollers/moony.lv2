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
#include <assert.h>

#include <varchunk.h>

#if !defined(_WIN32)
#	include <sys/mman.h>
#	include <sys/stat.h>
#	include <fcntl.h>
# include <string.h>
# define VARCHUNK_USE_SHARED_MEM

static const struct timespec req = {
	.tv_sec = 0,
	.tv_nsec = 1
};
#endif

static uint64_t iterations = 10000000;
#define THRESHOLD (RAND_MAX / 256)
#define PAD(SIZE) ( ( (size_t)(SIZE) + 7U ) & ( ~7U ) )

static void *
producer_main(void *arg)
{
	varchunk_t *varchunk = arg;
	uint8_t *ptr;
	const uint8_t *end;
	size_t written;
	uint64_t cnt = 0;

	while(cnt < iterations)
	{
#if !defined(_WIN32)
		if(rand() < THRESHOLD)
		{
			nanosleep(&req, NULL);
		}
#endif

		written = PAD(rand() * 1024.f / RAND_MAX);

		size_t maximum;
		if( (ptr = varchunk_write_request_max(varchunk, written, &maximum)) )
		{
			assert(maximum >= written);
			end = ptr + written;
			for(uint8_t *src=ptr; src<end; src+=sizeof(uint64_t))
			{
				*(uint64_t *)src = cnt;
				assert(*(uint64_t *)src == cnt);
			}
			varchunk_write_advance(varchunk, written);
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
	const uint8_t *ptr;
	const uint8_t *end;
	size_t toread;
	uint64_t cnt = 0;

	while(cnt < iterations)
	{
#if !defined(_WIN32)
		if(rand() < THRESHOLD)
		{
			nanosleep(&req, NULL);
		}
#endif

		if( (ptr = varchunk_read_request(varchunk, &toread)) )
		{
			end = ptr + toread;
			for(const uint8_t *src=ptr; src<end; src+=sizeof(uint64_t))
			{
				assert(*(const uint64_t *)src == cnt);
			}
			varchunk_read_advance(varchunk);
			cnt++;
		}
		else
		{
			// buffer empty
		}
	}

	return NULL;
}

static void
test_threaded()
{
	pthread_t producer;
	pthread_t consumer;
	varchunk_t *varchunk = varchunk_new(8192, true);
	assert(varchunk);

	pthread_create(&consumer, NULL, consumer_main, varchunk);
	pthread_create(&producer, NULL, producer_main, varchunk);

	pthread_join(producer, NULL);
	pthread_join(consumer, NULL);

	varchunk_free(varchunk);
}

#if defined(VARCHUNK_USE_SHARED_MEM)
typedef struct _varchunk_shm_t varchunk_shm_t;

struct _varchunk_shm_t {
	char *name;
	int fd;
	varchunk_t *varchunk;
};

static int
varchunk_shm_init(varchunk_shm_t *varchunk_shm, const char *name, size_t minimum, bool release_and_acquire)
{
	const size_t body_size = varchunk_body_size(minimum);
	const size_t total_size = sizeof(varchunk_t) + body_size;

	varchunk_shm->name = strdup(name);
	if(!varchunk_shm->name)
	{
		return -1;
	}

	bool is_first = true;
	varchunk_shm->fd = shm_open(varchunk_shm->name, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if(varchunk_shm->fd == -1)
	{
		is_first = false;
		varchunk_shm->fd = shm_open(varchunk_shm->name, O_RDWR , S_IRUSR | S_IWUSR);
	}
	if(varchunk_shm->fd == -1)
	{
		free(varchunk_shm->name);
		return -1;
	}

	if(  (ftruncate(varchunk_shm->fd, total_size) == -1)
		|| ((varchunk_shm->varchunk = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
					MAP_SHARED, varchunk_shm->fd, 0)) == MAP_FAILED) )
	{
		shm_unlink(varchunk_shm->name);
		close(varchunk_shm->fd);
		free(varchunk_shm->name);
		return -1;
	}

	if(is_first)
	{
		varchunk_init(varchunk_shm->varchunk, body_size, release_and_acquire);
	}

	return 0;
}

static void
varchunk_shm_deinit(varchunk_shm_t *varchunk_shm)
{
	const size_t total_size = sizeof(varchunk_t) + varchunk_shm->varchunk->size;

	munmap(varchunk_shm->varchunk, total_size);
	shm_unlink(varchunk_shm->name);
	close(varchunk_shm->fd);
	free(varchunk_shm->name);
}

static void
test_shared()
{
	const char *name = "/varchunk_shm_test";
	pid_t pid = fork();

	assert(pid != -1);

	if(pid == 0) // child
	{
		varchunk_shm_t varchunk_shm;
		assert(varchunk_shm_init(&varchunk_shm, name, 8192, true) == 0);

		consumer_main(varchunk_shm.varchunk);

		varchunk_shm_deinit(&varchunk_shm);
	}
	else // parent
	{
		varchunk_shm_t varchunk_shm;
		assert(varchunk_shm_init(&varchunk_shm, name, 8192, true) == 0);

		producer_main(varchunk_shm.varchunk);

		varchunk_shm_deinit(&varchunk_shm);
	}
}
#endif

int
main(int argc, char **argv)
{
#if !defined(_WIN32)
	const int seed = time(NULL);
	srand(seed);
#endif

	if(argc >= 2)
	{
		iterations = atoi(argv[1]);
	}

	assert(varchunk_is_lock_free());

	test_threaded();

#if defined(VARCHUNK_USE_SHARED_MEM)
	test_shared();
#endif

	return 0;
}
