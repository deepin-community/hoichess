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
#include "game.h"


GameEntry::GameEntry(const Board & board, Move mov,
		const Clock & wclock, const Clock & bclock,
		unsigned int flags)
{
	this->board = board;
	this->move = mov;
	this->clock[WHITE] = wclock;
	this->clock[BLACK] = bclock;
	this->flags = flags;
}


/*
 * Create a new game, starting from the position described by board
 * and assign a clock to each side.
 */
Game::Game(const Board & board, const Clock & wclock, const Clock & bclock)
{
	construct(board, wclock, bclock);
}

/*
 * Create a new game by loading a PGN.
 */
Game::Game(const PGN & pgn, const Clock & wclock, const Clock & bclock)
{
	construct(pgn.get_opening(), wclock, bclock);

	/* make moves */
	std::list<Move> moves = pgn.get_moves();
	for (std::list<Move>::const_iterator 
			it = moves.begin();
			it != moves.end();
			it++) {
		make_move(*it, 0);
	}
}

/* 
 * Common code for all constructors.
 * Remember that a constructor cannot call another constructor on 'this',
 * so we need a helper function...
 */
void Game::construct(const Board & board, const Clock & wclock, const Clock & bclock)
{
	initial_board = current_board = board;
	initial_clock[WHITE] = old_clock[WHITE] = current_clock[WHITE] = wclock;
	initial_clock[BLACK] = old_clock[BLACK] = current_clock[BLACK] = bclock;
	
	running = false;
	result = OPEN;
}

bool Game::is_over() const
{
	return result != OPEN;
}

bool Game::is_running() const
{
	return running;
}

int Game::get_result() const
{
	return result;
}

std::string Game::get_result_str() const
{
	return result_str;
}

std::string Game::get_result_comment() const
{
	return result_comment;
}

Board Game::get_board() const
{
	return current_board; 
}

int Game::get_side() const
{
	return current_board.get_side();
}

const Clock* Game::get_clock() const
{
	return &current_clock[get_side()];
}

Clock* Game::get_clock()
{
	return &current_clock[get_side()];
}

const Clock* Game::get_clock(Color side) const
{
	ASSERT(side == WHITE || side == BLACK);
	return &current_clock[side];
}

const Board& Game::get_opening() const
{
	return initial_board;
}

const std::list<GameEntry>& Game::get_entries() const
{
	return entries;
}


void Game::start()
{
	current_clock[get_side()].start();
	running = true;
}

void Game::make_move(Move mov, unsigned int flags)
{
	ASSERT(mov.is_valid(current_board));
	ASSERT(mov.is_legal(current_board));

	/* stop clock of side that has played the current move */
	current_clock[get_side()].stop();

	/* make sure we don't put any running clocks in game history */
	ASSERT(!current_clock[WHITE].is_running());
	ASSERT(!current_clock[BLACK].is_running());
	ASSERT(!old_clock[WHITE].is_running());
	ASSERT(!old_clock[BLACK].is_running());

	/* save current position */
	GameEntry entry(current_board, mov, old_clock[WHITE], old_clock[BLACK],
			flags);
	entries.push_back(entry);
	
	/* update current position with new move and clocks */
	current_board.make_move(mov);
	old_clock[WHITE] = current_clock[WHITE];
	old_clock[BLACK] = current_clock[BLACK];

	/* check result and start clock of side to move */
	check_result();
	if (!result) {
		current_clock[get_side()].start();
		running = true;
	} else {
		running = false;
	}

	undone_entries.clear();
}

bool Game::undo_move()
{
	if (entries.size() == 0) {
		running = false;
		return false;
	}

	/* get undo information */
	const GameEntry last = entries.back();
	entries.pop_back();
	
	/* save last state */
	old_clock[WHITE].stop();
	old_clock[BLACK].stop();
	GameEntry undone(current_board,
			last.get_move(),
			old_clock[WHITE], old_clock[BLACK],
			last.get_flags());
	undone_entries.push_front(undone);

	/* load last state */
	current_board = last.get_board();
	old_clock[WHITE] = current_clock[WHITE] = last.get_clock(WHITE);
	old_clock[BLACK] = current_clock[BLACK] = last.get_clock(BLACK);
	current_clock[current_board.get_side()].start();
	
	/* set result to open */
	running = true;
	result = OPEN;
	
	
	return true;
}

