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

#include <lua_lv2.h>

#include <lualib.h>
#include <lauxlib.h>

#define MEM_SIZE 0x20000UL // 128KB

// rt
static inline void *
rt_alloc(Lua_VM *lvm, size_t len)
{
	return tlsf_malloc(lvm->tlsf, len);
}

// rt
static inline void *
rt_realloc(Lua_VM *lvm, size_t len, void *buf)
{
	return tlsf_realloc(lvm->tlsf, buf, len);
}

// rt
static inline void
rt_free(Lua_VM *lvm, void *buf)
{
	tlsf_free(lvm->tlsf, buf);
}

// rt
static void *
lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	Lua_VM *lvm = ud;
	(void)osize;

	lvm->used += nsize - (ptr ? osize : 0);
	if(lvm->used > (lvm->space >> 1))
		lua_vm_mem_extend(lvm); // TODO make this rt-safe via worker

	if(nsize == 0)
	{
		if(ptr)
			rt_free(lvm, ptr);
		return NULL;
	}
	else
	{
		if(ptr)
			return rt_realloc(lvm, nsize, ptr);
		else
			return rt_alloc(lvm, nsize);
	}
}

// non-rt
int
lua_vm_init(Lua_VM *lvm)
{
	memset(lvm, 0x0, sizeof(Lua_VM));

	// initialzie array of increasing pool sizes
	lvm->size[0] = MEM_SIZE;
	for(int i=1, sum=MEM_SIZE; i<POOL_NUM; i++, sum*=2)
		lvm->size[i] = sum;

	// allocate first pool
	lvm->area[0] = lua_vm_mem_alloc(lvm->size[0]);
	if(!lvm->area[0])
		return -1;
	
	lvm->tlsf = tlsf_create_with_pool(lvm->area[0], lvm->size[0]);
	if(!lvm->tlsf)
		return -1;
	lvm->pool[0] = tlsf_get_pool(lvm->tlsf);
	lvm->space += lvm->size[0];
	
	lvm->L = lua_newstate(lua_alloc, lvm);
	if(!lvm->L)
		return -1;

	luaL_requiref(lvm->L, "base", luaopen_base, 0);
	luaL_requiref(lvm->L, "coroutine", luaopen_coroutine, 1);
	luaL_requiref(lvm->L, "table", luaopen_table, 1);
	luaL_requiref(lvm->L, "string", luaopen_string, 1);
	luaL_requiref(lvm->L, "utf8", luaopen_utf8, 1);
	luaL_requiref(lvm->L, "math", luaopen_math, 1);
	lua_pop(lvm->L, 6);

	//TODO overwrite print function
	
	//luaL_requiref(lvm->L, "io", luaopen_io, 1);
	//luaL_requiref(lvm->L, "os", luaopen_os, 1);
	//luaL_requiref(lvm->L, "bit32", luaopen_bit32, 1);
	//luaL_requiref(lvm->L, "debug", luaopen_debug, 1);
	//luaL_requiref(lvm->L, "package", luaopen_package, 1);

	lua_gc(lvm->L, LUA_GCSTOP, 0); // disable automatic garbage collection

	return 0;
}

// non-rt
int
lua_vm_deinit(Lua_VM *lvm)
{
	if(lvm->L)
		lua_close(lvm->L);
	lvm->L = NULL;
	lvm->used = 0;

	for(int i=(POOL_NUM-1); i>=0; i--)
	{
		if(!lvm->area[i])
			continue; // this memory slot is unused, skip it

		tlsf_remove_pool(lvm->tlsf, lvm->pool[i]);
		lua_vm_mem_free(lvm->area[i], lvm->size[i]);
		lvm->space -= lvm->size[i];

		lvm->size[i] = 0;
		lvm->area[i] = NULL;
		lvm->pool[i] = NULL;
	}

	assert(lvm->space == 0);
	tlsf_destroy(lvm->tlsf);
	lvm->tlsf = NULL;

	return 0;
}

// non-rt
void *
lua_vm_mem_alloc(size_t size)
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
lua_vm_mem_free(void *area, size_t size)
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
lua_vm_mem_extend(Lua_VM *lvm)
{
	lua_handle_t *lua_handle = (void *)lvm - offsetof(lua_handle_t, lvm);

	if(lua_handle->working) // request is already been processed
		return -1;

	for(int i=0; i<POOL_NUM; i++)
	{
		if(lvm->area[i])
			continue;

		const mem_request_t request = {
			.i = i,
			.mem = NULL
		};

		// schedule allocation of memory to _work
		LV2_Worker_Status status = lua_handle->sched->schedule_work(
			lua_handle->sched->handle, sizeof(mem_request_t), &request);

		// toggle working flag
		if(status == LV2_WORKER_SUCCESS)
			lua_handle->working = 1;

		return 0;
	}

	return -1;
}
