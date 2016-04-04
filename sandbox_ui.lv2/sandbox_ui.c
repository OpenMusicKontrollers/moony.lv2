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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>

#include <sandbox_ui.h>
#include <sandbox_master.h>
#include <lv2_external_ui.h> // kxstudio external-ui extension

#define SOCKET_PATH_LEN 32

typedef struct _plughandle_t plughandle_t;

struct _plughandle_t {
	int done;

	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;
	LV2UI_Port_Subscribe *subscribe;

	sandbox_master_driver_t driver;
	sandbox_master_t *sb;

	char *plugin_uri;
	char *bundle_path;
	char *executable;
	char *ui_uri;
	char *window_title;
	char socket_path [SOCKET_PATH_LEN];

	LV2_URID ui_window_title;
	LV2_URID atom_string;

	pid_t pid;

	struct {
		LV2_External_UI_Widget widget;
		const LV2_External_UI_Host *host;
	} kx;
};

static void
_recv(LV2UI_Controller controller, uint32_t port,
	uint32_t size, uint32_t protocol, const void *buf)
{
	plughandle_t *handle = controller;

	handle->write_function(handle->controller, port, size, protocol, buf);
}

static void
_subscribe(LV2UI_Controller controller, uint32_t port,
	uint32_t protocol, bool state)
{
	plughandle_t *handle = controller;

	if(handle->subscribe)
	{
		if(state)
			handle->subscribe->subscribe(handle->subscribe->handle,
				port, protocol, NULL);
		else
			handle->subscribe->unsubscribe(handle->subscribe->handle,
				port, protocol, NULL);
	}
}

// Show Interface
static inline int
_show_cb(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	if(!handle->done)
		return 0; // already showing

	strncpy(handle->socket_path, "ipc:///tmp/sandbox_ui_XXXXXX", SOCKET_PATH_LEN);
	int fd = mkstemp(&handle->socket_path[6]);
	if(!fd)
		return -1;
	close(fd);

	handle->sb = sandbox_master_new(&handle->driver, handle);
	if(!handle->sb)
		return -1;

	handle->pid = fork();
	if(handle->pid == 0) // child
	{
		char *const argv [] = {
			handle->executable,
			"-p", handle->plugin_uri,
			"-b", handle->bundle_path,
			"-u", handle->ui_uri,
			"-s", handle->socket_path,
			"-w", handle->window_title,
			NULL
		};
		execv(handle->executable, argv); // p = search PATH for executable

		printf("fork child failed\n");
		exit(-1);
	}
	else if(handle->pid < 0)
	{
		printf("fork failed\n");
		return -1;
	}

	handle->done = 0;

	return 0;
}

static inline int
_hide_cb(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	if(handle->pid > 0) // has child
	{
		kill(handle->pid, SIGINT);

		int status;
		waitpid(handle->pid, &status, 0);

		handle->pid = -1; // invalidate
	}

	if(handle->sb)
	{
		sandbox_master_free(handle->sb);
		handle->sb = NULL;
	}

	/* FIXME
	remove(&handle->socket_path[6]);
	*/

	if(handle->kx.host && handle->kx.host->ui_closed)
		handle->kx.host->ui_closed(handle->controller);

	handle->done = 1;

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
	plughandle_t *handle = instance;

	if(handle->pid > 0)
	{
		int status;
		int res;
		if((res = waitpid(handle->pid, &status, WNOHANG)) < 0)
		{
			if(errno == ECHILD)
			{
				handle->pid = -1; // invalidate
				//_hide_cb(ui);
				handle->done = 1;
			}
		}
		else if( (res > 0) && WIFEXITED(status) )
		{
			handle->pid = -1; // invalidate
			//_hide_cb(ui);
			handle->done = 1;
		}
	}

	if(!handle->done && handle->sb)
	{
		sandbox_master_recv(handle->sb);
		sandbox_master_flush(handle->sb);
	}

	return handle->done;
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle_cb
};

