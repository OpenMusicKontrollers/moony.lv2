/*
 * Copyright (c) 2016-2017 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the voiceied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef _LV2_XPRESS_H_
#define _LV2_XPRESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>
#ifndef _WIN32
#	include <sys/mman.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

/*****************************************************************************
 * API START
 *****************************************************************************/

#define XPRESS_SHM_ID				"/lv2_xpress_shm"
#define XPRESS_URI	  			"http://open-music-kontrollers.ch/lv2/xpress"
#define XPRESS_PREFIX				XPRESS_URI"#"

// Features
#define XPRESS__voiceMap		XPRESS_PREFIX"voiceMap"

// Message types
#define XPRESS__Token				XPRESS_PREFIX"Token"
#define XPRESS__Alive				XPRESS_PREFIX"Alive"

// Properties
#define XPRESS__source			XPRESS_PREFIX"source"
#define XPRESS__uuid				XPRESS_PREFIX"uuid"
#define XPRESS__zone				XPRESS_PREFIX"zone"
#define XPRESS__body   			XPRESS_PREFIX"body"
#define XPRESS__pitch				XPRESS_PREFIX"pitch"
#define XPRESS__pressure		XPRESS_PREFIX"pressure"
#define XPRESS__timbre			XPRESS_PREFIX"timbre"
#define XPRESS__dPitch			XPRESS_PREFIX"dPitch"
#define XPRESS__dPressure		XPRESS_PREFIX"dPressure"
#define XPRESS__dTimbre			XPRESS_PREFIX"dTimbre"

// types
typedef uint32_t xpress_uuid_t;

// structures
typedef struct _xpress_map_t xpress_map_t;
typedef struct _xpress_shm_t xpress_shm_t;
typedef struct _xpress_state_t xpress_state_t;
typedef struct _xpress_voice_t xpress_voice_t;
typedef struct _xpress_iface_t xpress_iface_t;
typedef struct _xpress_t xpress_t;

// function callbacks
typedef xpress_uuid_t (*xpress_map_new_uuid_t)(void *handle, uint32_t flag);

typedef void (*xpress_add_cb_t)(void *data, int64_t frames,
	const xpress_state_t *state, xpress_uuid_t uuid, void *target);

typedef xpress_add_cb_t xpress_set_cb_t;

typedef void (*xpress_del_cb_t)(void *data, int64_t frames,
	xpress_uuid_t uuid, void *target);

typedef enum _xpress_event_t {
	XPRESS_EVENT_ADD					= (1 << 0),
	XPRESS_EVENT_DEL					= (1 << 1),
	XPRESS_EVENT_SET					= (1 << 2)
} xpress_event_t;

#define XPRESS_EVENT_NONE		(0)
#define XPRESS_EVENT_ALL		(XPRESS_EVENT_ADD | XPRESS_EVENT_DEL | XPRESS_EVENT_SET)

struct _xpress_state_t {
	int32_t zone;

	float pitch;
	float pressure;
	float timbre;

	float dPitch;
	float dPressure;
	float dTimbre;
};

struct _xpress_iface_t {
	size_t size;

	xpress_add_cb_t add;
	xpress_set_cb_t set;
	xpress_del_cb_t del;
};

struct _xpress_voice_t {
	LV2_URID source;
	xpress_uuid_t uuid;
	bool alive;
	void *target;
};

struct _xpress_map_t {
	void *handle;
	xpress_map_new_uuid_t new_uuid;
};

struct _xpress_shm_t {
	atomic_uint voice_uuid;
};

struct _xpress_t {
	struct {
		LV2_URID xpress_Token;
		LV2_URID xpress_Alive;

		LV2_URID xpress_source;
		LV2_URID xpress_uuid;
		LV2_URID xpress_zone;
		LV2_URID xpress_body;

		LV2_URID xpress_pitch;
		LV2_URID xpress_pressure;
		LV2_URID xpress_timbre;

		LV2_URID xpress_dPitch;
		LV2_URID xpress_dPressure;
		LV2_URID xpress_dTimbre;
	} urid;

