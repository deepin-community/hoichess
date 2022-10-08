/* Copyright (C) 2005-2011 Holger Ruckdeschel <holger@hoicher.de>
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

/*
 * x86_64 assembler versions of msb/lsb scan routines.
 * This code was taken from Crafty and slightly modified to match
 * our interpretation of the bit positions.
 */

#ifdef USE_ASM_LSB
inline int Bitboard::lsb() const
{
	uint64_t dummy1, dummy2;
	asm("    bsfq    %1, %0"      "\n\t"
	    "    jnz     1f"          "\n\t"
	    "    movq    $-1, %0"     "\n\t"
	    "1:"
		: "=&r" (dummy1), "=&r" (dummy2)
		: "1" ((uint64_t) (bits))
		: "cc");
	
	return (dummy1);
}
#endif

#ifdef USE_ASM_MSB
inline int Bitboard::msb() const
{ 
	uint64_t dummy1, dummy2;
	asm("    bsrq    %1, %0"      "\n\t"
	    "    jnz     1f"          "\n\t"
	    "    movq    $-1, %0"     "\n\t"
	    "1:"
		: "=&r" (dummy1), "=&r" (dummy2)
		: "1" ((uint64_t) (bits))
		: "cc");
	
	return (dummy1);
}
#endif

#ifdef USE_ASM_POPCNT
#error "asm popcnt() not implemented"
#endif
