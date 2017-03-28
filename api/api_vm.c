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

#if !defined(_WIN32)
# include <sys/mman.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <moony.h>
#include <api_vm.h>

#include <lualib.h>
#include <lauxlib.h>

#include <laes128.h>

//FIXME put those into a header
extern int luaopen_lpeg(lua_State *L);
extern int luaopen_base64(lua_State *L);
extern int luaopen_ascii85(lua_State *L);

//#define MOONY_LOG_MEM
#ifdef MOONY_LOG_MEM
__realtime static inline void
_log_mem(moony_t *moony, void *ptr, size_t osize, size_t nsize)
{
	moony_vm_t *vm = &moony->vm;
	if(moony->log)
	{
		char suffix0 = ' ';
		size_t space = vm->space;
		if(space >= 1024)
		{
			suffix0 = 'K';
			space >>= 10;
		}
		if(space >= 1024)
		{
			suffix0 = 'M';
			space >>= 10;
		}

		char suffix1 = ' ';
		size_t used = vm->used;
		if(used >= 1024)
		{
			suffix1 = 'K';
			used >>= 10;
		}
		if(used >= 1024)
		{
			suffix1 = 'M';
			used >>= 10;
		}

		if(moony->log)
			lv2_log_trace(&moony->logger, "space: %4zu%c, used: %4zu%c, old: %4zu, new: %4zu, data: %p\n",
				space, suffix0, used, suffix1, osize, nsize, ptr);
	}
}
#endif

__realtime inline void *
moony_rt_alloc(moony_vm_t *vm, size_t nsize)
{
	vm->used += nsize;
	if(vm->used > (vm->space >> 1))
		moony_vm_mem_extend(vm);

#ifdef MOONY_LOG_MEM
	_log_mem(moony, NULL, 0, nsize);
#endif

	return tlsf_malloc(vm->tlsf, nsize);
}

__realtime inline void *
moony_rt_realloc(moony_vm_t *vm, void *buf, size_t osize, size_t nsize)
{
	vm->used -= osize;
	vm->used += nsize;
	if(vm->used > (vm->space >> 1))
		moony_vm_mem_extend(vm);

#ifdef MOONY_LOG_MEM
	_log_mem(moony, buf, osize, nsize);
#endif

	return tlsf_realloc(vm->tlsf, buf, nsize);
}

__realtime inline void
moony_rt_free(moony_vm_t *vm, void *buf, size_t osize)
{
	vm->used -= osize;
	if(vm->used > (vm->space >> 1))
		moony_vm_mem_extend(vm);

#ifdef MOONY_LOG_MEM
	_log_mem(moony, buf, osize, 0);
#endif

	tlsf_free(vm->tlsf, buf);
}

__realtime static void *
lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	moony_vm_t *vm = ud;

	if(nsize == 0)
	{
		if(ptr)
			moony_rt_free(vm, ptr, osize);
		return NULL;
	}
	else
	{
		if(ptr)
			return moony_rt_realloc(vm, ptr, osize, nsize);
		else
			return moony_rt_alloc(vm, nsize);
	}
}

__non_realtime moony_vm_t *
moony_vm_new(size_t mem_size, bool testing)
{
	moony_vm_t *vm = calloc(1, sizeof(moony_vm_t));
	if(!vm)
		return NULL;

	// initialize array of increasing pool sizes
	vm->size[0] = mem_size;
	for(int i=1, sum=mem_size; i<MOONY_POOL_NUM; i++, sum*=2)
		vm->size[i] = sum;

	// allocate first pool
	vm->area[0] = moony_vm_mem_alloc(vm->size[0]);
	if(!vm->area[0])
	{
		moony_vm_mem_free(vm->area[0], vm->size[0]);
		free(vm);
		return NULL;
	}
	
	vm->tlsf = tlsf_create_with_pool(vm->area[0], vm->size[0]);
	if(!vm->tlsf)
	{
		free(vm);
		return NULL;
	}

	vm->pool[0] = tlsf_get_pool(vm->tlsf);
	vm->space += vm->size[0];

	lua_State *L = lua_newstate(lua_alloc, vm);
	if(!L)
	{
		free(vm);
		return NULL;
	}

	vm->L = L;

	const int n = lua_gettop(L);

	luaL_requiref(L, "base", luaopen_base, 0);

	luaL_requiref(L, "coroutine", luaopen_coroutine, 1);
	luaL_requiref(L, "table", luaopen_table, 1);
	luaL_requiref(L, "string", luaopen_string, 1);
	luaL_requiref(L, "math", luaopen_math, 1);
	luaL_requiref(L, "utf8", luaopen_utf8, 1);
	luaL_requiref(L, "debug", luaopen_debug, 1);

	luaL_requiref(L, "lpeg", luaopen_lpeg, 1);
	luaL_requiref(L, "base64", luaopen_base64, 1);
	luaL_requiref(L, "ascii85", luaopen_ascii85, 1);
	luaL_requiref(L, "aes128", luaopen_aes128, 1);

	if(testing)
	{
		luaL_requiref(L, "io", luaopen_io, 1);
		luaL_requiref(L, "package", luaopen_package, 1);
		//luaL_requiref(L, "os", luaopen_os, 1);
		//luaL_requiref(L, "bit32", luaopen_bit32, 1);
	}

	lua_settop(L, n);

	if(!testing)
	{
		// clear dofile
		lua_pushnil(L);
		lua_setglobal(L, "dofile");

		// clear loadfile
		lua_pushnil(L);
		lua_setglobal(L, "loadfile");
	}

#ifdef USE_MANUAL_GC
	// manual garbage collector
	lua_gc(L, LUA_GCSTOP, 0); // disable automatic garbage collection
	lua_gc(L, LUA_GCSETPAUSE, 0); // don't wait to start a new cycle
	lua_gc(L, LUA_GCSETSTEPMUL, 100); // set step size to run 'as fast a memory allocation'
#else
	// automatic garbage collector
	lua_gc(L, LUA_GCRESTART, 0); // enable automatic garbage collection
	lua_gc(L, LUA_GCSETPAUSE, 105); // next step when memory increased by 5%
	lua_gc(L, LUA_GCSETSTEPMUL, 105); // run 5% faster than memory allocation
#endif

	return vm;
}