/*
 * FIXME This does not restore clocks correctly.
 */
Move Game::redo_move()
{
	if (undone_entries.size() == 0) {
		return NO_MOVE;
	}

	/* get undo information */
	GameEntry undone = undone_entries.front();
	undone_entries.pop_front();

	/* stop clock of side that has played the current move */
	current_clock[get_side()].stop();

	/* make sure we don't put any running clocks in game history */
	old_clock[WHITE].stop();
	old_clock[BLACK].stop();

	/* save current position */
	GameEntry entry(current_board,
			undone.get_move(),
			old_clock[WHITE], old_clock[BLACK],
			undone.get_flags());
	entries.push_back(entry);
	
	/* update current position with new move and clocks */
	current_board = undone.get_board();
	current_clock[WHITE] = old_clock[WHITE] = undone.get_clock(WHITE);
	current_clock[BLACK] = old_clock[BLACK] = undone.get_clock(BLACK);

	/* check result and start clock of side to move */
	check_result();
	if (!result) {
		current_clock[get_side()].start();
		running = true;
	} else {
		running = false;
	}

	return undone.get_move();
}

/*
 * Set up a new position based on the given board. Take the clocks as
 * they were at the _beginning_ of the old game.
 */
void Game::set_board(const Board & board)
{
	ASSERT(board.is_valid());
	ASSERT(board.is_legal());

	initial_board = current_board = board;
	old_clock[WHITE] = current_clock[WHITE] = initial_clock[WHITE];
	old_clock[BLACK] = current_clock[BLACK] = initial_clock[BLACK];
	
	entries.clear();

	running = false;
	check_result();
}

bool Game::set_board(const std::string& fen)
{
	Board board;
	if (!board.parse_fen(fen)) {
		entries.clear();
		running = false;
		result = ILLEGAL;
		return false;
	}
	
	set_board(board);
	return true;
}

/*
 * Replace the current _and_initial_ clocks of both sides by new ones.
 */
void Game::set_clocks(const Clock & wclock, const Clock & bclock)
{
	ASSERT(!wclock.is_running());
	ASSERT(!bclock.is_running());
	initial_clock[WHITE] = old_clock[WHITE] = current_clock[WHITE] = wclock;
	initial_clock[BLACK] = old_clock[BLACK] = current_clock[BLACK] = bclock;

	if (running) {
		current_clock[current_board.get_side()].start();
	}
}

void Game::turn_back_clock()
{
	current_clock[get_side()].turn_back();
}

void Game::set_remaining_time(Color side, unsigned int csecs)
{
	ASSERT(side == WHITE || side == BLACK);
	current_clock[side].set_remaining_time(Clock::from_cs(csecs));
}

/*
 * Count how often this board appeared in the game history,
 * _not_ including the current position.
 */
int Game::repetitions(const Board & board) const
{
	int rep = 0;
	
	for (std::list<GameEntry>::const_reverse_iterator it = entries.rbegin();
			it != entries.rend(); it++) {
		if (it->get_board() == board)
			rep++;
	}

	return rep;
}

/*
 * Count the number of (full-)moves of the current player since his last
 * book move. If the last move was out of book, this number is 1.
 * If no move was played out of book at all, return 0.
 */
int Game::last_bookmove() const
{
	unsigned int n = 0;
	for (std::list<GameEntry>::const_reverse_iterator it = entries.rbegin();
			it != entries.rend(); it++) {
		if (it->get_board().get_side() != get_side()) {
			continue;
		}
		
		n++;
		if (it->get_flags() & GameEntry::FLAG_BOOKMOVE) {
#ifdef DEBUG
			printf("last bookmove: %d moves ago\n", n);
#endif
			return n;
		}
	}

#ifdef DEBUG
	printf("last bookmove: not found\n");
#endif
	return 0;
}

/*
 * Check if the game has ended by rule.
 */
