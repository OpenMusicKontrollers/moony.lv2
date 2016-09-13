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

#ifndef _JSATOM_H
#define _JSATOM_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include <cJSON/cJSON.h>
#include <base64.h>

#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

#define JSATOM_MAX_TYPES 1
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"

#define JSON_LD_TYPE     "@type"
#define JSON_LD_ID       "@id"
#define JSON_LD_VALUE    "@value"
#define JSON_LD_LIST     "@list"
#define JSON_LD_LANGUAGE "@language"

typedef struct _jsatom_t jsatom_t;
typedef struct _jsatom_encode_item_t jsatom_encode_item_t;
typedef cJSON *(*jsatom_encode_t)(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body);

struct _jsatom_encode_item_t {
	LV2_URID type;
	jsatom_encode_t encode;
};

struct _jsatom_t {
	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;
	LV2_URID midi_MidiEvent;
	jsatom_encode_item_t encoders [JSATOM_MAX_TYPES];
};

static inline cJSON *
_jsatom_encode_raw(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body);

static inline cJSON *
jsatom_encode(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body);

static inline LV2_Atom_Forge_Ref
jsatom_decode(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node);

static inline cJSON *
_jsatom_encode_types(const char *uri, ...)
{
	cJSON *arr = cJSON_CreateArray();
  va_list args;
  va_start(args, uri);

	for( ; uri; uri = va_arg(args, const char *))
		cJSON_AddItemToArray(arr, cJSON_CreateString(uri));

	va_end(args);

	return arr;
}

static inline cJSON *
_jsatom_encode_value_string(const char *body)
{
	cJSON *arr = cJSON_CreateArray();
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateString(body));
	cJSON_AddItemToArray(arr, obj);
	return arr;
}

static inline cJSON *
_jsatom_encode_nil(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, cJSON_CreateNull());
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateNull());
	return obj;
}

static inline cJSON *
_jsatom_encode_int(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Int, NULL));
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateNumber(*(const int32_t *)body));
	return obj;
}

static inline cJSON *
_jsatom_encode_long(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Long, NULL));
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateNumber(*(const int64_t *)body));
	return obj;
}

static inline cJSON *
_jsatom_encode_float(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Float, NULL));
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateNumber(*(const float*)body));
	return obj;
}

static inline cJSON *
_jsatom_encode_double(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Double, NULL));
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateNumber(*(const double *)body));
	return obj;
}

static inline cJSON *
_jsatom_encode_bool(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Bool, NULL));
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateBool(*(const int32_t *)body ? 1 : 0));
	return obj;
}

static inline cJSON *
_jsatom_encode_uri(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	// @type is not needed
	// @value is not needed
	cJSON_AddItemToObject(obj, JSON_LD_ID, cJSON_CreateString(body));
	return obj;
}

static inline cJSON *
_jsatom_encode_urid(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	const char *uri = jsatom->unmap->unmap(jsatom->unmap->handle, *(const LV2_URID *)body);
	return _jsatom_encode_uri(jsatom, strlen(uri)+1, type, uri);
}

static inline cJSON *
_jsatom_encode_string(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__String, NULL));
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateString(body));
	return obj;
}

static inline cJSON *
_jsatom_encode_path(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Path, NULL));
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateString(body));
	return obj;
}

static inline cJSON *
_jsatom_encode_literal(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	const LV2_Atom_Literal_Body *lit_body = body;
	cJSON *obj = cJSON_CreateObject();
	if(lit_body->datatype)
	{
		const char *datatype = jsatom->unmap->unmap(jsatom->unmap->handle, lit_body->datatype);
		cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Literal, datatype, NULL));
	}
	else
	{
		cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Literal, NULL));
	}
	if(lit_body->lang)
	{
		const char *lang = jsatom->unmap->unmap(jsatom->unmap->handle, lit_body->lang);
		cJSON_AddItemToObject(obj, JSON_LD_LANGUAGE, cJSON_CreateString(lang));
	}
	cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateString(LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal_Body, lit_body)));
	return obj;
}

