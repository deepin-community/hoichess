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
#ifndef UTIL_H
#define UTIL_H

#include "common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#ifdef WITH_THREAD
extern Mutex stdout_mutex;
extern int atomic_printf(const char * fmt, ...);
extern int atomic_fprintf(FILE * fp, Mutex * mutex, const char * fmt, ...);
#else // !WITH_THREAD
#define atomic_printf printf
#define atomic_fprintf fprintf
#endif // !WITH_THREAD


extern uint64_t random64();
extern std::string strprintf(const char * fmt, ...);

extern bool parse_size(const char * s, ssize_t * n);

extern unsigned long long get_realtime_us();
extern unsigned long long get_cputime_us();


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


/* 
 * reverse byte order
 */

#define GETBYTE(v, n) ((v) >> ((n)*8) & 0xff)
#define SETBYTE(v, n) ((v) << ((n)*8))

inline uint32_t reverse_byte_order(uint16_t a)
{
	return    SETBYTE(GETBYTE(a, 0), 1)
		| SETBYTE(GETBYTE(a, 1), 0);
}

inline uint32_t reverse_byte_order(uint32_t a)
{
	return    SETBYTE(GETBYTE(a, 0), 3)
		| SETBYTE(GETBYTE(a, 1), 2)
		| SETBYTE(GETBYTE(a, 2), 1)
		| SETBYTE(GETBYTE(a, 3), 0);
}

inline uint64_t reverse_byte_order(uint64_t a)
{
	return    SETBYTE(GETBYTE(a, 0), 7)
		| SETBYTE(GETBYTE(a, 1), 6)
		| SETBYTE(GETBYTE(a, 2), 5)
		| SETBYTE(GETBYTE(a, 3), 4)
		| SETBYTE(GETBYTE(a, 4), 3)
		| SETBYTE(GETBYTE(a, 5), 2)
		| SETBYTE(GETBYTE(a, 6), 1)
		| SETBYTE(GETBYTE(a, 7), 0);
}

#undef GETBYTE
#undef SETBYTE				     


extern unsigned long long _timing_trace_us_start;
extern unsigned long long _timing_trace_us_last;

#define TIMING_TRACE_INIT() do { \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	_timing_trace_us_start = (unsigned long long) tv.tv_sec * (int)1E6 + tv.tv_usec; \
	_timing_trace_us_last = _timing_trace_us_start; \
	printf("%s:%d(%s) @%llu us +%llu us\n", \
		__FILE__, __LINE__, __func__, \
		0ULL, 0ULL); \
} while (0)

#define TIMING_TRACE() do { \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	unsigned long long us = (unsigned long long) tv.tv_sec * (int)1E6 + tv.tv_usec; \
	printf("%s:%d(%s) @%llu us +%llu us\n", \
		__FILE__, __LINE__, __func__, \
		us-_timing_trace_us_start, us-_timing_trace_us_last); \
	_timing_trace_us_last = us; \
} while (0)



#endif // UTIL_H
