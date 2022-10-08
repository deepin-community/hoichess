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
#ifndef BITBOARD_INLINES_H
#define BITBOARD_INLINES_H

#include "common.h"
#include "bitboard.h"


#if defined(USE_ASM) && defined(__GNUC__) && defined(__i386__)
# define USE_ASM_LSB
# define USE_ASM_MSB
//# define USE_ASM_POPCNT	// LUT version is faster
# define USE_LUT_POPCNT
# include "i386/bitboard_asm.h"
# define BITBOARD_FIRSTBIT msb
#elif defined(USE_ASM) && defined(__GNUC__) && defined(__x86_64__)
# define USE_ASM_LSB
# define USE_ASM_MSB
//# define USE_ASM_POPCNT	// not implemented
# define USE_LUT_POPCNT
# include "x86_64/bitboard_asm.h"
# define BITBOARD_FIRSTBIT msb
#elif defined(USE_ASM) && defined(__WIN32__) && !defined(__WIN64__)
# define USE_ASM_LSB
# define USE_ASM_MSB
//# define USE_ASM_POPCNT	// not implemented
# define USE_LUT_POPCNT
# include "win32/bitboard_asm.h"
# define BITBOARD_FIRSTBIT msb
#elif defined(__GNUC__)
# define USE_BUILTIN_LSB
# define USE_BUILTIN_MSB
//# define USE_BUILTIN_POPCNT	// LUT version is faster 
				// (at least on i386 and amd64: +12% nps)
# define USE_LUT_POPCNT
# define BITBOARD_FIRSTBIT lsb
#else
# define USE_LUT_LSB
# define USE_LUT_MSB
# define USE_LUT_POPCNT
# define BITBOARD_FIRSTBIT msb
#endif


/*****************************************************************************
 * Static Functions.
 */

inline unsigned int Bitboard::map_l90(unsigned int a)
{
	unsigned int b = (((7-a)&0x07)<<3) | ((a&0x38)>>3);
	ASSERT_DEBUG(b == _map_l90[a]);
	return b;
	//return _map_l90[a];
}

inline unsigned int Bitboard::map_l45(unsigned int a)
{
	return _map_l45[a];
}

inline unsigned int Bitboard::map_r45(unsigned int a)
{
	return _map_r45[a];
}

inline unsigned int Bitboard::inv_map_l90(unsigned int b)
{
	unsigned int a = ((b&0x07)<<3) | (7-((b&0x38)>>3));
	ASSERT_DEBUG(a == _inv_map_l90[b]);
	return a;
	//return _inv_map_l90[b];
}

inline unsigned int Bitboard::inv_map_l45(unsigned int b)
{
	return _inv_map_l45[b];
}

inline unsigned int Bitboard::inv_map_r45(unsigned int b)
{
	return _inv_map_r45[b];
}

inline unsigned int Bitboard::shift_0(unsigned int a)
{
	unsigned int s = (a&0x38);
	ASSERT_DEBUG(s == _shift_0[a]);
	return s;
	//return _shift_0[a];
}

inline unsigned int Bitboard::shift_l90(unsigned int a)
{
	unsigned int s = (((7-a)&0x07)<<3);
	ASSERT_DEBUG(s == _shift_l90[a]);
	return s;
	//return _shift_l90[a];
}

inline unsigned int Bitboard::shift_l45(unsigned int a)
{
	return _shift_l45[a];
}

inline unsigned int Bitboard::shift_r45(unsigned int a)
{
	return _shift_r45[a];
}

inline unsigned int Bitboard::diaglen_l45(unsigned int a)
{
	return _diaglen_l45[a];
}

inline unsigned int Bitboard::diaglen_r45(unsigned int a)
{
	return _diaglen_r45[a];
}

inline unsigned int Bitboard::diagmask_l45(unsigned int a)
{
	return _diagmask_l45[a];
}

inline unsigned int Bitboard::diagmask_r45(unsigned int a)
{
	return _diagmask_r45[a];
}





/*****************************************************************************
 * Non-static Functions.
 */

inline Bitboard::Bitboard()
	: bits((uint64_t) 0)
{}

inline Bitboard::Bitboard(uint64_t _bits)
	: bits(_bits)
{}