	LV2_URID_Map *map;
	xpress_map_t *voice_map;
	xpress_shm_t *xpress_shm;
	atomic_uint voice_uuid;
	bool synced;
	xpress_uuid_t source;

	xpress_event_t event_mask;
	const xpress_iface_t *iface;
	void *data;

	unsigned max_nvoices;
	unsigned nvoices;
	xpress_voice_t voices [1];
};

#define XPRESS_CONCAT_IMPL(X, Y) X##Y
#define XPRESS_CONCAT(X, Y) XPRESS_CONCAT_IMPL(X, Y)

#define XPRESS_T(XPRESS, MAX_NVOICES) \
	xpress_t (XPRESS); \
	xpress_voice_t XPRESS_CONCAT(_voices, __COUNTER__) [(MAX_NVOICES - 1)]

#define XPRESS_VOICE_FOREACH(XPRESS, VOICE) \
	for(xpress_voice_t *(VOICE) = &(XPRESS)->voices[(int)(XPRESS)->nvoices - 1]; \
		(VOICE) >= (XPRESS)->voices; \
		(VOICE)--)

#define XPRESS_VOICE_FREE(XPRESS, VOICE) \
	for(xpress_voice_t *(VOICE) = &(XPRESS)->voices[(int)(XPRESS)->nvoices - 1]; \
		(VOICE) >= (XPRESS)->voices; \
		(VOICE)->uuid = 0, (VOICE)--, (XPRESS)->nvoices--)

// non rt-safe
static inline int
xpress_init(xpress_t *xpress, const size_t max_nvoices, LV2_URID_Map *map,
	xpress_map_t *voice_map, xpress_event_t event_mask, const xpress_iface_t *iface,
	void *target, void *data);

static void
xpress_deinit(xpress_t *xpress);

// rt-safe
static inline void *
xpress_get(xpress_t *xpress, xpress_uuid_t uuid);

// rt-safe
static inline int
xpress_advance(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	const LV2_Atom_Object *obj, LV2_Atom_Forge_Ref *ref);

// rt-safe
static inline void
xpress_pre(xpress_t *xpress);

// rt-safe
static inline void
xpress_rst(xpress_t *xpress);

// rt-safe
static inline bool
xpress_synced(xpress_t *xpress);

// rt-safe
static inline void
xpress_post(xpress_t *xpress, int64_t frames);

// rt-safe
static inline void *
xpress_add(xpress_t *xpress, xpress_uuid_t uuid);

// rt-safe
static inline void *
xpress_create(xpress_t *xpress, xpress_uuid_t *uuid);

// rt-safe
static inline int
xpress_free(xpress_t *xpress, xpress_uuid_t uuid);

// rt-safe
static inline LV2_Atom_Forge_Ref
xpress_token(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	xpress_uuid_t uuid, const xpress_state_t *state);

// rt-safe
static inline LV2_Atom_Forge_Ref
xpress_alive(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames);

// rt-safe
static inline int32_t
xpress_map(xpress_t *xpress);

/*****************************************************************************
 * API END
 *****************************************************************************/

static const xpress_state_t xpress_vanilla;

//tools.ietf.org/html/rfc4122 version 4
static LV2_URID
_xpress_urn_uuid(LV2_URID_Map *map)
{
	uint8_t bytes [0x10];

	srand(time(NULL));

	for(unsigned i=0x0; i<0x10; i++)
		bytes[i] = rand() & 0xff;

	bytes[6] = (bytes[6] & 0x0f) | 0x40; // set four most significant bits of 7th byte to 0b0100
	bytes[8] = (bytes[8] & 0x3f) | 0x80; // set two most significant bits of 9th byte to 0b10

	char uuid [46];
	snprintf(uuid, sizeof(uuid),
		"urn:uuid:%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		bytes[0x0], bytes[0x1], bytes[0x2], bytes[0x3],
		bytes[0x4], bytes[0x5],
		bytes[0x6], bytes[0x7],
		bytes[0x8], bytes[0x9],
		bytes[0xa], bytes[0xb], bytes[0xc], bytes[0xd], bytes[0xe], bytes[0xf]);

	return map->map(map->handle, uuid);
}

