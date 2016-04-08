/*
 * Copyright (c) 2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/patch/patch.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>

/*****************************************************************************
 * API START
 *****************************************************************************/

#define XPRESS_PREFIX				"http://open-music-kontrollers.ch/lv2/xpress#"

// Features
#define XPRESS_VOICE_MAP		XPRESS_PREFIX"voiceMap"

// Message types
#define XPRESS_PUT					XPRESS_PREFIX"Put"
#define XPRESS_DELETE				XPRESS_PREFIX"Delete"
#define XPRESS_CLEAR				XPRESS_PREFIX"Clear"

// Properties
#define XPRESS_UUID					XPRESS_PREFIX"uuid"
#define XPRESS_ZONE					XPRESS_PREFIX"zone"
#define XPRESS_POSITION			XPRESS_PREFIX"position"
#define XPRESS_VELOCITY			XPRESS_PREFIX"velocity"
#define XPRESS_ACCELERATION	XPRESS_PREFIX"acceleration"

// types
typedef int64_t xpress_uuid_t;

// enumerations
typedef enum _xpress_event_t xpress_event_t;

// structures
typedef struct _xpress_map_t xpress_map_t;
typedef struct _xpress_state_t xpress_state_t;
typedef struct _xpress_voice_t xpress_voice_t;
typedef struct _xpress_iface_t xpress_iface_t;
typedef struct _xpress_t xpress_t;

// function callbacks
typedef xpress_uuid_t (*xpress_map_new_uuid_t)(void *handle);

typedef void (*xpress_state_cb_t)(void *data,
	int64_t frames, const xpress_state_t *state,
	xpress_uuid_t uuid, void *target);

enum _xpress_event_t {
	XPRESS_EVENT_ADD					= (1 << 0),
	XPRESS_EVENT_DEL					= (1 << 1),
	XPRESS_EVENT_PUT					= (1 << 2)
};

#define XPRESS_EVENT_NONE		(0)
#define XPRESS_EVENT_ALL		(XPRESS_EVENT_ADD | XPRESS_EVENT_DEL | XPRESS_EVENT_PUT)

#define XPRESS_MAX_NDIMS 3

struct _xpress_state_t {
	int32_t zone;

	float position [XPRESS_MAX_NDIMS];
	float velocity [XPRESS_MAX_NDIMS];
	float acceleration [XPRESS_MAX_NDIMS];
};

struct _xpress_iface_t {
	size_t size;

	xpress_state_cb_t add;
	xpress_state_cb_t put;
	xpress_state_cb_t del;
};

struct _xpress_voice_t {
	xpress_uuid_t uuid;
	void *target;
};

struct _xpress_map_t {
	void *handle;
	xpress_map_new_uuid_t new_uuid;
};

struct _xpress_t {
	struct {

		LV2_URID atom_int;
		LV2_URID atom_long;
		LV2_URID atom_float;
		LV2_URID atom_vector;

		LV2_URID xpress_Put;
		LV2_URID xpress_Delete;
		LV2_URID xpress_Clear;

		LV2_URID xpress_uuid;
		LV2_URID xpress_zone;
		LV2_URID xpress_position;
		LV2_URID xpress_velocity;
		LV2_URID xpress_acceleration;
	} urid;

	LV2_URID_Map *map;
	xpress_map_t *voice_map;

	xpress_event_t event_mask;
	const xpress_iface_t *iface;
	void *data;

	unsigned max_nvoices;
	unsigned nvoices;
	xpress_voice_t voices [0];
};

#define XPRESS_CONCAT_IMPL(X, Y) X##Y
#define XPRESS_CONCAT(X, Y) XPRESS_CONCAT_IMPL(X, Y)

#define XPRESS_T(XPRESS, MAX_NVOICES) \
	xpress_t (XPRESS); \
	xpress_voice_t XPRESS_CONCAT(_voices, __COUNTER__) [(MAX_NVOICES)];

#define XPRESS_VOICE_FOREACH(XPRESS, VOICE) \
	for(xpress_voice_t *VOICE = &(XPRESS)->voices[(int)(XPRESS)->nvoices - 1]; \
		VOICE >= (XPRESS)->voices; \
		VOICE--)

#define XPRESS_VOICE_FREE(XPRESS, VOICE) \
	for(xpress_voice_t *VOICE = &(XPRESS)->voices[(int)(XPRESS)->nvoices - 1]; \
		VOICE >= (XPRESS)->voices; \
		VOICE->uuid = 0, VOICE--, (XPRESS)->nvoices--)

// non rt-safe
static inline int
xpress_init(xpress_t *xpress, const size_t max_nvoices, LV2_URID_Map *map,
	xpress_map_t *voice_map, xpress_event_t event_mask, const xpress_iface_t *iface,
	void *target, void *data);

// rt-safe
static inline void *
xpress_get(xpress_t *xpress, xpress_uuid_t uuid);

// rt-safe
static inline int
xpress_advance(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	const LV2_Atom_Object *obj, LV2_Atom_Forge_Ref *ref);

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
xpress_del(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	xpress_uuid_t uuid);

