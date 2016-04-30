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

#ifndef _MOONY_COMMON_UI_H
#define _MOONY_COMMON_UI_H

#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#if !defined(_WIN32)
#	include <sys/wait.h>
#else
# include <windows.h>
#endif

#include <lv2/lv2plug.in/ns/ext/log/logger.h>

typedef struct _spawn_t spawn_t;

struct _spawn_t {
#if defined(_WIN32)
	PROCESS_INFORMATION pi;
#else
	pid_t pid;
#endif

	LV2_Log_Logger *logger;
};

static inline char **
_spawn_parse_env(char *env, char *path)
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

#if defined(_WIN32)
static inline int 
_spawn_spawn(spawn_t *spawn, char **args)
{
	STARTUPINFO si;
	memset(&si, 0x0, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);

	size_t len = 0;
	for(char **arg = args; *arg; arg++)
	{
		len += strlen(*arg) + 1; // + space
	}

	char *cmd = malloc(len + 1); // + zero byte
	if(!cmd)
		return -1;
	cmd[0] = '\0';

	for(char **arg = args; *arg; arg++)
	{
		cmd = strcat(cmd, *arg);
		cmd = strcat(cmd, " ");
	}

	if(!CreateProcess(
		NULL,           // No module name (use command line)
		cmd,            // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&spawn->pi )    // Pointer to PROCESS_INFORMATION structure
	)
	{
		if(spawn->logger)
			lv2_log_error(spawn->logger, "CreateProcess failed: %d\n", GetLastError());
		return -1;
	}

	free(cmd);

	return 0;
}

static inline int
_spawn_waitpid(spawn_t *spawn, bool blocking)
{
	if(blocking)
	{
		WaitForSingleObject(spawn->pi.hProcess, INFINITE);
		return 0;
	}

	// !blocking
	const DWORD status = WaitForSingleObject(spawn->pi.hProcess, 0);

	switch(WaitForSingleObject(spawn->pi.hProcess, 0))
	{
		case WAIT_TIMEOUT: // non-signaled, e.g. still running
			return 0;
		case WAIT_OBJECT_0: // signaled, e.g. not running anymore
			return -1;
		case WAIT_ABANDONED: // abandoned
			if(spawn->logger)
				lv2_log_error(spawn->logger, "WaitForSingleObject abandoned\n");
			return -1;
		case WAIT_FAILED: // failed, try later
			if(spawn->logger)
				lv2_log_error(spawn->logger, "WaitForSingleObject failed\n");
			return 0;
	}

	return 0; // try later
}

static inline void 
_spawn_kill(spawn_t *spawn, int sig)
{
	CloseHandle(spawn->pi.hProcess);
	CloseHandle(spawn->pi.hThread);
}

static inline bool
_spawn_has_child(spawn_t *spawn)
{
	return (spawn->pi.hProcess != 0x0) && (spawn->pi.hThread != 0x0);
}

static inline void
_spawn_invalidate_child(spawn_t *spawn)
{
	memset(&spawn->pi, 0x0, sizeof(PROCESS_INFORMATION));
}
#else // UNICES
static inline int 
_spawn_spawn(spawn_t *spawn, char **args)
{
	spawn->pid = fork();

	if(spawn->pid == 0) // child
	{
		execvp(args[0], args); // p = search PATH for executable

		if(spawn->logger)
			lv2_log_error(spawn->logger, "execvp failed\n");
		exit(-1);
	}
	else if(spawn->pid < 0)
	{
		if(spawn->logger)
			lv2_log_error(spawn->logger, "fork failed\n");
		return -1;
	}

	return 0; // fork succeeded
}

static inline int
_spawn_waitpid(spawn_t *spawn, bool blocking)
{
	int status;
	int res;

	if(blocking)
	{
		waitpid(spawn->pid, &status, WUNTRACED); // blocking waitpid
		return 0;
	}

	// !blocking
	if( (res = waitpid(spawn->pid, &status, WUNTRACED | WNOHANG)) < 0)
	{
		if(errno == ECHILD) // child not existing
		{
			if(spawn->logger)
				lv2_log_error(spawn->logger, "waitpid child not existing\n");
			return -1;
		}
	}
	else if(res == spawn->pid) // status change
	{
		if(!WIFSTOPPED(status) && !WIFCONTINUED(status)) // has actually exited, not only stopped
			return -1;
	}

	return 0; // no status change, still running
}

static inline void
_spawn_kill(spawn_t *spawn, int sig)
{
	kill(spawn->pid, SIGINT);
}

static inline bool
_spawn_has_child(spawn_t *spawn)
{
	return spawn->pid > 0;
}

static inline void
_spawn_invalidate_child(spawn_t *spawn)
{
	spawn->pid = -1;
}
#endif

#endif
