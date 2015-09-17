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

#ifndef _SYNTHPOD_EXT_URID_H
#define _SYNTHPOD_EXT_URID_H

#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

typedef struct _ext_urid_t ext_urid_t;

ext_urid_t *
ext_urid_new(void);

void
ext_urid_free(ext_urid_t *ext_urid);

LV2_URID_Map *
ext_urid_map_get(ext_urid_t *ext_urid);

LV2_URID_Unmap *
ext_urid_unmap_get(ext_urid_t *ext_urid);

LV2_URID
ext_urid_map(ext_urid_t *ext_urid, const char *uri);

const char *
ext_urid_unmap(ext_urid_t *ext_urid, const LV2_URID urid);

#endif // _SYNTHPOD_EXT_URID_H
