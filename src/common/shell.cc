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
#include "board.h"
#include "shell.h"

#include <errno.h> 
#ifdef HAVE_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif
#include <stdio.h>
#include <string.h>
#include <unistd.h>


Shell::Shell()
{
	xboard = false;
	
	flag_force = false;
	flag_ponder = false;
	flag_showthinking = false;
	flag_analyze = false;
	flag_playboth = false;

	source.name = "(stdin)";
	source.line = 0;
	source.fp = stdin;

	register_commands(shell_commands);

	register_option("verbose", (int *) &verbose, verbose);
	register_option("debug", (int *) &debug, debug);
#define SHELL_DEFINE_OPTION SHELL_REGISTER_OPTION
# include "shell_option_defs.h"
#undef SHELL_DEFINE_OPTION
	register_option("echo", &echo, echo);

	set_myname(NULL);

	Clock clock(5);
	game = new Game(Board(), clock, clock);	
	book = NULL;

#ifdef WITH_THREAD
	parallel = 0;
#endif
	search = new Search(this);

	maxdepth = MAXDEPTH;

	last_status_line_length = 0;
}

Shell::~Shell()
{
	delete search;
	delete book;
	delete game;
}

int Shell::main()
{
	cmd_new();

	if (!isatty(1)) {
		/* probably output is redirected to a log file, so
		 * don't print lines with \r that will be overwritten */
		print_search_output_terminal_newline = true;
	}

	if (!xboard && (source.fp != stdin || !isatty(0))) {
		printf("Reading %s\n", source.name.c_str());
	}	
	
	quit = false;
	while (!quit) {
		stop = false;
		
		if (game->is_over()) {
			/* nothing */
		} else if (flag_playboth) {
			/* let engine make a move */
			myside = game->get_side();
			engine_move();
			continue;
		} else if (!flag_force && game->get_side() == myside) {
			/* let engine make a move */
			engine_move();
			continue;
		} else if (flag_ponder && !flag_force && game->is_running()) {
			/* let engine search in background */
			engine_ponder();
		} else if (flag_analyze) {
			/* let engine search in background */
			engine_analyze();
		}

		if (input() != SHELL_CMD_OK) {
			return EXIT_FAILURE;
		}
	}

	stop_search();

	printf("\n");

	for (std::list<std::vector<std::string> >::iterator
			it = atexit_commands.begin();
			it != atexit_commands.end();
			it++) {
		exec_command(*it);
	}

	return EXIT_SUCCESS;
}

/* 
 * This function will be called by the SIGINT-handler
 * in main.cc, so keep it short.
 */
void Shell::interrupt()
{
	if (!xboard) {
		printf("Interrupt\n");
	}
	search->interrupt();
	stop = true;

	flag_playboth = false;
}

/*
 * Set the opening book. If bookfile is NULL, disable opening book.
 */
void Shell::set_book(const char * bookfile)
{
	if (bookfile) {
		delete book;
		bool ok;
		book = new Book(bookfile, &ok);
		if (!ok) {
			/* Yuck! Under xboard, we must not print messages
			 * containing things like 'no such file' because this
			 * will cause an immediate abort.
			 * Because when trying to open the default book
			 * xboard mode has not yet been set, we also check
			 * if stdout is a tty, supposing xboard mode if not. */
			if (xboard || !isatty(1)) {
				printf("Failed to open opening book `%s'.\n",
						bookfile);
			} else {
				printf("Failed to open opening book `%s': %s\n",
						bookfile, strerror(errno));
			}
			book = NULL;
		} else {
			printf("Opening book: %s\n", bookfile);
		}
	} else {
		delete book;
		book = NULL;
	}
}

/*
 * Set the size of the hash table in bytes. 0 disables hash table.
 */
void Shell::set_hash_size(size_t bytes)
{
	stop_search();
	search->set_hash_size(bytes);
	/* save value so that cmd_cores() has it available when
	 * re-creating search */
	hashsize = bytes;
}

size_t Shell::get_hash_size() const
{
	return hashsize;
}

/*
 * Set the size of the pawn hash table in bytes. 0 disables pawn hash table.
 */