static inline cJSON *
_jsatom_encode_vector(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	const LV2_Atom_Vector_Body *vec_body = body;
	const void *vec = LV2_ATOM_CONTENTS_CONST(LV2_Atom_Vector_Body, vec_body);
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Vector, NULL));
	cJSON *list = cJSON_CreateArray();
	const unsigned nelements = (size - sizeof(LV2_Atom_Vector_Body)) / vec_body->child_size;
	for(unsigned i = 0; i < nelements; i++)
	{
		cJSON_AddItemToArray(list, _jsatom_encode_raw(jsatom, vec_body->child_size, vec_body->child_type,
			vec + i*vec_body->child_size));
	}
	cJSON_AddItemToObject(obj, JSON_LD_LIST, list);
	return obj;
}

static inline cJSON *
_jsatom_encode_tuple(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Tuple, NULL));
	cJSON *list = cJSON_CreateArray();
	LV2_ATOM_TUPLE_BODY_FOREACH(body, size, atom)
	{
		cJSON_AddItemToArray(list, _jsatom_encode_raw(jsatom, atom->size, atom->type, LV2_ATOM_BODY_CONST(atom)));
	}
	cJSON_AddItemToObject(obj, JSON_LD_LIST, list);
	return obj;
}

static inline cJSON *
_jsatom_encode_object(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	const LV2_Atom_Object_Body *obj_body = body;
	cJSON *obj= cJSON_CreateObject();
	if(obj_body->id)
	{
		const char *id = jsatom->unmap->unmap(jsatom->unmap->handle, obj_body->id);
		cJSON_AddItemToObject(obj, JSON_LD_ID, cJSON_CreateString(id));
	}
	if(obj_body->otype)
	{
		const char *otype = jsatom->unmap->unmap(jsatom->unmap->handle, obj_body->otype);
		cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Object, otype, NULL));
	}
	else
	{
		cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_ATOM__Object, NULL));
	}
	LV2_ATOM_OBJECT_BODY_FOREACH(body, size, prop)
	{
		const char *key = jsatom->unmap->unmap(jsatom->unmap->handle, prop->key);
		const LV2_Atom *atom = &prop->value;
		cJSON_AddItemToObject(obj, key, jsatom_encode(jsatom, atom->size, atom->type, LV2_ATOM_BODY_CONST(atom)));
	}
	return obj;
}

static inline cJSON *
_jsatom_encode_base64(jsatom_t *jsatom, uint32_t size, const char *uri, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	//cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(uri, NS_XSD"base64Binary", NULL));
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(uri, NULL));
	char *base64 = base64_encode(body, size);
	if(base64)
	{
		cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateString(base64));
		free(base64);
	}
	return obj;
}

static inline cJSON *
_jsatom_encode_chunk(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	return _jsatom_encode_base64(jsatom, size, LV2_ATOM__Chunk, body);
}

static inline cJSON *
_jsatom_encode_fallback(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	const char *uri = jsatom->unmap->unmap(jsatom->unmap->handle, type);
	return uri ? _jsatom_encode_base64(jsatom, size, uri, body) : NULL;
}

static inline cJSON *
_jsatom_encode_midi(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *obj = cJSON_CreateObject();
	//cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_MIDI__MidiEvent, NS_XSD"hexBinary", NULL));
	cJSON_AddItemToObject(obj, JSON_LD_TYPE, _jsatom_encode_types(LV2_MIDI__MidiEvent, NULL));
	char *hex = malloc(size*2+1);
	if(hex)
	{
		const uint8_t *m = body;
		for(unsigned i = 0; i < size; i++)
			sprintf(&hex[i*2], "%02"PRIx8, m[i]);
		cJSON_AddItemToObject(obj, JSON_LD_VALUE, cJSON_CreateString(hex));
		free(hex);
	}
	return obj;
}

static inline void
jsatom_init(jsatom_t *jsatom, LV2_URID_Map *map, LV2_URID_Unmap *unmap)
{
	jsatom->map = map;
	jsatom->unmap = unmap;
	lv2_atom_forge_init(&jsatom->forge, map);
	jsatom->midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
}

