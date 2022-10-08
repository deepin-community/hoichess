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

#include "common.h"
#include "clock.h"
#include "util.h"

Clock::Clock()
{
	mode = NONE;
	running = false;
	limit = 0;
	hard_limit = 0;
	over_time = 0;
	lost_time = 0;
}

Clock::Clock(unsigned int secs)
{
	mode = EXACT;
	running = false;
	limit = from_s(secs);
	hard_limit = limit;
	over_time = 0;
	lost_time = 0;
}

Clock::Clock(unsigned int moves, unsigned int base_secs, unsigned int inc_secs)
{
	ASSERT(base_secs > 0);
	
	base_time = from_s(base_secs);
	base_moves = moves;
	increment = from_s(inc_secs);

	if (moves == 0 && inc_secs == 0) {
		mode = SUDDENDEATH;
	} else if (inc_secs == 0) {
		mode = CONV;
	} else {
		mode = INCR;
	}

	remaining_time = base_time;
	remaining_moves = base_moves;
	
	running = false;
	limit = 0;
	hard_limit = 0;
	over_time = 0;
	lost_time = 0;
}

void Clock::start()
{
	if (running)
		return;
	
	start_time = get_realtime_us();
	running = true;
}

void Clock::stop()
{
	if (!running)
		return;

	stop_time = get_realtime_us();
	running = false;

	val_t elapsed = get_elapsed_time();
	update_remaining(elapsed);

	/* determine how much search has exceeded its time limit */
	if (mode != NONE && elapsed > limit) {
		over_time = elapsed - limit;
	} else {
		over_time = 0;
	}

	DBG(1, "elapsed=%lu over_time=%lu\n", (unsigned long) elapsed,
			(unsigned long) over_time);

	if (mode == EXACT && elapsed > limit * 1.1) {
		WARN("time exeeded in exact clock mode:"
				" elapsed=%lu limit=%lu\n",
				(unsigned long) elapsed, (unsigned long) limit);
	}
}

void Clock::turn_back()
{
	/* This function is supposed to be called when switching one side
	 * from human to engine, e.g. through command 'go'. The saved delays
	 * must be reset, otherwise over_time could contain the human's
	 * thinking time etc.. */
	over_time = 0;
	lost_time = 0;

	if (!running)
		return;

	start_time = get_realtime_us();
}

void Clock::allocate_time()
{
	if (mode == NONE || mode == EXACT) {
		/* For EXACT, limit and hard_limit already set in
		 * constructor. For NONE, there is no limit. */
		return;
	}

	DBG(1, "over_time=%lu lost_time=%lu\n", (unsigned long) over_time,
			(unsigned long) lost_time);

	/* Safety margin for remaining time. Always add 1 second reserve
	 * so can be sure we never exceed the game's time limit. */
	val_t remaining_safe = remaining_time - 2*lost_time - from_s(1);
	if (remaining_safe <= 0) {
		limit = 0;
		hard_limit = 0;
		if (verbose) {
			atomic_printf("No time remaining!\n");
		}
		return;
	}
	DBG(1, "remaining_time=%lu remaining_safe=%lu\n",
			(unsigned long) remaining_time,
			(unsigned long) remaining_safe);

	/* determine search time limit */
	val_t limit_tmp;
	switch (mode) {
	case CONV:
		ASSERT(remaining_moves > 0);
		limit_tmp = remaining_safe / remaining_moves;
		break;		
	case INCR:
		limit_tmp = remaining_safe / 20 + increment;
		break;
	case SUDDENDEATH:
		limit_tmp = remaining_safe / 40;
		break;
	default:
		BUG("should not get here");
	}
	DBG(1, "limit_tmp=%lu\n", (unsigned long) limit_tmp);

	/* safety margin for limit */
	if (limit_tmp < 2*over_time) {
		limit = 0;
	} else {
		limit = limit_tmp - 2*over_time;
	}
	DBG(1, "limit=%lu\n", (unsigned long) limit);

	/* set hard limit */
	hard_limit = 2 * limit;
	if (hard_limit > remaining_safe) {
		hard_limit = remaining_safe;
		limit = hard_limit;
	}
	DBG(1, "limit=%lu hard_limit=%lu\n", (unsigned long) limit,
			(unsigned long) hard_limit);

	if (verbose) {
		atomic_printf(INFO_PRFX "searchtime_alloc=%.2f"
						" searchtime_alloc_max=%.2f\n",
				to_s_f(limit), to_s_f(hard_limit));
	}
}

