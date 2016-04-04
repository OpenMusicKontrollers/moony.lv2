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

#ifndef _SANDBOX_UI_H
#define _SANDBOX_UI_H

#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

#ifdef __cplusplus
extern "C" {
#endif

LV2UI_Handle
sandbox_ui_instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features);

void
sandbox_ui_cleanup(LV2UI_Handle instance);

void
sandbox_ui_port_event(LV2UI_Handle instance, uint32_t index, uint32_t size,
	uint32_t protocol, const void *buf);

const void *
sandbox_ui_extension_data(const char *uri);

#ifdef __cplusplus
}
#endif

#endif
