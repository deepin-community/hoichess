/* Copyright (C) 2005, 2006 Holger Ruckdeschel <holger@hoicher.de>
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
#include "evalcache.h"

#include <limits.h>


EvaluationCache::EvaluationCache(size_t bytes)
{
	ASSERT(bytes > 0);
	cache_size = bytes / sizeof(struct cacheentry);
	if (cache_size == 0) {
		cache_size = 1;
	}

	cache = new struct cacheentry[cache_size];
	for (unsigned long i = 0; i < cache_size; i++) {
		cache[i].score = INT_MIN;
	}

	reset_statistics();
}

EvaluationCache::~EvaluationCache()
{
	delete[] cache;
}

void EvaluationCache::clear()
{
	delete[] cache;
	cache = new struct cacheentry[cache_size];
	for (unsigned long i = 0; i < cache_size; i++) {
		cache[i].score = INT_MIN;
	}

	reset_statistics();
}

bool EvaluationCache::put(const Board & board, int score)
{
	const Hashkey hashkey = board.get_hashkey_noside();
	const unsigned long key = hashkey % cache_size;

	cache[key].hashkey = hashkey;
	cache[key].score = (board.get_side() == WHITE) ? score : -score;
	return true;
}

bool EvaluationCache::probe(const Board & board, int * score)
{
	ASSERT(score != NULL);
	stat_probes++;
	
	const Hashkey hashkey = board.get_hashkey_noside();
	const unsigned long key = hashkey % cache_size;

	if (cache[key].score == INT_MIN) {
		return false;
	} else if (cache[key].hashkey != hashkey) {
		return false;
	}

	*score = (board.get_side() == WHITE)
		? cache[key].score : -cache[key].score;
	stat_hits++;
	return true;
}

void EvaluationCache::print_info(FILE * fp) const
{
	fprintf(fp, INFO_PRFX "evalcache_size_entries=%lu"
				" evalcache_size_bytes=%lu\n",
			cache_size,
			(unsigned long) (cache_size * sizeof(cacheentry)));
}

void EvaluationCache::print_statistics(FILE * fp) const
{
	fprintf(fp, INFO_PRFX "evalcache_probes=%lu evalcache_hits=%lu\n",
			stat_probes, stat_hits);
}

void EvaluationCache::reset_statistics()
{
	stat_probes = 0;
	stat_hits = 0;
}
