/*
 * Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _SANDBOX_IO_H
#define _SANDBOX_IO_H

#include <sratom/sratom.h>

#include <nanomsg/nn.h>
#include <nanomsg/pair.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RDF_PREFIX "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

#undef LV2_ATOM_TUPLE_FOREACH // there is a bug in LV2 1.10.0
#define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))

typedef struct _atom_ser_t atom_ser_t;
typedef struct _sandbox_io_subscription_t sandbox_io_subscription_t;
typedef struct _sandbox_io_t sandbox_io_t;

typedef void (*_sandbox_io_recv_cb_t)(void *data, uint32_t index, uint32_t size,
	uint32_t format, const void *buf);
typedef void (*_sandbox_io_subscribe_cb_t)(void *data, uint32_t index,
	uint32_t protocol, bool state);

struct _atom_ser_t {
	uint32_t size;
	uint8_t *buf;
	uint32_t offset;
};

struct _sandbox_io_subscription_t {
	uint32_t protocol;
	int32_t state;
};

struct _sandbox_io_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;

	int sock;
	int id;

	Sratom *sratom;
	atom_ser_t ser;
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Forge forge;

	const char *base_uri;
	SerdNode subject;
	SerdNode predicate;

	LV2_URID float_protocol;
	LV2_URID peak_protocol;
	LV2_URID event_transfer;
	LV2_URID atom_transfer;
	LV2_URID core_index;
	LV2_URID rdf_value;
	LV2_URID ui_protocol;
	LV2_URID ui_period_start;
	LV2_URID ui_period_size;
	LV2_URID ui_peak;
	LV2_URID ui_window_title;
	LV2_URID ui_port_subscribe;
	LV2_URID params_sample_rate;
};

static inline LV2_Atom_Forge_Ref
_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
{
	atom_ser_t *ser = handle;

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;

	const uint32_t new_offset = ser->offset + size;
	if(new_offset > ser->size)
	{
		uint32_t new_size = ser->size << 1;
		while(new_offset > new_size)
			new_size <<= 1;

		if(!(ser->buf = realloc(ser->buf, new_size)))
			return 0; // realloc failed

		ser->size = new_size;
	}

	memcpy(ser->buf + ser->offset, buf, size);
	ser->offset = new_offset;

	return ref;
}

static inline LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

static inline void
_sandbox_io_reset(sandbox_io_t *io)
{
	atom_ser_t *ser = &io->ser;
	ser->offset = 0; //TODO free?

	lv2_atom_forge_set_sink(&io->forge, _sink, _deref, ser);
	lv2_atom_forge_tuple(&io->forge, &io->frame);
}

static inline void
_sandbox_io_recv(sandbox_io_t *io, _sandbox_io_recv_cb_t recv_cb,
	_sandbox_io_subscribe_cb_t subscribe_cb, void *data)
{
	char *ttl = NULL;
	int res;

	while((res = nn_recv(io->sock, &ttl, NN_MSG, NN_DONTWAIT)) != -1)
	{
		//printf("%s\n\n", ttl);
		LV2_Atom *atom = sratom_from_turtle(io->sratom, io->base_uri,
			&io->subject, &io->predicate, ttl);
		if(atom)
		{
			LV2_ATOM_TUPLE_FOREACH((LV2_Atom_Tuple *)atom, itm)
			{
				LV2_Atom_Object *obj = (LV2_Atom_Object *)itm;

				if(!lv2_atom_forge_is_object_type(&io->forge, obj->atom.type))
					continue;

				if(obj->body.otype == io->float_protocol)
				{
					const LV2_Atom_Int *index = NULL;
					const LV2_Atom_Float *value = NULL;
					LV2_Atom_Object_Query q [] = {
						{io->core_index, (const LV2_Atom **)&index},
						{io->rdf_value, (const LV2_Atom **)&value},
						{0, NULL}
					};
					lv2_atom_object_query(obj, q);

					if(  index && (index->atom.type == io->forge.Int)
						&& value && (value->atom.type == io->forge.Float)
							&& (value->atom.size == sizeof(float)) )
					{
						recv_cb(data, index->body,
							sizeof(float), io->float_protocol, &value->body);
						recv_cb(data, index->body,
							sizeof(float), 0, &value->body);
					}
				}
				else if(obj->body.otype == io->peak_protocol)
				{
					const LV2_Atom_Int *index = NULL;
					const LV2_Atom_Int *period_start = NULL;
					const LV2_Atom_Int *period_size = NULL;
					const LV2_Atom_Float *peak= NULL;
					LV2_Atom_Object_Query q [] = {
						{io->core_index, (const LV2_Atom **)&index},
						{io->ui_period_start, (const LV2_Atom **)&period_start},
						{io->ui_period_size, (const LV2_Atom **)&period_size},
						{io->ui_peak, (const LV2_Atom **)&peak},
						{0, NULL}
					};
					lv2_atom_object_query(obj, q);

					if(  index && (index->atom.type == io->forge.Int)
						&& period_start && (period_start->atom.type == io->forge.Int)
							&& (period_start->atom.size == sizeof(int32_t))
						&& period_size && (period_size->atom.type == io->forge.Int)
							&& (period_size->atom.size == sizeof(int32_t))
						&& peak && (peak->atom.type == io->forge.Float)
							&& (peak->atom.size == sizeof(float)) )
						{
							const LV2UI_Peak_Data peak_data = {
								.period_start = period_start->body,
								.period_size = period_size->body,
								.peak = peak->body
							};
							recv_cb(data, index->body,
								sizeof(LV2UI_Peak_Data), io->peak_protocol, &peak_data);
						}
				}
				else if(obj->body.otype == io->event_transfer)
				{
					const LV2_Atom_Int *index = NULL;
					const LV2_Atom *value = NULL;
					LV2_Atom_Object_Query q [] = {
						{io->core_index, (const LV2_Atom **)&index},
						{io->rdf_value, (const LV2_Atom **)&value},
						{0, NULL}
					};
					lv2_atom_object_query(obj, q);

					if(  index && (index->atom.type == io->forge.Int)
						&& value)
					{
						recv_cb(data, index->body,
							lv2_atom_total_size(value), io->event_transfer, value);
					}
				}
				else if(obj->body.otype == io->atom_transfer)
				{
					const LV2_Atom_Int *index = NULL;
					const LV2_Atom *value = NULL;
					LV2_Atom_Object_Query q [] = {
						{io->core_index, (const LV2_Atom **)&index},
						{io->rdf_value, (const LV2_Atom **)&value},
						{0, NULL}
					};
					lv2_atom_object_query(obj, q);

					if(  index && (index->atom.type == io->forge.Int)
						&& value)
					{
						recv_cb(data, index->body,
							lv2_atom_total_size(value), io->atom_transfer, value);
					}
				}
				else if(obj->body.otype == io->ui_port_subscribe)
				{
					const LV2_Atom_Int *index = NULL;
					const LV2_Atom_URID *protocol = NULL;
					const LV2_Atom_Bool *value = NULL;
					LV2_Atom_Object_Query q [] = {
						{io->core_index, (const LV2_Atom **)&index},
						{io->ui_protocol, (const LV2_Atom **)&protocol},
						{io->rdf_value, (const LV2_Atom **)&value},
						{0, NULL}
					};
					lv2_atom_object_query(obj, q);

					if(  index && (index->atom.type == io->forge.Int)
						&& value && (value->atom.type == io->forge.Bool)
						&& protocol && (protocol->atom.type == io->forge.URID))
					{
						if(subscribe_cb)
							subscribe_cb(data, index->body, protocol->body, value->body);
					}
				}
			}

			free(atom);
		}

		nn_freemsg(ttl);
	}

	switch(nn_errno())
	{
		case EAGAIN:
			// do nothing
			break;
		case ETERM:
			//FIXME done
			// fall-through
		default:
			fprintf(stderr, "nn_recv: %s\n", nn_strerror(nn_errno()));
			break;
	}
}

static inline bool
_sandbox_io_flush(sandbox_io_t *io)
{
	const LV2_Atom *atom = (const LV2_Atom *)io->ser.buf;

	bool more = false;
	if( (io->ser.offset == 0) || (atom->size == 0) )
		return more; // empty tuple

	char *ttl = sratom_to_turtle(io->sratom, io->unmap,
		io->base_uri, &io->subject, &io->predicate,
		atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));

	if(ttl)
	{
		const size_t len = strlen(ttl) + 1;
		//printf("sending: %zu\n\n%s\n\n", len, ttl);

		if(nn_send(io->sock, ttl, len, NN_DONTWAIT) == -1)
		{
			switch(nn_errno())
			{
				case EAGAIN:
					more = true;
					break;
				case ETERM:
					//FIXME done
					// fall-through
				default:
					fprintf(stderr, "nn_send: %s\n", nn_strerror(nn_errno()));
					break;
			}
		}
		else
		{
			_sandbox_io_reset(io);
		}

		free(ttl);
	}

	return more;
}

static inline void
_sandbox_io_clean(LV2_Atom_Forge *forge, LV2_Atom *atom)
{
	if(atom->type == forge->Object)
	{
		LV2_Atom_Object *obj = (LV2_Atom_Object *)atom;

		if(obj->body.id != 0)
			obj->body.id = 0; // if not, sratom will fail

		LV2_ATOM_OBJECT_FOREACH(obj, prop)
		{
			_sandbox_io_clean(forge, &prop->value);
		}
	}
	else if(atom->type == forge->Tuple)
	{
		LV2_Atom_Tuple *tup = (LV2_Atom_Tuple *)atom;

		LV2_ATOM_TUPLE_FOREACH(tup, itm)
		{
			_sandbox_io_clean(forge, itm);
		}
	}
	else if(atom->type == forge->Sequence)
	{
		LV2_Atom_Sequence *seq = (LV2_Atom_Sequence *)atom;

		LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
		{
			_sandbox_io_clean(forge, &ev->body);
		}
	}
}

static inline bool
_sandbox_io_send(sandbox_io_t *io, uint32_t index,
	uint32_t size, uint32_t protocol, const void *buf)
{
	LV2_Atom_Forge_Frame frame;

	if(protocol == 0)
		protocol = io->float_protocol;

	lv2_atom_forge_object(&io->forge, &frame, 0, protocol);

	lv2_atom_forge_key(&io->forge, io->core_index);
	lv2_atom_forge_int(&io->forge, index);

	if(protocol == io->float_protocol)
	{
		const float *value = buf;

		lv2_atom_forge_key(&io->forge, io->rdf_value);
		lv2_atom_forge_float(&io->forge, *value);
	}
	else if(protocol == io->peak_protocol)
	{
		const LV2UI_Peak_Data *peak_data = buf;

		lv2_atom_forge_key(&io->forge, io->ui_period_start);
		lv2_atom_forge_int(&io->forge, peak_data->period_start);

		lv2_atom_forge_key(&io->forge, io->ui_period_size);
		lv2_atom_forge_int(&io->forge, peak_data->period_size);

		lv2_atom_forge_key(&io->forge, io->ui_peak);
		lv2_atom_forge_float(&io->forge, peak_data->peak);
	}
	else if(protocol == io->event_transfer)
	{
		const LV2_Atom *atom = buf;
		LV2_Atom_Forge_Ref ref;

		lv2_atom_forge_key(&io->forge, io->rdf_value);
		ref = lv2_atom_forge_atom(&io->forge, atom->size, atom->type);
		lv2_atom_forge_write(&io->forge, LV2_ATOM_BODY_CONST(atom), atom->size);

		LV2_Atom *src= lv2_atom_forge_deref(&io->forge, ref);
		_sandbox_io_clean(&io->forge, src);
	}
	else if(protocol == io->atom_transfer)
	{
		const LV2_Atom *atom = buf;
		LV2_Atom_Forge_Ref ref;

		lv2_atom_forge_key(&io->forge, io->rdf_value);
		ref = lv2_atom_forge_atom(&io->forge, atom->size, atom->type);
		lv2_atom_forge_write(&io->forge, LV2_ATOM_BODY_CONST(atom), atom->size);

		LV2_Atom *src= lv2_atom_forge_deref(&io->forge, ref);
		_sandbox_io_clean(&io->forge, src);
	}
	else if(protocol == io->ui_port_subscribe)
	{
		const sandbox_io_subscription_t *sub = buf;

		lv2_atom_forge_key(&io->forge, io->rdf_value);
		lv2_atom_forge_bool(&io->forge, sub->state);

		lv2_atom_forge_key(&io->forge, io->ui_protocol);
		lv2_atom_forge_urid(&io->forge, protocol);
	}

	lv2_atom_forge_pop(&io->forge, &frame);

	//lv2_atom_forge_pop(&io->forge, &io->frame);
	
	return _sandbox_io_flush(io);
}

static inline int
_sandbox_io_fd_get(sandbox_io_t *io)
{
	int fd = -1;
	size_t sz = sizeof(int);

	if(io->sock == -1)
		return -1;

	if(nn_getsockopt(io->sock, NN_SOL_SOCKET, NN_RCVFD, &fd, &sz) < 0)
		return -1;

	//printf("_sandbox_io_fd_get: %i\n", fd);

	return fd;
}

static inline int
_sandbox_io_init(sandbox_io_t *io, LV2_URID_Map *map, LV2_URID_Unmap *unmap,
	const char *socket_path, bool is_master)
{
	io->base_uri = "file:///tmp/base";
	io->subject = serd_node_from_string(SERD_URI, (const uint8_t *)(""));
	io->predicate = serd_node_from_string(SERD_URI, (const uint8_t *)(LV2_ATOM__atomTransfer));

	io->map = map;
	io->unmap = unmap;

	io->ser.offset = 0;
	io->ser.size = 1024;
	io->ser.buf = malloc(io->ser.size);
	if(!io->ser.buf)
		return -1;

	if(!(io->sratom = sratom_new(map)))
		return -1;
	sratom_set_pretty_numbers(io->sratom, false);
	sratom_set_object_mode(io->sratom, SRATOM_OBJECT_MODE_BLANK);

	if((io->sock = nn_socket(AF_SP, NN_PAIR)) == -1)
		return -1;

	const int ms = 10000;
	if(nn_setsockopt(io->sock, NN_SOL_SOCKET, NN_LINGER, &ms, sizeof(ms)) == -1)
		return -1;

	if(is_master)
	{
		if((io->id = nn_bind(io->sock, socket_path)) == -1)
			return -1;
	}
	else // is_slave
	{
		if((io->id = nn_connect(io->sock, socket_path)) == -1)
			return -1;
	}

	lv2_atom_forge_init(&io->forge, map);

	io->float_protocol = map->map(map->handle, LV2_UI_PREFIX"floatProtocol");
	io->peak_protocol = map->map(map->handle, LV2_UI_PREFIX"peakProtocol");
	io->event_transfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	io->atom_transfer = map->map(map->handle, LV2_ATOM__atomTransfer);
	io->core_index = map->map(map->handle, LV2_CORE__index);
	io->rdf_value = map->map(map->handle, RDF_PREFIX"value");
	io->ui_protocol = map->map(map->handle, LV2_UI_PREFIX"protocol");
	io->ui_period_start = map->map(map->handle, LV2_UI_PREFIX"periodStart");
	io->ui_period_size = map->map(map->handle, LV2_UI_PREFIX"periodSize");
	io->ui_peak = map->map(map->handle, LV2_UI_PREFIX"peak");
	io->ui_window_title = map->map(map->handle, LV2_UI__windowTitle);
	io->ui_port_subscribe = map->map(map->handle, LV2_UI__portSubscribe);
	io->params_sample_rate = map->map(map->handle, LV2_PARAMETERS__sampleRate);

	_sandbox_io_reset(io);

	return 0;
}

static inline void
_sandbox_io_deinit(sandbox_io_t *io)
{
	if(io->id != -1)
	{
		int res = nn_shutdown(io->sock, io->id);
		if(res < 0)
			fprintf(stderr, "nn_shutdown failed: %s\n", nn_strerror(nn_errno()));
	}

	if(io->sock != -1)
	{
		int res = nn_close(io->sock);
		if(res < 0)
			fprintf(stderr, "nn_close failed: %s\n", nn_strerror(nn_errno()));
	}

	if(io->sratom)
		sratom_free(io->sratom);

	if(io->ser.buf)
		free(io->ser.buf);
}

#ifdef __cplusplus
}
#endif

#endif
