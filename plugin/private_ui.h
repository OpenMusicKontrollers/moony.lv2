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

#ifndef _MOONY_COMMON_UI_H
#define _MOONY_COMMON_UI_H

#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#if !defined(_WIN32)
#	include <sys/wait.h>
#	include <signal.h>
#else
#	include <fcntl.h>
#	include <sys/stat.h>
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

#if defined(_WIN32)
static inline char *
strsep(char **sp, char *sep)
{
	char *p, *s;
	if(sp == NULL || *sp == NULL || **sp == '\0')
		return(NULL);
	s = *sp;
	p = s + strcspn(s, sep);
	if(*p != '\0')
		*p++ = '\0';
	*sp = p;
	return(s);
}
#endif

static inline char **
_spawn_parse_env(char *env, char *path)
{
	unsigned n = 0;
	char **args = malloc((n+1) * sizeof(char *));
	char **oldargs = NULL;
	if(!args)
		goto fail;
	args[n] = NULL;

	const char *sep = " \t";
	for(char *bufp = env, *pch = strsep(&bufp, sep);
		pch;
		pch = strsep(&bufp, sep) )
	{
		args[n++] = pch;
		oldargs = args;
		args = realloc(args, (n+1) * sizeof(char *));
		if(!args)
			goto fail;
		oldargs = NULL;
		args[n] = NULL;
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

	int ret = 0;
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
		ret = -1;
	}

	free(cmd);

	return ret;
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
				lv2_log_note(spawn->logger, "WaitForSingleObject abandoned\n");
			return -1;
		case WAIT_FAILED: // failed, try later
			if(spawn->logger)
				lv2_log_note(spawn->logger, "WaitForSingleObject failed\n");
			return 0;
	}

	return 0; // try later
}

static inline void 
_spawn_kill(spawn_t *spawn)
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
	if(!args)
	{
		if(spawn->logger)
			lv2_log_error(spawn->logger, "nil argument list\n");
		return -1;
	}

	spawn->pid = vfork();

	if(spawn->pid == 0) // child
	{
		execvp(args[0], args); // p = search PATH for executable

		if(spawn->logger)
			lv2_log_error(spawn->logger, "execvp failed\n");
		_exit(-1);
	}
	else if(spawn->pid < 0)
	{
		if(spawn->logger)
			lv2_log_error(spawn->logger, "vfork failed\n");
		return -1;
	}

	return 0; // vfork succeeded
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
				lv2_log_note(spawn->logger, "waitpid child not existing\n");
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
_spawn_kill(spawn_t *spawn)
{
	kill(spawn->pid, SIGTERM);
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

#if defined(_WIN32)
static int
_mkstemp_init(char *__template, char **suffix, size_t *length, DWORD *val,
              size_t suffixlen)
{
   *length = strlen(__template);
   if ((*length < (6 + suffixlen))
       || (strncmp(__template + *length - 6 - suffixlen, "XXXXXX", 6) != 0))
     {
        errno = EINVAL;
        return 0;
     }

   *suffix = __template + *length - 6 - suffixlen;

   *val = GetTickCount();
   *val += GetCurrentProcessId();

   return 1;
}

static int
_mkstemp(char *suffix, int val)
{
   const char lookup[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
   DWORD v = val;

   suffix[0] = lookup[v % 62];
   v /= 62;
   suffix[1] = lookup[v % 62];
   v /= 62;
   suffix[2] = lookup[v % 62];
   v /= 62;
   suffix[3] = lookup[v % 62];
   v /= 62;
   suffix[4] = lookup[v % 62];
   v /= 62;
   suffix[5] = lookup[v % 62];
   v /= 62;

   val += 7777;

   return val;
}

static int
mkstemps(char *__template, int suffixlen)
{
   char      *suffix;
   DWORD      val;
   size_t     length;
   int        i;

   if (!__template || (suffixlen < 0))
     return 0;

   if (!_mkstemp_init(__template, &suffix, &length, &val, (size_t) suffixlen))
     return -1;

   for (i = 0; i < 32768; i++)
     {
        int fd;

        val = _mkstemp(suffix, val);

        fd = open(__template, _O_RDWR | _O_BINARY | _O_CREAT | _O_EXCL, _S_IREAD | _S_IWRITE);
        if (fd >= 0)
          return fd;
     }

   errno = EEXIST;
   return -1;
}
#endif

#endif
