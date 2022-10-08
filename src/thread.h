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
#ifndef THREAD_H
#define THREAD_H

#include "common.h"


#if defined(HAVE_PTHREAD)
# include <pthread.h>
#elif defined(WIN32)
# include <windows.h>
#endif


class Thread {
      private:
#if defined(HAVE_PTHREAD)
	pthread_t threadid;
#elif defined(WIN32)
	HANDLE hndl;
#endif

	void * (*startfunc)(void *);
#if defined(WIN32)
	/* because for win32, the thread main function is declared
	 * differently than for pthread, we need to use a wrapper
	 * function (win32_startfunc_wrapper) and pass argument
	 * and return value of startfunc through the Thread object */
	void * startfunc_arg;
	void * startfunc_ret;
#endif
	
      public:
	Thread(void * (*startfunc)(void *));
	~Thread();

      public:
	void start(void * arg);
	void* wait();

      private:
#if defined(WIN32)
	static DWORD WINAPI win32_startfunc_wrapper(LPVOID arg);
#endif
};

#endif // THREAD_H
