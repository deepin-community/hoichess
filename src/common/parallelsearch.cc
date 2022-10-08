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

#include "common.h"

#include "parallelsearch.h"
#include "shell.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define SHOPT(x) (shell->get_option_##x())

/*****************************************************************************
 *
 * Constructor / Destructor
 *
 *****************************************************************************/

ParallelSearch::ParallelSearch(Shell * shell, unsigned int nthreads)
	: Search(shell)
{
	for (unsigned int i=0; i<nthreads; i++) {
		add_slave(new Search(shell));
	}

	shared_hashtable = NULL;
}

ParallelSearch::~ParallelSearch()
{
	for (unsigned int i=0; i<slaves.size(); i++) {
		delete slaves[i].search;
		delete slaves[i].nodealloc;
	}

	delete shared_hashtable;
}

/*****************************************************************************
 *
 * Slave management functions
 *
 *****************************************************************************/

void ParallelSearch::add_slave(Search * slave_search)
{
	struct slave s;
	s.search = slave_search;
	s.free = true;
	/* Only the slave's starting node is allocated by this NodeAllocator.
	 * The slave Search itself has its own allocator. */
	s.nodealloc = new NodeAllocator(1);
	s.node = NULL;
	slaves.push_back(s);
}

/* returns slave index, or -1 if no free slave exists */
int ParallelSearch::find_free_slave() const
{
	int id = -1;
	for (unsigned int i=0; i<slaves.size(); i++) {
		if (slaves[i].free) {
			id = i;
			break;
		}
	}
	return id;
}

bool ParallelSearch::all_slaves_free() const
{
	for (unsigned int i=0; i<slaves.size(); i++) {
		if (!slaves[i].free) {
			return false;
		}
	}
	return true;
}

/* returns slave index */
unsigned int ParallelSearch::wait_slave_ready()
{
	return ready_queue.get();
}

void ParallelSearch::abort_all_slaves()
{
	for (unsigned int i=0; i<slaves.size(); i++) {
		if (slaves[i].free) {
			continue;
		}

		/* interrupt slave and wait for termination */
		slaves[i].search->interrupt();
		slaves[i].search->stop_slave_thread();

		/* get statistics from slave */
		get_slave_statistics(i);

		/* free the slave */
		slaves[i].node->free();
		slaves[i].free = true;
	}

	ready_queue.clear();
}

void ParallelSearch::get_slave_statistics(unsigned int id)
{
	unsigned long long tmp;

	tmp = slaves[id].search->get_nodes_fullwidth();
	nodes_fullwidth += tmp - slaves[id].last_nodes_fullwidth;
	slaves[id].last_nodes_fullwidth	= tmp;

	tmp = slaves[id].search->get_nodes_quiesce();
	nodes_quiesce += tmp - slaves[id].last_nodes_quiesce;
	slaves[id].last_nodes_quiesce	= tmp;

	unsigned int tmp2;

	tmp2 = slaves[id].search->get_maxplyreached_fullwidth();
	if (tmp2 > maxplyreached_fullwidth) {
		maxplyreached_fullwidth = tmp2;
	}
	
	tmp2 = slaves[id].search->get_maxplyreached_quiesce();
	if (tmp2 > maxplyreached_quiesce) {
		maxplyreached_quiesce = tmp2;
	}
}

/*****************************************************************************
 *
 * Callbacks from slave.
 * These are called within slave thread context.
 *
 *****************************************************************************/

void ParallelSearch::slave_ready(const Search * me)
{
	/* The ready_queue stores the slave index, so look up the slave
	 * pointer within slaves[].
	 * Doing it here (i.e. in slave thread context) saves a tiny little bit
	 * of computation time in the master thread, and the slave thread
	 * won't get new work anyway as long as the master is busy. */
	for (unsigned int i=0; i<slaves.size(); i++) {
		if (slaves[i].search == me) {
			ready_queue.put(i);
			return;
		}
	}

	BUG("callback from unknown slave 0x%p", me);
}

/*****************************************************************************
 *
 * Parallel search functions.
 *
 *****************************************************************************/

/* Shortcut */
#define failsoft (shell->get_option_search_failsoft())

int ParallelSearch::search(Node * node, unsigned int ply, int depth, int extend,
		int alpha, int beta)
{
	/*
	 * For shallow depth, don't do any parallel search. Instead,
	 * call one slave for the whole current node.
	 * The other slaves can catch up through shared hash table
	 * and the like.
	 */

	if (depth < SHOPT(search_parallel_min_depth)) {
		return nonparallel_search(node, ply, depth, extend,
				alpha, beta);
	}

	/*
	 * If there are only very few (legal) moves, it is not worth
	 * searching in parallel, so we go back to normal search for
	 * this node.
	 */

	node->generate_all_moves();
	unsigned int nmoves = node->get_movelist_size();
	unsigned int minmoves = SHOPT(search_parallel_min_move_ratio)
		* slaves.size();
	if (nmoves < minmoves) {
		return Search::search(node, ply, depth, extend,
				alpha, beta);
	}

	/*
	 * Search the node in parallel.
	 */

	return parallel_search(node, ply, depth, extend,
				alpha, beta);
}

