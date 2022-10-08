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
#include "search.h"
#include "shell.h"
#ifdef WITH_THREAD
# include "parallelsearch.h"
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define SHOPT(x) (shell->get_option_##x())

/*****************************************************************************
 *
 * Constructor / Destructor
 *
 *****************************************************************************/

Search::Search(Shell * shell)
		: nodealloc(MAXPLY)
{
	this->shell = shell;

	evaluator = new Evaluator();
	hashtable = NULL;
	shared_hashtable = false;
	histtable[WHITE] = new HistoryTable();
	histtable[BLACK] = new HistoryTable();

	game = NULL;
	clock = NULL;
	rootnode = NULL;
	
	maxdepth = MAXDEPTH;

#ifdef WITH_THREAD
	thread = NULL;
#endif

	stop = false;
	timecheck_interval_nodes = 100;
}

Search::~Search()
{
	delete evaluator;
	if (!shared_hashtable) {
		delete hashtable;
	}
	delete histtable[WHITE];
	delete histtable[BLACK]; 
}




/*****************************************************************************
 * 
 * These functions will be called by the shell
 * to set up and control the search.
 *
 *****************************************************************************/

Move Search::start(const Game * _game, Clock * _clock,
		int _mode, Color _myside, unsigned int _maxdepth)
{
#ifdef WITH_THREAD
	DBG(2, "locking start_mutex");
	start_mutex.lock();
	DBG(2, "locked start_mutex");
	ASSERT(!thread);
#endif

	ASSERT(game == NULL);
	game = _game;
	clock = _clock;
	mode = _mode;
	myside = _myside;
	maxdepth = _maxdepth;

	stop = false;
	Move best = main();

	game = NULL;
	clock = NULL;
	
#ifdef WITH_THREAD
	DBG(2, "unlocking start_mutex");
	start_mutex.unlock();
	DBG(2, "unlocked start_mutex");
#endif

	return best;
}

Move Search::start(const Board & board, const Clock & clock, int mode,
		unsigned int maxdepth)
{
	Game game(board, clock, clock);
	game.start();
	return start(&game, game.get_clock(), mode, NO_COLOR, maxdepth);
}

#ifdef WITH_THREAD
void Search::start_thread(const Game * _game, Clock * _clock,
		int _mode, Color _myside, unsigned int _maxdepth)
{
	DBG(2, "locking start_mutex");
	start_mutex.lock();
	DBG(2, "locked start_mutex");

	if (!thread) {
		DBG(2, "starting thread");
		/* args will be freed again by stop_thread() */
		struct thread_args * args = new struct thread_args;

		args->self = this;
		args->game = _game;
		args->clock = _clock;
		args->mode = _mode;
		args->myside = _myside;
		args->maxdepth = _maxdepth;

		stop = false;
		thread = new Thread(thread_main);
		thread->start((void *) args);
		/* thread_main() will return args again so that
		 * stop_thread() can get output variables from there
		 * and free it afterwards */
	} else {
		DBG(2, "thread already running");
	}
	
	DBG(2, "unlocking start_mutex");
	start_mutex.unlock();
	DBG(2, "unlocked start_mutex");
}

void Search::start_slave_thread(ParallelSearch * master,
		const Game * game, Clock * clock, int mode, Color myside,
		Node * node, unsigned int ply, int depth, int extend,
		int alpha, int beta)
{
	DBG(2, "locking start_mutex");
	start_mutex.lock();
	DBG(2, "locked start_mutex");

	if (!thread) {
		DBG(2, "starting thread");
		/* args will be freed again by stop_slave_thread() */
		struct slave_thread_args * args = new struct slave_thread_args;

		args->self = this;
		args->master = master;
		args->game = game;
		args->clock = clock;
		args->mode = mode;
		args->myside = myside;
		args->search_args.node = node;
		args->search_args.ply = ply;
		args->search_args.depth = depth;
		args->search_args.extend = extend;
		args->search_args.alpha = alpha;
		args->search_args.beta = beta;

		stop = false;
		thread = new Thread(slave_thread_main);
		thread->start((void *) args);
		/* slave_thread_main() will return args again so that
		 * stop_slave_thread() can get output variables from there
		 * and free it afterwards */
	} else {
		DBG(2, "thread already running");
	}

	DBG(2, "unlocking start_mutex");
	start_mutex.unlock();
	DBG(2, "unlocked start_mutex");
}

void Search::stop_thread()
{
	DBG(2, "locking start_mutex");
	start_mutex.lock();
	DBG(2, "locked start_mutex");
	
	if (thread) {
		interrupt();
		DBG(2, "waiting for thread to terminate");
		struct thread_args * args =
				(struct thread_args *) thread->wait();
		DBG(2, "thread has terminated");
		delete thread;
		thread = NULL;

		//Move best = args->best;

		delete args;
	} else {
		DBG(2, "thread not running");
	}
	
	DBG(2, "unlocking start_mutex");
	start_mutex.unlock();
	DBG(2, "unlocked start_mutex");
}

int Search::stop_slave_thread()
{
	DBG(2, "locking start_mutex");
	start_mutex.lock();
	DBG(2, "locked start_mutex");

	int score;

	if (thread) {
		interrupt();
		DBG(2, "waiting for thread to terminate");
		struct slave_thread_args * args =
				(struct slave_thread_args *) thread->wait();
		DBG(2, "thread has terminated");
		delete thread;
		thread = NULL;

		score = args->score;

		delete args;
	} else {
		BUG("thread not running");
	}

	DBG(2, "unlocking start_mutex");
	start_mutex.unlock();
	DBG(2, "unlocked start_mutex");

	return score;
}

void * Search::thread_main(void * arg)
{
	struct thread_args * args = (struct thread_args *) arg;
	Search * self = args->self;

	ASSERT(self->game == NULL);
	self->game = args->game;
	self->clock = args->clock;
	self->mode = args->mode;
	self->myside = args->myside;
	self->maxdepth = args->maxdepth;
	args->best = self->main();
	self->game = NULL;
	self->clock = NULL;

	return args;
}

void * Search::slave_thread_main(void * arg)
{
	struct slave_thread_args * args = (struct slave_thread_args *) arg;
	Search * self = args->self;

	ASSERT(self->game == NULL);
	self->game = args->game;
	self->clock = args->clock;
	self->mode = args->mode;
	self->myside = args->myside;

	args->score = self->slave_main(args->search_args);

	self->game = NULL;
	/* must not set self->clock = NULL, because clock is required
	 * by print_statistics() which is called by master when slave
	 * search is not running anymore */

	args->master->slave_ready(self);	

	return args;
}
#endif // WITH_THREAD

void Search::interrupt()
{
	DBG(2, "interrupt");
	stop = true;
}


/*****************************************************************************
 * 
 * Main search function. Initialize everything and start
 * iterative deepening search.
 *
 *****************************************************************************/

Move Search::main()
{
#ifdef WITH_THREAD
	DBG(2, "locking main_mutex");
	main_mutex.lock();
	DBG(2, "locked main_mutex");
#endif

	slave = false;

	last_timecheck_csecs = 0;
	next_timecheck_nodes = timecheck_interval_nodes;
	next_update_csecs = 0;

	reset_statistics();

	histtable[WHITE]->reset();
	histtable[BLACK]->reset();

	/* Convert the game entries into a list of node. The
	 * last one will be the root node of the search tree. */
	const std::list<GameEntry>& gameentries = game->get_entries();
	NodeAllocator gamenodealloc(gameentries.size() + 1);
	Node openingnode(game->get_opening());
	Node * node = &openingnode;
	for (std::list<GameEntry>::const_iterator it = gameentries.begin();
			it != gameentries.end(); it++) {
		node = node->make_move(it->get_move(), &gamenodealloc);
	}
	rootnode = node;

	/* Initialize the root node. init_root() already assigns one legal
	 * move as best, in case search terminates without choosing a move. */
	ASSERT(rootnode->get_board() == game->get_board());
	rootnode->init_root();
	rootdepth = 0;

	/* If we're not supplied with a clock, create a local one just for
	 * measuring elapsed time. */
	Clock tmp_clock;
	if (clock == NULL) {
		ASSERT(mode == ANALYZE || mode == PONDER);
		tmp_clock.start();
		clock = &tmp_clock;
	}

	if (verbose >= 2) {
		clock->print();
	}

	/* Allocate search time. */
	if (mode == MOVE) {
		ASSERT(clock->is_running());
		clock->allocate_time();
	}

	print_header();

	Move best = iterate(maxdepth);

	if (verbose) {
		print_statistics();
	}
	
	rootnode = NULL;
	
#ifdef WITH_THREAD
	DBG(2, "unlocking main_mutex");
	main_mutex.unlock();
	DBG(2, "unlocked main_mutex");
#endif

	return best;
}

/*****************************************************************************
 *
 * Main function of slave search. Initialize everything and start
 * full-width search.
 *
 *****************************************************************************/

#ifdef WITH_THREAD
int Search::slave_main(const struct slave_search_args& search_args)
{
	DBG(2, "locking main_mutex");
	main_mutex.lock();
	DBG(2, "locked main_mutex");

	slave = true;

	/* clock is passed from master and set by slave_thread_main */
	last_timecheck_csecs = 0;
	next_timecheck_nodes = timecheck_interval_nodes;
	next_update_csecs = 0;

	/* statistics are reset by ParallelSearch::reset_statistics() */

	/* for non-parallel and master search, these are reset in
	 * iterate(), so for slaves, it must be done here */
	maxplyreached_fullwidth = 0;
	maxplyreached_quiesce = 0;

	histtable[WHITE]->reset();
	histtable[BLACK]->reset();

	/* There is no direct root node for a slave search. */
	rootnode = NULL;
	rootdepth = 0;

	int score = search(search_args.node, search_args.ply,
			search_args.depth, search_args.extend,
			search_args.alpha, search_args.beta);

	DBG(2, "unlocking main_mutex");
	main_mutex.unlock();
	DBG(2, "unlocked main_mutex");

	return score;
}
#endif /* WITH_THREAD */

/*****************************************************************************
 *
 * Tree Search Functions.
 *
 *****************************************************************************/

/* Aspiration window for iterative deepening */
#define WINDOW	50

/* Shortcut */
#define failsoft (shell->get_option_search_failsoft())

Move Search::iterate(unsigned int depth)
{
	Move best = rootnode->get_best_move(); /* set by Node::init_root() */
	int score = 0;
	int alpha = -INFTY;
	int beta = INFTY;
	
	/* stop search after current iteration, even in case of fail-low */
	stop_iteration = false;

	ply1_pvline_map.clear();

	for (rootdepth = 1; rootdepth <= depth; rootdepth++) {
again:
		maxplyreached_fullwidth = 0;
		maxplyreached_quiesce = 0;

		iteration_start_csecs = Clock::to_cs(clock->get_elapsed_time());

		score = search_root(rootnode, 0, rootdepth, alpha, beta);
		if (!failsoft && (score < alpha || score > beta) && !stop) {
			printf("iterate(): fail hard condition violated:"
					" score=%d alpha=%d beta=%d\n",
					score, alpha, beta);
		}

		check_time(true, true);
		if (stop) {
			unsigned int moves_total;
			unsigned int moves_done;
			get_root_progress(&moves_total, &moves_done, NULL);
			unsigned long csecs_wasted =
				Clock::to_cs(clock->get_elapsed_time())
				- iteration_start_csecs;
			printf("\nIteration aborted after %u/%u moves (%u%%)"
					", %.2f s wasted\n",
					moves_done, moves_total, 
					100 * moves_done / moves_total,
					(float) csecs_wasted / 100);
			break;
		}
		
		/* If search failed low or high, re-search with a wider
		 * window. Best move is not updated in this case.
		 * Otherwise adjust window for next iteration.
		 * When a mate is detected, we can stop immediately. */
		if (score <= alpha && score > -MATE) {
			print_result(rootdepth, score, searchresult::FAILLOW,
					rootnode->get_best_line());
			if (stop_iteration) {
				break;
			}
			alpha = -INFTY;
			//beta = beta;
			goto again;
		} else if (score >= beta && score < MATE) {
			print_result(rootdepth, score, searchresult::FAILHIGH,
					rootnode->get_best_line());
			if (stop_iteration) {
				break;
			}
			//alpha = alpha;
			beta = INFTY;
			goto again;
		} else {
			Move mov = rootnode->get_best_move();
			if (mov) {
				best = mov;
			}

			print_result(rootdepth, score, searchresult::DEPTH,
					rootnode->get_best_line());
			if (stop_iteration) {
				break;
			} else if (score >= MATE || score <= -MATE) {
				break;
			}

			alpha = score - WINDOW;
			beta = score + WINDOW;
		}
		
		/* Decide if time is sufficient to start a new iteration. */
		if (mode == MOVE && !clock->is_exact()
				&& SHOPT(search_time_for_new_iteration_enable)
				&& !time_for_new_iteration()) {
			printf("No time for another iteration.\n");
			break;
		}

	}

	return best;
}

int Search::search_root(Node * node, unsigned int ply, int depth,
		int alpha, int beta)
{
	ASSERT_DEBUG(ply == 0);

//	DBG(3, "depth=%d, alpha=%d, beta=%d\n", depth, alpha, beta);
	
	int save_alpha = alpha;
	int score;
	int bestscore = -INFTY;
	int moves = 0;
	bool first = true;
	for (Move mov = node->first(); mov; mov = node->next()) {
		/* make move */
		Node * child = node->make_move(mov, &nodealloc);
		if (!child->get_board().is_legal()) {
			BUG("illegal move at root node: %s", mov.str().c_str());
		}
		moves++;

		/* Get the child node's PV of the previous iteration. */
		std::map<Move, struct Node::pvline>::iterator mit
				= ply1_pvline_map.find(mov);
		if (mit != ply1_pvline_map.end()) {
			child->set_pvline(mit->second);
		}

		check_time(true, true);

		/* Search the current move. We use a standard
		 * principal variation search here. */
		bool nullwin = (SHOPT(search_pvs_mode) == 1 && !first)
			|| (SHOPT(search_pvs_mode) == 2 && alpha > save_alpha);
		if (nullwin) {
			score = -search(child, ply+1, depth-1, 0,
					-alpha-1, -alpha);
			if (score > alpha && score < beta) {
				score = -search(child, ply+1, depth-1, 0,
						-beta, -alpha);
			}			
		} else {
			score = -search(child, ply+1, depth-1, 0,
					-beta, -alpha);
		}
		first = false;

		if (!failsoft && (score < alpha || score > beta) && !stop) {
			printf("search_root(): fail hard condition violated:"
					" score=%d alpha=%d beta=%d\n",
					score, alpha, beta);
		}

		ply1_pvline_map[mov] = child->get_best_line();

		if (stop) {
			child->free();
			/* We don't get any useful result after canceling the
			 * alpha-beta search, so iterate() must return the
			 * result of the previous (completed) iteration.
			 * For that reason, we can return an arbitrary value
			 * here, because iterate() won't use it anyway. */
			return INT_MIN;
		}
		
		/* this is taken for move ordering at the next iteration */
		node->set_current_score(score);
		
		if (score > bestscore) {
			bestscore = score;
			node->set_best(mov, child);
			print_result(depth, score, searchresult::INTERMEDIATE,
					node->get_best_line());
		}

		child->free();

		if (score > alpha) {
			alpha = score;
		}

		if (score >= beta) {
			if (!failsoft) {
				score = beta; /* fail hard */
			}
			stat_cut++;
			break;
		}
	}
	
	add_history(node);

	stat_moves_sum += moves;
	stat_moves_cnt++;

	if (failsoft) {
		return bestscore;
	} else {
		return alpha;
	}
}

int Search::search(Node * node, unsigned int ply, int depth, int extend,
		int alpha, int beta)
{
	/* If maximum search depth is reached, begin quiescence search. */
	if (depth <= 0) {
		return quiescence_search(node, ply, alpha, beta);
	}

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
	bool first = true;
	nodes_fullwidth++;
	
	if (ply > maxplyreached_fullwidth) {
		maxplyreached_fullwidth = ply;
	}

	if (probe_hashtable(node, depth, alpha, beta, &score)) {
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
	 * Futility pruning, extended futility pruning, and razoring.
	 */
	if (depth == 3  &&  (node->material_balance() + 900 <= alpha)) {
		/* razoring */
		stat_razcut++;
		depth--;
	}

	bool fprune = false;
	if (depth == 1  &&  (node->material_balance() + 300 <= alpha)) {
		/* futility pruning */
		fprune = true;
	} else if (depth == 2  &&  (node->material_balance() + 500 <= alpha)) {
		/* extended futility pruning */
		stat_xfutcut++;
		fprune = true;
		depth--;
	}

	/*
	 * Search all successor moves.
	 */
	for (Move mov = node->first(); mov; mov = node->next()) {
		Node * child = node->make_move(mov, &nodealloc);
		if (!child->get_board().is_legal()) {
			child->free();
			continue;
		}
		moves++;

		/* Futility pruning */
		if (fprune && !node->in_check() && !child->in_check()
				&& !mov.is_capture()
#ifdef HOICHESS
				&& !mov.is_enpassant()
				&& !mov.is_promotion()
#endif // HOICHESS
				) {
			stat_futcut++;
			child->free();
			continue;
		}


		/* Search the current move. We use a standard
		 * principal variation search here. */
		bool nullwin = (SHOPT(search_pvs_mode) == 1 && !first)
			|| (SHOPT(search_pvs_mode) == 2 && alpha > save_alpha);
		if (nullwin) {
			score = -search(child, ply+1, depth-1, extend,
					-alpha-1, -alpha);
			if (score > alpha && score < beta) {
				score = -search(child, ply+1, depth-1, extend,
						-beta, -alpha);
			}			
		} else {
			score = -search(child, ply+1, depth-1, extend,
					-beta, -alpha);
		}
		first = false;

		if (!failsoft && (score < alpha || score > beta) && !stop) {
			printf("search(): fail hard condition violated:"
					" score=%d alpha=%d beta=%d\n",
					score, alpha, beta);
		}

		check_time(false, false);
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
			stat_cut++;
			break;
		}
	}

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

int Search::quiescence_search(Node * node, unsigned int ply, 
		int alpha, int beta)
{
	int save_alpha = alpha; /* need old value to save in hash table */
	int score = 0; /* initialized only to avoid compiler warning */
	int bestscore = -INFTY;
	int moves = 0;
	
	nodes_quiesce++;

	if (ply > maxplyreached_quiesce) {
		maxplyreached_quiesce = ply;
	}

	if (probe_hashtable(node, 0, alpha, beta, &score)) {
		return score;
	}
	
	/*
	 * Evaluate board position.
	 *
	 * This score will be returned if none
	 * of the moves is better than alpha.
	 */
	score = evaluator->eval(node->get_board(), alpha, beta, myside);
	if (score >= beta) {
		if (failsoft) {
			return score;
		} else {
			return beta; /* fail hard */
		}
	}
	if (score > alpha) {
		alpha = score;
	}
	if (failsoft) {
		bestscore = score;
	}

	/* MAXPLY is an absolute depth limit */
	if (ply == MAXPLY-1) {
		WARN("reached maximum tree depth: %d", ply);
		return score;
	}
	
	node->set_type(Node::QUIESCE);
	
	for (Move mov = node->first(); mov; mov = node->next()) {
		Node * child = node->make_move(mov, &nodealloc);
		if (!child->get_board().is_legal()) {
			child->free();
			continue;
		}
		moves++;
		
		score = -quiescence_search(child, ply+1, -beta, -alpha);

		if (!failsoft && (score < alpha || score > beta) && !stop) {
			printf("q_search(): fail hard condition violated:"
					" score=%d alpha=%d beta=%d\n",
					score, alpha, beta);
		}

		check_time(false, false);
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
			stat_cut++;
			break;
		}
	}

	/* Test for checkmate */
	if (moves == 0 && node->in_check()) {
		int matescore = -INFTY + ply;
		if (failsoft) {
			bestscore = matescore;
		} else { /* strict fail hard */
			alpha = bound_score(matescore, alpha, beta);
		}
	}
	
	/* Save search result in hash table. */
	if (failsoft) {
		store_hashtable(node, 0, save_alpha, beta, bestscore);
	} else {
		store_hashtable(node, 0, save_alpha, beta, alpha);
	}

	stat_moves_sum_quiesce += moves;
	stat_moves_cnt_quiesce++;
	
	if (failsoft) {
		return bestscore;
	} else {
		return alpha;
	}
}

/* 
 * Return true if the current position is a draw by rule. The score assigned
 * to the draw is returned by the score output parameter.
 * This is because the code also handles perpetual check for xiangqi, in which
 * case a mate score is assigned. It could also be used for chess to assign
 * different draw scores in different situations.
 */
bool Search::is_draw(const Node * node, unsigned int ply, int * score)
{
	const Board & board = node->get_board();
	
	/* 50 move rule */
	if (board.get_movecnt50() >= 100) {
		*score = DRAW;
		return true;
	}
	
	/* Draw due to insufficient material */
	if (board.is_material_draw()) {
		*score = DRAW;
		return true;
	}
	
	/* Look for repetitions */
	if (is_repetition(node, ply, score)) {
		return true;
	}

	return false;
}

/* 
 * Return true if the current position is a repetition. The score assigned
 * to the repetition is returned by the score output parameter.
 *
 * For xiangqi, this also handled perpetual check: If between the repeating
 * positions, one side always checks but the other side does not, a mate
 * score is returned.
 */
bool Search::is_repetition(const Node * node, unsigned int ply, int * score)
{
#ifndef HOIXIANGQI
	(void) ply;
#endif

	/* Count repetitions until next irreversible move. */
	int rep = 0;

#ifdef HOIXIANGQI
	unsigned int moves[2] = {0, 0}; /* [0] is side */
	unsigned int checks[2] = {0, 0}; /* [0] is side */
	/* in the current position, xside has moved last */
	unsigned int side = 1;
#endif

	for (const Node * p = node; p != NULL; p = p->get_parent()) {
		if (p->get_played_move().is_irreversible()) {
			break;
		}

		if (p != node && p->get_board().get_hashkey() 
				== node->get_board().get_hashkey()) {
			rep++;
		}

#ifdef HOIXIANGQI
		moves[side]++;
		if (p->in_check()) {
			checks[side]++;
		}
		side = 1 - side;
#endif

		if (rep) {
			break;
		}
	}

	if (rep) {
#ifdef HOIXIANGQI
		if (moves[0] > 0 && checks[0] == moves[0]
				&& moves[1] > 0 && checks[1] == moves[1]) {
			*score = DRAW;
			printf("perpetual both, %u/%u\n", checks[0], checks[1]);
		} else if (moves[0] > 0 && checks[0] == moves[0]) {
			*score = -INFTY + ply;
			printf("perpetual side, %u/%u\n", checks[0], checks[1]);
		} else if (moves[1] > 0 && checks[1] == moves[1]) {
			*score = INFTY - ply;
			printf("perpetual xside, %u/%u\n", checks[0],checks[1]);
		} else {
			*score = DRAW;
		}
#else
		*score = DRAW;
#endif
		return true;
	} else {
		*score = INT_MIN; /* never used */
		return false;
	}
}

/* Returns true if the probe result can be directly used as search
 * return value. */
bool Search::probe_hashtable(Node * node, int depth, int alpha, int beta,
		int * score)
{
	if (!hashtable) {
		return false;
	}

	/* Constructor of Node::pvline is expensive as it contains a
	 * quite large array of Move elements. So we add a pvline as
	 * class member to avoid construction each time probe_hashtable()
	 * runs. */
	//struct Node::pvline pvline;
	struct Node::pvline & pvline = _probe_hashtable_pvline;

	HashEntry entry;
	if (!hashtable->probe(node->get_board(), &entry, &pvline)) {
		return false;
	}

	node->set_hashmv(entry.get_move());

	/* Overwrite the node's existing PV line only if the line in the
	 * hash table is longer. */
	if (pvline.nmoves > node->get_pvline().nmoves) {
		node->set_pvline(pvline);
	}

	if (entry.get_depth() >= (unsigned) depth) {
		int s = entry.get_score();
		switch (entry.get_type()) {
		case HashEntry::EXACT:
			if (failsoft) {
				*score = s;
			} else { /* strict fail hard */
				*score = bound_score(s, alpha, beta);
			}
			return true;
		case HashEntry::ALPHA:
			if (s <= alpha) {
				if (failsoft) {
					*score = s;
				} else { /* strict fail hard */
					*score = bound_score(s, alpha, beta);
				}
				return true;
			}
			break;
		case HashEntry::BETA:
			if (s >= beta) {
				if (failsoft) {
					*score = s;
				} else { /* strict fail hard */
					*score = bound_score(s, alpha, beta);
				}
				return true;
			}
			break;
		}
	}

	return false;
}

void Search::store_hashtable(Node * node, int depth, int alpha, int beta,
		int score)
{
	if (!hashtable) {
		return;
	}

	/* Translate ply-dependent mate scores into ply-independent
	 * mate scores.
	 * See also http://www.brucemo.com/compchess/programming/matehash.htm.
	 * FIXME The cases "fail-low at mate" (alpha >= score >= MATE)
	 * and "fail-high at being mated" (-MATE >= score >= beta) are
	 * not yet handled, because they seem to be always triggered
	 * during PVS null window search. And I don't understand all
	 * this yet.
	 */

	int scoretype;
	if (score >= MATE) {
		scoretype = HashEntry::BETA;
		score = MATE;
	} else if (score <= -MATE) {
		scoretype = HashEntry::ALPHA;
		score = -MATE;
	} else if (score >= beta) {
		scoretype = HashEntry::BETA;
	} else if (score <= alpha) {
		scoretype = HashEntry::ALPHA;
	} else {
		scoretype = HashEntry::EXACT;
	}

	HashEntry hashentry(node->get_board(), score, node->get_best_move(),
			depth, scoretype);
	const struct Node::pvline & best_line = node->get_best_line();
	hashtable->put(hashentry, &best_line);
}

void Search::add_history(Node * node)
{
	if (node->get_best_move()) {
		histtable[node->get_board().get_side()]->add(node->get_best_move());
	}
}

void Search::add_killer(Node * node)
{
	if (node->get_best_move() != node->get_hashmv()
			&& !(node->get_best_move().is_capture())
#ifdef HOICHESS
			&& !(node->get_best_move().is_enpassant())
			&& !(node->get_best_move().is_promotion())
#endif // HOICHESS
			) {
		node->add_killer(node->get_best_move());
	}
}

int Search::bound_score(int score, int alpha, int beta)
{
	if (score > beta) {
		return beta;
	} else if (score < alpha) {
		return alpha;
	} else {
		return score;
	}
}

void Search::check_time(bool force_check, bool force_update)
{
	/* check time only after a certain number of nodes to reduce
	 * system call overhead */
	unsigned long long nodes = nodes_fullwidth + nodes_quiesce;
	if (nodes < next_timecheck_nodes && !force_check) {
		return;
	}

	/* check time */
	if (clock->timeout()) {
		stop = true;
		return;
	}

	unsigned long elapsed_csecs = Clock::to_cs(clock->get_elapsed_time());

	/* update and adjust time check interval */
	if (!force_check) {
		adjust_timecheck_interval(elapsed_csecs);
	}
	last_timecheck_csecs = elapsed_csecs;
	next_timecheck_nodes = nodes + timecheck_interval_nodes;

	/* thinking output (not in slave mode) */
	if (!slave && (elapsed_csecs >= next_update_csecs || force_update)) {
		next_update_csecs = elapsed_csecs
			+ SHOPT(search_update_interval_csecs);
		print_thinking(rootdepth);
	}

	/* time extensions (not in slave mode) */
	if (mode == MOVE && !slave && !clock->is_exact()
			&& SHOPT(search_extend_time_iteration_enable)) {
		extend_time_iteration();
	}
}
	
/* 
 * Adjust timecheck_interval_nodes based on time since last check.
 */
void Search::adjust_timecheck_interval(unsigned long elapsed_csecs)
{
	unsigned long diff = elapsed_csecs - last_timecheck_csecs;
	unsigned long dmin = SHOPT(search_timecheck_interval_csecs_min);
	unsigned long dmax = SHOPT(search_timecheck_interval_csecs_max);

	if (dmax == 0) {
		timecheck_interval_nodes = 1;
		DBG(1, "elapsed_csecs=%u last_timecheck_csecs=%lu diff=%u"
				" timecheck_interval_nodes=%lu\n",
				elapsed_csecs, last_timecheck_csecs, diff,
				timecheck_interval_nodes);
	} else if (diff < dmin
			&& timecheck_interval_nodes < ULONG_MAX/2) {
		timecheck_interval_nodes *= 2;
		DBG(1, "elapsed_csecs=%u last_timecheck_csecs=%lu diff=%u"
				" timecheck_interval_nodes=%lu\n",
				elapsed_csecs, last_timecheck_csecs, diff,
				timecheck_interval_nodes);
	} else if (diff > dmax && timecheck_interval_nodes > 1) {
		timecheck_interval_nodes /= 2;
		DBG(1, "elapsed_csecs=%u last_timecheck_csecs=%lu diff=%u"
				" timecheck_interval_nodes=%lu\n",
				elapsed_csecs, last_timecheck_csecs, diff,
				timecheck_interval_nodes);
	}
	
	DBG(2, "elapsed_csecs=%u last_timecheck_csecs=%lu diff=%u"
			" timecheck_interval_nodes=%lu\n",
			elapsed_csecs, last_timecheck_csecs, diff,
			timecheck_interval_nodes);
}


/*
 * Extend search time when remaining time is expected to be
 * slightly too short to finish current iteration.
 */
void Search::extend_time_iteration()
{
	unsigned int rootmoves_total;
	unsigned int rootmoves_done;
	get_root_progress(&rootmoves_total, &rootmoves_done, NULL);

	unsigned int elapsed_csecs = Clock::to_cs(clock->get_elapsed_time());

	const unsigned int min_percent_done =
		SHOPT(search_extend_time_iteration_min_percent_done);
	const float expect_safety_factor = (float)
		SHOPT(search_extend_time_iteration_expect_safety_factor_tenth)
		/ 10;
	const float extend_safety_factor = (float)
		SHOPT(search_extend_time_iteration_extend_safety_factor_tenth)
		/ 10;
		// ontop of expect_safety_factor

	if (100 * rootmoves_done / rootmoves_total >= min_percent_done) {
		/* Assuming that each of the remaining moves will take the
		 * average time spent on the completed moves (plus safety
		 * margin), check if the remaining time would be sufficient
		 * to complete the iteration. If not, extend by the difference
		 * between expected time required for remaining moves and
		 * remaining time. */
		unsigned long csecs_avg =
			(elapsed_csecs - iteration_start_csecs)
			/ rootmoves_done;
		unsigned long csecs_expect =
			csecs_avg
			* (rootmoves_total - rootmoves_done)
			* expect_safety_factor;
		/* NOTE that this can be negative! */
		long csecs_remain = Clock::to_cs(clock->get_limit())
			- elapsed_csecs;

		if (csecs_remain < (signed long) csecs_expect
				&& !stop_iteration) {
			unsigned long csecs_extend =
				(csecs_expect - csecs_remain)
				* extend_safety_factor;

#if 0
			printf("\nextend_time_iteration:"
					" %u/%u done, avg=%.2f, expect=%.2f"
					", remain=%.2f, diff=%.2f"
					", extend=%.2f\n",
					rootmoves_done,
					rootmoves_total,
					(float) csecs_avg / 100,
					(float) csecs_expect / 100,
					(float) csecs_remain / 100,
					(float) csecs_remain / 100
						- (float) csecs_expect / 100,
					(float) csecs_extend / 100);
#endif

			if (clock->allocate_more_time(
					Clock::from_cs(csecs_extend))) {
				/* do not start a new iteration anymore */
				stop_iteration = true;
			}
		}
	}
}

/* Decide if time is likely sufficient to start a new iteration. */
bool Search::time_for_new_iteration()
{
	unsigned long elapsed_csecs = Clock::to_cs(clock->get_elapsed_time());
	unsigned long iteration_csecs = elapsed_csecs - iteration_start_csecs;
	/* NOTE that this can be negative! */
	long remain_csecs = Clock::to_cs(clock->get_limit()) - elapsed_csecs;

	const float f = (float)
		SHOPT(search_time_for_new_iteration_expect_factor_tenth)
		/ 10;
	unsigned long expected_csecs = iteration_csecs * f;

#if 0
	printf("time_for_new_iteration:"
			" last=%.2f, remain=%.2f, expect=%.2f\n",
			(float) iteration_csecs / 100,
			(float) remain_csecs / 100,
			(float) expected_csecs / 100);
#endif

	return (remain_csecs >= (signed long) expected_csecs);
}
