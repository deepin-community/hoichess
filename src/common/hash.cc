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
#include "board.h"
#include "hash.h"
#include "move.h"

#include <stdio.h>


/*****************************************************************************
 * 
 * Member functions of class HashTable.
 *
 *****************************************************************************/

HashTable::HashTable(size_t bytes, bool enable_pvline)
{
	ASSERT(bytes > 0);

	if (enable_pvline) {
		table_size = bytes / (sizeof(HashEntry) + sizeof(HashEntryPV));
	} else {
		table_size = bytes / sizeof(HashEntry);
	}

	if (table_size == 0) {
		table_size = 1;
	}

	table = new HashEntry[table_size];
	if (enable_pvline) {
		pvtable = new HashEntryPV[table_size];
	} else {
		pvtable = NULL;
	}

	reset_statistics();
}

HashTable::~HashTable()
{
	delete[] table;
	delete[] pvtable;
}

void HashTable::clear()
{
	delete[] table;
	table = new HashEntry[table_size];

	/* pvtable does not need to be cleared, because its contents are
	 * only used in combination with a normal table entry */

	reset_statistics();
}

bool HashTable::put(const HashEntry & entry, 
		const struct Node::pvline * pvline)
{
	const unsigned long long key = entry.hashkey % table_size;

	lock(key);

	table[key] = entry;

	/* An optimization would be to store the first move of pvline in
	 * the normal table entry. However due to alignment, reducing 
	 * HASHENTRYPV_MAXMOVES by 1 might not save space anyway, so we 
	 * start without this optimization to keep the code simple. */
	if (pvtable != NULL && pvline != NULL && pvline->nmoves > 0) {
		unsigned int n = MIN(pvline->nmoves, HASHENTRYPV_MAXMOVES);
		pvtable[key].len = n;
		memcpy(pvtable[key].moves, pvline->moves, n * sizeof(Move));
	} else if (pvtable != NULL) {
		pvtable[key].len = 0;
	}

	unlock(key);
	return true;
}

bool HashTable::probe(const Board & board, HashEntry * entry,
		struct Node::pvline * pvline)
{
	const unsigned long long key = board.get_hashkey() % table_size;

	lock_stats();
	stat_probes++;
	unlock_stats();
	
	lock(key);
	{

	/* For efficiency, first use a reference and copy only if
	 * it is a hit. */
	const HashEntry & e = table[key];

	if (e.type == HashEntry::NONE || e.hashkey != board.get_hashkey()) {
		unlock(key);
		return false;
	}

	if (pvtable != NULL && pvline != NULL)  {
		unsigned int n = MIN(pvtable[key].len, NODE_PVLINE_MAXMOVES);
		memcpy(pvline->moves, pvtable[key].moves, n * sizeof(Move));
		pvline->nmoves = n;
	} else if (pvline != NULL) {
		pvline->nmoves = 0;
	}

	*entry = e;

	} /* end of scope of e */
	unlock(key);

	/* If this entry has a move, make sure it is
	 * valid for the given board position. */
	if (entry->move) {
		if (!entry->move.is_valid(board)) {
			lock_stats();
			stat_collisions2++;
			unlock_stats();
			return false;
		}

		if (!entry->move.is_legal(board)) {
			WARN("illegal move in hash table");
			return false;
		}
	}
	
	lock_stats();
	stat_hits++;
	unlock_stats();

	return true;
}

void HashTable::print_info(FILE * fp) const
{
	size_t bytes;
	if (pvtable) {
		bytes = table_size * (sizeof(HashEntry) + sizeof(HashEntryPV));
	} else { 
		bytes = table_size * sizeof(HashEntry);
	}

	fprintf(fp, INFO_PRFX "hash_size_entries=%lu hash_size_bytes=%lu"
							" hash_pvtable=%d\n",
			table_size, (unsigned long) bytes,
			pvtable ? 1 : 0);
}

void HashTable::print_statistics(FILE * fp) const
{
	fprintf(fp, INFO_PRFX "hash_probes=%lu hash_hits=%lu"
				" hash_collisions2=%lu\n",
			stat_probes, stat_hits,
			stat_collisions2);
}

void HashTable::reset_statistics()
{
	stat_probes = 0;
	stat_hits = 0;
	stat_collisions2 = 0;
}