int ParallelSearch::parallel_search(Node * node, unsigned int ply,
		int depth, int extend,
		int alpha, int beta)
{
	/* We do not support quiescence search. */
	ASSERT(depth > 0);

	int drawscore;
	if (is_draw(node, ply, &drawscore)) {
		if (failsoft) {
			return drawscore;
		} else { /* strict fail hard */
			return bound_score(drawscore, alpha, beta);
		}
	}

	int save_alpha = alpha; /* need old value to save in hash table */
	int score;
	int bestscore = -INFTY;
	int moves = 0;

	nodes_fullwidth++;
	
	if (ply > maxplyreached_fullwidth) {
		maxplyreached_fullwidth = ply;
	}

	if (depth > 0 && probe_hashtable(node, depth, alpha, beta, &score)) {
		return score;
	}

	/* Null-move forward pruning */
	bool null_ok = !(node->get_parent()->get_played_move().is_null())
		    && !(Evaluator::get_phase(node->get_board())
				   == Evaluator::ENDGAME);
	if (!node->in_check() && null_ok) {
		Node * child = node->make_move(Move::null(), &nodealloc);
		score = -search(child, ply+1, depth-2-1, 0, -beta, -beta+1);
		child->free();
		if (score >= beta) {
			stat_nullcut++;
			if (failsoft) {
				return score;
			} else {
				return beta; /* fail hard */
			}
		}
	}

	/*
	 * Internal iterative deepening:
	 *
	 * If we don't have a move from the hash table, search 
	 * this node with a shallower depth to get a good move
	 * to search first.
	 */
	if (!node->get_pvmove() && !node->get_hashmv() && depth > 2) {
		search(node, ply, depth-2, 0, alpha, beta);
		node->set_hashmv(node->get_best_move());
	}

	node->set_type(Node::FULLWIDTH);
	node->set_historytable(histtable[node->get_board().get_side()]);

	/*
	 * Search the first move by its own using a recursive call
	 * of the master search. The remaining moves are searched
	 * in parallel by the available slaves.
	 * The same will happen for the first move one ply deeper,
	 * and so on.
	 */

	Move mov = node->first();
	Node * child = node->make_move(mov, &nodealloc);
	if (!child->get_board().is_legal()) {
		BUG("illegal move at parallel node: %s",
				mov.str().c_str());
	}
	moves++;

	score = -search(child, ply+1, depth-1, extend, -beta, -alpha);
	if (!failsoft && (score < alpha || score > beta) && !stop) {
		printf("parallel_search(first): fail hard condition violated:"
				" score=%d alpha=%d beta=%d\n",
				score, alpha, beta);
	}

	check_time(true, false);
	if (stop) {
		child->free();
		/* We don't get any useful result after canceling the
		 * alpha-beta search, so iterate() must return the
		 * result of the previous (completed) iteration.
		 * For that reason, we can return an arbitrary value
		 * here, because iterate() won't use it anyway. */
		return INT_MIN;
	}

	if (score > bestscore) {
		bestscore = score;
		node->set_best(mov, child);
	}

	child->free();

	if (score > alpha) {
		alpha = score;
	}

	if (score >= beta) {
		if (!failsoft) {
			score = beta; /* fail hard */
		}
		goto done;
	}

	/* Parallel search of remaining moves. */
	mov = node->next();
	int id; /* slave ID */
	while (1) {
		/* first, give all free slaves a legal move to search */
		for (; mov && (id = find_free_slave()) >= 0;
				mov = node->next()) {
			Node * child = node->make_move(mov,
					slaves[id].nodealloc);
			if (!child->get_board().is_legal()) {
				BUG("illegal move at parallel node: %s",
						mov.str().c_str());
				//child->free();
				//continue;
			}
			moves++;

			/* The slave has its own hash table, but also probe
			 * the master's table. Maybe we can provide the slave
			 * a PV found earlier by another slave. As we do not
			 * use the score, the valus for alpha and beta are
			 * not important. */
			probe_hashtable(child, depth-1, -beta, -alpha, &score);

			/* principal variation search */
			bool nullwin = (SHOPT(search_parallel_pvs_mode) == 1)
					/* this is never the first move here */
				|| (SHOPT(search_parallel_pvs_mode) == 2
					&& alpha > save_alpha);

			/* start the slave */
			DBG(1, "start slave %d\n", id);
			ASSERT(slaves[id].free);
			slaves[id].free = false;
			slaves[id].node = child;
			if (nullwin) {
				slaves[id].pvs_nullwin = true;
				slaves[id].search->start_slave_thread(this,
						game, clock, mode, myside,
						child, ply+1, depth-1, extend,
						-alpha-1, -alpha);
			} else {
				slaves[id].pvs_nullwin = false;
				slaves[id].search->start_slave_thread(this,
						game, clock, mode, myside,
						child, ply+1, depth-1, extend,
						-beta, -alpha);
			}
			slaves[id].calls++;
		}

		/* if now all slaves are free, there were no more moves
		 * left to be assigned, so we're done */
		if (all_slaves_free()) {
			break;
		}

		/* if time is over (or we're stopped by other means),
		 * terminate all slaves and return */
		check_time(true, false);
		if (stop) {
			DBG(1, "time over, abort all slaves\n");
			abort_all_slaves();
			/* We don't get any useful result after canceling the
			 * alpha-beta search, so iterate() must return the
			 * result of the previous (completed) iteration.
			 * For that reason, we can return an arbitrary value
			 * here, because iterate() won't use it anyway. */
			return INT_MIN;
		}

		/* wait until the next slave gets ready */
		DBG(1, "wait for slave ready\n");
		id = wait_slave_ready();
		DBG(1, "slave %d ready\n", id);

		/* get result from slave */
		score = -slaves[id].search->stop_slave_thread();
		Move slave_mov = slaves[id].node->get_played_move();

		/* get statistics from slave */
		get_slave_statistics(id);

		/* if the PVS assumption failed, restart slave search for the
		 * same node, but this time with full alpha-beta window */
		if (slaves[id].pvs_nullwin && score > alpha && score < beta) {
			DBG(1, "start slave %d again\n", id);
			slaves[id].pvs_nullwin = false;
			slaves[id].search->start_slave_thread(this,
					game, clock, mode, myside,
					slaves[id].node, ply+1, depth-1, extend,
					-beta, -alpha);
			slaves[id].calls++;
			continue;
		}

		/* check for alpha not possible because alpha may have been
		 * improved since the slave was started */
		if (!failsoft && score > beta && !stop) {
			printf("parallel_search(): fail hard condition violated:"
					" score=%d alpha=%d beta=%d\n",
					score, alpha, beta);
		}

		if (score > bestscore) {
			bestscore = score;
			node->set_best(slave_mov, slaves[id].node);
		}

		/* see above call to probe_hashtable() */
		store_hashtable(slaves[id].node, depth-1, -beta, -save_alpha, score);

		/* free the slave */
		slaves[id].node->free();
		slaves[id].free = true;

		if (score > alpha) {
			alpha = score;
		}

		if (score >= beta) {
			if (!failsoft) {
				score = beta; /* fail hard */
			}
			/* alpha has already been updated */
			DBG(1, "cutoff, abort all slaves\n");
			abort_all_slaves();
			break;
		}
	}
done:

	/* Test for checkmate or stalemate */
	if (moves == 0) {
#if defined(HOICHESS)
		int matescore = node->in_check() ? (-INFTY + ply) : DRAW;
#elif defined(HOIXIANGQI)
		int matescore = -INFTY + ply;
#else
# error "neither HOICHESS nor HOIXIANGQI is defined"
#endif
		if (failsoft) {
			bestscore = matescore;
		} else { /* strict fail hard */
			alpha = bound_score(matescore, alpha, beta);
		}
	}

	/* Save search result in hash table. */
	if (failsoft) {
		store_hashtable(node, depth, save_alpha, beta, bestscore);
	} else {
		store_hashtable(node, depth, save_alpha, beta, alpha);
	}
	
	add_history(node);
	add_killer(node);

	stat_moves_sum += moves;
	stat_moves_cnt++;
	
	if (failsoft) {
		return bestscore;
	} else {
		return alpha;
	}
}

