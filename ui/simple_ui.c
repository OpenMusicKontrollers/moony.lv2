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

#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <moony.h>

#include <uv.h>
#include <lv2_external_ui.h> // kxstudio external-ui extension

typedef struct _UI UI;

struct _UI {
	struct {
		LV2_URID moony_code;
		LV2_URID moony_error;
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

	uv_loop_t loop;
	uv_process_t req;
	uv_process_options_t opts;
#if USE_FS_EVENT
	uv_fs_event_t fs;
	uv_timespec_t mtim;
#else
	uv_fs_poll_t pol;
#endif
	int done;
	int ignore;

	struct {
		LV2_External_UI_Widget widget;
		const LV2_External_UI_Host *host;
	} kx;

	uint8_t buf [0x10000];
	char dir [512];
	char path [512];
};

static inline void
_err2(UI *ui, const char *from)
{
	if(ui->log)
		lv2_log_error(&ui->logger, "%s", from);
	else
		fprintf(stderr, "%s\n", from);
}

static inline void
_err(UI *ui, const char *from, int ret)
{
	if(ui->log)
		lv2_log_error(&ui->logger, "%s: %s", from, uv_strerror(ret));
	else
		fprintf(stderr, "%s: %s\n", from, uv_strerror(ret));
}

static void
_moony_message_send(UI *ui, LV2_URID key, const char *str, uint32_t size)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
	if(_moony_patch(&ui->uris.patch, &ui->forge, key, str, size))
	{
		// trigger update
		ui->write_function(ui->controller, ui->control_port, lv2_atom_total_size(&obj->atom),
			ui->uris.event_transfer, ui->buf);
		if(ui->log)
			lv2_log_note(&ui->logger, str && size ? "send code chunk" : "query code chunk");
	}
	else if(ui->log)
		lv2_log_error(&ui->logger, "code chunk too long");
}

static void
_load_chosen(UI *ui, const char *path)
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

static inline void
_hide(UI *ui)
{
	int ret;
#if USE_FS_EVENT
	if(uv_is_active((uv_handle_t *)&ui->fs))
	{
		if((ret = uv_fs_event_stop(&ui->fs)))
			_err(ui, "uv_fs_event_stop", ret);
	}
	if(!uv_is_closing((uv_handle_t *)&ui->fs))
		uv_close((uv_handle_t *)&ui->fs, NULL);
#else
	if(uv_is_active((uv_handle_t *)&ui->pol))
	{
		if((ret = uv_fs_poll_stop(&ui->pol)))
			_err(ui, "uv_fs_poll_stop", ret);
	}
	if(!uv_is_closing((uv_handle_t *)&ui->pol))
		uv_close((uv_handle_t *)&ui->pol, NULL);
#endif

	if(uv_is_active((uv_handle_t *)&ui->req))
	{
		if((ret = uv_process_kill(&ui->req, SIGKILL)))
			_err(ui, "uv_process_kill", ret);
	}
	if(!uv_is_closing((uv_handle_t *)&ui->req))
		uv_close((uv_handle_t *)&ui->req, NULL);

	uv_stop(&ui->loop);
	uv_run(&ui->loop, UV_RUN_DEFAULT); // cleanup
	
	ui->done = 0;
}

static void
_on_exit(uv_process_t *req, int64_t exit_status, int term_signal)
{
	UI *ui = req ? (void *)req - offsetof(UI, req) : NULL;
	if(!ui)
		return;

	ui->done = 1;

	if(ui->kx.host && ui->kx.host->ui_closed && ui->controller)
	{
		_hide(ui);
		ui->kx.host->ui_closed(ui->controller);
	}
}

#if USE_FS_EVENT
static void
_on_fs_event(uv_fs_event_t *fs, const char *path, int events, int status)
{
	UI *ui = fs ? (void *)fs - offsetof(UI, fs) : NULL;
	if(!ui)
		return;

	if(events & UV_CHANGE)
	{
		uv_fs_t req;
		int ret;
		if((ret = uv_fs_stat(&ui->loop, &req, ui->path, NULL)))
			_err(ui, "uv_fs_stat", ret);
		else
		{
			if(  (ui->mtim.tv_sec == req.statbuf.st_mtim.tv_sec)
				&& (ui->mtim.tv_nsec == req.statbuf.st_mtim.tv_nsec) )
			{
				// same timestamp as before, e.g. false alarm
				return;
			}
			else
			{
				ui->mtim.tv_sec = req.statbuf.st_mtim.tv_sec;
				ui->mtim.tv_nsec = req.statbuf.st_mtim.tv_nsec;
			}
		}

		if(ui->ignore)
			ui->ignore = 0;
		else
			_load_chosen(ui, ui->path);
	}

	if(events & UV_RENAME)
	{
		int ret;
		if(uv_is_active((uv_handle_t *)&ui->fs))
		{
			if((ret = uv_fs_event_stop(&ui->fs)))
				_err(ui, "uv_fs_event_stop", ret);
		}

		//restart watcher
		if((ret = uv_fs_event_start(&ui->fs, _on_fs_event, ui->path, 0)))
			_err(ui, "uv_fs_event_start", ret);
	}
}
#else
static void
_on_fs_poll(uv_fs_poll_t *pol, int status, const uv_stat_t* prev, const uv_stat_t* curr)
{
	UI *ui = pol ? (void *)pol - offsetof(UI, pol) : NULL;
	if(!ui)
		return;

	if(  (prev->st_mtim.tv_sec == curr->st_mtim.tv_sec)
		&& (prev->st_mtim.tv_nsec == curr->st_mtim.tv_nsec) )
	{
		// same timestamp as before, e.g. false alarm
		return;
	}

	if(ui->ignore)
		ui->ignore = 0;
	else
		_load_chosen(ui, ui->path);
}
#endif

