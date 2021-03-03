/*
 * Copyright (c) 2018-2019 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <signal.h>

#include <d2tk/util.h>

typedef struct _clone_data_t clone_data_t;

struct _clone_data_t {
	char **argv;
};

static int
_clone(void *data)
{
	clone_data_t *clone_data = data;

	execvp(clone_data->argv[0], clone_data->argv);
	_exit(EXIT_FAILURE);

	return 0;
}

D2TK_API int
d2tk_util_spawn(char **argv)
{
	clone_data_t clone_data = {
		.argv = argv
	};

	const int pid = vfork();
	switch(pid)
	{
		case -1:
		{
			 // nothing to do
		} return -1;

    case 0: //child
		{
			// everything is done in _clone
		} return _clone(&clone_data);

		default: // parent
		{
#if D2TK_DEBUG == 1
			fprintf(stderr, "[%s] child with pid %i has spawned\n", __func__, pid);
#endif
		} return pid;
	}
}

D2TK_API int
d2tk_util_kill(int *kid)
{
	if(*kid <= 0)
	{
		return 0;
	}

	while(true)
	{
		usleep(1000);

		kill(*kid, SIGINT);
		kill(*kid, SIGQUIT);
		kill(*kid, SIGTERM);
		kill(*kid, SIGKILL);

		int stat = 0;
		const int pid = waitpid(*kid, &stat, 0);

		if(pid == -1)
		{
			continue;
		}

		if(pid == 0)
		{
			continue;
		}

		// has exited
		if(WIFSIGNALED(stat))
		{
#if D2TK_DEBUG == 1
			fprintf(stderr, "[%s] child with pid %d has exited with signal %d\n",
				__func__, *kid, WTERMSIG(stat));
#endif
		}
		else if(WIFEXITED(stat))
		{
#if D2TK_DEBUG == 1
			fprintf(stderr, "[%s] child with pid %d has exited with status %d\n",
				__func__, *kid, WEXITSTATUS(stat));
#endif
		}

		*kid = -1;
		break;
	}

	return 0;
}

D2TK_API int
d2tk_util_wait(int *kid)
{
	if(*kid <= 0)
	{
		return 0;
	}

	int stat = 0;
	const int pid = waitpid(*kid, &stat, WNOHANG);

	if(pid == -1)
	{
		return 1;
	}

	if(pid == 0)
	{
		return 1;
	}

	// has exited
	if(WIFSIGNALED(stat))
	{
#if D2TK_DEBUG == 1
		fprintf(stderr, "[%s] child with pid %d has exited with signal %d\n",
			__func__, *kid, WTERMSIG(stat));
#endif
	}
	else if(WIFEXITED(stat))
	{
#if D2TK_DEBUG == 1
		fprintf(stderr, "[%s] child with pid %d has exited with status %d\n",
			__func__, *kid, WEXITSTATUS(stat));
#endif
	}

	*kid = -1;
	return 0;
}