void Shell::set_pawnhash_size(size_t bytes)
{
	stop_search();
	search->set_pawnhash_size(bytes);
	/* save value so that cmd_cores() has it available when
	 * re-creating search */
	pawnhashsize = bytes;
}

size_t Shell::get_pawnhash_size() const
{
	return pawnhashsize;
}

/*
 * Set the size of the evaluation cache in bytes. 0 disables evaluation cache.
 */
void Shell::set_evalcache_size(size_t bytes)
{
	stop_search();
	search->set_evalcache_size(bytes);
	/* save value so that cmd_cores() has it available when
	 * re-creating search */
	evalcachesize = bytes;
}

size_t Shell::get_evalcache_size() const
{
	return evalcachesize;
}

/*
 * Set the engine's name.
 */
void Shell::set_myname(const char * name)
{
	if (name) {
		myname = name;
	} else {
		myname = PROGNAME + std::string(" ") + VERSION;
	}
}

/*
 * Get the engine's name.
 */
const char * Shell::get_myname() const
{
	return myname.c_str();
}

/*
 * Set/unset xboard mode.
 */
void Shell::set_xboard(bool x)
{
	xboard = x;
	if (xboard) {
		setbuf(stdout, NULL);
	}
}


void Shell::source_file(FILE * fp, const char * name)
{
	ASSERT(fp != NULL);

	/* save current source */
	sources.push_back(source);

	/* open new source */
	source.name = name;
	source.line = 0;
	source.fp = fp;
}

int Shell::exec_command(const char * s)
{
	/* Need s writable for strtok. */
	char * s1 = strdup(s);
	ASSERT(s1 != NULL);

	/* Tokenize the input. */
	std::vector<std::string> args;
	const char * delim = " \t\n";
	char * strtok_r_buf;
	for (char * p = strtok_r(s1, delim, &strtok_r_buf); 
			p != NULL;
			p = strtok_r(NULL, delim, &strtok_r_buf)) {
		args.push_back(p);
	}

	free(s1);

	/* Ignore empty commands and comments. */
	if (args.size() == 0 || args[0][0] == '#') {
		return SHELL_CMD_OK;
	}

	return exec_command(args);
}

int Shell::exec_command(const std::vector<std::string>& args)
{
	cmd_args = args;

#ifdef WIN32
	const char * vbegin_delim = "$%";
#else
	const char * vbegin_delim = "$";
#endif
	/* this allows variable names to start with a number, but we can
	 * live with that */
	const char * varchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz0123456789_";

	/* Environment variable substitution. Currently only $VAR syntax,
	 * not ${VAR}. On Windows, also accept %VAR%. */
	for (std::vector<std::string>::iterator it = cmd_args.begin();
			it != cmd_args.end(); it++) {
		size_t pos = 0;
		size_t vbegin, vend;
		while ((vbegin = it->find_first_of(vbegin_delim, pos))
				!= std::string::npos) {
			size_t len;
			if ((*it)[vbegin] == '%') {
				vend = it->find_first_of("%", vbegin + 1);
				len = vend - vbegin + 1;
				if (vend == std::string::npos) {
					break;
				}
			} else {
				vend = it->find_first_not_of(varchars,
					vbegin + 1);
				len = vend - vbegin;
			}

			std::string var = it->substr(vbegin + 1,
					vend - vbegin - 1);

			const char * tmp = getenv(var.c_str());
			if (tmp != NULL) {
				std::string val = tmp;
				it->replace(vbegin, len, val);
				pos = vbegin + val.length();
			} else {
				pos += len;
			}
		}
	}

	if (echo) {
		printf("#");
		for (std::vector<std::string>::const_iterator
				it = cmd_args.begin(); it != cmd_args.end();
				it++) {
			printf(" %s", it->c_str());
		}
		printf("\n");
	}

	/* Walk through the list of registered commands. */
	for (std::list<struct command>::iterator it = commands.begin();
			it != commands.end(); it++) {
		if (cmd_args[0] != it->name)
			continue;

		/* If a function is registered, call it. */
		if (it->func != NULL) { 
			return (this->*it->func)();
		} else {
			printf("Error (command not implemented): %s\n",
					cmd_args[0].c_str());
			return SHELL_CMD_FAIL;
		}
	}

	/* The input wasn't recognized as a command,
	 * so check if it is a move. */
	if (cmd_args.size() == 1) {
		if (input_move(cmd_args[0])) {
			return SHELL_CMD_OK;
		}
	}

	printf("Error (unknown command): %s\n", cmd_args[0].c_str());
	return SHELL_CMD_FAIL;
}

