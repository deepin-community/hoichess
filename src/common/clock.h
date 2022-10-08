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
#ifndef CLOCK_H
#define CLOCK_H

#include "common.h"

#include <limits.h>

class Clock
{
      public:
	typedef long val_t; /* milliseconds */
	static const val_t VAL_T_MAX = LONG_MAX;

      private:
	enum clock_mode { NONE, CONV, INCR, SUDDENDEATH, EXACT } mode;
	bool running;

	unsigned long long start_time;
	unsigned long long stop_time;
	
	val_t limit;
	val_t hard_limit;
	val_t remaining_time;
	unsigned int remaining_moves;
	
	val_t base_time;
	unsigned int base_moves;
	val_t increment;

	/* how much the elapsed time exceeds the limit when clock is stopped */
	val_t over_time;

	/* lost time because set_remaining_time() set a lower value than
	 * remaining_time had before */
	val_t lost_time;

      public:
	Clock();
	Clock(unsigned int secs);
	Clock(unsigned int moves, unsigned int base_secs,
			unsigned int inc_secs);
	~Clock() {}

      public:
	void start();
	void stop();
	void turn_back();

      public:
	void allocate_time();
	bool allocate_more_time(val_t t);

      private:
	void update_remaining(val_t elapsed);

      public:
	bool timeout() const;
	bool is_exact() const;
	bool is_running() const;
	val_t get_limit() const;
	val_t get_elapsed_time() const;
	val_t get_remaining_time() const;
	void set_remaining_time(val_t t);
	void print(FILE * fp = stdout) const;

      public:
	static inline long to_s(val_t v);
	static inline float to_s_f(val_t v);
	static inline long to_cs(val_t v);
	static inline long long to_us(val_t v);
	static inline val_t from_s(long s);
	static inline val_t from_cs(long cs);
};

inline long Clock::to_s(val_t v)
{
	return v / 1000;
}

inline float Clock::to_s_f(val_t v)
{
	return (float) v / 1000;
}

inline long Clock::to_cs(val_t v)
{
	return v / 10;
}

inline long long Clock::to_us(val_t v)
{
	return v * 1000;
}

inline Clock::val_t Clock::from_s(long s)
{
	return s * 1000;
}

inline Clock::val_t Clock::from_cs(long cs)
{
	return cs * 10;
}

#endif // CLOCK_H