// rt-safe
static inline LV2_Atom_Forge_Ref
xpress_put(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	xpress_uuid_t uuid, const xpress_state_t *state);

// rt-safe
static inline int32_t
xpress_map(xpress_t *xpress);

/*****************************************************************************
 * API END
 *****************************************************************************/

static inline void
_xpress_float_vec(xpress_t *xpress, float *dst, const LV2_Atom_Vector *vec)
{
	const float *src = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector, &vec->atom);
	unsigned nelements = (vec->atom.size - sizeof(LV2_Atom_Vector_Body)) / sizeof(float);

	// only copy as many floats as present on either side
	if(nelements > XPRESS_MAX_NDIMS)
		nelements = XPRESS_MAX_NDIMS;

	memcpy(dst, src, nelements * sizeof(float));
}

static inline void
_xpress_qsort(xpress_voice_t *a, unsigned n)
{
	if(n < 2)
		return;
	
	const xpress_voice_t *p = &a[n/2];

	unsigned i, j;
	for(i=0, j=n-1; ; i++, j--)
	{
		while(a[i].uuid > p->uuid)
			i++;

		while(p->uuid > a[j].uuid)
			j--;

		if(i >= j)
			break;

		const xpress_voice_t t = a[i];
		a[i] = a[j];
		a[j] = t;
	}

	_xpress_qsort(a, i);
	_xpress_qsort(&a[i], n - i);
}

static inline xpress_voice_t *
_xpress_bsearch(xpress_uuid_t p, xpress_voice_t *a, unsigned n)
{
	unsigned start = 0;
	unsigned end = n;

	while(start < end)
	{
		const unsigned mid = start + (end - start)/2;
		xpress_voice_t *dst = &a[mid];

		if(p > dst->uuid)
			end = mid;
		else if(p < dst->uuid)
			start = mid + 1;
		else
			return dst;
	}

	return NULL;
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
_xpress_voice_add(xpress_t *xpress, xpress_uuid_t uuid)
{
	if(xpress->nvoices >= xpress->max_nvoices)
		return NULL; // failed

	xpress_voice_t *voice = &xpress->voices[xpress->nvoices++];
	voice->uuid = uuid;
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
	
	xpress->urid.atom_int = map->map(map->handle, LV2_ATOM__Int);
	xpress->urid.atom_long = map->map(map->handle, LV2_ATOM__Long);
	xpress->urid.atom_float = map->map(map->handle, LV2_ATOM__Float);
	xpress->urid.atom_vector = map->map(map->handle, LV2_ATOM__Vector);

	xpress->urid.xpress_Put = map->map(map->handle, XPRESS_PUT);
	xpress->urid.xpress_Delete = map->map(map->handle, XPRESS_DELETE);
	xpress->urid.xpress_Clear = map->map(map->handle, XPRESS_CLEAR);

	xpress->urid.xpress_uuid = map->map(map->handle, XPRESS_UUID);
	xpress->urid.xpress_zone = map->map(map->handle, XPRESS_ZONE);
	xpress->urid.xpress_position = map->map(map->handle, XPRESS_POSITION);
	xpress->urid.xpress_velocity = map->map(map->handle, XPRESS_VELOCITY);
	xpress->urid.xpress_acceleration = map->map(map->handle, XPRESS_ACCELERATION);

	for(unsigned i = 0; i < xpress->max_nvoices; i++)
	{
		xpress_voice_t *voice = &xpress->voices[i];

		voice->uuid = 0;
		voice->target = target && iface
			? target + i*iface->size
			: NULL;
	}

	return 1;
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
	const LV2_Atom_Object *obj, LV2_Atom_Forge_Ref *ref)
{
	if(!lv2_atom_forge_is_object_type(forge, obj->atom.type))
		return 0;

	if(obj->body.otype == xpress->urid.xpress_Put)
	{
		const LV2_Atom_Long *uuid = NULL;
		const LV2_Atom_Int *zone = NULL;
		const LV2_Atom_Vector *pos = NULL;
		const LV2_Atom_Vector *vel = NULL;
		const LV2_Atom_Vector *acc = NULL;

		LV2_Atom_Object_Query q [] = {
			{ xpress->urid.xpress_uuid, (const LV2_Atom **)&uuid },
			{ xpress->urid.xpress_zone, (const LV2_Atom **)&zone },
			{ xpress->urid.xpress_position, (const LV2_Atom **)&pos },
			{ xpress->urid.xpress_velocity, (const LV2_Atom **)&vel },
			{ xpress->urid.xpress_acceleration, (const LV2_Atom **)&acc },
			{ 0, NULL}
		};
		lv2_atom_object_query(obj, q);

		if(  !uuid || (uuid->atom.type != xpress->urid.atom_long)
			|| !zone || (zone->atom.type != xpress->urid.atom_int)
			|| !pos || (pos->atom.type != xpress->urid.atom_vector)
				|| (pos->body.child_type != xpress->urid.atom_float)
				|| (pos->body.child_size != sizeof(float))
			|| !vel || (vel->atom.type != xpress->urid.atom_vector)
				|| (vel->body.child_type != xpress->urid.atom_float)
				|| (vel->body.child_size != sizeof(float))
			|| !acc || (acc->atom.type != xpress->urid.atom_vector)
				|| (acc->body.child_type != xpress->urid.atom_float)
				|| (acc->body.child_size != sizeof(float)) )
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
			if(!(target = _xpress_voice_add(xpress, uuid->body)))
				return 0;

			added = true;
		}

		xpress_state_t state;
		memset(&state, 0x0, sizeof(xpress_state_t));
		state.zone = zone->body;
		_xpress_float_vec(xpress, state.position, pos);
		_xpress_float_vec(xpress, state.velocity, vel);
		_xpress_float_vec(xpress, state.acceleration, acc);

		if(added)
		{
			if( (xpress->event_mask & XPRESS_EVENT_ADD) && xpress->iface->add)
				xpress->iface->add(xpress->data, frames, &state, uuid->body, target);
		}
		else
		{
			if( (xpress->event_mask & XPRESS_EVENT_PUT) && xpress->iface->put)
				xpress->iface->put(xpress->data, frames, &state, uuid->body, target);
		}

		return 1;
	}
	else if(obj->body.otype == xpress->urid.xpress_Delete)
	{
		const LV2_Atom_Long *uuid = NULL;

		LV2_Atom_Object_Query q [] = {
			{ xpress->urid.xpress_uuid, (const LV2_Atom **)&uuid },
			{ 0, NULL}
		};
		lv2_atom_object_query(obj, q);

		if(!uuid || (uuid->atom.type != xpress->urid.atom_long))
			return 0;

		xpress_voice_t *voice = _xpress_voice_get(xpress, uuid->body);
		if(!voice)
			return 0;

		const LV2_URID voice_uuid = voice->uuid;
		void *voice_target = voice->target;
	
		_xpress_voice_free(xpress, voice);

		if( (xpress->event_mask & XPRESS_EVENT_DEL) && xpress->iface->del)
			xpress->iface->del(xpress->data, frames, NULL, voice_uuid, voice_target);

		return 1;
	}
	else if(obj->body.otype == xpress->urid.xpress_Clear)
	{
		XPRESS_VOICE_FREE(xpress, voice)
		{
			const LV2_URID voice_uuid = voice->uuid;
			void *voice_target = voice->target;

			if( (xpress->event_mask & XPRESS_EVENT_DEL) && xpress->iface->del)
				xpress->iface->del(xpress->data, frames, NULL, voice_uuid, voice_target);
		}

		return 1;
	}

	return 0; // did not handle a patch event
}

