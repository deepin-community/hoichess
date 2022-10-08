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

#include "common.h"
#include "bitboard.h"


/*****************************************************************************
 *
 * Stuff needed for rotated bitboards.
 *
 *****************************************************************************/

const unsigned int Bitboard::_shift_0[] = {
	 0,  0,  0,  0,  0,  0,  0,  0,
	 8,  8,  8,  8,  8,  8,  8,  8,
	16, 16, 16, 16, 16, 16, 16, 16,
	24, 24, 24, 24, 24, 24, 24, 24,
	32, 32, 32, 32, 32, 32, 32, 32,
	40, 40, 40, 40, 40, 40, 40, 40,
	48, 48, 48, 48, 48, 48, 48, 48,
	56, 56, 56, 56, 56, 56, 56, 56
};

const unsigned int Bitboard::_map_l90[] = {
	56, 48, 40, 32, 24, 16,  8,  0,
	57, 49, 41, 33, 25, 17,  9,  1,
	58, 50, 42, 34, 26, 18, 10,  2,
	59, 51, 43, 35, 27, 19, 11,  3,
	60, 52, 44, 36, 28, 20, 12,  4,
	61, 53, 45, 37, 29, 21, 13,  5,
	62, 54, 46, 38, 30, 22, 14,  6,
	63, 55, 47, 39, 31, 23, 15,  7
};

const unsigned int Bitboard::_inv_map_l90[] = {
	H1, H2, H3, H4, H5, H6, H7, H8,
	G1, G2, G3, G4, G5, G6, G7, G8,
	F1, F2, F3, F4, F5, F6, F7, F8,
	E1, E2, E3, E4, E5, E6, E7, E8,
	D1, D2, D3, D4, D5, D6, D7, D8,
	C1, C2, C3, C4, C5, C6, C7, C8,
	B1, B2, B3, B4, B5, B6, B7, B8,
	A1, A2, A3, A4, A5, A6, A7, A8
};

const unsigned int Bitboard::_shift_l90[] = {
	56, 48, 40, 32, 24, 16,  8,  0,
	56, 48, 40, 32, 24, 16,  8,  0,
	56, 48, 40, 32, 24, 16,  8,  0,
	56, 48, 40, 32, 24, 16,  8,  0,
	56, 48, 40, 32, 24, 16,  8,  0,
	56, 48, 40, 32, 24, 16,  8,  0,
	56, 48, 40, 32, 24, 16,  8,  0,
	56, 48, 40, 32, 24, 16,  8,  0
};

const unsigned int Bitboard::_map_l45[] = {
	28, 21, 15, 10,  6,  3,  1,  0,
	36, 29, 22, 16, 11,  7,  4,  2,
	43, 37, 30, 23, 17, 12,  8,  5,
	49, 44, 38, 31, 24, 18, 13,  9,
	54, 50, 45, 39, 32, 25, 19, 14,
	58, 55, 51, 46, 40, 33, 26, 20,
	61, 59, 56, 52, 47, 41, 34, 27,
	63, 62, 60, 57, 53, 48, 42, 35
};

const unsigned int Bitboard::_inv_map_l45[] = {
			H1,
		      G1, H2,
		    F1, G2, H3,
		  E1, F2, G3, H4,
		D1, E2, F3, G4, H5,
              C1, D2, E3, F4, G5, H6,
	    B1, C2, D3, E4, F5, G6, H7,
	  A1, B2, C3, D4, E5, F6, G7, H8,
	    A2, B3, C4, D5, E6, F7, G8,
	      A3, B4, C5, D6, E7, F8,
	        A4, B5, C6, D7, E8,
		  A5, B6, C7, D8,
		    A6, B7, C8,
		      A7, B8,
		        A8
};

const unsigned int Bitboard::_shift_l45[] = {
	28, 21, 15, 10,  6,  3,  1,  0,
	36, 28, 21, 15, 10,  6,  3,  1,
	43, 36, 28, 21, 15, 10,  6,  3,
	49, 43, 36, 28, 21, 15, 10,  6,
	54, 49, 43, 36, 28, 21, 15, 10,
	58, 54, 49, 43, 36, 28, 21, 15,
	61, 58, 54, 49, 43, 36, 28, 21,
	63, 61, 58, 54, 49, 43, 36, 28
};

const unsigned int Bitboard::_diaglen_l45[] = {
	 8,  7,  6,  5,  4,  3,  2,  1,
	 7,  8,  7,  6,  5,  4,  3,  2,
	 6,  7,  8,  7,  6,  5,  4,  3,
	 5,  6,  7,  8,  7,  6,  5,  4,
	 4,  5,  6,  7,  8,  7,  6,  5,
	 3,  4,  5,  6,  7,  8,  7,  6,
	 2,  3,  4,  5,  6,  7,  8,  7,
	 1,  2,  3,  4,  5,  6,  7,  8
};

