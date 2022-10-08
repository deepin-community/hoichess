/* Copyright (C) 2004, 2005 Holger Ruckdeschel <holger@hoicher.de>
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
#include "util.h"
#include "signal.h"
#ifdef WITH_THREAD
# include "mutex.h"
# include "thread.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
# include <sys/time.h>
# include <sys/times.h>
#endif
#include <time.h>
#include <unistd.h>


#ifdef WITH_THREAD
Mutex stdout_mutex;

int atomic_printf(const char * fmt, ...)
{
	stdout_mutex.lock();
	
	va_list args;
	va_start(args, fmt);
	int ret = vfprintf(stdout, fmt, args);
	va_end(args);
	
	stdout_mutex.unlock();
	return ret;
}

int atomic_fprintf(FILE * fp, Mutex * mutex, const char * fmt, ...)
{
	ASSERT(mutex != NULL);
	mutex->lock();
	
	va_list args;
	va_start(args, fmt);
	int ret = vfprintf(fp, fmt, args);
	va_end(args);
	
	mutex->unlock();
	return ret;
}
#endif // WITH_THREAD


/*
 * Win32 rand() returns only a 15 bit random number. We just take this
 * function on all systems - it is only used during initialization, so
 * it is not performance critical.
 *
 * Thanks to Bruce Moreland for this snippet,
 * and to Jim Ablett who brought it to me :-).
 */
uint64_t random64()
{
	uint64_t tmp = rand();
	tmp ^= ((uint64_t) rand() << 15);
	tmp ^= ((uint64_t) rand() << 30);
	tmp ^= ((uint64_t) rand() << 45);
	tmp ^= ((uint64_t) rand() << 60);
	return tmp;
}

#if 0
uint64_t random64()
{
	uint64_t tmp = random();
	tmp <<= 32;
	tmp |= random();
	return tmp;
}
#endif

std::string strprintf(const char * fmt, ...)
{
	char buf[65536]; // should be enough

	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	ASSERT(ret >= 0);
	if ((unsigned) ret >= sizeof(buf)) {
		WARN("buffer overflow, output truncated");
	}

	return std::string(buf);
}


bool parse_size(const char * s, ssize_t * n)
{
	ASSERT(s != NULL);
	ASSERT(s != NULL);

	long tmp;
	
	if (s[strlen(s)-1] == 'M' && sscanf(s, "%ldM", &tmp) == 1) {
		*n = tmp * (1<<20);
		return true;
	} else if (s[strlen(s)-1] == 'K' && sscanf(s, "%ldK", &tmp) == 1) {
		*n = tmp * (1<<10);
		return true;
	} else if (sscanf(s, "%ld", &tmp) == 1) {
		*n = tmp;
		return true;
	} else {
		return false;
	}
}

/* Returns real time (wall clock) in microseconds since an undefined point
 * in the past. Use for time measurement. */
unsigned long long get_realtime_us()
{
	unsigned long long t;
#ifdef WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	/* resolution is 100 ns */
	uint64_t tmp = ((uint64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	t = tmp / 10;
#else
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1) {
		perror("gettimeofday() failed");
		exit(EXIT_FAILURE);
	}
	t = (unsigned long long) tv.tv_sec * (int)1E6 + tv.tv_usec;
#endif
	return t;
}

#if !defined(WIN32) && !defined(__leonbare__)
/* FIXME assumes that sysconf() never fails */
static long sc_clk_tck = sysconf(_SC_CLK_TCK);
#endif

/* Returns CPU time (user time) in microseconds since an undefined point
 * in the past. Use for time measurement. */
unsigned long long get_cputime_us()
{
	unsigned long long t;
#if defined(WIN32)
	FILETIME ct, et, kt, ut;
	BOOL ok = GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut);
	ASSERT(ok);
	/* resolution is 100 ns */
	uint64_t tmp = ((uint64_t) ut.dwHighDateTime << 32) | ut.dwLowDateTime;
	t = tmp / 10;
#elif defined(__leonbare__)
	t = get_realtime_us();
#else
	struct tms tmp;
	if (times(&tmp) == -1) {
		perror("times() failed");
		exit(EXIT_FAILURE);
	}
	t = (int)1E6 * (unsigned long long) (tmp.tms_utime + tmp.tms_cutime)
		/ sc_clk_tck;
#endif
	return t;
}

unsigned long long _timing_trace_us_start;
unsigned long long _timing_trace_us_last;