static inline void
_xpress_qsort(xpress_voice_t *A, int n)
{
	if(n < 2)
		return;

	const xpress_voice_t *p = A;

	int i = -1;
	int j = n;

	while(true)
	{
		do {
			i += 1;
		} while(A[i].uuid > p->uuid);

		do {
			j -= 1;
		} while(A[j].uuid < p->uuid);

		if(i >= j)
			break;

		const xpress_voice_t tmp = A[i];
		A[i] = A[j];
		A[j] = tmp;
	}

	_xpress_qsort(A, j + 1);
	_xpress_qsort(A + j + 1, n - j - 1);
}

static inline xpress_voice_t *
_xpress_bsearch(xpress_uuid_t p, xpress_voice_t *a, int n)
{
	xpress_voice_t *base = a;

	for(int N = n, half; N > 1; N -= half)
	{
		half = N/2;
		xpress_voice_t *dst = &base[half];
		base = (dst->uuid < p) ? base : dst;
	}

	return (base->uuid == p) ? base : NULL;
}

static inline void
_xpress_sort(xpress_t *xpress)
{
	_xpress_qsort(xpress->voices, xpress->nvoices);
}

static inline xpress_voice_t *
_xpress_voice_get(xpress_t *xpress, xpress_uuid_t uuid)
{
	return _xpress_bsearch(uuid, xpress->voices, xpress->nvoices);
}

static inline void *
_xpress_voice_add(xpress_t *xpress, LV2_URID source, xpress_uuid_t uuid, bool alive)
{
	if(xpress->nvoices >= xpress->max_nvoices)
		return NULL; // failed

	xpress_voice_t *voice = &xpress->voices[xpress->nvoices++];
	voice->source = source;
	voice->uuid = uuid;
	voice->alive = alive;
	void *target = voice->target;

	_xpress_sort(xpress);

	return target;
}

static inline void
_xpress_voice_free(xpress_t *xpress, xpress_voice_t *voice)
{
	voice->uuid = 0;

	_xpress_sort(xpress);

	xpress->nvoices--;
}

xpress_shm_t *
_xpress_shm_init()
{
	xpress_shm_t *xpress_shm = NULL;
#ifndef _WIN32
	const size_t total_size = sizeof(xpress_shm_t);

	bool is_first = true;
	int fd = shm_open(XPRESS_SHM_ID, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

	if(fd == -1)
	{
		is_first = false;
		fd = shm_open(XPRESS_SHM_ID, O_RDWR, S_IRUSR | S_IWUSR);
	}

	if(fd == -1)
	{
		return NULL;
	}

	if(  (ftruncate(fd, total_size) == -1)
		|| ((xpress_shm = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, 0)) == MAP_FAILED) )
	{
		shm_unlink(XPRESS_SHM_ID);
		close(fd);
		return NULL;
	}

	close(fd);

	if(is_first)
	{
		atomic_init(&xpress_shm->voice_uuid, 1);
	}
#endif

	return xpress_shm;
}

void
_xpress_shm_deinit(xpress_shm_t *xpress_shm)
{
#ifndef _WIN32
	const size_t total_size = sizeof(xpress_shm_t);

	munmap(xpress_shm, total_size);
	shm_unlink(XPRESS_SHM_ID);
#else
	(void)xpress_shm;
#endif
}