void Game::check_result()
{
	const Board & board = get_board();
	if (board.is_mate()) {
		if (board.get_side() == WHITE) {
			result = BLACKMATES;
			result_str = "0-1";
			result_comment = "Black mates";
		} else {
			result = WHITEMATES;
			result_str = "1-0";
			result_comment = "White mates";
		}
		
	} else if (board.is_stalemate()) {
#if defined(HOICHESS)
		result = STALEMATE;
		result_str = "1/2-1/2";
		result_comment = "Stalemate";
#elif defined(HOIXIANGQI)
		if (board.get_side() == WHITE) {
			result = BLACKSTALEMATES;
			result_str = "0-1";
			result_comment = "Black stalemates";
		} else {
			result = WHITESTALEMATES;
			result_str = "1-0";
			result_comment = "White stalemates";
		}
#else
# error "neither HOICHESS nor HOIXIANGQI defined"
#endif
	} else if (board.get_movecnt50() == 100) {
		result = RULE50;
		result_str = "1/2-1/2";
		result_comment = "50 move rule";
	} else if (repetitions(board) >= 2) {
		result = REPS3;
		result_str = "1/2-1/2";
		result_comment = "3 repetitions";
	} else if (board.is_material_draw()) {
		result = MATERIAL;
		result_str = "1/2-1/2";
		result_comment = "Insufficient material";
	} else {
		result = OPEN;
		result_str = "*";
		result_comment = "Open";
	}
}

void Game::print(FILE * fp) const
{
	fprintf(fp, "Positions in game history: %d\n", (int) entries.size());
	for (std::list<GameEntry>::const_iterator it = entries.begin();
			it != entries.end(); it++) {
		fprintf(fp, "---------------------------------------------\n");
		it->get_board().print_small(fp);
		fprintf(fp, "\n");
		
		fprintf(fp, "White clock:\n");
		it->get_clock(WHITE).print(fp);
		fprintf(fp, "Black clock:\n");
		it->get_clock(BLACK).print(fp);
		fprintf(fp, "\n");
		
		fprintf(fp, "Move played: %s\n",
				it->get_move().san(it->get_board()).c_str());
		fprintf(fp, "Move flags:");
		if (it->get_flags() & GameEntry::FLAG_COMPUTER) {
			fprintf(fp, " computer");
		}
		if (it->get_flags() & GameEntry::FLAG_BOOKMOVE) {
			fprintf(fp, " bookmove");
		}
		fprintf(fp, "\n\n");
	}
}

void Game::write_pgn(FILE * fp) const
{
	/* standard tags (seven tag roster) */
	fprintf(fp, "[Event \"unknown\"]\n");
	fprintf(fp, "[Site \"unknown\"]\n");
	fprintf(fp, "[Date \"unknown\"]\n");
	fprintf(fp, "[Round \"unknown\"]\n");
	fprintf(fp, "[White \"unknown\"]\n");
	fprintf(fp, "[Black \"unknown\"]\n");
	fprintf(fp, "[Result \"%s\"]\n", get_result_str().c_str());

	/* additional tag: FEN if non-standard starting position */
	std::string ofen = 
		(entries.size() > 0)
		? entries.begin()->get_board().get_fen()
		: current_board.get_fen();
	if (ofen != opening_fen()) {
		fprintf(fp, "[FEN \"%s\"]\n", ofen.c_str());
	}
	fprintf(fp, "\n");
	
	/* moves */
	unsigned int i = 1;
	unsigned int nchars = 0;
	for (std::list<GameEntry>::const_iterator it = entries.begin();
			it != entries.end(); it++) {
		std::string s = it->get_move().san(it->get_board());

		if (it->get_board().get_side() == WHITE) {
			nchars += fprintf(fp, "%d. %s ", i, s.c_str());
		} else {
			nchars += fprintf(fp, "%s ", s.c_str());
			i++;
		}

		if (nchars > 70) {
			fprintf(fp, "\n");
			nchars = 0;
		}
	}
	fprintf(fp, "\n");

	/* result */
	fprintf(fp, "%s {%s}\n",
			get_result_str().c_str(),
			get_result_comment().c_str());
}