static inline cJSON *
_jsatom_encode_raw(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	jsatom_encode_t encode;

	if(type == 0)
		encode = _jsatom_encode_nil;
	else if(type == jsatom->forge.Int)
		encode = _jsatom_encode_int;
	else if(type == jsatom->forge.Long)
		encode = _jsatom_encode_long;
	else if(type == jsatom->forge.Float)
		encode = _jsatom_encode_float;
	else if(type == jsatom->forge.Double)
		encode = _jsatom_encode_double;
	else if(type == jsatom->forge.Bool)
		encode = _jsatom_encode_bool;
	else if(type == jsatom->forge.URID)
		encode = _jsatom_encode_urid;
	else if(type == jsatom->forge.URI)
		encode = _jsatom_encode_uri;
	else if(type == jsatom->forge.String)
		encode = _jsatom_encode_string;
	else if(type == jsatom->forge.Path)
		encode = _jsatom_encode_path;
	else if(type == jsatom->forge.Literal)
		encode = _jsatom_encode_literal;
	else if(type == jsatom->forge.Vector)
		encode = _jsatom_encode_vector;
	else if(type == jsatom->forge.Tuple)
		encode = _jsatom_encode_tuple;
	else if(type == jsatom->forge.Object)
		encode = _jsatom_encode_object;
	else if(type == jsatom->forge.Chunk)
		encode = _jsatom_encode_chunk;
	else if(type == jsatom->midi_MidiEvent)
		encode = _jsatom_encode_midi;
	else
		encode = _jsatom_encode_fallback;

	return encode(jsatom, size, type, body);
}

static inline cJSON *
jsatom_encode(jsatom_t *jsatom, uint32_t size, LV2_URID type, const void *body)
{
	cJSON *arr = cJSON_CreateArray();
	cJSON *obj = _jsatom_encode_raw(jsatom, size, type, body);
	cJSON_AddItemToArray(arr, obj);
	return arr;
}

static inline cJSON *
_jsatom_object_key(cJSON *obj, const char *key)
{
	if( obj && (obj->type == cJSON_Object) )
		return cJSON_GetObjectItem(obj, key);

	return NULL;
}

static inline cJSON *
_jsatom_node_key(cJSON *node, const char *key)
{
	if( node && (node->type == cJSON_Array) && (cJSON_GetArraySize(node) == 1))
		return _jsatom_object_key(node->child, key);
	else
		return _jsatom_object_key(node, key);

	return NULL;
}

static inline bool
_jsatom_type_is_a(cJSON *type, const char *uri)
{
	if( type && (type->type == cJSON_String) && !strcmp(type->valuestring, uri) )
		return true;

	return false;
}

static inline bool
_jsatom_types_is_a(cJSON *types, const char *uri)
{
	if(types)
	{
		if(types->type == cJSON_Array)	
		{
			for(cJSON *type = types->child; type; type = type->next)
			{
				if(_jsatom_type_is_a(type, uri))
					return true;
			}
		}
		else if(_jsatom_type_is_a(types, uri))
			return true;
	}

	return false;
}

