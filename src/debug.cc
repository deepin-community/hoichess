/* Copyright (C) 2005 Holger Ruckdeschel <holger@hoicher.de>
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
#include "debug.h"
#ifdef WITH_THREAD
# include "mutex.h"
# include "thread.h"
#endif
#include "eval.h"	/* for debug_print_storagesizes() */
#include "node.h"	/* for debug_print_storagesizes() */
#include "util.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>


void __debug_helper::__dbg(unsigned int level, const char * fmt, ...)
{
	if (debug < level) {
		return;
	}

#ifdef WITH_THREAD
	if (mutex) {
		mutex->lock();
	}
#endif
	
	/* write message to fp */
	fprintf(fp, "debug[%d]: %s:%d: %s: ", level, file, line, function);
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	if (fmt[strlen(fmt)-1] != '\n') {
		fprintf(fp, "\n");
	}

#ifdef WITH_THREAD
	if (mutex) {
		mutex->unlock();
	}
#endif
}

void __debug_helper::__warn(const char * fmt, ...)
{
#ifdef WITH_THREAD
	if (mutex) {
		mutex->lock();
	}
#endif
	
	/* write message to fp */
	fprintf(fp, "warning: %s:%d: %s: ", file, line, function);
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	if (fmt[strlen(fmt)-1] != '\n') {
		fprintf(fp, "\n");
	}

#ifdef WITH_THREAD
	if (mutex) {
		mutex->unlock();
	}
#endif
}

void __debug_helper::__bug(const char * fmt, ...)
{
#ifdef WITH_THREAD
	if (mutex) {
		mutex->lock();
	}
#endif

	/* write message to fp */
	fprintf(fp, "bug: %s:%d: %s: ", file, line, function);
	va_list args;
	va_start(args, fmt);
	vfprintf(fp, fmt, args);
	va_end(args);
	if (fmt[strlen(fmt)-1] != '\n') {
		fprintf(fp, "\n");
	}

	fprintf(fp, "This is a bug in %s. Please report it to %s.\n",
			PROGNAME, AUTHOR_EMAIL);

#ifdef WITH_THREAD
	if (mutex) {
		mutex->unlock();
	}
#endif

	raise(SIGABRT);
	exit(EXIT_FAILURE);
}