static inline int
xpress_init(xpress_t *xpress, const size_t max_nvoices, LV2_URID_Map *map,
	xpress_map_t *voice_map, xpress_event_t event_mask, const xpress_iface_t *iface,
	void *target, void *data)
{
	if(!map || ( (event_mask != XPRESS_EVENT_NONE) && !iface))
		return 0;

	xpress->nvoices = 0;
	xpress->max_nvoices = max_nvoices;
	xpress->map = map;
	xpress->voice_map = voice_map;
	xpress->event_mask = event_mask;
	xpress->iface = iface;
	xpress->data = data;
	
	xpress->urid.xpress_Token = map->map(map->handle, XPRESS__Token);
	xpress->urid.xpress_Alive = map->map(map->handle, XPRESS__Alive);

	xpress->urid.xpress_source = map->map(map->handle, XPRESS__source);
	xpress->urid.xpress_uuid = map->map(map->handle, XPRESS__uuid);
	xpress->urid.xpress_zone = map->map(map->handle, XPRESS__zone);

	xpress->urid.xpress_body = map->map(map->handle, XPRESS__body);

	xpress->urid.xpress_pitch = map->map(map->handle, XPRESS__pitch);
	xpress->urid.xpress_pressure = map->map(map->handle, XPRESS__pressure);
	xpress->urid.xpress_timbre = map->map(map->handle, XPRESS__timbre);

	xpress->urid.xpress_dPitch = map->map(map->handle, XPRESS__dPitch);
	xpress->urid.xpress_dPressure = map->map(map->handle, XPRESS__dPressure);
	xpress->urid.xpress_dTimbre = map->map(map->handle, XPRESS__dTimbre);

	for(unsigned i = 0; i < xpress->max_nvoices; i++)
	{
		xpress_voice_t *voice = &xpress->voices[i];

		voice->uuid = 0;
		voice->target = target && iface
			? (uint8_t *)target + i*iface->size
			: NULL;
	}

	xpress->source = _xpress_urn_uuid(map);

	if(!xpress->voice_map)
	{
		// fall-back to shared memory
		xpress->xpress_shm = _xpress_shm_init();

		if(!xpress->xpress_shm)
		{
			// fall-back to local memory
			const uint32_t pseudo_unique =  (const uintptr_t)xpress;
			atomic_init(&xpress->voice_uuid, pseudo_unique);
		}
	}

	return 1;
}

static void
xpress_deinit(xpress_t *xpress)
{
	if(xpress->xpress_shm)
	{
		_xpress_shm_deinit(xpress->xpress_shm);
	}
}

static inline void *
xpress_get(xpress_t *xpress, xpress_uuid_t uuid)
{
	xpress_voice_t *voice = _xpress_voice_get(xpress, uuid);
	if(voice)
		return voice->target;

	return NULL;
}

