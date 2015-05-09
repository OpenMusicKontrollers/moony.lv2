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

#include <sys/mman.h>
#include <assert.h>

#include <moony.h>

#include <lualib.h>
#include <lauxlib.h>

#define MEM_SIZE 0x20000UL // 128KB

// rt
static inline void *
rt_alloc(moony_vm_t *vm, size_t len)
{
	return tlsf_malloc(vm->tlsf, len);
}

// rt
static inline void *
rt_realloc(moony_vm_t *vm, size_t len, void *buf)
{
	return tlsf_realloc(vm->tlsf, buf, len);
}

// rt
static inline void
rt_free(moony_vm_t *vm, void *buf)
{
	tlsf_free(vm->tlsf, buf);
}

// rt
static void *
lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	moony_vm_t *vm = ud;
	(void)osize;

	vm->used += nsize - (ptr ? osize : 0);
	if(vm->used > (vm->space >> 1))
		moony_vm_mem_extend(vm); // TODO make this rt-safe via worker

	if(nsize == 0)
	{
		if(ptr)
			rt_free(vm, ptr);
		return NULL;
	}
	else
	{
		if(ptr)
			return rt_realloc(vm, nsize, ptr);
		else
			return rt_alloc(vm, nsize);
	}
}

// non-rt
int
moony_vm_init(moony_vm_t *vm)
{
	memset(vm, 0x0, sizeof(moony_vm_t));

	// initialzie array of increasing pool sizes
	vm->size[0] = MEM_SIZE;
	for(int i=1, sum=MEM_SIZE; i<MOONY_POOL_NUM; i++, sum*=2)
		vm->size[i] = sum;

	// allocate first pool
	vm->area[0] = moony_vm_mem_alloc(vm->size[0]);
	if(!vm->area[0])
		return -1;
	
	vm->tlsf = tlsf_create_with_pool(vm->area[0], vm->size[0]);
	if(!vm->tlsf)
		return -1;
	vm->pool[0] = tlsf_get_pool(vm->tlsf);
	vm->space += vm->size[0];
	
	vm->L = lua_newstate(lua_alloc, vm);
	if(!vm->L)
		return -1;

	luaL_requiref(vm->L, "base", luaopen_base, 0);
	luaL_requiref(vm->L, "coroutine", luaopen_coroutine, 1);
	luaL_requiref(vm->L, "table", luaopen_table, 1);
	luaL_requiref(vm->L, "string", luaopen_string, 1);
	luaL_requiref(vm->L, "utf8", luaopen_utf8, 1);
	luaL_requiref(vm->L, "math", luaopen_math, 1);
	lua_pop(vm->L, 6);

	//TODO overwrite print function
	
	//luaL_requiref(vm->L, "io", luaopen_io, 1);
	//luaL_requiref(vm->L, "os", luaopen_os, 1);
	//luaL_requiref(vm->L, "bit32", luaopen_bit32, 1);
	//luaL_requiref(vm->L, "debug", luaopen_debug, 1);
	//luaL_requiref(vm->L, "package", luaopen_package, 1);

	lua_gc(vm->L, LUA_GCSTOP, 0); // disable automatic garbage collection

	return 0;
}

// non-rt
int
moony_vm_deinit(moony_vm_t *vm)
{
	if(vm->L)
		lua_close(vm->L);
	vm->L = NULL;
	vm->used = 0;

	for(int i=(MOONY_POOL_NUM-1); i>=0; i--)
	{
		if(!vm->area[i])
			continue; // this memory slot is unused, skip it

		tlsf_remove_pool(vm->tlsf, vm->pool[i]);
		moony_vm_mem_free(vm->area[i], vm->size[i]);
		vm->space -= vm->size[i];

		vm->size[i] = 0;
		vm->area[i] = NULL;
		vm->pool[i] = NULL;
	}

	assert(vm->space == 0);
	tlsf_destroy(vm->tlsf);
	vm->tlsf = NULL;

	return 0;
}

// non-rt
void *
moony_vm_mem_alloc(size_t size)
{
	void *area = NULL;

#if defined(_WIN32)
	area = _aligned_malloc(size, 8);
#else
	posix_memalign(&area, 8, size);
	if(area)
		mlock(area, size);
#endif

	return area;
}

// non-rt
void
moony_vm_mem_free(void *area, size_t size)
{
	if(!area)
		return;

#if !defined(_WIN32)
	munlock(area, size);
#endif
	free(area);
}

// non-rt
int
moony_vm_mem_extend(moony_vm_t *vm)
{
	moony_t *moony = (void *)vm - offsetof(moony_t, vm);

	if(moony->working) // request is already been processed
		return -1;

	for(int i=0; i<MOONY_POOL_NUM; i++)
	{
		if(vm->area[i])
			continue;

		const moony_mem_t request = {
			.i = i,
			.mem = NULL
		};

		// schedule allocation of memory to _work
		LV2_Worker_Status status = moony->sched->schedule_work(
			moony->sched->handle, sizeof(moony_mem_t), &request);

		// toggle working flag
		if(status == LV2_WORKER_SUCCESS)
			moony->working = 1;

		return 0;
	}

	return -1;
}