static inline cJSON *
_jsatom_types_is_not_a(cJSON *types, const char *uri)
{
	if( types && (types->type == cJSON_Array) )	
	{
		for(cJSON *type = types->child; type; type = type->next)
		{
			if(!_jsatom_type_is_a(type, uri))
				return type; // return first non-matching type
		}
	}

	return false;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_int(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_Number) )
		return lv2_atom_forge_int(forge, value->valueint);

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_long(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_Number) )
		return lv2_atom_forge_long(forge, value->valueint);

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_float(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_Number) )
		return lv2_atom_forge_float(forge, value->valuedouble);

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_double(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_Number) )
		return lv2_atom_forge_double(forge, value->valuedouble);

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_bool(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if(value)
	{
		if(value->type == cJSON_True)
			return lv2_atom_forge_bool(forge, 1);
		else if(value->type == cJSON_False)
			return lv2_atom_forge_bool(forge, 0);
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_string(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_String) )
		return lv2_atom_forge_string(forge, value->valuestring, strlen(value->valuestring));

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_path(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_String) )
		return lv2_atom_forge_path(forge, value->valuestring, strlen(value->valuestring));

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_literal(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	cJSON *lang= _jsatom_node_key(node, JSON_LD_LANGUAGE);
	cJSON *datatype = _jsatom_types_is_not_a(types, LV2_ATOM__Literal);
	const LV2_URID datatype_u = datatype && (datatype->type == cJSON_String)
		? jsatom->map->map(jsatom->map->handle, datatype->valuestring)
		: 0;
	const LV2_URID lang_u = lang && (lang->type == cJSON_String)
		? jsatom->map->map(jsatom->map->handle, lang->valuestring)
		: 0;
	if( value && (value->type == cJSON_String) )
		return lv2_atom_forge_literal(forge, value->valuestring, strlen(value->valuestring), datatype_u, lang_u);

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_tuple(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_LIST);
	if( value && (value->type == cJSON_Array) )
	{
		LV2_Atom_Forge_Frame frame;
		LV2_Atom_Forge_Ref ref = lv2_atom_forge_tuple(forge, &frame);
		for(cJSON *child = value->child; ref && child; child = child->next)
			ref = jsatom_decode(jsatom, forge, child);
		if(ref)
		{
			lv2_atom_forge_pop(forge, &frame);
			return ref;
		}
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_object(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *id = _jsatom_node_key(node, JSON_LD_ID);
	cJSON *otype = _jsatom_types_is_not_a(types, LV2_ATOM__Object);
	const LV2_URID id_u = id && (id->type == cJSON_String)
		? jsatom->map->map(jsatom->map->handle, id->valuestring)
		: 0;
	const LV2_URID otype_u = otype && (otype->type == cJSON_String)
		? jsatom->map->map(jsatom->map->handle, otype->valuestring)
		: 0;
	LV2_Atom_Forge_Frame frame;
	cJSON *value = node->type == cJSON_Array
		? node->child
		: node;
	if( value && (value->type == cJSON_Object) )
	{
		LV2_Atom_Forge_Ref ref = lv2_atom_forge_object(forge, &frame, id_u, otype_u);
		for(cJSON *child = value->child; ref && child; child = child->next)
		{
			if( !strcmp(child->string, JSON_LD_ID) || !strcmp(child->string, JSON_LD_TYPE) )
				continue;
			const LV2_URID key_u = child->string
				? jsatom->map->map(jsatom->map->handle, child->string)
				: 0;
			if(key_u)
			{
				ref = lv2_atom_forge_key(forge, key_u);
				if(ref)
					ref = jsatom_decode(jsatom, forge, child);
			}
		}
		if(ref)
		{
			lv2_atom_forge_pop(forge, &frame);
			return ref;
		}
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_vector(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_LIST);
	if( value && (value->type == cJSON_Array) )
	{
		LV2_Atom_Forge_Frame frame;
		cJSON *child_types = _jsatom_object_key(value->child, JSON_LD_TYPE);
		uint32_t child_size = 0;
		LV2_URID child_type = 0;
		if(_jsatom_types_is_a(child_types, LV2_ATOM__Int))
		{
			child_size = sizeof(int32_t);
			child_type = forge->Int;
		}
		else if(_jsatom_types_is_a(child_types, LV2_ATOM__Long))
		{
			child_size = sizeof(int64_t);
			child_type = forge->Long;
		}
		else if(_jsatom_types_is_a(child_types, LV2_ATOM__Float))
		{
			child_size = sizeof(float);
			child_type = forge->Float;
		}
		else if(_jsatom_types_is_a(child_types, LV2_ATOM__Double))
		{
			child_size = sizeof(double);
			child_type = forge->Double;
		}
		else if(_jsatom_types_is_a(child_types, LV2_ATOM__Bool))
		{
			child_size = sizeof(int32_t);
			child_type = forge->Bool;
		}
		if(child_size && child_type)
		{
			LV2_Atom_Forge_Ref ref = lv2_atom_forge_vector_head(forge, &frame, child_size, child_type);
			unsigned nelements = 0;
			for(cJSON *child = value->child; ref && child; child = child->next, nelements++)
			{
				cJSON *child_value = _jsatom_object_key(child, JSON_LD_VALUE);
				if(!child_value || (child_value->type != cJSON_Number) )
					continue;
				union {
					int32_t i;
					int64_t l;
					float f;
					double d;
				} raw = {.l = 0};
				if(child_value->type == cJSON_Number)
				{
					if(child_type == forge->Int)
						raw.i = child_value->valueint;
					else if(child_type == forge->Long)
						raw.l = child_value->valueint;
					else if(child_type == forge->Float)
						raw.f = child_value->valuedouble;
					else if(child_type == forge->Double)
						raw.d = child_value->valuedouble;
				}
				else if( (child_value->type == cJSON_True) && (child_type == forge->Bool) )
					raw.i = 1;
				else if( (child_value->type == cJSON_False) && (child_type == forge->Bool) )
					raw.i = 0;
				if(ref)
					ref = lv2_atom_forge_raw(forge, &raw, child_size);
			}
			if(ref)
			{
				lv2_atom_forge_pad(forge, nelements * child_size);
				lv2_atom_forge_pop(forge, &frame);
				return ref;
			}
		}
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_chunk(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_String) )
	{
		size_t size;
		uint8_t *body = base64_decode(value->valuestring, strlen(value->valuestring), &size);
		if(body)
		{
			LV2_Atom_Forge_Ref ref = lv2_atom_forge_atom(forge, size, forge->Chunk);
			if(ref)
				ref = lv2_atom_forge_write(forge, body, size);
			free(body);
			return ref;
		}
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_midi(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_String) )
	{
		const uint32_t size = strlen(value->valuestring) / 2;
		LV2_Atom_Forge_Ref ref = lv2_atom_forge_atom(forge, size, jsatom->midi_MidiEvent);
		for(unsigned i=0; i<size; i++)
		{
			uint8_t byte;
			if(ref && (sscanf(&value->valuestring[i*2], "%"SCNx8, &byte) == 1) )
				ref = lv2_atom_forge_raw(forge, &byte, 1);
		}
		if(ref)
		{
			lv2_atom_forge_pad(forge, size);
			return ref;
		}
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_fallback(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *value = _jsatom_node_key(node, JSON_LD_VALUE);
	if( value && (value->type == cJSON_String) )
	{
		const LV2_URID type_u = jsatom->map->map(jsatom->map->handle, types->child->valuestring);
		size_t size;
		uint8_t *body = base64_decode(value->valuestring, strlen(value->valuestring), &size);
		if(body)
		{
			LV2_Atom_Forge_Ref ref = lv2_atom_forge_atom(forge, size, type_u);
			if(ref)
				lv2_atom_forge_write(forge, body, size);
			free(body);
			return ref;
		}
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
_jsatom_decode_urid(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node, cJSON *types)
{
	cJSON *id = _jsatom_node_key(node, JSON_LD_ID);
	if( id && (id->type == cJSON_String) )
	{
		const LV2_URID id_u = jsatom->map->map(jsatom->map->handle, id->valuestring);
		return lv2_atom_forge_urid(forge, id_u);
	}

	return 0;
}

static inline LV2_Atom_Forge_Ref
jsatom_decode(jsatom_t *jsatom, LV2_Atom_Forge *forge, cJSON *node)
{
	cJSON *types = _jsatom_node_key(node, JSON_LD_TYPE);

	if(!types)
		return _jsatom_decode_urid(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Int))
		return _jsatom_decode_int(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Long))
		return _jsatom_decode_long(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Float))
		return _jsatom_decode_float(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Double))
		return _jsatom_decode_double(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Bool))
		return _jsatom_decode_bool(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__String))
		return _jsatom_decode_string(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Path))
		return _jsatom_decode_path(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Literal))
		return _jsatom_decode_literal(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Tuple))
		return _jsatom_decode_tuple(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Object))
		return _jsatom_decode_object(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Vector))
		return _jsatom_decode_vector(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_ATOM__Chunk))
		return _jsatom_decode_chunk(jsatom, forge, node, types);
	else if(_jsatom_types_is_a(types, LV2_MIDI__MidiEvent))
		return _jsatom_decode_midi(jsatom, forge, node, types);

	return _jsatom_decode_fallback(jsatom, forge, node, types);
}

#endif