void Shell::register_commands(const struct Shell::command* cmds)
{
	for (int i=0; cmds[i].name != NULL; i++) {
		commands.push_back(cmds[i]);
	}
}

void Shell::register_option(const char * name, int * varptr, int defval)
{
	struct option opt;
	opt.name = name;
	opt.varptr = varptr;
	option_list.push_back(opt);
	*varptr = defval;
}

int Shell::input()
{
	char * line = get_line(get_prompt().c_str());
	if (line == NULL) {
		quit = true;
		return SHELL_CMD_OK;
	}
	
	/* Strip trailing \n and \r */
	while (line[strlen(line)-1] == '\n' || line[strlen(line)-1] == '\r') {
		line[strlen(line)-1] = '\0';
	}

	/* When command from script fails, exit */
	int ret = exec_command(line);
	if (ret != SHELL_CMD_OK && (source.fp != stdin || !isatty(0))) {
		return SHELL_CMD_FAIL;
	}

	free(line);
	return SHELL_CMD_OK;
}

char * Shell::get_line(const char * prompt)
{
again:
	char * line;

	if (source.fp == NULL) {
		return NULL;
	}

	int fd = fileno(source.fp);
	if (xboard) {
		line = get_line_fgets(source.fp, NULL);
	} else if (fd == 0 && isatty(fd)) {
#ifdef HAVE_READLINE
		line = get_line_readline(prompt);
#else
		line = get_line_fgets(stdin, prompt);
#endif
	} else {
		line = get_line_fgets(source.fp, NULL);
	}	
	source.line++;
	
	if (line == NULL && sources.size() > 0) {
		fclose(source.fp);
		source = sources.back();
		sources.pop_back();

		if (!xboard && (source.fp != stdin || !isatty(0))) {
			printf("Reading %s\n", source.name.c_str());
		}	
	
		goto again;
	}
		
	return line;
}

char * Shell::get_line_fgets(FILE * fp, const char * prompt)
{
	ASSERT(fp != NULL);
	if (prompt) {
		printf("%s", prompt);
	}

	char buf[1024];
	char * p = fgets(buf, sizeof(buf)-1, fp);
	if (p != NULL) {
		char * ret = (char *) malloc((strlen(p)+1) * sizeof(char));
		strcpy(ret, p);
		return ret;
	} else {
		return NULL;
	}
}

#ifdef HAVE_READLINE
char * Shell::get_line_readline(const char * prompt)
{
	char * line = readline(prompt);
	if (line && *line) {
		add_history(line);
	}
	return line;
}
#endif

std::string Shell::get_prompt()
{
	const char * a1 = ansicolor ? "\033[1m" : "";
	const char * a2 = ansicolor ? "\033[0m" : "";
		
	if (game->is_over()) {
		return strprintf("%s(game over):%s ", a1, a2);
	}

	std::string s = a1;

	if (flag_analyze) {
		s += "(analyze mode) ";
	} else if (flag_ponder && !flag_force && game->is_running()) {
		s += "(pondering) ";
	}
	
	s += strprintf("%s (%d)",
			game->get_board().get_side() == WHITE 
					? "White" : "Black",
			game->get_board().get_moveno());

	if (game->is_running() && !flag_analyze) {
		const Clock * clock = game->get_clock();
		Clock::val_t elapsed = clock->get_elapsed_time();
		if (!clock->is_exact()) {
			Clock::val_t remaining = clock->get_remaining_time();
			s += strprintf(" (%.2f/%.2f sec)",
					Clock::to_s_f(elapsed),
					Clock::to_s_f(remaining));;
		} else {
			s += strprintf(" (%.2f sec)", Clock::to_s_f(elapsed));
		}
	}

	s += ": ";
	s += a2;
	
	if (flag_analyze || (flag_ponder && flag_showthinking)) {
		s += "\n";
	}

	return s;
}

