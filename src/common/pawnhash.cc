/* Copyright (C) 2004-2006 Holger Ruckdeschel <holger@hoicher.de>
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
#include "pawnhash.h"

#include <stdio.h>


/*****************************************************************************
 * 
 * Member functions of class PawnHashTable.
 *
 *****************************************************************************/

PawnHashTable::PawnHashTable(size_t bytes)
{
	ASSERT(bytes > 0);
	table_size = bytes / sizeof(PawnHashEntry);
	if (table_size == 0) {
		table_size = 1;
	}

	table = new PawnHashEntry[table_size];

	reset_statistics();
}

PawnHashTable::~PawnHashTable()
{
	delete[] table;
}

void PawnHashTable::clear()
{
	delete[] table;
	table = new PawnHashEntry[table_size];

	reset_statistics();
}

bool PawnHashTable::put(const PawnHashEntry & entry)
{
	const unsigned long key = entry.hashkey % table_size;

	table[key] = entry;
	return true;
}

bool PawnHashTable::probe(Hashkey hashkey, PawnHashEntry * entry)
{
	stat_probes++;

	const unsigned long key = hashkey % table_size;
	const PawnHashEntry & e = table[key];
	
	if (e.phase == -1 || e.hashkey != hashkey) {
		entry->phase = -1;
		return false;
	}

	*entry = e;
	stat_hits++;
	return true;
}

void PawnHashTable::print_info(FILE * fp) const
{
	fprintf(fp, INFO_PRFX "pawnhash_size_entries=%lu"
						" pawnhash_size_bytes=%lu\n",
			table_size,
			(unsigned long) (table_size * sizeof(PawnHashEntry)));
}

void PawnHashTable::print_statistics(FILE * fp) const
{
	fprintf(fp, INFO_PRFX "pawnhash_probes=%lu"
			" pawnhash_hits=%lu pawnhash_hits2=%lu\n",
			stat_probes, stat_hits, stat_hits2);
}

void PawnHashTable::reset_statistics()
{
	stat_probes = 0;
	stat_hits = 0;
	stat_hits2 = 0;
}
