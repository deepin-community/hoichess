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
#include "clock.h"
#include "search.h"
#include "shell.h"

#include <stdio.h>


/*****************************************************************************
 *
 * Search statistics.
 *
 *****************************************************************************/

void Search::print_statistics()
{
	/* If print_statistics() is called before the first search has
	 * been started, there is no clock yet. This can happen e.g.
	 * during parallel search when a slave is never started due to
	 * small search tree. We don't want to hide this problem, so
	 * just check and expect the caller to handle it. */
	ASSERT(clock != NULL);

	int csecs = Clock::to_cs(clock->get_elapsed_time());
	unsigned long long nodes_total = nodes_fullwidth + nodes_quiesce;
	
	printf(INFO_PRFX "nodes_total=%llu nodes_fullwidth=%llu"
						" nodes_quiesce=%llu\n",
			nodes_total, nodes_fullwidth, nodes_quiesce);
	printf(INFO_PRFX "searchtime=%.2f nps=%.0f\n",
			(float) csecs / 100, 
			nodes_total / ((float) csecs / 100) );

	printf(INFO_PRFX "cuts_beta=%ld cuts_null=%ld cuts_fut=%ld"
					" cuts_xfut=%ld cuts_razor=%ld\n",
			stat_cut, stat_nullcut, stat_futcut,
			stat_xfutcut, stat_razcut);
	printf(INFO_PRFX "avg_branchfactor_fullwidth=%.2f"
					" avg_branchfactor_quiesce=%.2f\n",
		(float) stat_moves_sum / stat_moves_cnt,
		(float) stat_moves_sum_quiesce / stat_moves_cnt_quiesce);
	
	if (hashtable) {
		hashtable->print_statistics();
	}
	evaluator->print_statistics();
}

void Search::reset_statistics()
{
	nodes_fullwidth = 0;
	nodes_quiesce = 0;
	/* maxplyreached_fullwidth and maxplyreached_quiesce are reset
	 * in iterate() so they can be tracked for each iteration
	 * separately (matter of taste) */
	stat_cut = 0;
	stat_nullcut = 0;
	stat_futcut = 0;
	stat_xfutcut = 0;
	stat_razcut = 0;
	stat_moves_sum = 0;
	stat_moves_cnt = 0;
	stat_moves_sum_quiesce = 0;
	stat_moves_cnt_quiesce = 0;

	if (hashtable) {
		hashtable->reset_statistics();
	}
	evaluator->reset_statistics();
}

unsigned long long Search::get_nodes_fullwidth() const
{
	return nodes_fullwidth;
}

unsigned long long Search::get_nodes_quiesce() const
{
	return nodes_quiesce;
}

unsigned int Search::get_maxplyreached_fullwidth() const
{
	return maxplyreached_fullwidth;
}

unsigned int Search::get_maxplyreached_quiesce() const
{
	return maxplyreached_quiesce;
}


/*****************************************************************************
 *
 * Thinking output.
 *
 *****************************************************************************/

void Search::print_header()
{
	shell->print_search_header();
}

/*
 * Print thinking output.
 */
void Search::print_thinking(unsigned int depth)
{
	unsigned long csecs = Clock::to_cs(clock->get_elapsed_time());

	/* pack all information into structure that is passed to shell */
	struct searchinfo si;
	si.depth = depth;
	si.csecs = csecs;
	si.csecs_alloc = Clock::to_cs(clock->get_limit());
	si.nodes_total = nodes_fullwidth + nodes_quiesce;
	si.maxplyreached_fullwidth = maxplyreached_fullwidth;
	si.maxplyreached_quiesce = maxplyreached_quiesce;
	get_root_progress(&si.n, &si.i, &si.mov);
	si.board = rootnode->get_board();

	/* call shell for actual output */
	shell->print_search_info(&si);
}

/*
 * Print search result.
 */
void Search::print_result(unsigned int depth, int score, enum searchresult::resulttype type,
		const struct Node::pvline& pvline)
{
	int csecs = Clock::to_cs(clock->get_elapsed_time());

	/* pack all information into structure that is passed to shell */
	struct searchresult sr;
	sr.type = type;
	sr.depth = depth;
	sr.score = score;
	sr.csecs = csecs;
	sr.csecs_alloc = Clock::to_cs(clock->get_limit());
	sr.nodes_total = nodes_fullwidth + nodes_quiesce;
	sr.best_line = Node::pvline2str(pvline, rootnode->get_board(), true);
	sr.maxplyreached_fullwidth = maxplyreached_fullwidth;
	sr.maxplyreached_quiesce = maxplyreached_quiesce;

	/* call shell for actual output */
	shell->print_search_result(&sr);
}

void Search::get_root_progress(unsigned int * moves_total,
		unsigned int * current_move_no, Move * current_move) const
{
	if (moves_total != NULL) {
		*moves_total = rootnode->get_movelist_size();
	}
	if (current_move_no != NULL) {
		*current_move_no = rootnode->get_current_move_no();
	}
	if (current_move != NULL) {
		*current_move = rootnode->get_current_move();
	}
}

/*****************************************************************************
 *
 * These functions will be called by the shell to configure and control
 * the search.
 *
 *****************************************************************************/

void Search::set_hash_size(size_t bytes)
{
	if (!shared_hashtable) {
		delete hashtable;
	}

	if (bytes > 0) {
		hashtable = new HashTable(bytes);
		hashtable->print_info();
	} else {
		hashtable = NULL;
	}

	shared_hashtable = false;
}

void Search::set_hash_size_pvline(size_t bytes)
{
	if (!shared_hashtable) {
		delete hashtable;
	}

	if (bytes > 0) {
		hashtable = new HashTable(bytes, true);
		hashtable->print_info();
	} else {
		hashtable = NULL;
	}

	shared_hashtable = false;
}

void Search::set_hash_table(HashTable * table)
{
	if (!shared_hashtable) {
		delete hashtable;
	}

	hashtable = table;
	hashtable->print_info();
	shared_hashtable = true;
}

void Search::clear_hash()
{
	if (hashtable) {
		hashtable->clear();
	}
}

void Search::set_pawnhash_size(size_t bytes)
{
	evaluator->set_pawnhash_size(bytes);
}

void Search::clear_pawnhash()
{
	evaluator->clear_pawnhash();
}

void Search::set_evalcache_size(size_t bytes)
{
	evaluator->set_evalcache_size(bytes);
}

void Search::clear_evalcache()
{
	evaluator->clear_evalcache();
}

