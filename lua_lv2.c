/*
 * Copyright (c) 2014 Hanspeter Portner (dev@open-music-kontrollers.ch)
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 */

#include <lua_lv2.h>

#include <sys/mman.h>

#define MEM_SIZE 0x1000000UL // 16MB

extern const LV2_Descriptor lv2_lua_control;
extern const LV2_Descriptor lv2_lua_midi;
extern const LV2_Descriptor lv2_lua_osc;

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
			return &lv2_lua_midi;
		case 2:
			return &lv2_lua_osc;
		default:
			return NULL;
	}
}