const unsigned int Bitboard::_diagmask_l45[] = {
	0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01,
	0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03,
	0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07,
	0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f,
	0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,
	0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f,
	0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f,
	0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};

const unsigned int Bitboard::_map_r45[] = {
	 0,  2,  5,  9, 14, 20, 27, 35,
	 1,  4,  8, 13, 19, 26, 34, 42,
	 3,  7, 12, 18, 25, 33, 41, 48,
	 6, 11, 17, 24, 32, 40, 47, 53,
	10, 16, 23, 31, 39, 46, 52, 57,
	15, 22, 30, 38, 45, 51, 56, 60,
	21, 29, 37, 44, 50, 55, 59, 62,
	28, 36, 43, 49, 54, 58, 61, 63
};

const unsigned int Bitboard::_inv_map_r45[] = {		
                        A1,
		      A2, B1,
		    A3, B2, C1,
		  A4, B3, C2, D1,
		A5, B4, C3, D2, E1,
              A6, B5, C4, D3, E2, F1,
	    A7, B6, C5, D4, E3, F2, G1,
	  A8, B7, C6, D5, E4, F3, G2, H1,
	    B8, C7, D6, E5, F4, G3, H2,
	      C8, D7, E6, F5, G4, H3,
	        D8, E7, F6, G5, H4,
		  E8, F7, G6, H5,
		    F8, G7, H6,
		      G8, H7,
		        H8
};

const unsigned int Bitboard::_shift_r45[] = {
	 0,  1,  3,  6, 10, 15, 21, 28,
	 1,  3,  6, 10, 15, 21, 28, 36,
	 3,  6, 10, 15, 21, 28, 36, 43,
	 6, 10, 15, 21, 28, 36, 43, 49,
	10, 15, 21, 28, 36, 43, 49, 54,
	15, 21, 28, 36, 43, 49, 54, 58,
	21, 28, 36, 43, 49, 54, 58, 61,
	28, 36, 43, 49, 54, 58, 61, 63
};

const unsigned int Bitboard::_diaglen_r45[] = {
	 1,  2,  3,  4,  5,  6,  7,  8,
	 2,  3,  4,  5,  6,  7,  8,  7,
	 3,  4,  5,  6,  7,  8,  7,  6,
	 4,  5,  6,  7,  8,  7,  6,  5,
	 5,  6,  7,  8,  7,  6,  5,  4,
	 6,  7,  8,  7,  6,  5,  4,  3,
	 7,  8,  7,  6,  5,  4,  3,  2,
	 8,  7,  6,  5,  4,  3,  2,  1
};

const unsigned int Bitboard::_diagmask_r45[] = {
	0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
	0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f,
	0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f,
	0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,
	0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f,
	0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07,
	0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03,
	0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};

Bitboard Bitboard::rot_atk_0[64][256];
Bitboard Bitboard::rot_atk_l90[64][256];
Bitboard Bitboard::rot_atk_l45[64][256];
Bitboard Bitboard::rot_atk_r45[64][256];

void Bitboard::init_rot_atk()
{
	Bitboard tmp;
	unsigned int f, t;
	Square to;
	
	for (Bitboard occ=((uint64_t) 0); occ<((uint64_t) 256); occ++) {
		for (Square from=A1; from<=H8; from++) {
			
			/* rot_atk_0[][] */
			tmp = NULLBITBOARD;
			f = from % 8;
			for (t=0; t<8; t++) {
				if (occ & ray_bb[f][t] || f == t)
					continue;
				
				tmp.setbit(t);
			}
			rot_atk_0[from][occ] = tmp << shift_0(from);

			/* rot_atk_l90[][] */
			tmp = NULLBITBOARD;
			f = map_l90(from) % 8;
			for (t=0; t<8; t++) {
				if (occ & ray_bb[f][t] || f == t)
					continue;

				to = inv_map_l90(shift_l90(from) + t);
				tmp.setbit(to);
			}
			rot_atk_l90[from][occ] = tmp;

			/* rot_atk_l45[][] */
			tmp = NULLBITBOARD;
			f = map_l45(from) - shift_l45(from);
			for (t=0; t<diaglen_l45(from); t++) {
				if (occ & ray_bb[f][t] || f == t)
					continue;

				to = inv_map_l45(shift_l45(from) + t);
				tmp.setbit(to);
			}				
			rot_atk_l45[from][occ] = tmp;
			
			/* rot_atk_r45[][] */
			tmp = NULLBITBOARD;
			f = map_r45(from) - shift_r45(from);
			for (t=0; t<diaglen_r45(from); t++) {
				if (occ & ray_bb[f][t] || f == t)
					continue;

				to = inv_map_r45(shift_r45(from) + t);
				tmp.setbit(to);
			}				
			rot_atk_r45[from][occ] = tmp;
		}
	}
}