int ParallelSearch::nonparallel_search(Node * node, unsigned int ply,
		int depth, int extend,
		int alpha, int beta)
{
	int drawscore;
	if (depth > 0 && is_draw(node, ply, &drawscore)) {
		if (failsoft) {
			return drawscore;
		} else { /* strict fail hard */
			return bound_score(drawscore, alpha, beta);
		}
	}

	int score;

	/* Probe hash table. Although the slave has its own hash table,
	 * we can possibly provide a PV (TODO) or even avoid calling the slave
	 * at all. */
	if (depth > 0 && probe_hashtable(node, depth, alpha, beta, &score)) {
		return score;
	}

	/* start the slave */
	int id = find_free_slave();
	ASSERT(id >= 0);
	DBG(1, "start slave %d\n", id);
	slaves[id].free = false;
	slaves[id].node = node;
	slaves[id].search->start_slave_thread(this, game, clock, mode, myside,
			node, ply, depth, extend, alpha, beta);
	slaves[id].calls++;

	/* wait until the slave gets ready */
	int id1 = wait_slave_ready();
	ASSERT(id1 == id);

	/* get result from slave */
	score = slaves[id].search->stop_slave_thread();
	DBG(1, "slave %d finished\n", id);

	/* slave may have terminated due to time limit, so check time also
	 * in master so that the stop flag can get set */
	check_time(true, false);

	if (!failsoft && (score < alpha || score > beta) && !stop) {
		printf("nonparallel_search(): fail hard condition violated:"
				" score=%d alpha=%d beta=%d\n",
				score, alpha, beta);
	}

	/* get statistics from slave */
	get_slave_statistics(id);

	/* free the slave */
	slaves[id].free = true;

	/* Save search result in hash table. */
	store_hashtable(node, depth, alpha, beta, score);

	return score;
}