static inline void *
xpress_add(xpress_t *xpress, xpress_uuid_t uuid)
{
	return _xpress_voice_add(xpress, uuid);
}

static inline void *
xpress_create(xpress_t *xpress, xpress_uuid_t *uuid)
{
	*uuid = xpress->voice_map->new_uuid(xpress->voice_map->handle);

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
xpress_del(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	xpress_uuid_t uuid)
{
	LV2_Atom_Forge_Frame obj_frame;

	LV2_Atom_Forge_Ref ref = lv2_atom_forge_frame_time(forge, frames);

	if(ref)
		ref = lv2_atom_forge_object(forge, &obj_frame, 0, xpress->urid.xpress_Delete);
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_uuid);
		if(ref)
			ref = lv2_atom_forge_long(forge, uuid);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &obj_frame);

	return ref;
}

static inline LV2_Atom_Forge_Ref
xpress_put(xpress_t *xpress, LV2_Atom_Forge *forge, uint32_t frames,
	xpress_uuid_t uuid, const xpress_state_t *state)
{
	LV2_Atom_Forge_Frame obj_frame;

	LV2_Atom_Forge_Ref ref = lv2_atom_forge_frame_time(forge, frames);

	if(ref)
		ref = lv2_atom_forge_object(forge, &obj_frame, 0, xpress->urid.xpress_Put);
	{
		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_uuid);
		if(ref)
			ref = lv2_atom_forge_long(forge, uuid);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_zone);
		if(ref)
			ref = lv2_atom_forge_int(forge, state->zone);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_position);
		if(ref)
			ref = lv2_atom_forge_vector(forge, sizeof(float), xpress->urid.atom_float,
				XPRESS_MAX_NDIMS, state->position);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_velocity);
		if(ref)
			ref = lv2_atom_forge_vector(forge, sizeof(float), xpress->urid.atom_float,
				XPRESS_MAX_NDIMS, state->velocity);

		if(ref)
			ref = lv2_atom_forge_key(forge, xpress->urid.xpress_acceleration);
		if(ref)
			ref = lv2_atom_forge_vector(forge, sizeof(float), xpress->urid.atom_float,
				XPRESS_MAX_NDIMS, state->acceleration);
	}
	if(ref)
		lv2_atom_forge_pop(forge, &obj_frame);

	return ref;
}

static inline int32_t
xpress_map(xpress_t *xpress)
{
	return xpress->voice_map->new_uuid(xpress->voice_map->handle);
}

#ifdef __cplusplus
}
#endif

#endif // _LV2_XPRESS_H_
