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

#define MEM_SIZE 0x1000000UL // 16MB

static inline void *
rt_alloc(Lua_VM *lvm, size_t len)
{
	void *data = NULL;
	
	data = tlsf_malloc(lvm->tlsf, len);
	if(!data)
		; //FIXME

	return data;
}

static inline void *
rt_realloc(Lua_VM *lvm, size_t len, void *buf)
{
	void *data = NULL;
	
	data =tlsf_realloc(lvm->tlsf, buf, len);
	if(!data)
		; //FIXME

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

	if(nsize == 0) {
		if(ptr)
			rt_free(lvm, ptr);
		return NULL;
	}
	else {
		if(ptr)
			return rt_realloc(lvm, nsize, ptr);
		else
			return rt_alloc(lvm, nsize);
	}
}

int
lua_vm_init(Lua_VM *lvm)
{
	lvm->area = mmap(NULL, MEM_SIZE, PROT_READ|PROT_WRITE,
									MAP_32BIT|MAP_PRIVATE|MAP_ANONYMOUS|MAP_LOCKED, -1, 0);
	if(!lvm->area)
		return -1;
	
	lvm->tlsf = tlsf_create_with_pool(lvm->area, MEM_SIZE);
	if(!lvm->tlsf)
		return -1;
	lvm->pool = tlsf_get_pool(lvm->tlsf);
	
	lvm->L = lua_newstate(lua_alloc, lvm);
	if(!lvm->L)
		return -1;
	luaL_openlibs(lvm->L);

	lua_gc(lvm->L, LUA_GCSTOP, 0); // disable automatic garbage collection

	return 0;
}

int
lua_vm_deinit(Lua_VM *lvm)
{
	if(lvm->chunk)
		free(lvm->chunk);
	
	if(lvm->L)
		lua_close(lvm->L);

	tlsf_remove_pool(lvm->tlsf, lvm->pool);
	tlsf_destroy(lvm->tlsf);
	munmap(lvm->area, MEM_SIZE);

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
