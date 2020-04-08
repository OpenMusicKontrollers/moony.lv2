/*
 * Copyright (c) 2015-2019 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <moony.h>
#include <private_ui.h>

#include <lv2_external_ui.h> // kxstudio external-ui extension

typedef struct _ui_t ui_t;

struct _ui_t {
	struct {
		LV2_URID moony_code;
		LV2_URID event_transfer;
		patch_t patch;
	} uris;

	LV2_Log_Log *log;
	LV2_Log_Logger logger;

	LV2_Atom_Forge forge;
	LV2_URID_Map *map;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	LV2UI_Port_Map *port_map;
	uint32_t control_port;
	uint32_t notify_port;

	struct stat stat;
	int done;

	spawn_t spawn;

	struct {
		LV2_External_UI_Widget widget;
		const LV2_External_UI_Host *host;
	} kx;

	uint8_t buf [0x20000];
	char path [1024];
};

static void
_moony_message_send(ui_t *ui, LV2_URID key, const char *str, uint32_t size)
{
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
	if(_moony_patch(&ui->uris.patch, &ui->forge, key, str, size))
	{
		// trigger update
		ui->write_function(ui->controller, ui->control_port, lv2_atom_total_size(&obj->atom),
			ui->uris.event_transfer, ui->buf);
	}
	else if(ui->log)
		lv2_log_error(&ui->logger, "simple_ui: code is too long\n");
}

static void
_load_chosen(ui_t *ui, const char *path)
{
	if(!path)
		return;

	// load file
	FILE *f = fopen(path, "rb");
	if(f)
	{
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *str = malloc(fsize + 1);
		if(str)
		{
			if(fread(str, fsize, 1, f) == 1)
			{
				str[fsize] = 0;
				_moony_message_send(ui, ui->uris.moony_code, str, fsize);
			}

			free(str);
		}

		fclose(f);
	}
}

// Show Interface
static inline int
_show_cb(LV2UI_Handle instance)
{
	ui_t *ui = instance;

	if(!ui->done)
		return 0; // already showing

#if defined(_WIN32)
	const char *command = "cmd /c start /wait";
#elif defined(__APPLE__)
	const char *command = "open -nW";
#else // Linux/BSD
	//const char *command = "xdg-open";
	const char *command = "xterm -e vi";
#endif

	// get default editor from environment
	const char *moony_editor = getenv("MOONY_EDITOR");
	if(!moony_editor)
		moony_editor = command;
	char *dup = strdup(moony_editor);
	char **args = dup ? _spawn_parse_env(dup, ui->path) : NULL;

	const int status = _spawn_spawn(&ui->spawn, args);

	if(args)
		free(args);
	if(dup)
		free(dup);

	if(status)
		return -1; // failed to spawn

	ui->done = 0;

	return 0;
}

static inline int
_hide_cb(LV2UI_Handle instance)
{
	ui_t *ui = instance;

	if(_spawn_has_child(&ui->spawn))
	{
		_spawn_kill(&ui->spawn);

		_spawn_waitpid(&ui->spawn, true);

		_spawn_invalidate_child(&ui->spawn);
	}

	ui->done = 1;

	return 0;
}

static const LV2UI_Show_Interface show_ext = {
	.show = _show_cb,
	.hide = _hide_cb
};

// Idle interface
static inline int
_idle_cb(LV2UI_Handle instance)
{
	ui_t *ui = instance;

	if(_spawn_has_child(&ui->spawn))
	{
		int res;
		if((res = _spawn_waitpid(&ui->spawn, false)) < 0)
		{
			_spawn_invalidate_child(&ui->spawn);
			ui->done = 1; // xdg-open may return immediately
		}
	}

	if(!ui->done)
	{
		struct stat stat1;
		memset(&stat1, 0x0, sizeof(struct stat));

		if(stat(ui->path, &stat1) < 0)
		{
			lv2_log_error(&ui->logger, "simple_ui: stat failed\n");

			ui->done = 1; // no file or other error
		}
		else if(stat1.st_mtime != ui->stat.st_mtime)
		{
			_load_chosen(ui, ui->path);

			ui->stat = stat1; // update stat
		}
	}

	return ui->done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle_cb
};

// External-ui_t Interface
static inline void
_kx_run(LV2_External_UI_Widget *widget)
{
	ui_t *handle = (void *)widget - offsetof(ui_t, kx.widget);

	if(_idle_cb(handle))
	{
		if(handle->kx.host && handle->kx.host->ui_closed)
			handle->kx.host->ui_closed(handle->controller);
		_hide_cb(handle);
	}
}

static inline void
_kx_hide(LV2_External_UI_Widget *widget)
{
	ui_t *handle = (void *)widget - offsetof(ui_t, kx.widget);

	_hide_cb(handle);
}

static inline void
_kx_show(LV2_External_UI_Widget *widget)
{
	ui_t *handle = (void *)widget - offsetof(ui_t, kx.widget);

	_show_cb(handle);
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	if(		strcmp(plugin_uri, MOONY_C1XC1_URI)
		&&	strcmp(plugin_uri, MOONY_C2XC2_URI)
		&&	strcmp(plugin_uri, MOONY_C4XC4_URI)

		&&	strcmp(plugin_uri, MOONY_A1XA1_URI)
		&&	strcmp(plugin_uri, MOONY_A2XA2_URI)
		&&	strcmp(plugin_uri, MOONY_A4XA4_URI)

		&&	strcmp(plugin_uri, MOONY_C1A1XC1A1_URI)
		&&	strcmp(plugin_uri, MOONY_C2A1XC2A1_URI)
		&&	strcmp(plugin_uri, MOONY_C4A1XC4A1_URI) )
	{
		return NULL;
	}

	ui_t *ui = calloc(1, sizeof(ui_t));
	if(!ui)
		return NULL;

	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			ui->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portMap))
			ui->port_map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_LOG__log))
			ui->log = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) && (descriptor == &simple_kx))
			ui->kx.host = features[i]->data;
	}

	ui->kx.widget.run = _kx_run;
	ui->kx.widget.show = _kx_show;
	ui->kx.widget.hide = _kx_hide;

	if(descriptor == &simple_kx)
		*(LV2_External_UI_Widget **)widget = &ui->kx.widget;
	else
		*(void **)widget = NULL;

	if(!ui->map)
	{
		fprintf(stderr, "%s: Host does not support urid:map\n", descriptor->URI);
		free(ui);
		return NULL;
	}
	if(!ui->port_map)
	{
		fprintf(stderr, "%s: Host does not support ui:portMap\n", descriptor->URI);
		free(ui);
		return NULL;
	}

	// query port index of "control" port
	ui->control_port = ui->port_map->port_index(ui->port_map->handle, "control");
	ui->notify_port = ui->port_map->port_index(ui->port_map->handle, "notify");

	ui->uris.moony_code = ui->map->map(ui->map->handle, MOONY_CODE_URI);
	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

	ui->uris.patch.self = ui->map->map(ui->map->handle, plugin_uri);
	ui->uris.patch.get = ui->map->map(ui->map->handle, LV2_PATCH__Get);
	ui->uris.patch.set = ui->map->map(ui->map->handle, LV2_PATCH__Set);
	ui->uris.patch.put = ui->map->map(ui->map->handle, LV2_PATCH__Put);
	ui->uris.patch.patch = ui->map->map(ui->map->handle, LV2_PATCH__Patch);
	ui->uris.patch.body = ui->map->map(ui->map->handle, LV2_PATCH__body);
	ui->uris.patch.subject = ui->map->map(ui->map->handle, LV2_PATCH__subject);
	ui->uris.patch.property = ui->map->map(ui->map->handle, LV2_PATCH__property);
	ui->uris.patch.value = ui->map->map(ui->map->handle, LV2_PATCH__value);
	ui->uris.patch.add = ui->map->map(ui->map->handle, LV2_PATCH__add);
	ui->uris.patch.remove = ui->map->map(ui->map->handle, LV2_PATCH__remove);
	ui->uris.patch.wildcard = ui->map->map(ui->map->handle, LV2_PATCH__wildcard);
	ui->uris.patch.writable = ui->map->map(ui->map->handle, LV2_PATCH__writable);
	ui->uris.patch.readable = ui->map->map(ui->map->handle, LV2_PATCH__readable);
	ui->uris.patch.destination = ui->map->map(ui->map->handle, LV2_PATCH__destination);

	lv2_atom_forge_init(&ui->forge, ui->map);
	if(ui->log)
		lv2_log_logger_init(&ui->logger, ui->map, ui->log);

	ui->write_function = write_function;
	ui->controller = controller;

	char *tmp_template;
#if defined(_WIN32)
	char tmp_dir[MAX_PATH + 1];
	GetTempPath(MAX_PATH + 1, tmp_dir);
	const char *sep = tmp_dir[strlen(tmp_dir) - 1] == '\\' ? "" : "\\";
#else
	const char *tmp_dir = P_tmpdir;
	const char *sep = tmp_dir[strlen(tmp_dir) - 1] == '/' ? "" : "/";
#endif
	asprintf(&tmp_template, "%s%smoony_XXXXXX.lua", tmp_dir, sep);

	if(!tmp_template)
	{
		fprintf(stderr, "%s: out of memory\n", descriptor->URI);
		free(ui);
		return NULL;
	}

	int fd = mkstemps(tmp_template, 4);
	if(fd)
		close(fd);

	snprintf(ui->path, sizeof(ui->path), "%s", tmp_template);

	if(ui->log)
		lv2_log_note(&ui->logger, "simple_ui: opening %s\n", ui->path);

	if(stat(ui->path, &ui->stat) < 0) // update modification timestamp
		lv2_log_error(&ui->logger, "simple_ui: stat failed\n");

	free(tmp_template);

	_moony_message_send(ui, ui->uris.moony_code, NULL, 0);
	ui->done = 1;

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	ui_t *ui = handle;

	unlink(ui->path);

	if(ui->log)
		lv2_log_note(&ui->logger, "simple_ui: closing %s\n", ui->path);

	free(ui);
}

static void
port_event(LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	ui_t *ui = handle;

	if( (port_index == ui->notify_port) && (format == ui->uris.event_transfer) )
	{
		const LV2_Atom_Object *obj = buffer;

		if(  lv2_atom_forge_is_object_type(&ui->forge, obj->atom.type)
			&& (obj->body.otype == ui->uris.patch.set) )
		{
			const LV2_Atom_URID *subject = NULL;
			const LV2_Atom_URID *property = NULL;
			const LV2_Atom_String *value = NULL;

			lv2_atom_object_get(obj,
				ui->uris.patch.subject, &subject,
				ui->uris.patch.property, &property,
				ui->uris.patch.value, &value,
				0);

			//FIXME check subject

			if(  property && value
				&& (property->atom.type == ui->forge.URID)
				&& (value->atom.type == ui->forge.String) )
			{
				const char *str = LV2_ATOM_BODY_CONST(value);

				if(property->body == ui->uris.moony_code)
				{
					FILE *f = fopen(ui->path, "wb");
					if(f)
					{
						if( (fwrite(str, value->atom.size-1, 1, f) != 1)
							&& ui->log )
						{
							lv2_log_error(&ui->logger, "simple_ui: fwrite failed\n");
						}

						fclose(f);

						if(stat(ui->path, &ui->stat) < 0) // update modification timestamp
							lv2_log_error(&ui->logger, "simple_ui: stat failed\n");
					}
					else if(ui->log)
					{
						lv2_log_error(&ui->logger, "simple_ui: fopen failed\n");
					}
				}
			}
		}
	}
}

static const void *
ui_extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__showInterface))
		return &show_ext;
		
	return NULL;
}

const LV2UI_Descriptor simple_ui = {
	.URI						= MOONY_SIMPLE_UI_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= ui_extension_data
};

const LV2UI_Descriptor simple_kx = {
	.URI						= MOONY_SIMPLE_KX_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= NULL
};
