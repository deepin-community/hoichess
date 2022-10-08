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
#include "shell.h"
#ifdef WITH_THREAD
# include "parallelsearch.h"
#endif
#include "epd.h"
#include "pgn.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

const struct Shell::command Shell::shell_commands[] = {
	/* xboard protocol commands */
	{ "xboard",	&Shell::cmd_xboard,	""	},
	{ "protover",	&Shell::cmd_protover,	""	},
	{ "accepted",	&Shell::cmd_accepted,	""	},
	{ "rejected",	&Shell::cmd_rejected,	""	},
	{ "new",	&Shell::cmd_new,	"Start a new game" },
	{ "variant",	&Shell::cmd_variant,	""	},
	{ "quit",	&Shell::cmd_quit,	"Quit" },
	{ "random",	&Shell::cmd_null,	""	},
	{ "force",	&Shell::cmd_force,	"Let engine make no moves at all" },
	{ "go",		&Shell::cmd_go,		"Switch sides, let computer make next move" },
	{ "playother",	NULL,			""	},
	{ "white",	NULL,			""	},
	{ "black",	NULL,			""	},
	{ "level",	&Shell::cmd_level,	""	},
	{ "st",		&Shell::cmd_st,		""	},
	{ "sd",		&Shell::cmd_sd,		""	},
	{ "time",	&Shell::cmd_time,	""	},
	{ "otim",	&Shell::cmd_otim,	""	},
	{ "usermove",	&Shell::cmd_usermove,	""	},
	{ "?",		&Shell::cmd_null,	""	},
	{ "ping",	&Shell::cmd_ping,	""	},
	{ "draw",	&Shell::cmd_null,	""	},
	{ "result",	&Shell::cmd_null,	""	},
	{ "setboard",	&Shell::cmd_setboard,	""	},
	{ "edit",	NULL,			""	},
	{ "hint",	NULL,			""	},
	{ "bk",		&Shell::cmd_bk,		""	},
	{ "undo",	&Shell::cmd_undo,	""	},
	{ "remove",	&Shell::cmd_remove,	""	},
#ifdef WITH_THREAD
	{ "hard",	&Shell::cmd_hard,	"Turn on pondering (thinking on opponent's time)" },
	{ "easy",	&Shell::cmd_easy,	"Turn off pondering" },
#else
	{ "hard",	&Shell::cmd_null,	"Ignored (feature not compiled in)" },
	{ "easy",	&Shell::cmd_null,	"Ignored (feature not compiled in)" },
#endif
	{ "post",	&Shell::cmd_post,	"Show thinking output" },
	{ "nopost",	&Shell::cmd_nopost,	"Hide thinking output" },
#ifdef WITH_THREAD
	{ "analyze",	&Shell::cmd_analyze,	"Enter analysis mode" },
	{ "exit",	&Shell::cmd_exit,	"Leave analysis mode" },
#endif
	{ "name",	&Shell::cmd_null,	""	},
	{ "rating",	&Shell::cmd_null,	""	},
	{ "ics",	NULL,			""	},
	{ "computer",	&Shell::cmd_null,	""	},
	{ "pause",	NULL,			""	},
	{ "resume",	NULL,			""	},
	{ ".",		&Shell::cmd_null,	""	},
#ifdef WITH_THREAD
	{ "cores",	&Shell::cmd_cores,	""	},
#endif
	{ "option",	&Shell::cmd_option,	""	},
	
	/* own commands */
	{ "verbose",	&Shell::cmd_verbose,	""	},
	{ "debug",	&Shell::cmd_debug,	""	},
	{ "help",	&Shell::cmd_help,	""	},
	{ "source",	&Shell::cmd_source,	""	},
	{ "echo",	&Shell::cmd_echo,	""	},
	{ "noxboard",	&Shell::cmd_noxboard,	""	},
	{ "show",	&Shell::cmd_show,	""	},
	{ "solve",	&Shell::cmd_solve,	""	},
	{ "book",	&Shell::cmd_book,	""	},
	{ "hash",	&Shell::cmd_hash,	""	},
	{ "pawnhash",	&Shell::cmd_pawnhash,	""	},
	{ "evalcache",	&Shell::cmd_evalcache,	""	},
	{ "set",	&Shell::cmd_set,	""	},
	{ "get",	&Shell::cmd_get,	""	},
	{ "playboth",	&Shell::cmd_playboth,	""	},
	{ "loadgame",	&Shell::cmd_loadgame,	""	},
	{ "savegame",	&Shell::cmd_savegame,	""	},
	{ "redo",	&Shell::cmd_redo,	""	},
	{ "options",	&Shell::cmd_options,	"List all options and their current values"	},
	{ "atexit",	&Shell::cmd_atexit,	""	},
	
	{ NULL, NULL, NULL }
};