// External-UI Interface
static inline void
_kx_run(LV2_External_UI_Widget *widget)
{
	plughandle_t *handle = (void *)widget - offsetof(plughandle_t, kx.widget);

	if(_idle_cb(handle))
		_hide_cb(handle);
}

static inline void
_kx_hide(LV2_External_UI_Widget *widget)
{
	plughandle_t *handle = (void *)widget - offsetof(plughandle_t, kx.widget);

	_hide_cb(handle);
}

static inline void
_kx_show(LV2_External_UI_Widget *widget)
{
	plughandle_t *handle = (void *)widget - offsetof(plughandle_t, kx.widget);

	_show_cb(handle);
}

static inline void
_free_strdups(plughandle_t *handle)
{
	if(handle->plugin_uri)
		free(handle->plugin_uri);
	if(handle->bundle_path)
		free(handle->bundle_path);
	if(handle->executable)
		free(handle->executable);
	if(handle->ui_uri)
		free(handle->ui_uri);
	if(handle->window_title)
		free(handle->window_title);
};

LV2UI_Handle
sandbox_ui_instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	handle->write_function = write_function;
	handle->controller = controller;

	LV2_Options_Option *opts = NULL; // optional
	for(unsigned i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->driver.map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			handle->driver.unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__portSubscribe))
			handle->subscribe = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host))
			handle->kx.host = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_OPTIONS__options))
			opts = features[i]->data;
	}

	if(!handle->driver.map || !handle->driver.unmap)
	{
		free(handle);
		return NULL;
	}

	handle->ui_window_title = handle->driver.map->map(handle->driver.map->handle,
		LV2_UI__windowTitle);
	handle->atom_string = handle->driver.map->map(handle->driver.map->handle,
		LV2_ATOM__String);

	handle->plugin_uri = strdup(plugin_uri);
	handle->bundle_path = strdup(bundle_path);
	if(asprintf(&handle->executable, "%ssandbox_efl", bundle_path) == -1)
		handle->executable = NULL; // failed
	handle->ui_uri = strdup(descriptor->URI);
		sprintf(&handle->ui_uri[strlen(handle->ui_uri) - 4], "%s", "3_eo"); //TODO more elegant way?

	if(opts)
	{
		for(LV2_Options_Option *opt = opts;
			(opt->key != 0) && (opt->value != NULL);
			opt++)
		{
			if( (opt->key == handle->ui_window_title) && (opt->type == handle->atom_string) )
			{
				handle->window_title = strdup(opt->value);
				break;
			}
		}
	}
	if(!handle->window_title && handle->kx.host && handle->kx.host->plugin_human_id)
		handle->window_title = strdup(handle->kx.host->plugin_human_id);
	if(!handle->window_title)
		handle->window_title = strdup(descriptor->URI);

	if(!handle->plugin_uri || !handle->bundle_path || !handle->executable || !handle->ui_uri || !handle->window_title)
	{
		_free_strdups(handle);
		free(handle);
		return NULL;
	}

	handle->driver.socket_path = handle->socket_path;
	handle->driver.recv_cb = _recv;
	handle->driver.subscribe_cb = _subscribe;

	handle->pid = -1; // invalidate

	handle->kx.widget.run = _kx_run;
	handle->kx.widget.show = _kx_show;
	handle->kx.widget.hide = _kx_hide;

	if(strstr(descriptor->URI, "_kx"))
		*(LV2_External_UI_Widget **)widget = &handle->kx.widget;
	else
		*widget = NULL;

	handle->done = 1;

	return handle;
}

void
sandbox_ui_cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	_free_strdups(handle);
	free(handle);
}

void
sandbox_ui_port_event(LV2UI_Handle instance, uint32_t index, uint32_t size,
	uint32_t protocol, const void *buf)
{
	plughandle_t *handle = instance;

	if(handle->sb)
		sandbox_master_send(handle->sb, index, size, protocol, buf);
}

// extension data callback for show interface UI
const void *
sandbox_ui_extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__showInterface))
		return &show_ext;
		
	return NULL;
}