static inline int
xpress_advance(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	const LV2_Atom_Object *obj, LV2_Atom_Forge_Ref *ref __attribute__((unused)))
{
	if(!lv2_atom_forge_is_object_type(forge, obj->atom.type))
		return 0;

	if(obj->body.otype == xpress->urid.xpress_Token)
	{
		const LV2_Atom_URID *source = NULL;
		const LV2_Atom_Int *uuid = NULL;
		const LV2_Atom_Int *zone = NULL;
		const LV2_Atom_Float *pitch = NULL;
		const LV2_Atom_Float *pressure = NULL;
		const LV2_Atom_Float *timbre = NULL;
		const LV2_Atom_Float *dPitch = NULL;
		const LV2_Atom_Float *dPressure = NULL;
		const LV2_Atom_Float *dTimbre = NULL;

		lv2_atom_object_get(obj,
			xpress->urid.xpress_source, &source,
			xpress->urid.xpress_uuid, &uuid,
			xpress->urid.xpress_zone, &zone,
			xpress->urid.xpress_pitch, &pitch,
			xpress->urid.xpress_pressure, &pressure,
			xpress->urid.xpress_timbre, &timbre,
			xpress->urid.xpress_dPitch, &dPitch,
			xpress->urid.xpress_dPressure, &dPressure,
			xpress->urid.xpress_dTimbre, &dTimbre,
			0);

		if(  !source || (source->atom.type != forge->URID)
			|| !uuid || (uuid->atom.type != forge->Int) )
		{
			return 0;
		}

		bool added;
		xpress_voice_t *voice = _xpress_voice_get(xpress, uuid->body);
		void *target;
		if(voice)
		{
			target = voice->target;

			added = false;
		}
		else
		{
			if(!(target = _xpress_voice_add(xpress, source->body, uuid->body, false)))
				return 0;

			added = true;
		}

		xpress_state_t state = xpress_vanilla;
		if(zone && (zone->atom.type == forge->Int))
			state.zone = zone->body;
		if(pitch && (pitch->atom.type == forge->Float))
			state.pitch = pitch->body;
		if(pressure && (pressure->atom.type == forge->Float))
			state.pressure = pressure->body;
		if(timbre && (timbre->atom.type == forge->Float))
			state.timbre = timbre->body;
		if(dPitch && (dPitch->atom.type == forge->Float))
			state.dPitch = dPitch->body;
		if(dPressure && (dPressure->atom.type == forge->Float))
			state.dPressure = dPressure->body;
		if(dTimbre && (dTimbre->atom.type == forge->Float))
			state.dTimbre = dTimbre->body;

		if(added)
		{
			if( (xpress->event_mask & XPRESS_EVENT_ADD) && xpress->iface->add)
				xpress->iface->add(xpress->data, frames, &state, uuid->body, target);
		}
		else
		{
			if( (xpress->event_mask & XPRESS_EVENT_SET) && xpress->iface->set)
				xpress->iface->set(xpress->data, frames, &state, uuid->body, target);
		}

		return 1;
	}
	else if(obj->body.otype == xpress->urid.xpress_Alive)
	{
		const LV2_Atom_URID *source = NULL;
		const LV2_Atom_Tuple *body = NULL;

		lv2_atom_object_get(obj,
			xpress->urid.xpress_source, &source,
			xpress->urid.xpress_body, &body,
			0);

		if(  !source || (source->atom.type != forge->URID) )
			return 0;

		if(body && (body->atom.type == forge->Tuple) ) // non-existent body is a valid empty body
		{
			LV2_ATOM_TUPLE_FOREACH(body, item)
			{
				const LV2_Atom_Int *uuid = (const LV2_Atom_Int *)item;

				if(uuid->atom.type == forge->Int)
				{
					xpress_voice_t *voice = _xpress_voice_get(xpress, uuid->body);
					if(voice)
					{
						voice->alive = true;
					}
					else	
					{
						void *target = _xpress_voice_add(xpress, source->body, uuid->body, true);
						if(target)
						{
							xpress_state_t state = xpress_vanilla;
							if( (xpress->event_mask & XPRESS_EVENT_ADD) && xpress->iface->add)
								xpress->iface->add(xpress->data, frames, &state, uuid->body, target);
						}
					}
				}
			}
		}

		unsigned freed = 0;

		XPRESS_VOICE_FOREACH(xpress, voice)
		{
			if(  (voice->source == source->body)
				&& !voice->alive )
			{
				if( (xpress->event_mask & XPRESS_EVENT_DEL) && xpress->iface->del)
					xpress->iface->del(xpress->data, frames, voice->uuid, voice->target);

				freed += 1;
				voice->uuid = 0; // invalidate
			}
		}

		if(freed > 0)
		{
			_xpress_sort(xpress);
			xpress->nvoices -= freed;
		}

		return 1;
	}

	return 0; // did not handle a patch event
}

static inline void
xpress_pre(xpress_t *xpress)
{
	XPRESS_VOICE_FOREACH(xpress, voice)
	{
		voice->alive = false;
	}
}

static inline void
xpress_rst(xpress_t *xpress)
{
	xpress->synced = false; // e.g. needs an xpress#alive
}

static inline bool
xpress_synced(xpress_t *xpress)
{
	return xpress->synced;
}

static inline void
xpress_post(xpress_t *xpress, int64_t frames)
{
	unsigned freed = 0;

	XPRESS_VOICE_FOREACH(xpress, voice)
	{
		if(!voice->alive)
		{
			if( (xpress->event_mask & XPRESS_EVENT_DEL) && xpress->iface->del)
				xpress->iface->del(xpress->data, frames, voice->uuid, voice->target);

			freed += 1;
			voice->uuid = 0; // invalidate
		}
	}

	if(freed > 0)
	{
		_xpress_sort(xpress);
		xpress->nvoices -= freed;
	}
}

