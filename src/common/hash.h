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
#ifndef HASH_H
#define HASH_H

#include "common.h"
#include "board.h"
#include "move.h"
#include "node.h"
#include "util.h"
#include "spinlock.h"


/*****************************************************************************
 *
 * Class HashEntry
 *
 *****************************************************************************/

class HashEntry
{
	friend class HashTable;

      public:
	enum hashentry_type { NONE, EXACT, ALPHA, BETA };

      private:
	Hashkey hashkey;
	unsigned short type;
	unsigned short depth;
	int score;
	Move move;	

      public:
	FORCEINLINE HashEntry();
	inline HashEntry(const Board & board, int score, Move move, int depth,
			int type);
	FORCEINLINE ~HashEntry() {}

      public:
	inline unsigned int get_depth() const;
	inline int get_score() const;
	inline int get_type() const;
	inline Move get_move() const;
};

inline HashEntry::HashEntry()
{
	type = NONE;
}

inline HashEntry::HashEntry(const Board & board, int score, Move move,
		int depth, int type) 
{
	this->hashkey = board.get_hashkey();
	this->type = type;
	this->depth = depth;
	this->score = score;
	this->move = move;
}

inline unsigned int HashEntry::get_depth() const
{
	return depth;
}

inline int HashEntry::get_score() const
{
	return score;
}

inline int HashEntry::get_type() const
{
	return type; 
}

inline Move HashEntry::get_move() const
{
	return move;
}


/*****************************************************************************
 *
 * Class HashEntryPV
 *
 *****************************************************************************/

#define HASHENTRYPV_MAXMOVES 8

class HashEntryPV
{
	friend class HashTable;

      private:
	Move moves[HASHENTRYPV_MAXMOVES];
	unsigned int len;

      public:
	FORCEINLINE HashEntryPV();
	FORCEINLINE ~HashEntryPV() {}
};

inline HashEntryPV::HashEntryPV()
{
	len = 0;
}

/*****************************************************************************
 *
 * Class HashTable
 *
 *****************************************************************************/

/* To reduce the average time that threads need to wait for the lock, devide
 * the table into multiple domains, each with its own lock. If the number of
 * lock domains is sufficiently large, the probability that a thread needs to
 * access a table entry of an already locked domain should be rather low.
 * (The extreme case would be one lock per table entry, but that is 
 * infeasible of course.)
 * For efficiency, we start with a constant power of two here and see how
 * it works. The lock domain is determined from the table key by a simple
 * modulo division.
 */
#define HASHTABLE_LOCK_DOMAINS 64

class HashTable
{
      private:
	unsigned long table_size;
	HashEntry * table;
	HashEntryPV * pvtable;
	Spinlock spinlock[HASHTABLE_LOCK_DOMAINS];
	Spinlock stats_spinlock;

	unsigned long stat_probes;
	unsigned long stat_hits;
	unsigned long stat_collisions2;

      public:
	HashTable(size_t bytes, bool enable_pvline = false);
	~HashTable();

      public:
	void clear();
	bool put(const HashEntry & entry,
			const struct Node::pvline * pvline = NULL);
	bool probe(const Board & board, HashEntry * entry,
			struct Node::pvline * pvline = NULL);

	void print_info(FILE * fp = stdout) const;
	void print_statistics(FILE * fp = stdout) const;
	void reset_statistics();

      private:
	inline void lock(unsigned long long key);
	inline void unlock(unsigned long long key);
	inline void lock_stats();
	inline void unlock_stats();
};

inline void HashTable::lock(unsigned long long key)
{
	/* Unconditionally use the lock. We assume that spinlock
	 * implementation is so efficient that an extra check if
	 * this is a shared hash table is not worth or even increases
	 * overhead. */
	unsigned int domain = key % HASHTABLE_LOCK_DOMAINS;
	spinlock[domain].lock();
}

inline void HashTable::unlock(unsigned long long key)
{
	unsigned int domain = key % HASHTABLE_LOCK_DOMAINS;
	spinlock[domain].unlock();
}

inline void HashTable::lock_stats()
{
	/* Unconditionally use the lock. We assume that spinlock
	 * implementation is so efficient that an extra check if
	 * this is a shared hash table is not worth or even increases
	 * overhead. */
	stats_spinlock.lock();
}

inline void HashTable::unlock_stats()
{
	stats_spinlock.unlock();
}

#endif // HASH_H
