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

#include <lua_lv2.h>

#include <sys/mman.h>

#include <lualib.h>

#define MEM_SIZE 0x100000UL // 1MB

static inline void *
rt_alloc(Lua_VM *lvm, size_t len)
{
	void *data = NULL;

	data = tlsf_malloc(lvm->tlsf, len);
	if(!data)
		printf("tlsf_malloc failed\n");

	return data;
}

static inline void *
rt_realloc(Lua_VM *lvm, size_t len, void *buf)
{
	void *data = NULL;
	
	data =tlsf_realloc(lvm->tlsf, buf, len);
	if(!data)
		printf("tlsf_realloc failed\n");

	return data;
}

static inline void
rt_free(Lua_VM *lvm, void *buf)
{
	tlsf_free(lvm->tlsf, buf);
}

static void *
lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
	Lua_VM *lvm = ud;
	(void)osize;

	lvm->used += nsize - (ptr ? osize : 0);
	//printf("rt_alloc: %zu\n", lvm->used);

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

int
lua_vm_init(Lua_VM *lvm)
{
	memset(lvm, 0x0, sizeof(Lua_VM));

#if defined(_WIN32)
	lvm->area= _aligned_malloc(MEM_SIZE, 8);
#else
	posix_memalign(&lvm->area, 8, MEM_SIZE);
	mlock(lvm->area, MEM_SIZE);
#endif
	if(!lvm->area)
		return -1;
	
	lvm->tlsf = tlsf_create_with_pool(lvm->area, MEM_SIZE);
	if(!lvm->tlsf)
		return -1;
	lvm->pool = tlsf_get_pool(lvm->tlsf);
	
	lvm->L = lua_newstate(lua_alloc, lvm);
	if(!lvm->L)
		return -1;

	luaopen_base(lvm->L);
	luaopen_coroutine(lvm->L);
	luaopen_table(lvm->L);
	//luaopen_io(lvm->L);
	//luaopen_os(lvm->L);
	luaopen_string(lvm->L);
	luaopen_utf8(lvm->L);
	//luaopen_bit32(lvm->L);
	luaopen_math(lvm->L);
	//luaopen_debug(lvm->L);
	//luaopen_package(lvm->L);
	lua_pop(lvm->L, 6);

	lua_gc(lvm->L, LUA_GCSTOP, 0); // disable automatic garbage collection

	return 0;
}

int
lua_vm_deinit(Lua_VM *lvm)
{
	if(lvm->L)
		lua_close(lvm->L);

	tlsf_remove_pool(lvm->tlsf, lvm->pool);
	tlsf_destroy(lvm->tlsf);

#if !defined(_WIN32)
	munlock(lvm->area, MEM_SIZE);
#endif
	free(lvm->area);

	return 0;
}

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &lv2_lua_control;
		case 1:
			return &lv2_lua_atom;
		default:
			return NULL;
	}
}