bool Clock::allocate_more_time(val_t t)
{
	if (mode == NONE || mode == EXACT) {
		return false;
	}

	limit += t;

	if (limit > hard_limit) {
		limit = hard_limit;
	}

	if (verbose) {
		atomic_printf(INFO_PRFX "searchtime_alloc=%.2f"
						" searchtime_alloc_max=%.2f\n",
				to_s_f(limit), to_s_f(hard_limit));
	}

	return true;
}

void Clock::update_remaining(val_t elapsed)
{
	switch (mode) {
	case NONE:
	case EXACT:
		break;
	case CONV:
		remaining_time -= elapsed;
		remaining_moves--;
		if (remaining_moves == 0) {
			remaining_time += base_time;
			remaining_moves = base_moves;
		}
		break;
	case INCR:
		remaining_time -= elapsed;
		remaining_time += increment;
		break;
	case SUDDENDEATH:
		remaining_time -= elapsed;
		break;
	}
}

bool Clock::timeout() const
{
	if (mode == NONE) {
		return false;
	}
	
	if (!running) {
		WARN("Clock::timeout() called with stopped clock");
	}
	
	val_t elapsed = get_elapsed_time();
	return (elapsed >= limit);
}

bool Clock::is_exact() const
{
	return (mode == EXACT);
}

bool Clock::is_running() const
{
	return running;
}

Clock::val_t Clock::get_limit() const
{
	return limit;
}

Clock::val_t Clock::get_elapsed_time() const
{
	val_t elapsed;

	if (!running) {
		elapsed = (stop_time - start_time) / 1000;
	} else {
		unsigned long long now = get_realtime_us();
		elapsed = (now - start_time) / 1000;
	}

	return elapsed;
}

Clock::val_t Clock::get_remaining_time() const
{
	return remaining_time;
}

void Clock::set_remaining_time(val_t t)
{
	/* Due to external delay, the remaining time may be less than
	 * the stored value, so track this difference to tune safety margin
	 * for time allocation accordingly. */
	if (t < remaining_time) {
		lost_time = remaining_time - t;
	} else {
		lost_time = 0;
	}
	DBG(1, "lost_time=%lu\n", (unsigned long) lost_time);

	remaining_time = t;
}

void Clock::print(FILE * fp) const
{
	switch (mode) {
	case NONE:
		fprintf(fp, "Clock mode: none\n");
		break;
	case CONV:
		fprintf(fp, "Clock mode: conventional\n");
		fprintf(fp, "Remaining time: %.2f sec, remaining moves: %d\n",
				to_s_f(remaining_time),
				remaining_moves);
		break;
	case INCR:
		fprintf(fp, "Clock mode: incremental\n");
		fprintf(fp, "Remaining time: %.2f sec, increment: %.2fs\n",
				to_s_f(remaining_time), to_s_f(increment));
		break;
	case SUDDENDEATH:
		fprintf(fp, "Clock mode: sudden death\n");
		fprintf(fp, "Remaining time: %.2f sec\n",
				to_s_f(remaining_time));
		break;
	case EXACT:
		fprintf(fp, "Clock mode: exact\n");
		fprintf(fp, "Time per move: %.2f sec\n",
				to_s_f(limit));
		break;
	}

	val_t elapsed = get_elapsed_time();
	if (running) {
		fprintf(fp, "Clock is running, elapsed time: %.2fs\n",
				to_s_f(elapsed));
	}
}