bool Shell::input_move(std::string input)
{
	if (game->is_over()) {
		printf("Illegal move (game over): %s\n", input.c_str());
		return false;
	} else if (game->get_side() == myside) {
		printf("Illegal move (it's my turn): %s\n", input.c_str());
		return false;
	}

	Board board = game->get_board();
	Move mov = board.parse_move(input);
	if (mov) {
		stop_search();
		user_move(mov);
		return true;
	} else {
		printf("Illegal move: %s\n", input.c_str());
		return false;
	}
}

void Shell::user_move(Move mov)
{
	Board board = game->get_board();
	
	if (!mov.is_valid(board)) {
		BUG("user_move() called with invalid move: %s",
				mov.str().c_str());
	} else if (!mov.is_legal(board)) {
		BUG("user_move() called with illegal move: %s",
				mov.str().c_str());
	}
	
	std::string san = mov.san(board);

	game->make_move(mov, 0);

	if (!xboard) {
		printf("\n");
		game->get_board().print(stdout, mov);
		printf("\nYour move was: %s\n\n", san.c_str());
	}
	//game->get_board().print(stdout, mov);

	DBG(2, "user move: %s", san.c_str());

	DBG(2, "result = %d", game->get_result());
	if (game->get_result()) {
		print_result();
	}
}

void Shell::engine_move()
{
	if (!xboard) {
		 printf("Thinking...\n");
	}

	game->start();
	
	Board board = game->get_board();
	Movelist movelist;
	board.generate_moves(&movelist);
	movelist.filter_illegal(board);
	Move mov;
	bool bookmove;

	BookEntry bookentry;
	if (book && book->lookup(board, &bookentry)) {
		mov = bookentry.choose();
		bookmove = true;
		if (!mov.is_valid(board)) {
			BUG("book returned invalid move: %s",
					mov.str().c_str());
		} else if (!mov.is_legal(board)) {
			BUG("book returned illegal move: %s",
					mov.str().c_str());
		}
	} else if (movelist.size() == 1) {
		/* If there is only one move, don't waste any time
		 * searching it. */
		mov = movelist[0];
		bookmove = false;
	} else {
		DBG(1, "starting search...");
		mov = search->start(game, game->get_clock(), Search::MOVE,
				myside, maxdepth);
		DBG(1, "search terminated");
		bookmove = false;
		if (!mov.is_valid(board)) {
			BUG("search returned invalid move: %s",
					mov.str().c_str());
		} else if (!mov.is_legal(board)) {
			BUG("search returned illegal move: %s",
					mov.str().c_str());
		}
	}

	if (verbose >= 1) {
		printf(INFO_PRFX "bookmove=%d\n", bookmove);
	}

	std::string san = mov.san(board);
	
	game->make_move(mov, GameEntry::FLAG_COMPUTER
			| (bookmove ? GameEntry::FLAG_BOOKMOVE : 0));

	if (xboard) {
		std::string str = mov.str();
		atomic_printf("move %s\n", str.c_str());
	} else {
		printf("\n");
		game->get_board().print(stdout, mov);
		printf("\nMy move is: %s\n\n", san.c_str());
	}
	//game->get_board().print(stdout, mov);
	
	DBG(2, "engine move: %s", san.c_str());

	DBG(2, "result = %d", game->get_result());
	if (game->get_result()) {
		print_result();
	}
}

void Shell::engine_analyze()
{
#ifdef WITH_THREAD
	DBG(1, "starting background search...");
	search->start_thread(game, NULL, Search::ANALYZE, NO_COLOR, maxdepth);
#else
	BUG("no thread support compiled in");
#endif
}

void Shell::engine_ponder()
{
#ifdef WITH_THREAD
	DBG(1, "starting background search...");
	search->start_thread(game, NULL, Search::PONDER, myside, maxdepth);
#else
	BUG("no thread support compiled in");
#endif
}

void Shell::stop_search()
{
#ifdef WITH_THREAD
	search->stop_thread();
#endif
}

void Shell::print_result()
{
	atomic_printf("%s {%s}\n",
			game->get_result_str().c_str(),
			game->get_result_comment().c_str());
}

