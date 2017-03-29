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

typedef enum _moony_job_enum_t moony_job_enum_t;
typedef struct _moony_vm_t moony_vm_t;
typedef struct _moony_job_t moony_job_t;

struct _moony_vm_t {
	tlsf_t tlsf;

	size_t size [MOONY_POOL_NUM];
	void *area [MOONY_POOL_NUM];
	pool_t pool [MOONY_POOL_NUM];

	size_t space;
	size_t used;

	lua_State *L;

	bool locked;
};

enum _moony_job_enum_t {
	MOONY_JOB_MEM_ALLOC,
	MOONY_JOB_MEM_FREE,
	MOONY_JOB_VM_ALLOC,
	MOONY_JOB_VM_FREE,
	MOONY_JOB_STATE_FREE
};

struct _moony_job_t {
	moony_job_enum_t type;

	union {
		struct {
			int32_t i;
			void *ptr;
		} mem;
		moony_vm_t *vm;
		LV2_Atom *state_atom;
		char chunk [0];
	};
};

moony_vm_t *moony_vm_new(size_t mem_size, bool testing);
void moony_vm_free(moony_vm_t *vm);

void *moony_vm_mem_alloc(size_t size);
void moony_vm_mem_free(void *area, size_t size);
int moony_vm_mem_extend(moony_vm_t *vm);

void moony_vm_lock(moony_vm_t *vm);
void moony_vm_unlock(moony_vm_t *vm);

#endif
