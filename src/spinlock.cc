/* Copyright (C) 2005-2017 Holger Ruckdeschel <holger@hoicher.de>
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
#include "spinlock.h"


Spinlock::Spinlock()
{
#if defined(HAVE_PTHREAD)
	int res = pthread_spin_init(&lck, PTHREAD_PROCESS_PRIVATE);
	ASSERT(res == 0);
#elif defined(WIN32)
	BOOL res = InitializeCriticalSectionAndSpinCount(&cs, (DWORD) -1);
	ASSERT(res); 
#endif
}

Spinlock::~Spinlock()
{
#if defined(HAVE_PTHREAD)
	int res = pthread_spin_destroy(&lck);
	ASSERT(res == 0);
#elif defined(WIN32)
	DeleteCriticalSection(&cs); /* void */
#endif
}