int Shell::cmd_null()
{
	return SHELL_CMD_OK;
}

int Shell::cmd_xboard()
{
	set_xboard(true);
	printf("\n"); 
	return SHELL_CMD_OK;
}

int Shell::cmd_protover()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	std::ostringstream ss;
	ss << "feature";

	/* replace " by ' and \ by / in myname */
	std::string myname_sanitized = myname;
	for (size_t i = 0; i<myname_sanitized.length(); i++) {
		if (myname_sanitized[i] == '"') {
			myname_sanitized[i] = '\'';
		} else if (myname_sanitized[i] == '\\') {
			myname_sanitized[i] = '/';
		}
	}
	ss << " myname=\"" << myname_sanitized << "\"";

#if defined(HOICHESS)
	ss << " variants=\"normal\"";
#elif defined(HOIXIANGQI)
	ss << " variants=\"xiangqi\"";
#else
# error "neither HOICHESS nor HOIXIANGQI defined"
#endif
	
	ss << " ping=1 setboard=1 time=1 sigint=0 sigterm=0 colors=0";

#ifdef WITH_THREAD
	ss << " analyze=1";
#else
	ss << " analyze=0";
#endif

	ss << " name=1";
#ifdef WITH_THREAD
	ss << " smp=1";
#endif
	
	for (std::list<struct option>::iterator it = option_list.begin();
			it != option_list.end(); it++) {
		ss << " option=\""
			<< it->name << " -spin "
			<< *it->varptr
			<< " " << INT_MIN
			<< " " << INT_MAX
			<< "\"";
	}

	ss << " done=1\n";

	atomic_printf("%s", ss.str().c_str());

	return SHELL_CMD_OK;
}

int Shell::cmd_accepted()
{
	/* no reaction */
	return SHELL_CMD_OK;
}

int Shell::cmd_rejected()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	
	printf("tellusererror Feature `%s' was rejected, expect problems\n",
			cmd_args[1].c_str());
	return SHELL_CMD_OK;
}

