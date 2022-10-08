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
#include "eval.h"
#include "board.h"


Evaluator::Evaluator()
{
	pawnhashtable = NULL;
	evalcache = NULL;
	reset_statistics();
}

Evaluator::~Evaluator()
{
	delete pawnhashtable;
	delete evalcache;
}


/*****************************************************************************
 *
 * Main evaluation functions.
 *
 *****************************************************************************/

/* TODO
 * - This could be different for Chess and Xiangqi...
 * - This should be configurable on runtime, because test tourneys have
 *   shown that it depends on available search time.
 */
#define EVAL_CUTOFF_MATERIAL	150
#define EVAL_CUTOFF_PHASE1	  5
#define EVAL_ENABLE_PHASE2

int Evaluator::eval(const Board & board, int alpha, int beta, Color _myside)
{
	myside = _myside;
	
	int score;

	/* TODO is_draw() still empty */
#if 0
	if (is_draw(board))
		return DRAW;
#endif
	
	const Color side = board.get_side();
	const Color xside = XSIDE(side);
	stat_evals++;

	
	/*
	 * material 
	 */
	
	score = material_balance(board.get_material(side),
				 board.get_material(xside));
	if (score >= beta + EVAL_CUTOFF_MATERIAL
			|| score <= alpha - EVAL_CUTOFF_MATERIAL) {
		return score;
	}
	
	
	/*
	 * do normal evaluation 
	 */
	
	if (evalcache && evalcache->probe(board, &score)) {
		return score;
	}
	
	setup(&board);

	
	/*
	 * phase 1 
	 */
	{
		stat_evals_phase1++;
		
		/* call plugins */
		int score1 = 0;
		for (int i=0; plugins[i].name != NULL; i++) {
			ASSERT_DEBUG(plugins[i].func != NULL);
			score1 += (this->*plugins[i].func)(side)
				- (this->*plugins[i].func)(xside);
		}
		score += score1;
#ifdef EVAL_CUTOFF_PHASE1
		if (score1 > EVAL_CUTOFF_PHASE1
				|| score1 < -EVAL_CUTOFF_PHASE1) {
			goto done;
		}
#endif
	}

#ifdef EVAL_ENABLE_PHASE2
	/*
	 * phase 2 
	 */
	{
		stat_evals_phase2++;

		/* call plugins2 */
		int score2 = 0;
		for (int i=0; plugins2[i].name != NULL; i++) {
			ASSERT_DEBUG(plugins2[i].func != NULL);
			score2 += (this->*plugins2[i].func)(side)
				- (this->*plugins2[i].func)(xside);
		}
		score += score2;
	}
#endif

	/*
	 * done 
	 */
	goto done;	// avoid warning about unused label
done:
	finish();

	if (evalcache) {
		evalcache->put(board, score);
	}
	
	return score;
}

void Evaluator::print_eval(const Board & board, Color _myside, FILE * fp)
{
	myside = _myside;
	setup(&board);
	
#if 0
	fprintf(fp, "material: %d/%d\n", 
			board.material[WHITE], board.material[BLACK]);
	fprintf(fp, "material difference: %d\n",
			board.material[WHITE] - board.material[BLACK]);
	fprintf(fp, "material balance: %d\n",
			 material_balance(board.material[WHITE],
				 	  board.material[BLACK]));
	fprintf(fp, "phase: %u\n", phase);

	fprintf(fp, "draw: %s\n", is_draw(board) ? "yes" : "no");
	fprintf(fp, "myside: %s\n",
			(myside == WHITE ? "white"
		 		: (myside == BLACK ? "black" : "none")));
#else
	fprintf(fp, INFO_PRFX "eval_material_white=%d"
				" eval_material_black=%d\n", 
			board.material[WHITE], board.material[BLACK]);
	fprintf(fp, INFO_PRFX "eval_material_difference=%d\n",
			board.material[WHITE] - board.material[BLACK]);
	fprintf(fp, INFO_PRFX "eval_material_balance=%d\n",
			 material_balance(board.material[WHITE],
				 	  board.material[BLACK]));
	fprintf(fp, INFO_PRFX "eval_phase=%u eval_isdraw=%d\n",
			phase, is_draw(board));
#endif

#if 0
	fprintf(fp, "scoring plugins:\n");
#endif
	for (int i=0; plugins[i].name != NULL; i++) {
		ASSERT(plugins[i].func != NULL);
		int score_white = (this->*plugins[i].func)(WHITE);
		int score_black = (this->*plugins[i].func)(BLACK);

#if 0
		fprintf(fp, "\t%s: %d/%d\n", plugins[i].name,
				score_white, score_black);
#endif
		fprintf(fp, INFO_PRFX "eval_plugin_%s_white=%d"
					" eval_plugin_%s_black=%d\n",
				plugins[i].name, score_white,
				plugins[i].name, score_black);
	}
	
#if 0
	fprintf(fp, "scoring plugins2:\n");
#endif
	for (int i=0; plugins2[i].name != NULL; i++) {
		ASSERT(plugins2[i].func != NULL);
		int score_white = (this->*plugins2[i].func)(WHITE);
		int score_black = (this->*plugins2[i].func)(BLACK);

#if 0
		fprintf(fp, "\t%s: %d/%d\n", plugins2[i].name,
				score_white, score_black);
#endif
		fprintf(fp, INFO_PRFX "eval_plugin2_%s_white=%d"
					" eval_plugin2_%s_black=%d\n",
				plugins2[i].name, score_white,
				plugins2[i].name, score_black);
	}


}


/*****************************************************************************
 *
 * Utility functions.
 *
 *****************************************************************************/

void Evaluator::reset_statistics()
{
	stat_evals = 0;
	stat_evals_phase1 = 0;
	stat_evals_phase2 = 0;

	if (pawnhashtable) {
		pawnhashtable->reset_statistics();
	}
	
	if (evalcache) {
		evalcache->reset_statistics();
	}
}

void Evaluator::print_statistics(FILE * fp) const
{
#if 0
	fprintf(fp, "Evaluations: %lu/%lu/%lu\n",
			stat_evals, stat_evals_phase1, stat_evals_phase2);
#else
	fprintf(fp, INFO_PRFX "evals=%lu evals_phase1=%lu evals_phase2=%lu\n",
			stat_evals, stat_evals_phase1, stat_evals_phase2);
#endif

	if (pawnhashtable) {
		pawnhashtable->print_statistics(fp);
	}
	
	if (evalcache) {
		evalcache->print_statistics(fp);
	}
}


void Evaluator::set_pawnhash_size(size_t bytes)
{
	if (bytes > 0) {
		delete pawnhashtable;
		pawnhashtable = new PawnHashTable(bytes);
		pawnhashtable->print_info();
	} else {
		delete pawnhashtable;
		pawnhashtable = NULL;
	}
}
	
void Evaluator::clear_pawnhash()
{
	if (pawnhashtable) {
		pawnhashtable->clear();
	}
}

void Evaluator::set_evalcache_size(size_t bytes)
{
	if (bytes > 0) {
		delete evalcache;
		evalcache = new EvaluationCache(bytes);
		evalcache->print_info();
	} else {
		delete evalcache;
		evalcache = NULL;
	}
}

void Evaluator::clear_evalcache()
{
	if (evalcache) {
		evalcache->clear();
	}
}

