/* Copyright (C) 2005-2008 Holger Ruckdeschel <holger@hoicher.de>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include "common.h"
#include "thread.h"


Thread::Thread(void * (*_startfunc)(void *))
{
	startfunc = _startfunc;

#if defined(HAVE_PTHREAD)
#elif defined(WIN32)
#endif
}

Thread::~Thread()
{
#if defined(HAVE_PTHREAD)
#elif defined(WIN32)
#endif
}

void Thread::start(void * arg)
{
#if defined(HAVE_PTHREAD)
	int res = pthread_create(&threadid, NULL, startfunc, arg);
	ASSERT(res == 0);
#elif defined(WIN32)
	startfunc_arg = arg;
	startfunc_ret = NULL;
	DWORD dwThreadId;
	hndl = CreateThread(0, 0, win32_startfunc_wrapper, (LPVOID) this, 0,
			&dwThreadId);
	ASSERT(hndl != NULL);
#else
	(void) arg;
#endif
}

void* Thread::wait()
{
	void * retval;
#if defined(HAVE_PTHREAD)
	int res = pthread_join(threadid, &retval);
	ASSERT(res == 0);
	threadid = 0;
#elif defined(WIN32)
	DWORD res = WaitForSingleObject(hndl, INFINITE);
	ASSERT(res == WAIT_OBJECT_0);
	BOOL res2 = CloseHandle(hndl);
	ASSERT(res2);
	hndl = INVALID_HANDLE_VALUE;
	retval = startfunc_ret;
#else
	retval = NULL;
#endif
	return retval;
}

#if defined(WIN32)
DWORD WINAPI Thread::win32_startfunc_wrapper(LPVOID arg)
{
	Thread * self = (Thread *) arg;
	self->startfunc_ret = (*(self->startfunc))(self->startfunc_arg);
	return 0;
}
#endif

