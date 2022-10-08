/* Copyright (C) 2004-2008 Holger Ruckdeschel <holger@hoicher.de>
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
#ifndef CONFIG_H
#define CONFIG_H

/* Use assembler code where supported */
#define USE_ASM

/* Use a single board in search tree, in connection with Board::unmake_move().
 * Otherwise, the board will be copied from node to node each time a move is
 * made. */
//#define USE_UNMAKE_MOVE

/* Maximum depth for full-width search */
#define MAXDEPTH		900

/* Maximum search tree depth (full-width + quiescence search) */
#define MAXPLY			1024

/* Maximum size of a Movelist */
#define MOVELIST_MAXSIZE	256

#endif // CONFIG_H