/*****************************************************************************
 *
 * These functions will be called by the shell to configure and control
 * the search.
 *
 *****************************************************************************/

void ParallelSearch::interrupt()
{
	DBG(2, "interrupt");
	stop = true;
	unsigned int nslaves = slaves.size();
	for (unsigned int i=0; i<nslaves; i++) {
		slaves[i].search->interrupt();
	}
}

void ParallelSearch::set_hash_size(size_t bytes)
{
	/* We distribute the given size equally among all slaves
	 * and the master. */
	unsigned int nslaves = slaves.size();
	size_t bytes_master = bytes / (nslaves + 1);
	size_t bytes_slave = bytes_master;
	size_t bytes_slave_shared = bytes - bytes_master;

	/* master */
	if (SHOPT(search_parallel_hash_pvtable)) {
		Search::set_hash_size_pvline(bytes_master);
	} else {
		Search::set_hash_size(bytes_master);
	}

	/* slaves */
	if (SHOPT(search_parallel_shared_hash)) {
		delete shared_hashtable;
		shared_hashtable = new HashTable(bytes_slave_shared);

		for (unsigned int i=0; i<nslaves; i++) {
			slaves[i].search->set_hash_table(shared_hashtable);
		}
	} else {
		delete shared_hashtable;
		shared_hashtable = NULL;

		for (unsigned int i=0; i<nslaves; i++) {
			slaves[i].search->set_hash_size(bytes_slave);
		}
	}
}

void ParallelSearch::clear_hash()
{
	Search::clear_hash();
	unsigned int nslaves = slaves.size();
	for (unsigned int i=0; i<nslaves; i++) {
		slaves[i].search->clear_hash();
	}
}

void ParallelSearch::set_pawnhash_size(size_t bytes)
{
	/* We distribute the given size equally among all slaves.
	 * Of course, the master does not get a pawn hash. */
	unsigned int nslaves = slaves.size();
	size_t bytes1 = bytes / nslaves;
	for (unsigned int i=0; i<nslaves; i++) {
		slaves[i].search->set_pawnhash_size(bytes1);
	}
}

void ParallelSearch::clear_pawnhash()
{
	unsigned int nslaves = slaves.size();
	for (unsigned int i=0; i<nslaves; i++) {
		slaves[i].search->clear_pawnhash();
	}
}

void ParallelSearch::set_evalcache_size(size_t bytes)
{
	/* We distribute the given size equally among all slaves.
	 * Of course, the master does not get an evaluation cache. */
	unsigned int nslaves = slaves.size();
	size_t bytes1 = bytes / nslaves;
	for (unsigned int i=0; i<nslaves; i++) {
		slaves[i].search->set_evalcache_size(bytes1);
	}
}

void ParallelSearch::clear_evalcache()
{
	unsigned int nslaves = slaves.size();
	for (unsigned int i=0; i<nslaves; i++) {
		slaves[i].search->clear_evalcache();
	}
}

/*****************************************************************************
 *
 * Search statistics.
 *
 *****************************************************************************/

void ParallelSearch::print_statistics()
{
	printf(INFO_PRFX "=== parallel search statistics ===\n");
	printf(INFO_PRFX "--- master ---\n");
	Search::print_statistics();
	for (unsigned int i=0; i<slaves.size(); i++) {
		printf(INFO_PRFX "--- slave %u ---\n", i);
		printf(INFO_PRFX "calls=%u\n", slaves[i].calls);
		/* Search::print_statistics() does not work correctly
		 * when no search has ever been run (see comment there). */
		if (slaves[i].calls != 0) {
			slaves[i].search->print_statistics();		
		}
	}
	printf(INFO_PRFX "==================================\n");
}

void ParallelSearch::reset_statistics()
{
	Search::reset_statistics();
	for (unsigned int i=0; i<slaves.size(); i++) {
		slaves[i].search->reset_statistics();		
		slaves[i].calls = 0;
		slaves[i].last_nodes_fullwidth = 0;
		slaves[i].last_nodes_quiesce = 0;
	}
}
