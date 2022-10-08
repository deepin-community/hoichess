/* Copyright (C) 2015 Holger Ruckdeschel <holger@hoicher.de>
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
#ifndef PARALLELSEARCH_H
#define PARALLELSEARCH_H

#include "common.h"
#include "search.h"
#include "queue.h"

#include <vector>

class ParallelSearch : public Search
{
      public:
	struct slave {
		Search * search;
		bool free;
		NodeAllocator * nodealloc;
		Node * node;
		bool pvs_nullwin;
		unsigned int calls;
		unsigned long long last_nodes_fullwidth;
		unsigned long long last_nodes_quiesce;
	};

      protected:
	std::vector<struct slave> slaves;
      private:
	HashTable * shared_hashtable;

      private:
	Queue<unsigned int> ready_queue; /* stores slave index */

      public:
	ParallelSearch(Shell * shell, unsigned int nthreads);
      public:
	virtual ~ParallelSearch();

      protected:
	void add_slave(Search * slave_search);
	int find_free_slave() const;
	bool all_slaves_free() const;
	unsigned int wait_slave_ready();
	void abort_all_slaves();
	void get_slave_statistics(unsigned int id);

      public:
	void slave_ready(const Search * me);

      protected:
	virtual int search(Node * node, unsigned int ply, int depth, int extend,
			int alpha, int beta);
      private:
	int parallel_search(Node * node, unsigned int ply,
			int depth, int extend,
			int alpha, int beta);
	int nonparallel_search(Node * node, unsigned int ply,
			int depth, int extend,
			int alpha, int beta);

      public:
	virtual void interrupt();

	virtual void set_hash_size(size_t bytes);
	virtual void clear_hash();
	virtual void set_pawnhash_size(size_t bytes);
	virtual void clear_pawnhash();
	virtual void set_evalcache_size(size_t bytes);
	virtual void clear_evalcache();

      public:
	virtual void print_statistics();
	virtual void reset_statistics();
};

#endif // PARALLELSEARCH_H