static inline char **
_parse_env(char *env, char *path)
{
	unsigned n = 0;
	char **args = malloc((n+1) * sizeof(char *));
	char **oldargs = NULL;
	if(!args)
		goto fail;
	args[n] = NULL;

	char *pch = strtok(env," \t");
	while(pch)
	{
		args[n++] = pch;
		oldargs = args;
		args = realloc(args, (n+1) * sizeof(char *));
		if(!args)
			goto fail;
		oldargs = NULL;
		args[n] = NULL;

		pch = strtok(NULL, " \t");
	}

	args[n++] = path;
	oldargs = args;
	args = realloc(args, (n+1) * sizeof(char *));
	if(!args)
		goto fail;
	oldargs = NULL;
	args[n] = NULL;

	return args;

fail:
	if(oldargs)
		free(oldargs);
	if(args)
		free(args);
	return 0;
}

static inline void
_show(UI *ui)
{
#if defined(_WIN32)
	const char *command = "cmd /c start /wait";
#elif defined(__APPLE__)
	const char *command = "open -nW";
#else // Linux/BSD
	//const char *command = "xdg-open";
	const char *command = "xterm -e vim";
#endif

	// get default editor from environment
	const char *moony_editor = getenv("MOONY_EDITOR");
	if(!moony_editor)
		moony_editor = getenv("EDITOR");
	if(!moony_editor)
		moony_editor = command;
	char *dup = strdup(moony_editor);
	char **args = dup ? _parse_env(dup, ui->path) : NULL;
	
	ui->opts.exit_cb = _on_exit;
	ui->opts.file = args ? args[0] : NULL;
	ui->opts.args = args;
#if defined(_WIN32)
	ui->opts.flags = UV_PROCESS_WINDOWS_HIDE | UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS;
#endif

	// touch file
	FILE *f = fopen(ui->path, "wb");
	if(f)
		fclose(f);

#if USE_FS_EVENT
	if(!uv_is_active((uv_handle_t *)&ui->fs))
	{
		int ret;
		if((ret = uv_fs_event_start(&ui->fs, _on_fs_event, ui->path, 0)))
			_err(ui, "uv_fs_event_start", ret);
	}
#else
	if(!uv_is_active((uv_handle_t *)&ui->pol))
	{
		int ret;
		if((ret = uv_fs_poll_start(&ui->pol, _on_fs_poll, ui->path, 1000))) // ms
			_err(ui, "uv_fs_poll_start", ret);
	}
#endif

	if(!uv_is_active((uv_handle_t *)&ui->req))
	{
		int ret;
		if((ret = uv_spawn(&ui->loop, &ui->req, &ui->opts)))
			_err(ui, "uv_spawn", ret);
	}

	if(dup)
		free(dup);
	if(args)
		free(args);
}

// External-UI Interface
static inline void
_kx_run(LV2_External_UI_Widget *widget)
{
	UI *ui = widget ? (void *)widget - offsetof(UI, kx.widget) : NULL;
	if(ui)
		uv_run(&ui->loop, UV_RUN_NOWAIT);
}

static inline void
_kx_hide(LV2_External_UI_Widget *widget)
{
	UI *ui = widget ? (void *)widget - offsetof(UI, kx.widget) : NULL;
	if(ui)
		_hide(ui);
}

static inline void
_kx_show(LV2_External_UI_Widget *widget)
{
	UI *ui = widget ? (void *)widget - offsetof(UI, kx.widget) : NULL;
	if(ui)
		_show(ui);
}

// Show Interface
static inline int
_show_cb(LV2UI_Handle instance)
{
	UI *ui = instance;
	if(ui)
		_show(ui);
	return 0;
}