inline Bitboard & Bitboard::operator=(const Bitboard & bb)
{
	bits = bb.bits;
	return *this;
}

inline Bitboard::operator uint64_t() const
{
	return bits;
}


inline Bitboard Bitboard::operator&(const Bitboard & bb) const
{
	return bits & bb.bits;
}

inline Bitboard Bitboard::operator|(const Bitboard & bb) const
{
	return bits | bb.bits;
}

inline Bitboard Bitboard::operator~() const
{
	return ~bits;
}

inline Bitboard & Bitboard::operator&=(const Bitboard & bb)
{
	bits &= bb.bits;
	return *this;
}

inline Bitboard & Bitboard::operator|=(const Bitboard & bb)
{
	bits |= bb.bits;
	return *this;
}

/* This is the prefix operator++ */
inline Bitboard & Bitboard::operator++()
{
	bits++;
	return *this;
}

/* This is the postfix operator++ */
inline Bitboard Bitboard::operator++(int)
{
	Bitboard tmp = *this;
	bits++;
	return tmp;
}


inline void Bitboard::setbit(unsigned int b)
{
	bits |= (((uint64_t) 1) << b);
}

inline void Bitboard::clearbit(unsigned int b)
{
	bits &= ~(((uint64_t) 1) << b);
}

inline bool Bitboard::testbit(unsigned int b) const
{
	return (bits & (((uint64_t) 1) << b));
}


inline int Bitboard::firstbit() const
{
	return BITBOARD_FIRSTBIT();
}


/*
 * Lookup table version of msb/lsb scan routines.
 * This code was taken from GNU Chess.
 */

#ifdef USE_LUT_LSB
inline int Bitboard::lsb() const
{
	if (bits & 0xffff) return lsb_lut[bits & 0xffff];
	if ((bits >> 16) & 0xffff) return lsb_lut[(bits >> 16) & 0xffff] + 16;
	if ((bits >> 32) & 0xffff) return lsb_lut[(bits >> 32) & 0xffff] + 32;
	if ((bits >> 48) & 0xffff) return lsb_lut[(bits >> 48) & 0xffff] + 48;
	return -1;
}
#endif

#ifdef USE_LUT_MSB
inline int Bitboard::msb() const
{
	if (bits >> 48) return msb_lut[bits >> 48] + 48;
	if (bits >> 32) return msb_lut[bits >> 32] + 32;
	if (bits >> 16) return msb_lut[bits >> 16] + 16;
	return msb_lut[bits];
}
#endif

/*
 * Lookup table version of population count routine.
 * This code was taken from GNU Chess.
 */

#ifdef USE_LUT_POPCNT
inline int Bitboard::popcnt() const
{
	return popcnt_lut[bits & 0xffff]
		+ popcnt_lut[(bits >> 16) & 0xffff]
		+ popcnt_lut[(bits >> 32) & 0xffff]
		+ popcnt_lut[(bits >> 48) & 0xffff];
}
#endif


#ifdef USE_BUILTIN_LSB
inline int Bitboard::lsb() const
{
	if (bits) {
		return __builtin_ctzll(bits);
	} else {
		return -1;
	}
}
#endif

#ifdef USE_BUILTIN_MSB
inline int Bitboard::msb() const
{
	if (bits) {
		return 63 - __builtin_clzll(bits);
	} else {
		return -1;
	}
}
#endif

#ifdef USE_BUILTIN_POPCNT
inline int Bitboard::popcnt() const
{
	return __builtin_popcountll(bits);
}
#endif


/*
 * Rotated attack functions.
 * They obviously only make sense when called
 * for the matching rotated bitboard.
 */

inline Bitboard Bitboard::atk0(Square from) const
{
	return rot_atk_0[from][(bits >> shift_0(from)) & 0xff];
}

inline Bitboard Bitboard::atkl90(Square from) const
{
	return rot_atk_l90[from][(bits >> shift_l90(from)) & 0xff];
}

inline Bitboard Bitboard::atkl45(Square from) const
{
	return rot_atk_l45[from][(bits >> shift_l45(from)) & diagmask_l45(from)];
}

inline Bitboard Bitboard::atkr45(Square from) const
{
	return rot_atk_r45[from][(bits >> shift_r45(from)) & diagmask_r45(from)];
}
	

#endif // BITBOARD_INLINES_H