int Shell::cmd_new()
{
	stop_search();
	
	if (!game->set_board(opening_fen())) {
		BUG("Failed to set up standard opening position");
	}

	if (!flag_analyze) {
		flag_force = false;
		myside = BLACK;
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_variant()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	const std::string& v = cmd_args[1];

#if defined(HOICHESS)
	if (0) {
		/* for standard chess, the 'variant' command is never sent */
#elif defined(HOIXIANGQI)
	if (v == "xiangqi") {
		/* used by Winboard_F by H.G. Muller */
#else
# error "neither HOICHESS nor HOIXIANGQI defined"
#endif
	} else {
		printf("Error (variant not supported): %s\n", v.c_str());
		return SHELL_CMD_FAIL;
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_quit()
{
	quit = true;
	return SHELL_CMD_OK;
}

int Shell::cmd_force()
{
	stop_search();
	flag_force = true;
	flag_playboth = false;
	myside = NO_COLOR;
	return SHELL_CMD_OK;
}

int Shell::cmd_go()
{
	stop_search();
	flag_force = false;
	flag_playboth = false;
	myside = game->get_side();

	/* Turn back the current side's clock, so we can think the full amount
	 * of time, even though we've switched from human to engine. */
	game->turn_back_clock();

	return SHELL_CMD_OK;
}

int Shell::cmd_level()
{
	SHELL_CMD_REQUIRE_ARGS(3);
	
	int moves;
	if (sscanf(cmd_args[1].c_str(), "%d", &moves) != 1) {
		moves = -1;
	}

	/* Argument might be '5' (= 5 minutes) or '0:30' (= 30 seconds) */
	int mins, secs;
	if (sscanf(cmd_args[2].c_str(), "%d:%d", &mins, &secs) != 2) {
		secs = 0;
		if (sscanf(cmd_args[2].c_str(), "%d", &mins) != 1) {
			mins = -1;
		}
	}
	secs = mins * 60 + secs;
	
	int inc;
	if (sscanf(cmd_args[3].c_str(), "%d", &inc) != 1) {
		inc = -1;
	}

	if (moves < 0 || secs <= 0 || inc < 0) {
		printf("Illegal argument to command `level': %s %s %s\n",
				cmd_args[1].c_str(), 
				cmd_args[2].c_str(),
				cmd_args[3].c_str());
		return SHELL_CMD_FAIL;
	}

	Clock clock(moves, secs, inc);
	game->set_clocks(clock, clock);
	return SHELL_CMD_OK;
}

int Shell::cmd_st()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	
	int secs = atoi(cmd_args[1].c_str());
	if (secs > 0) {
		Clock clock(secs);
		game->set_clocks(clock, clock);
	} else {
		printf("Illegal time value.\n");
		return SHELL_CMD_FAIL;
	}
	return SHELL_CMD_OK;
}

int Shell::cmd_sd()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	
	unsigned int depth = atoi(cmd_args[1].c_str());
	if (depth == 0 || depth > MAXDEPTH) {
		maxdepth = MAXDEPTH;
	} else {
		maxdepth = depth;
	}
	printf("Search depth limited to %u ply.\n", maxdepth);
	return SHELL_CMD_OK;
}

int Shell::cmd_time()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	unsigned int csecs;
	if (sscanf(cmd_args[1].c_str(), "%u", &csecs) != 1) {
		printf("Illegal argument to command 'time': %s\n",
				cmd_args[1].c_str());
		return SHELL_CMD_FAIL;
	}

	if (myside != NO_COLOR) {
		game->set_remaining_time(myside, csecs);
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_otim()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	unsigned int csecs;
	if (sscanf(cmd_args[1].c_str(), "%u", &csecs) != 1) {
		printf("Illegal argument to command 'otim': %s\n",
				cmd_args[1].c_str());
		return SHELL_CMD_FAIL;
	}

	if (myside != NO_COLOR) {
		game->set_remaining_time(XSIDE(myside), csecs);
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_usermove()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	if (input_move(cmd_args[1])) {
		return SHELL_CMD_OK;
	} else {
		return SHELL_CMD_FAIL;
	}
}

int Shell::cmd_ping()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	atomic_printf("pong %s\n", cmd_args[1].c_str());
	return SHELL_CMD_OK;
}

int Shell::cmd_setboard()
{
	stop_search();
	
#if defined(HOICHESS)
	SHELL_CMD_REQUIRE_ARGS(6);
	std::string fen = cmd_args[1] + " " + cmd_args[2] + " "
		+ cmd_args[3] + " " + cmd_args[4] + " "
		+ cmd_args[5] + " " + cmd_args[6];
#elif defined(HOIXIANGQI)
	SHELL_CMD_REQUIRE_ARGS(4);
	std::string fen = cmd_args[1] + " " + cmd_args[2] + " "
		+ cmd_args[3] + " " + cmd_args[4];
#endif
	
	if (!game->set_board(fen.c_str())) {
		if (xboard) {
			printf("tellusererror Illegal position\n");
		} else {
			printf("Error (illegal position): %s\n",
					fen.c_str());
		}
		return SHELL_CMD_FAIL;
	}
	
	if (verbose) {
		print_result();
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_bk()
{
	BookEntry entry;
	if (book && book->lookup(game->get_board(), &entry)) {
		entry.print(game->get_board());
	} else {
		printf(" Nothing found in book\n");
	}
	
	/* Must finish with an empty line */
	printf("\n");

	return SHELL_CMD_OK;
}

int Shell::cmd_undo()
{
	stop_search();
	if (cmd_args.size() == 2 && cmd_args[1] == "all") {
		while (game->undo_move()) {}		
	} else {
		if (!game->undo_move()) {
			if (!xboard) {
				printf("No move to be undone.\n");
			}
		}
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_remove()
{
	stop_search();
	game->undo_move();
	game->undo_move();
	return SHELL_CMD_OK;
}

int Shell::cmd_hard()
{
	flag_ponder = true;
	return SHELL_CMD_OK;
}

int Shell::cmd_easy()
{
	stop_search();
	flag_ponder = false;
	return SHELL_CMD_OK;
}

int Shell::cmd_post()
{
	flag_showthinking = true;
	return SHELL_CMD_OK;
}

int Shell::cmd_nopost()
{
	flag_showthinking = false;
	return SHELL_CMD_OK;
}

#ifdef WITH_THREAD
int Shell::cmd_analyze()
{
	stop_search();
	cmd_force();
	cmd_post();
	flag_analyze = true;
	return SHELL_CMD_OK;
}

int Shell::cmd_exit()
{
	stop_search();
	flag_analyze = false;
	return SHELL_CMD_OK;
}
#endif // WITH_THREAD

#ifdef WITH_THREAD
/*
 * cores 1         activates standard search
 * cores N, N>=2   activates parallel search with N threads
 * cores 0         activates parallel search with 1 thread (for testing)
 */
int Shell::cmd_cores()
{
	unsigned int was_parallel = parallel;
	if (cmd_args.size() == 2) {
		unsigned int tmp = 0;
		if (sscanf(cmd_args[1].c_str(), "%u", &tmp) == 1) {
			if (tmp == 0) {
				parallel = 1;
			} else if (tmp == 1) {
				parallel = 0;
			} else {
				parallel = tmp;
			}
		} else {
			fprintf(stderr, "Illegal argument: %s\n",
					cmd_args[1].c_str());
			return SHELL_CMD_FAIL;
		}
	}

	if (parallel == was_parallel) {
		/* no change */
		return SHELL_CMD_OK;
	}

	if (parallel) {
		printf("Switching to parallel search with %u thread%s\n",
				parallel, (parallel==1 ? "" : "s"));
		stop_search();
		delete search;
		search = new ParallelSearch(this, parallel);
	} else {
		printf("Switching to non-parallel search\n");
		stop_search();
		delete search;
		search = new Search(this);
	}

	/* restore hash size etc. */
	search->set_hash_size(hashsize);
	search->set_pawnhash_size(pawnhashsize);
	search->set_evalcache_size(evalcachesize);

	return SHELL_CMD_OK;
}
#endif // WITH_THREAD

int Shell::cmd_option()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	const std::string& tmp = cmd_args[1];
	size_t i = tmp.find("=");
	if (i == std::string::npos) {
		printf("Illegal argument: %s\n", tmp.c_str());
		return SHELL_CMD_FAIL;
	}
	std::string name = tmp.substr(0, i);
	std::string val = tmp.substr(i+1, tmp.length());
	
	for (std::list<struct option>::iterator it = option_list.begin();
			it != option_list.end(); it++) {
		if (it->name == name) {
			int tmp;
			if (sscanf(val.c_str(), "%d", &tmp) != 1) {
				printf("Illegal value: %s\n", val.c_str());
				return SHELL_CMD_FAIL;
			}
			*it->varptr = tmp;
			return SHELL_CMD_OK;
		}
	}

	printf("Illegal option: %s\n", name.c_str());
	return SHELL_CMD_FAIL;
}

int Shell::cmd_verbose()
{
	if (cmd_args.size() == 2) {
		unsigned int tmp;
		if (sscanf(cmd_args[1].c_str(), "%d", &tmp) == 1) {
			verbose = tmp;
			printf("Verbosity set to %d.\n", verbose);
		}
	} else {
		printf("Verbosity set to %d.\n", verbose);
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_debug()
{
	if(cmd_args.size() == 2) {
		unsigned int tmp;
		if (sscanf(cmd_args[1].c_str(), "%d", &tmp) == 1) {
			debug = tmp;
			printf("Debug level set to %d.\n", debug);
		}
	} else {
		printf("Debug level set to %d.\n", debug);
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_help()
{
	printf("Available commands:\n");

	printf("\t<move>\t\tPlay move (coordinate notation or SAN)\n");
	for (std::list<struct command>::iterator it = commands.begin();
			it != commands.end(); it++) {
		if (it->func == NULL)
			continue;

		printf("\t%s\t\t%s\n", it->name, it->usage);
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_source()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	const char * filename = cmd_args[1].c_str();

	FILE * fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open %s for reading: %s\n",
				filename, strerror(errno));
		return SHELL_CMD_FAIL;
	}

	source_file(fp, filename);
	return SHELL_CMD_OK;
}

int Shell::cmd_echo()
{
	std::string s;
	for (unsigned int i=1; i<cmd_args.size(); i++) {
		if (i > 1) {
			s += " ";
		}
		s += cmd_args[i];
	}
	atomic_printf("%s\n", s.c_str());
	return SHELL_CMD_OK;
}

int Shell::cmd_noxboard()
{
	set_xboard(false);
	return SHELL_CMD_OK;
}

int Shell::cmd_show()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	std::string param = cmd_args[1];

	const Board & board = game->get_board();
	
	if (param == "board") {
		board.print();
	} else if (param == "fen") {
		printf("%s\n", board.get_fen().c_str());
	} else if (param == "moves" || param == "captures"
			|| param == "noncaptures" || param == "escapes") {
		Movelist moves;
		if (param == "moves") {
			board.generate_moves(&moves);
		} else if (param == "captures") {
			board.generate_captures(&moves);
		} else if (param == "noncaptures") {
			board.generate_noncaptures(&moves);
		} else if (param == "escapes") {
			board.generate_escapes(&moves);
		} else {
			BUG("param == %s", param.c_str());
		}

		moves.filter_illegal(board);

		unsigned int j=1;
		for (unsigned int i=0; i<moves.size(); i++) {
			if (!moves[i].is_legal(board)) {
				printf("(%s)\t", moves[i].san(board).c_str());
			} else {
				printf("%s\t", moves[i].san(board).c_str());
			}

			j++;
			if (j == 8) {
				printf("\n");
				j=1;
			}
		}
		if (j != 1 && j != 8) {
			printf("\n");
		}
	} else if (param == "eval") {
		Evaluator eval;
		printf("Symmetric evaluation:\n");
		eval.print_eval(board, NO_COLOR);
		printf("--------------------------------------------------\n");
		printf("Evaluation if I would play white:\n");
		eval.print_eval(board, WHITE);
		printf("--------------------------------------------------\n");
		printf("Evaluation if I would play black:\n");
		eval.print_eval(board, BLACK);
	} else if (param == "clocks") {
		printf("[White]\n"); game->get_clock(WHITE)->print();
		printf("\n[Black]\n"); game->get_clock(BLACK)->print();
	} else if (param == "game") {
		game->print(stdout);
	} else if (param == "pgn") {
		game->write_pgn(stdout);
	} else {
		printf("Usage: show {board|fen}\n");
		printf("       show {moves|captures|noncaptures|escapes}\n");
		printf("       show eval\n");
		printf("       show clocks\n");
		printf("       show game\n");
		printf("       show pgn\n");
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_solve()
{
	stop_search();
	
	SHELL_CMD_REQUIRE_ARGS(1);
	const char * filename = cmd_args[1].c_str();
	
	FILE * fp;
	bool close_fp;
	if (strcmp(filename, "-") == 0) {
		fp = stdin;
		close_fp = false;
	} else {
		fp = fopen(filename, "r");
		if (fp == NULL) {
			printf("Cannot open %s: %s\n",
					filename, strerror(errno));
			return SHELL_CMD_FAIL;
		}
		close_fp = true;
	}

	int right = 0;
	int wrong = 0;
	int total = 0;
	int skipped = 0;
	
	char buf[1024];
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		/* Strip trailing \n and \r */
		while (buf[strlen(buf)-1] == '\n' || buf[strlen(buf)-1] == '\r') {
			buf[strlen(buf)-1] = '\0';
		}

		if (strcmp(buf, ".") == 0) {
			break;
		}

		printf("--------------------------------------------------\n");
		
		/* Parse EPD, print FEN */
		EPD epd(buf);
		Board board(epd.get_fen().c_str());
		printf("[%s] %s\n", 
				epd.get1("id").c_str(),
				epd.get_fen().c_str());

		/* Get the list of best moves */
		std::list<Move> bms = epd.get_bm();
		if (bms.size() == 0) {
			printf("No best move associated to this position,"
					" skipping.\n");
			skipped++;
			continue;
		}

		/* Print the list of best moves */
		printf("[%s] best move:", epd.get1("id").c_str());
		for (std::list<Move>::const_iterator it = bms.begin();
				it != bms.end(); it++) {
			printf(" %s", it->san(board).c_str());
		}
		printf("\n");

		/* Reset search stuff */
		search->clear_hash();
		search->clear_pawnhash();
		search->clear_evalcache();

		/* Must copy the clock from the game because this is set
		 * to the desired time limit. If a previous search was
		 * interrupted, we might get here with a running clock,
		 * so restart its copy. */
		Clock clock = *game->get_clock();
		clock.stop();
		clock.turn_back();
		clock.start();

		/* Start search */
		printf("\n");
		board.print_small();
		printf("\n");
		printf("Thinking...\n");
		Move mov = search->start(board, clock, Search::MOVE, maxdepth);
		if (!mov) {
			printf("Warning: search returned NO_MOVE"
					", excluding position from result\n");
			continue;
		}
		
		/* Look if our move is among the best. */
		bool correct = false;
		std::list<Move>::const_iterator it;
		for (it = bms.begin(); it != bms.end(); it++) {
			if (*it == mov) {
				right++;
				correct = true;
				break;
			}
		}
		if (it == bms.end()) {
			wrong++;
			correct = false;
		}
		
		printf("My move: %s (%s)\n", mov.san(board).c_str(),
				correct ? "correct" : "incorrect");
		
		total = right + wrong;
		printf("Correct: %d of %d (%d%%), skipped: %d\n",
				right, total, 100 * right / total, skipped);
		printf("SOLVE %u %u %s %u %s\n",
				total, right,
				mov.san(board).c_str(), correct,
				epd.get1("id").c_str());

		if (stop) {
			break;
		}
	}	

	printf("SOLVE_DONE %u %u\n", total, right);

	if (close_fp) {
		fclose(fp);
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_book()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	const std::string param = cmd_args[1];

	if (param == "close" || param == "off") {
		delete book;
		book = NULL;
	} else if (param == "open") {
		SHELL_CMD_REQUIRE_ARGS(2);
		const char * file = cmd_args[2].c_str();
		set_book(file);
	} else if (param == "create") {
		SHELL_CMD_REQUIRE_ARGS(5);
		const char * destfile = cmd_args[2].c_str();
		const char * srcfile = cmd_args[3].c_str();
		int depth;
		if (sscanf(cmd_args[4].c_str(), "%d", &depth) != 1) {
			printf("Error: argument <depth> must be non-negative"
					" integer\n");
			return SHELL_CMD_FAIL;
		}
		int min_move_count;
		if (sscanf(cmd_args[5].c_str(), "%d", &min_move_count) != 1) {
			printf("Error: argument <min_move_count> must be"
					" non-negative integer\n");
			return SHELL_CMD_FAIL;
		}

		printf("Creating opening book `%s' from `%s' ...\n",
				destfile, srcfile);
		Book::create_from_pgn(destfile, srcfile, depth, min_move_count);
	} else {
		printf("Usage: book close\n");
		printf("       book open <bookfile>\n");
		printf("       book create <bookfile> <pgnfile> <depth>"
							" <min_move_count>\n");
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_hash()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	const std::string param = cmd_args[1];

	if (param == "clear") {
		search->clear_hash();
	} else if (param == "size") {
	 	SHELL_CMD_REQUIRE_ARGS(2);
		const std::string& s = cmd_args[2];
		ssize_t size;
		if (s == "-") {
			size = get_hash_size();
		} else if (!parse_size(s.c_str(), &size) || size < 0) {
			printf("Illegal value for hash table size: %s\n",
					s.c_str());
			return SHELL_CMD_FAIL;
		}
		stop_search();
		set_hash_size((size_t) size);
	} else if (param == "off") {
		stop_search();
		set_hash_size(0);
	} else {
		printf("Usage: hash clear\n");
		printf("       hash size <size>\n");
		printf("       hash off\n");
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_pawnhash()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	const std::string param = cmd_args[1];

	if (param == "clear") {
		search->clear_pawnhash();
	} else if (param == "size") {
	 	SHELL_CMD_REQUIRE_ARGS(2);
		const char * s = cmd_args[2].c_str();
		ssize_t size;
		if (!parse_size(s, &size) || size < 0) {
			printf("Illegal value for pawn hash table size: %s\n",
					s);
			return SHELL_CMD_FAIL;
		}
		stop_search();
		set_pawnhash_size((size_t) size);
	} else if (param == "off") {
		stop_search();
		set_pawnhash_size(0);
	} else {
		printf("Usage: pawnhash clear\n");
		printf("       pawnhash size <size>\n");
		printf("       pawnhash off\n");
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_evalcache()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	const std::string param = cmd_args[1];

	if (param == "clear") {
		search->clear_evalcache();
	} else if (param == "size") {
	 	SHELL_CMD_REQUIRE_ARGS(2);
		const char * s = cmd_args[2].c_str();
		ssize_t size;
		if (!parse_size(s, &size) || size < 0) {
			printf("Illegal value for evaluation cache size: %s\n",
					s);
			return SHELL_CMD_FAIL;
		}
		stop_search();
		set_evalcache_size((size_t) size);
	} else if (param == "off") {
		stop_search();
		set_evalcache_size(0);
	} else {
		printf("Usage: evalcache clear\n");
		printf("       evalcache size <size>\n");
		printf("       evalcache off\n");
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_set()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	if (cmd_args[1] == "myname") {
		SHELL_CMD_REQUIRE_ARGS(2);
		std::string tmp;
		for (unsigned int i=2; i<cmd_args.size(); i++) {
			if (i > 2) {
				tmp += " ";
			}
			tmp += cmd_args[i];
		}
		set_myname(tmp.c_str());
		printf("myname set to \"%s\"\n", tmp.c_str());
	} else {
		printf("Illegal argument to command 'set': '%s'\n",
				cmd_args[1].c_str());
		return SHELL_CMD_FAIL;
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_get()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	if (cmd_args[1] == "myname") {
		printf("myname = %s\n", myname.c_str());
	} else {
		printf("Illegal argument to command 'get': '%s'\n",
				cmd_args[1].c_str());
		return SHELL_CMD_FAIL;
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_playboth()
{
	stop_search();
	flag_playboth = true;
	flag_force = false;
	return SHELL_CMD_OK;
}

int Shell::cmd_loadgame()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	const char * pgnfile  = cmd_args[1].c_str();

	FILE* fp = fopen(pgnfile, "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open %s for reading: %s\n",
				pgnfile, strerror(errno));
		return SHELL_CMD_FAIL;
	}

	PGN pgn;
	pgn.parse(fp);
	fclose(fp);

	Game g(pgn, Clock(), Clock());
	printf("--- begin read game ---\n");
	g.write_pgn(stdout);
	printf("--- end read game ---\n");

	stop_search();
	*game = g;

	return SHELL_CMD_OK;
}

int Shell::cmd_savegame()
{
	SHELL_CMD_REQUIRE_ARGS(1);
	const char * pgnfile  = cmd_args[1].c_str();

	FILE* fp = fopen(pgnfile, "w");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open %s for writing: %s\n",
				pgnfile, strerror(errno));
		return SHELL_CMD_FAIL;
	}
	
	game->write_pgn(fp);
	
	fclose(fp);

	return SHELL_CMD_OK;
}

int Shell::cmd_redo()
{
	stop_search();
	
	Move mov;
	if (cmd_args.size() == 2 && cmd_args[1] == "all") {
		Move mov0;
		do {
			mov = mov0;
			mov0 = game->redo_move();
		} while (mov0);

		if (!mov) {
			if (!xboard) {
				printf("No move to be redone.\n");
			}
			return SHELL_CMD_OK;
		}
	} else {
		mov = game->redo_move();
		if (!mov) {
			if (!xboard) {
				printf("No move to be redone.\n");
			}
			return SHELL_CMD_OK;
		}
	}

	if (!xboard) {
		game->get_board().print(stdout, mov);
		if (game->get_result()) {
			print_result();
		}
	}

	return SHELL_CMD_OK;
}

int Shell::cmd_options()
{
	for (std::list<struct option>::iterator it = option_list.begin();
			it != option_list.end(); it++) {
		printf("%s = %d\n", it->name, *it->varptr);
	}
	return SHELL_CMD_OK;
}

int Shell::cmd_atexit()
{
	SHELL_CMD_REQUIRE_ARGS(1);

	std::vector<std::string> tmp = cmd_args;
	tmp.erase(tmp.begin());
	atexit_commands.push_back(tmp);

	return SHELL_CMD_OK;
}
