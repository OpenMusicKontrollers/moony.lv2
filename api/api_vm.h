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

#ifndef _MOONY_API_VM_H
#define _MOONY_API_VM_H

#include <tlsf.h>
#include <lua.h>

// from vm.c
#define MOONY_POOL_NUM 8

typedef struct _moony_vm_t moony_vm_t;
typedef struct _moony_mem_t moony_mem_t;

struct _moony_vm_t {
	tlsf_t tlsf;

	size_t size [MOONY_POOL_NUM];
	void *area [MOONY_POOL_NUM]; // 128K, 256K, 512K, 1M, 2M, 4M, 8M, 16M
	pool_t pool [MOONY_POOL_NUM];

	size_t space;
	size_t used;

	lua_State *L;
};

struct _moony_mem_t {
	int i;
	void *mem;
};

int moony_vm_init(moony_vm_t *vm);
int moony_vm_deinit(moony_vm_t *vm);
void *moony_vm_mem_alloc(size_t size);
void moony_vm_mem_free(void *area, size_t size);
int moony_vm_mem_extend(moony_vm_t *vm);

#endif
