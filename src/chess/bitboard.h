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
#ifndef BITBOARD_H
#define BITBOARD_H

#include "common.h"
#include "basic.h"


#define NULLBITBOARD	(Bitboard(0ULL))
#define LIGHTBITBOARD	(Bitboard(0x55aa55aa55aa55aaULL))
#define DARKBITBOARD	(Bitboard(0xaa55aa55aa55aa55ULL))


class Bitboard
{
      private:
	uint64_t bits;

	/* Constructors */
      public:
	FORCEINLINE Bitboard();
	FORCEINLINE Bitboard(uint64_t bits);

	/* Operators / Casts */
      public:
	FORCEINLINE Bitboard & operator=(const Bitboard & bb);
	FORCEINLINE operator uint64_t() const;

	FORCEINLINE Bitboard operator&(const Bitboard & bb) const;
	FORCEINLINE Bitboard operator|(const Bitboard & bb) const;
	FORCEINLINE Bitboard operator~() const;
	FORCEINLINE Bitboard & operator&=(const Bitboard & bb);
	FORCEINLINE Bitboard & operator|=(const Bitboard & bb);
	FORCEINLINE Bitboard & operator++();
	FORCEINLINE Bitboard operator++(int);

	/* Basic bitboard functions */
      public:
	FORCEINLINE void setbit(unsigned int b);
	FORCEINLINE void clearbit(unsigned int b);
	FORCEINLINE bool testbit(unsigned int b) const;
	
	inline int firstbit() const;
	inline int lsb() const;
	inline int msb() const;
	inline int popcnt() const;

	/* Rotated attack functions */
      public:
	inline Bitboard atk0(Square from) const;
	inline Bitboard atkl90(Square from) const;
	inline Bitboard atkl45(Square from) const;
	inline Bitboard atkr45(Square from) const;
	
	/* Utility functions */
      public:
	void print() const;
	void print2() const;

	/* Static Data Members */
      public:
	static Bitboard file[8];
	static Bitboard rank[8];
	static Bitboard attack_bb[6][64];
	static Bitboard pawn_capt_bb[2][64];
	static Bitboard ray_bb[64][64];
	
	static Bitboard passed_pawn_mask[2][64];
	static Bitboard isolated_pawn_mask[64];
	static Bitboard connected_pawn_mask[64];

      private:
	static int8_t lsb_lut[65536];
	static int8_t msb_lut[65536];
	static int8_t popcnt_lut[65536];
	
	static const unsigned int _map_l90[64];
	static const unsigned int _map_l45[64];
	static const unsigned int _map_r45[64];
	static const unsigned int _inv_map_l90[64];
	static const unsigned int _inv_map_l45[64];
	static const unsigned int _inv_map_r45[64];
	static const unsigned int _shift_0[64];
	static const unsigned int _shift_l90[64];
	static const unsigned int _shift_l45[64];
	static const unsigned int _shift_r45[64];
	static const unsigned int _diaglen_l45[64];
	static const unsigned int _diaglen_r45[64];
	static const unsigned int _diagmask_l45[64];
	static const unsigned int _diagmask_r45[64];
	static Bitboard rot_atk_0[64][256];
	static Bitboard rot_atk_l90[64][256];
	static Bitboard rot_atk_l45[64][256];
	static Bitboard rot_atk_r45[64][256];
	
	/* Static Member Functions */
      public:
	static void init();
      private:
	static void init_luts();
	static void init_attack_bb();
	static void init_pawn_capt_bb();
	static void init_ray_bb();
	static void init_masks();
	static void init_rot_atk();

      public:
	static inline unsigned int map_l90(unsigned int a);
	static inline unsigned int map_l45(unsigned int a);
	static inline unsigned int map_r45(unsigned int a);
	static inline unsigned int inv_map_l90(unsigned int b);
	static inline unsigned int inv_map_l45(unsigned int b);
	static inline unsigned int inv_map_r45(unsigned int b);
	static inline unsigned int shift_0(unsigned int a);
	static inline unsigned int shift_l90(unsigned int a);
	static inline unsigned int shift_l45(unsigned int a);
	static inline unsigned int shift_r45(unsigned int a);
	static inline unsigned int diaglen_l45(unsigned int a);
	static inline unsigned int diaglen_r45(unsigned int a);
	static inline unsigned int diagmask_l45(unsigned int a);
	static inline unsigned int diagmask_r45(unsigned int a);
};

#include "bitboard_inlines.h"

#endif // BITBOARD_H
