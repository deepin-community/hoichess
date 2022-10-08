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
 * Bitboard utility functions.
 *
 *****************************************************************************/

void Bitboard::print() const
{
	for (int r=0; r<8; r++) {
		printf("%2d..%2d  ", r*8, r*8+7);
		for (int c=0; c<8; c++) {
			if (testbit(r*8 + c)) {
				printf(" X");
			} else {
				printf(" +");
			}
		}
		printf("\n");
	}
}

void Bitboard::print2() const
{
	for (int r=7; r>=0; r--) {
		printf("%d ", r+1);
		for (int f=0; f<8; f++) {
			if (testbit(SQUARE(r, f))) {
				printf(" X");
			} else {
				printf(" +");
			}
		}
		printf("\n");
	}
	printf("   a b c d e f g h\n");
}

/*****************************************************************************
 *
 * Bitboard initialization.
 *
 *****************************************************************************/

int8_t Bitboard::lsb_lut[65536];
int8_t Bitboard::msb_lut[65536];
int8_t Bitboard::popcnt_lut[65536];

Bitboard Bitboard::file[8];
Bitboard Bitboard::rank[8];
Bitboard Bitboard::attack_bb[6][64];
Bitboard Bitboard::pawn_capt_bb[2][64];
Bitboard Bitboard::ray_bb[64][64];
Bitboard Bitboard::passed_pawn_mask[2][64];
Bitboard Bitboard::isolated_pawn_mask[64];
Bitboard Bitboard::connected_pawn_mask[64];


void Bitboard::init()
{
	init_luts();
	init_attack_bb();
	init_pawn_capt_bb();
	init_ray_bb();
	init_masks();
	init_rot_atk();
}

void Bitboard::init_luts()
{
	int i;
	
	/* lsb position lookup table */
	for (i=0; i<65536; i++) {
		lsb_lut[i] = -1;
		for (int j=0; j<16; j++) {
			if (i & (1<<j)) {
				lsb_lut[i] = (int8_t) j;
				break;
			}
		}
	}
	
	/* msb position lookup table */
	for (i=0; i<65536; i++) {
		msb_lut[i] = -1;
		for (int j=15; j>=0; j--) {
			if (i & (1<<j)) {
				msb_lut[i] = (int8_t) j;
				break;
			}
		}
	}

	/* population count lookup table */
	for (i=0; i<65536; i++) {
		popcnt_lut[i] = 0;
		for (int j=0; j<16; j++) {
			if (i & (1<<j))
				popcnt_lut[i]++;
		}		
	}
	
	/* file bitboards */
	for (int f=0; f<8; f++) {
		file[f] = NULLBITBOARD;
		for (int r=0; r<8; r++) {
			file[f].setbit(SQUARE(r,f));
		}
	}
	
	/* rank bitboards */
	for (int r=0; r<8; r++) {
		rank[r] = NULLBITBOARD;
		for (int f=0; f<8; f++) {
			rank[r].setbit(SQUARE(r,f));
		}
	}
}

/*
 * These are used in the initialization functions together
 * with the '0x88' technique to initialize the various
 * constant bitboards.
 * No need to have them in the Bitboard class namespace.
 */
static int map0x88[] = {
	 0,  1,  2,  3,  4,  5,  6,  7, -1, -1, -1, -1, -1, -1, -1, -1,
	 8,  9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1,
	16, 17, 18, 19, 20, 21, 22, 23, -1, -1, -1, -1, -1, -1, -1, -1,
	24, 25, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1,
	32, 33, 34, 35, 36, 37, 38, 39, -1, -1, -1, -1, -1, -1, -1, -1,
	40, 41, 42, 43, 44, 45, 46, 47, -1, -1, -1, -1, -1, -1, -1, -1,
	48, 49, 50, 51, 52, 53, 54, 55, -1, -1, -1, -1, -1, -1, -1, -1,
	56, 57, 58, 59, 60, 61, 62, 63, -1, -1, -1, -1, -1, -1, -1, -1
};
static int knight_dir0x88[] = { -31, -33, 31, 33, -18, 18, -14, 14 };
static int king_dir0x88[] = { -1, 1, -16, 16, -15, 15, -17, 17 };
static int bishop_dir0x88[] = { -15, 15, -17, 17 };
static int rook_dir0x88[] = { -1, 1, -16, 16 };

void Bitboard::init_attack_bb()
{
	Square from, to;
	int i;
		
	for (int f=0; f<128; f++) {
		if (f & 0x88)
			continue;

		from = map0x88[f];

		/* attack_bb[PAWN][] is unused */
		
		/* knight */
		attack_bb[KNIGHT][from] = NULLBITBOARD;
		for (i=0; i<8; i++) {
			int t = f+knight_dir0x88[i];
			if (t & 0x88)
				continue;
			to = map0x88[t];
			attack_bb[KNIGHT][from].setbit(to);
		}

		/* bishop */
		attack_bb[BISHOP][from] = NULLBITBOARD;
		for (i=0; i<4; i++) {
			int t = f+bishop_dir0x88[i];
			while (!(t & 0x88)) {
				to = map0x88[t];
				attack_bb[BISHOP][from].setbit(to);
				t += bishop_dir0x88[i];
			}
		}
		
		/* rook */
		attack_bb[ROOK][from] = NULLBITBOARD;
		for (i=0; i<4; i++) {
			int t = f+rook_dir0x88[i];
			while (!(t & 0x88)) {
				to = map0x88[t];
				attack_bb[ROOK][from].setbit(to);
				t += rook_dir0x88[i];
			}
		}

		/* queen */
		attack_bb[QUEEN][from] = attack_bb[BISHOP][from] 
				| attack_bb[ROOK][from];

		/* king */
		attack_bb[KING][from] = NULLBITBOARD;
		for (i=0; i<8; i++) {
			int t = f+king_dir0x88[i];
			if (t & 0x88)
				continue;
			to = map0x88[t];
			attack_bb[KING][from].setbit(to);
		}
	}
}

