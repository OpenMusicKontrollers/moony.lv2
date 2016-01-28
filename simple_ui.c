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
		LV2_URID moony_message;
		LV2_URID moony_code;
		LV2_URID moony_error;
		LV2_URID event_transfer;
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
_moony_message_send(UI *ui, LV2_URID otype, LV2_URID key,
	uint32_t size, const char *str)
{
	LV2_Atom_Forge_Frame frame;
	LV2_Atom_Object *obj = (LV2_Atom_Object *)ui->buf;

	lv2_atom_forge_set_buffer(&ui->forge, ui->buf, 0x10000);
	if(_moony_message_forge(&ui->forge, otype, key, size, str))
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
				_moony_message_send(ui, ui->uris.moony_message, ui->uris.moony_code, fsize, str);
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
	uv_close((uv_handle_t *)&ui->fs, NULL);
#else
	if(uv_is_active((uv_handle_t *)&ui->pol))
	{
		if((ret = uv_fs_poll_stop(&ui->pol)))
			_err(ui, "uv_fs_poll_stop", ret);
	}
	uv_close((uv_handle_t *)&ui->pol, NULL);
#endif

	if(uv_is_active((uv_handle_t *)&ui->req))
	{
		if((ret = uv_process_kill(&ui->req, SIGKILL)))
			_err(ui, "uv_process_kill", ret);
	}
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

	_load_chosen(ui, ui->path);
}
#endif

static inline char **
_parse_env(char *env, char *path)
{
	unsigned n = 0;
	char **args = malloc((n+1) * sizeof(char *));
	if(!args)
		goto fail;
	args[n] = NULL;

	char *pch = strtok(env," \t");
	while(pch)
	{
		args[n++] = pch;
		args = realloc(args, (n+1) * sizeof(char *));
		if(!args)
			goto fail;
		args[n] = NULL;

		pch = strtok(NULL, " \t");
	}

	args[n++] = path;
	args = realloc(args, (n+1) * sizeof(char *));
	if(!args)
		goto fail;
	args[n] = NULL;

	return args;

fail:
	if(args)
		free(args);
	return NULL;
}

static inline void
_show(UI *ui)
{
#if defined(_WIN32)
	const char *command = "START \"Moony\" /WAIT";
#elif defined(__APPLE__)
	const char *command = "open -nW";
#else // Linux/BSD
	//const char *command = "xdg-open";
	const char *command = "xterm -e vim";
#endif

	// get default editor from environment
	const char *moony_editor = getenv("MOONY_EDITOR");
	if(!moony_editor)
		moony_editor = command;
	char *dup = strdup(moony_editor);
	char **args = dup ? _parse_env(dup, ui->path) : NULL;
	
	ui->opts.exit_cb = _on_exit;
	ui->opts.file = args ? args[0] : NULL;
	ui->opts.args = args;

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

	ui->uris.moony_message = ui->map->map(ui->map->handle, MOONY_MESSAGE_URI);
	ui->uris.moony_code = ui->map->map(ui->map->handle, MOONY_CODE_URI);
	ui->uris.moony_error = ui->map->map(ui->map->handle, MOONY_ERROR_URI);
	ui->uris.event_transfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);

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
	asprintf(&tmp_template, "%s\\moony_XXXXXX", tmp_dir);
#else
	const char *tmp_dir = P_tmpdir;
	asprintf(&tmp_template, "%s/moony_XXXXXX", tmp_dir);
#endif

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
	sprintf(ui->path, "%s\\moony.lua", req.path);
#else
	sprintf(ui->path, "%s/moony.lua", req.path);
#endif

	uv_fs_req_cleanup(&req); // deallocates req.path
	free(tmp_template);

	if(ui->log)
	{
		lv2_log_note(&ui->logger, "dir: %s", ui->dir);
		lv2_log_note(&ui->logger, "path: %s", ui->path);
	}

	_moony_message_send(ui, ui->uris.moony_message, ui->uris.moony_code, 0, NULL);

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
			&& (obj->body.otype == ui->uris.moony_message) )
		{
			const LV2_Atom_String *moony_error = NULL;
			const LV2_Atom_String *moony_code = NULL;
			
			LV2_Atom_Object_Query q[] = {
				{ ui->uris.moony_error, (const LV2_Atom **)&moony_error },
				{ ui->uris.moony_code, (const LV2_Atom **)&moony_code },
				LV2_ATOM_OBJECT_QUERY_END
			};
			lv2_atom_object_query(obj, q);

			if(moony_code)
			{
				const char *str = LV2_ATOM_BODY_CONST(&moony_code->atom);

				int ret;
#if USE_FS_EVENT
				if((ret = uv_fs_event_stop(&ui->fs)))
					_err(ui, "uv_fs_event_stop", ret);
#else
				if((ret = uv_fs_poll_stop(&ui->pol)))
					_err(ui, "uv_fs_poll_stop", ret);
#endif

				FILE *f = fopen(ui->path, "wb");
				if(f)
				{
					if(fwrite(str, moony_code->atom.size-1, 1, f) != 1)
						_err2(ui, "fwrite");

					fclose(f);
				}
				else
					_err2(ui, "fopen");

#if USE_FS_EVENT
				if((ret = uv_fs_event_start(&ui->fs, _on_fs_event, ui->path, 0)))
					_err(ui, "uv_fs_event_start", ret);
#else
				if((ret = uv_fs_poll_start(&ui->pol, _on_fs_poll, ui->path, 1000))) // ms
					_err(ui, "uv_fs_poll_start", ret);
#endif
			}
			else if(moony_error)
			{
				const char *str = LV2_ATOM_BODY_CONST(&moony_error->atom);

				int ret;
#if USE_FS_EVENT
				if((ret = uv_fs_event_stop(&ui->fs)))
					_err(ui, "uv_fs_event_stop", ret);
#else
				if((ret = uv_fs_poll_stop(&ui->pol)))
					_err(ui, "uv_fs_poll_stop", ret);
#endif

				FILE *f = fopen(ui->path, "ab");
				if(f)
				{
					const char *pre =
						"\n-- FIXME ----------------------------------------------------------------------\n-- ";
					fwrite(pre, strlen(pre), 1, f); //TODO check
					fwrite(str, moony_error->atom.size-1, 1, f); //TODO check
					fclose(f);
				}

#if USE_FS_EVENT
				if((ret = uv_fs_event_start(&ui->fs, _on_fs_event, ui->path, 0)))
					_err(ui, "uv_fs_event_start", ret);
#else
				if((ret = uv_fs_poll_start(&ui->pol, _on_fs_poll, ui->path, 1000))) // ms
					_err(ui, "uv_fs_poll_start", ret);
#endif
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