static inline void *
xpress_add(xpress_t *xpress, xpress_uuid_t uuid)
{
	return _xpress_voice_add(xpress, xpress->source, uuid, false);
}

static inline void *
xpress_create(xpress_t *xpress, xpress_uuid_t *uuid)
{
	*uuid = xpress_map(xpress);

	return xpress_add(xpress, *uuid);
}

static inline int
xpress_free(xpress_t *xpress, xpress_uuid_t uuid)
{
	xpress_voice_t *voice = _xpress_voice_get(xpress, uuid);
	if(!voice)
		return 0; // failed

	_xpress_voice_free(xpress, voice);

	return 1;
}

static inline LV2_Atom_Forge_Ref
xpress_token(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	xpress_uuid_t uuid, const xpress_state_t *state)
{
	LV2_Atom_Forge_Frame obj_frame;

	LV2_Atom_Forge_Ref ref = lv2_atom_forge_frame_time(forge, frames);

	if(ref)
		ref = lv2_atom_forge_object(forge, &obj_frame, 0, xpress->urid.xpress_Token);
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_source);
		if(ref)
			ref = lv2_atom_forge_urid(forge, xpress->source);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_uuid);
		if(ref)
			ref = lv2_atom_forge_int(forge, uuid);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_zone);
		if(ref)
			ref = lv2_atom_forge_int(forge, state->zone);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_pitch);
		if(ref)
			ref = lv2_atom_forge_float(forge, state->pitch);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_pressure);
		if(ref)
			ref = lv2_atom_forge_float(forge, state->pressure);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_timbre);
		if(ref)
			ref = lv2_atom_forge_float(forge, state->timbre);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_dPitch);
		if(ref)
			ref = lv2_atom_forge_float(forge, state->dPitch);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_dPressure);
		if(ref)
			ref = lv2_atom_forge_float(forge, state->dPressure);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_dTimbre);
		if(ref)
			ref = lv2_atom_forge_float(forge, state->dTimbre);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &obj_frame);

	xpress->synced = false; // e.g. needs an xpress#alive

	return ref;
}

static inline LV2_Atom_Forge_Ref
xpress_alive(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames)
{
	LV2_Atom_Forge_Frame obj_frame;
	LV2_Atom_Forge_Frame tup_frame;

	LV2_Atom_Forge_Ref ref = lv2_atom_forge_frame_time(forge, frames);

	if(ref)
		ref = lv2_atom_forge_object(forge, &obj_frame, 0, xpress->urid.xpress_Alive);
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_source);
		if(ref)
			ref = lv2_atom_forge_urid(forge, xpress->source);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_body);
		if(ref)
			ref = lv2_atom_forge_tuple(forge, &tup_frame);
		{
			XPRESS_VOICE_FOREACH(xpress, voice)
			{
				if(ref)
					ref = lv2_atom_forge_int(forge, voice->uuid);
			}
		}
		if(ref)
			lv2_atom_forge_pop(forge, &tup_frame);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &obj_frame);

	xpress->synced = true;

	return ref;
}

static inline int32_t
xpress_map(xpress_t *xpress)
{
	if(xpress->voice_map)
	{
		xpress_map_t *voice_map = xpress->voice_map;

		return voice_map->new_uuid(voice_map->handle, 0);
	}
	else if(xpress->xpress_shm)
	{
		xpress_shm_t *xpress_shm = xpress->xpress_shm;

		return atomic_fetch_add_explicit(&xpress_shm->voice_uuid, 1, memory_order_relaxed);
	}

	// fall-back
	return atomic_fetch_add_explicit(&xpress->voice_uuid, 1, memory_order_relaxed);
}

#ifdef __cplusplus
}
#endif

#endif // _LV2_XPRESS_H_