void Bitboard::init_pawn_capt_bb()
{
	Square from, to;
	int t;

	for (int f=0; f<128; f++) {
		if (f & 0x88)
			continue;

		from = map0x88[f];

		/* white pawns */
		pawn_capt_bb[WHITE][from] = NULLBITBOARD;
		t = f+15;
		if (!(t & 0x88)) {
			to = map0x88[t];
			pawn_capt_bb[WHITE][from].setbit(to);
		}
		t = f+17;
		if (!(t & 0x88)) {
			to = map0x88[t];
			pawn_capt_bb[WHITE][from].setbit(to);
		}

		/* black pawns */
		pawn_capt_bb[BLACK][from] = NULLBITBOARD;
		t = f-15;
		if (!(t & 0x88)) {
			to = map0x88[t];
			pawn_capt_bb[BLACK][from].setbit(to);
		}
		t = f-17;
		if (!(t & 0x88)) {
			to = map0x88[t];
			pawn_capt_bb[BLACK][from].setbit(to);
		}
	}
}

/*
 * Make sure that attack_bb[BISHOP][] is initialized
 * before calling this function.
 */
void Bitboard::init_ray_bb()
{
	Square from, to, sq;

	for (int f=0; f<128; f++) {
		if (f & 0x88)
			continue;

		from = map0x88[f];

		for (int t=0; t<128; t++) {
			if (t & 0x88)
				continue;

			to = map0x88[t];

			ray_bb[from][to] = NULLBITBOARD;

			int dir;
			if (RNK(from) == RNK(to) 
					&& FIL(from) < FIL(to)) {
				dir = 1;
			} else if (RNK(from) == RNK(to)
					&& FIL(from) > FIL(to)) {
				dir = -1;
			} else if (RNK(from) < RNK(to)
					&& FIL(from) == FIL(to)) {
				dir = 16;
			} else if (RNK(from) > RNK(to)
					&& FIL(from) == FIL(to)) {
				dir = -16;
			} else if (RNK(from) < RNK(to)
					&& FIL(from) < FIL(to)
					&& attack_bb[BISHOP][from]
							.testbit(to)) {
				dir = 17;
			} else if (RNK(from) < RNK(to)
					&& FIL(from) > FIL(to)
					&& attack_bb[BISHOP][from]
							.testbit(to)) {
				dir = 15;
			} else if (RNK(from) > RNK(to)
					&& FIL(from) < FIL(to)
					&& attack_bb[BISHOP][from]
							.testbit(to)) {
				dir = -15;
			} else if (RNK(from) > RNK(to)
					&& FIL(from) > FIL(to)
					&& attack_bb[BISHOP][from]
							.testbit(to)) {
				dir = -17;
			} else {
				continue;
			}

			int s = f+dir;
			while (s != t) {
				sq = map0x88[s];
				ray_bb[from][to].setbit(sq);
				s += dir;
			}				
		}
	}			
}

void Bitboard::init_masks()
{
	for (Square sq=A1; sq<=H8; sq++) {
		int r, f;
	

		/* passed_pawn_mask[WHITE]:
		 *  + + X X X + + +
		 *  + + X X X + + +
		 *  + + + P + + + + 
		 *  + + + + + + + +
		 *  + + + + + + + +
		 */
		passed_pawn_mask[WHITE][sq] = NULLBITBOARD;
		for (r=RNK(sq)+1; r<=7; r++) {
			f = FIL(sq);
			
			passed_pawn_mask[WHITE][sq].setbit(SQUARE(r, f));
			if (f > 0) {
				passed_pawn_mask[WHITE][sq]
					.setbit(SQUARE(r, f-1));
			}
			if (f < 7) {
				passed_pawn_mask[WHITE][sq]
					.setbit(SQUARE(r, f+1));
			}
		}

		/* passed_pawn_mask[BLACK] ... */
		passed_pawn_mask[BLACK][sq] = NULLBITBOARD;
		for (r=RNK(sq)-1; r>=0; r--) {
			f = FIL(sq);
			
			passed_pawn_mask[BLACK][sq].setbit(SQUARE(r, f));
			if (f > 0) {
				passed_pawn_mask[BLACK][sq]
					.setbit(SQUARE(r, f-1));
			}
			if (f < 7) {
				passed_pawn_mask[BLACK][sq]
					.setbit(SQUARE(r, f+1));
			}
		}
	
		/* isolated_pawn_mask:
		 *  + + X X X + + +
		 *  + + X X X + + +
		 *  + + X P X + + + 
		 *  + + X X X + + +
		 *  + + X X X + + +
		 */
		f = FIL(sq);
		isolated_pawn_mask[sq] = file[f];
		if (f > 0) {
			isolated_pawn_mask[sq] |= file[f-1];
		}
		if (f < 7) {
			isolated_pawn_mask[sq] |= file[f+1];
		}

		/* connected_pawn_mask:
		 *  + + + + + + + +
		 *  + + X + X + + +
		 *  + + X P X + + + 
		 *  + + X + X + + +
		 *  + + + + + + + +
		 */
		connected_pawn_mask[sq] = attack_bb[KING][sq]
			& ~file[FIL(sq)];
	}
}

