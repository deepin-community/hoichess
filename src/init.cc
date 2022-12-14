/* Copyright (C) 2004-2007 Holger Ruckdeschel <holger@hoicher.de>
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
#include "basic.h"
#include "board.h"

#include <time.h>

#ifdef __leonbare__
extern "C" {
# include <asm-leon/timer.h>
# define SOLARIS_NP
# include <fsu_pthread.h>
}
#endif



/*****************************************************************************
 * 
 * This is the main initialization function.
 * Make sure it is called before doing anything else.
 *
 *****************************************************************************/

void init()
{
#ifdef __leonbare__
	//leonbare_init_ticks();
	pthread_init();
#endif

#ifdef HOICHESS
	Bitboard::init();
#endif
	Board::init();

	srand(time(NULL));

	
#ifdef WIN32
	/* Completely disable I/O buffering on WIN32 platforms. */
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
#endif
}


/*****************************************************************************
 * 
 * Default builtin initial commands.
 * Declared with weak linkage so that they can be overridden at build
 * time.
 *
 * This is for embedded systems where command line arguments (e.g. --initcmd)
 * or script files are not available.
 *
 *****************************************************************************/

const char * builtin_initcmds[]
#ifndef WIN32
__attribute__((weak))
#endif
	= { NULL }; /* {
	"echo default builtin initcmd",
	NULL
};*/