static inline int
_hide_cb(LV2UI_Handle instance)
{
	UI *ui = instance;
	if(ui)
		_hide(ui);
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
	UI *ui = instance;
	if(!ui)
		return -1;
	uv_run(&ui->loop, UV_RUN_NOWAIT);
	return ui->done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle_cb
};

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

	UI *ui = calloc(1, sizeof(UI));
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
	ui->uris.moony_error = ui->map->map(ui->map->handle, MOONY_ERROR_URI);
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

	lv2_atom_forge_init(&ui->forge, ui->map);
	if(ui->log)
		lv2_log_logger_init(&ui->logger, ui->map, ui->log);

	ui->write_function = write_function;
	ui->controller = controller;

	int ret;
	if((ret = uv_loop_init(&ui->loop)))
	{
		fprintf(stderr, "%s: %s\n", descriptor->URI, uv_strerror(ret));
		free(ui);
		return NULL;
	}

	char *tmp_template;
#if defined(_WIN32)
	char tmp_dir[MAX_PATH + 1];
	GetTempPath(MAX_PATH + 1, tmp_dir);
	const char *sep = tmp_dir[strlen(tmp_dir) - 1] == '\\' ? "" : "\\";
#else
	const char *tmp_dir = P_tmpdir;
	const char *sep = tmp_dir[strlen(tmp_dir) - 1] == '/' ? "" : "/";
#endif
	asprintf(&tmp_template, "%s%smoony_XXXXXX", tmp_dir, sep);

	if(!tmp_template)
	{
		fprintf(stderr, "%s: out of memory\n", descriptor->URI);
		free(ui);
		return NULL;
	}

	uv_fs_t req;
	if((ret = uv_fs_mkdtemp(&ui->loop, &req, tmp_template, NULL)))
	{
		fprintf(stderr, "%s: %s\n", descriptor->URI, uv_strerror(ret));
		free(ui);
		return NULL;
	}

	sprintf(ui->dir, "%s", req.path);

#if defined(_WIN32)
	sep = req.path[strlen(req.path) - 1] == '\\' ? "" : "\\";
#else
	sep = req.path[strlen(req.path) - 1] == '/' ? "" : "/";
#endif
	sprintf(ui->path, "%s%smoony.lua", req.path, sep);

	uv_fs_req_cleanup(&req); // deallocates req.path
	free(tmp_template);

	if(ui->log)
	{
		lv2_log_note(&ui->logger, "dir: %s", ui->dir);
		lv2_log_note(&ui->logger, "path: %s", ui->path);
	}

	_moony_message_send(ui, ui->uris.moony_code, NULL, 0);

#if USE_FS_EVENT
	if((ret = uv_fs_event_init(&ui->loop, &ui->fs)))
		_err(ui, "uv_fs_event_init", ret);
#else
	if((ret = uv_fs_poll_init(&ui->loop, &ui->pol)))
		_err(ui, "uv_fs_poll_init", ret);
#endif

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	UI *ui = handle;

	int ret;
	uv_fs_t req;
	if((ret = uv_fs_unlink(&ui->loop, &req, ui->path, NULL)))
		_err(ui, "uv_fs_unlink", ret);
	if((ret = uv_fs_rmdir(&ui->loop, &req, ui->dir, NULL)))
		_err(ui, "uv_fs_rmdir", ret);

	uv_loop_close(&ui->loop);
}

static void
port_event(LV2UI_Handle handle, uint32_t port_index, uint32_t buffer_size,
	uint32_t format, const void *buffer)
{
	UI *ui = handle;

	if( (port_index == ui->notify_port) && (format == ui->uris.event_transfer) )
	{
		const LV2_Atom_Object *obj = buffer;

		if(  lv2_atom_forge_is_object_type(&ui->forge, obj->atom.type)
			&& (obj->body.otype == ui->uris.patch.set) )
		{
			const LV2_Atom_URID *subject = NULL;
			const LV2_Atom_URID *property = NULL;
			const LV2_Atom_String *value = NULL;
			
			LV2_Atom_Object_Query q[] = {
				{ ui->uris.patch.subject, (const LV2_Atom **)&subject },
				{ ui->uris.patch.property, (const LV2_Atom **)&property },
				{ ui->uris.patch.value, (const LV2_Atom **)&value },
				{ 0, NULL }
			};
			lv2_atom_object_query(obj, q);

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
						if(fwrite(str, value->atom.size-1, 1, f) != 1)
							_err2(ui, "fwrite");

						fclose(f);

						ui->ignore = 1; // ignore next fs change event
					}
					else
						_err2(ui, "fopen");
				}
				else if(property->body == ui->uris.moony_error)
				{
					FILE *f = fopen(ui->path, "ab");
					if(f)
					{
						const char *pre =
							"\n-- FIXME ----------------------------------------------------------------------\n-- ";
						fwrite(pre, strlen(pre), 1, f); //TODO check
						fwrite(str, value->atom.size-1, 1, f); //TODO check
						fclose(f);

						ui->ignore = 1; // ignore net fs change event
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