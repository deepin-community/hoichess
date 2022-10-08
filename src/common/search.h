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
#ifndef SEARCH_H
#define SEARCH_H

#include "common.h"
#include "board.h"
#include "clock.h"
#include "eval.h"
#include "game.h"
#include "historytable.h"
#include "move.h"
#include "movelist.h"
//#include "shell.h"
#ifdef WITH_THREAD
# include "mutex.h"
# include "thread.h"
#endif
#include "node.h"

/* forward declarations */
class Shell;
class ParallelSearch;
class HashTable;

class Search
{
      public:
	enum search_modes_e { MOVE, ANALYZE, PONDER };

	struct searchinfo {
		unsigned int depth;	// search depth
		unsigned long csecs;	// elapsed time
		unsigned long csecs_alloc;	// allocated search time
		unsigned long long nodes_total;	// total nodes searched
		unsigned int maxplyreached_fullwidth;	// maximum ply reached
		unsigned int maxplyreached_quiesce;	// ... during q.s.
		unsigned int i;		// number of current move
		unsigned int n;		// total number of moves
		Move mov;		// current move
		Board board;		// current position (tree root)
	};

	struct searchresult {
		enum resulttype { INTERMEDIATE, DEPTH, FAILLOW, FAILHIGH, FINAL } type;
		unsigned int depth;	// search depth
		int score;		// score
		unsigned long csecs;	// elapsed time
		unsigned long csecs_alloc;	// allocated search time
		unsigned long long nodes_total;	// total nodes searched
		std::string best_line;		// line of best moves
		unsigned int maxplyreached_fullwidth;	// maximum ply reached
		unsigned int maxplyreached_quiesce;	// ... during q.s.
	};

#ifdef WITH_THREAD
      protected:
	struct thread_args {
		Search * self;
		const Game * game;
		Clock * clock;
		int mode;
		Color myside;
		unsigned int maxdepth;
		/* Return values: We return by output parameters so we
		 * don't need to manage a separate return data structure
		 * to pass from the thread back to the caller. Instead,
		 * the thread just returns its own argument. */
		Move best;
	};

	struct slave_search_args {
		Node * node;
		unsigned int ply;
		int depth;
		int extend;
		int alpha;
		int beta;
	};

	struct slave_thread_args {
		Search * self;
		ParallelSearch * master;
		const Game * game;
		Clock * clock;
		int mode;
		Color myside;
		struct slave_search_args search_args;
		/* Return values: We return by output parameters so we
		 * don't need to manage a separate return data structure
		 * to pass from the thread back to the caller. Instead,
		 * the thread just returns its own argument. */
		int score;
	};
#endif

      protected:
	Shell * shell;
	
      protected:
	NodeAllocator nodealloc;
	Evaluator * evaluator;
	HashTable * hashtable;
	bool shared_hashtable;
	HistoryTable * histtable[2];
	
	/* information about game */
      protected:
	const Game * game;
	Clock * clock;

      protected:
	Node * rootnode;
      private:
	unsigned int rootdepth;

	/* search mode */
      protected:
	int mode;
	bool slave;
	Color myside;
      private:
	int maxdepth;
	
#ifdef WITH_THREAD
	/* thread+mutexes */
	Mutex start_mutex;
	Mutex main_mutex;
	Thread * thread;
#endif
	
	/* control variable to stop running search */
      protected:
	volatile bool stop;
	bool stop_iteration;
	
	/* basic statistics */
      protected:
	unsigned long long nodes_fullwidth;
	unsigned long long nodes_quiesce;
	unsigned int maxplyreached_fullwidth;
	unsigned int maxplyreached_quiesce;
	
	/* time check and thinking output interval */
      private:
	unsigned long long next_timecheck_nodes;
	unsigned long next_update_csecs;
	unsigned long last_timecheck_csecs;
	unsigned long timecheck_interval_nodes;
	unsigned long iteration_start_csecs;
	
      protected:
	/* extended statistics */
	unsigned long stat_cut;
	unsigned long stat_nullcut;
	unsigned long stat_futcut;
	unsigned long stat_xfutcut;
	unsigned long stat_razcut;
	unsigned long stat_moves_sum;
	unsigned long stat_moves_cnt;
	unsigned long stat_moves_sum_quiesce;
	unsigned long stat_moves_cnt_quiesce;

      private:
	/* For each ply-1 node, stores the PV of the previous iteration,
	 * starting from that node. This allows us to provide a PV line 
	 * for move ordering for every ply-1 move, rather than just the
	 * first. */
	std::map<Move, struct Node::pvline> ply1_pvline_map;

	/* see comment in probe_hashtable() */
	struct Node::pvline _probe_hashtable_pvline;

      public:
	Search(Shell * shell);
	virtual ~Search();
	
      public:
	Move start(const Game * game, Clock * clock,
			int mode, Color myside, unsigned int maxdepth);
	Move start(const Board & board, const Clock & clock, int mode,
			unsigned int maxdepth);
#ifdef WITH_THREAD
	void start_thread(const Game * game, Clock * clock,
			int mode, Color myside, unsigned int maxdepth);
	void start_slave_thread(ParallelSearch * master,
			const Game * game, Clock * clock, int mode, Color myside,
			Node * node, unsigned int ply, int depth, int extend,
			int alpha, int beta);
	void stop_thread();
	int stop_slave_thread();
      private:
	static void * thread_main(void * arg);
	static void * slave_thread_main(void * arg);
#endif

      public:
	virtual void interrupt();

	virtual void set_hash_size(size_t bytes);
	virtual void set_hash_size_pvline(size_t bytes);
	virtual void set_hash_table(HashTable * table);
	virtual void clear_hash();
	virtual void set_pawnhash_size(size_t bytes);
	virtual void clear_pawnhash();
	virtual void set_evalcache_size(size_t bytes);
	virtual void clear_evalcache();
	
      protected:
	virtual Move main();
#ifdef WITH_THREAD
	virtual int slave_main(const struct slave_search_args& search_args);
#endif
	virtual Move iterate(unsigned int depth);
	virtual int search_root(Node * node, unsigned int ply, int depth,
			int alpha, int beta);
	virtual int search(Node * node, unsigned int ply, int depth, int extend,
			int alpha, int beta);
	virtual int quiescence_search(Node * node, unsigned int ply, 
			int alpha, int beta);
	bool is_draw(const Node * node, unsigned int ply, int * score);
	bool is_repetition(const Node * node, unsigned int ply, int * score);
	bool probe_hashtable(Node * node, int depth, int alpha, int beta,
			int * score);
	void store_hashtable(Node * node, int depth, int alpha, int beta,
			int score);
	void add_history(Node * node);
	void add_killer(Node * node);
	int bound_score(int score, int alpha, int beta);

      protected:
	void check_time(bool force_check, bool force_update);
      private:
	void adjust_timecheck_interval(unsigned long elapsed_csecs);
	void extend_time_iteration();
	bool time_for_new_iteration();
      public:
	virtual void print_statistics();
	virtual void reset_statistics();
	unsigned long long get_nodes_fullwidth() const;
	unsigned long long get_nodes_quiesce() const;
	unsigned int get_maxplyreached_fullwidth() const;
	unsigned int get_maxplyreached_quiesce() const;
      private:
	void print_header();
	void print_thinking(unsigned int depth);
	void print_result(unsigned int depth, int score, enum searchresult::resulttype type,
			const struct Node::pvline& pvline);
      protected:
	virtual void get_root_progress(unsigned int * moves_total,
			unsigned int * current_move_no, Move * current_move) const;
};

#endif // SEARCH_H