__non_realtime void
moony_vm_free(moony_vm_t *vm)
{
	if(vm->L)
		lua_close(vm->L);

	vm->used = 0;

	for(int i=(MOONY_POOL_NUM-1); i>=0; i--)
	{
		if(!vm->area[i])
			continue; // this memory slot is unused, skip it

		tlsf_remove_pool(vm->tlsf, vm->pool[i]);
		moony_vm_mem_free(vm->area[i], vm->size[i]);
		vm->space -= vm->size[i];

		vm->area[i] = NULL;
		vm->pool[i] = NULL;
	}

	assert(vm->space == 0);
	tlsf_destroy(vm->tlsf);
	vm->tlsf = NULL;

	free(vm);
}

__non_realtime void *
moony_vm_mem_alloc(size_t size)
{
	void *area = NULL;

	//printf("moony_vm_mem_alloc: %zu\n", size);

#if defined(_WIN32)
	area = _aligned_malloc(size, 8);
#else
	posix_memalign(&area, 8, size);
#endif
	if(!area)
		return NULL;

	mlock(area, size);
	return area;
}

__non_realtime void
moony_vm_mem_free(void *area, size_t size)
{
	if(!area)
		return;
	
	//printf("moony_vm_mem_free: %zu\n", size);

	munlock(area, size);
	free(area);
}

__realtime int
moony_vm_mem_extend(moony_vm_t *vm)
{
	moony_t *moony = (void *)vm - offsetof(moony_t, vm);

	// request processing or fully extended?
	if(moony->working || moony->fully_extended)
		return -1;

	for(int i=0; i<MOONY_POOL_NUM; i++)
	{
		if(vm->area[i]) // pool already allocated/in-use
			continue;

		if(vm->locked)
		{
			vm->area[i] = moony_vm_mem_alloc(vm->size[i]);
			if(vm->area[i])
			{
				vm->pool[i] = tlsf_add_pool(vm->tlsf,
					vm->area[i], vm->size[i]); //FIXME stoat complains about printf

				if(vm->pool[i])
					vm->space += vm->size[i];
				else
					moony_vm_mem_free(vm->area[i], vm->size[i]);
			}
		}
		else
		{
			const moony_job_t req = {
				.mem = {
					.tup = {
						.atom.size = 32,
						.atom.type = moony->forge.Tuple,
					},
					.i32 = {
						.atom.size = sizeof(int32_t),
						.atom.type = moony->forge.Int,
						.body = i,
					},
					.i64 = {
						.atom.size = sizeof(int64_t),
						.atom.type = moony->forge.Long,
						.body = 0
					}
				}
			};

			// schedule allocation of memory to _work
			const LV2_Worker_Status status = moony->sched->schedule_work(
				moony->sched->handle, lv2_atom_total_size(&req.atom), &req);

			// toggle working flag
			if(status == LV2_WORKER_SUCCESS)
				moony->working = 1;
		}

		return 0;
	}

	moony->fully_extended = 1;

	return -1;
}

void 
moony_vm_lock(moony_vm_t *vm)
{
	vm->locked = true;
}

void
moony_vm_unlock(moony_vm_t *vm)
{
	vm->locked = false;
}
