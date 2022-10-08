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
#ifndef SHELL_H
#define SHELL_H

#include "common.h"
#include "board.h"
#include "book.h"
#include "clock.h"
#include "game.h"
#include "hash.h"
#include "search.h"
#include "basic.h"

#include <string>
#include <sstream>
#include <vector>

#define SHELL_STRUCT_OPTION(name, defval) \
	private: int option_##name; \
	public: inline int get_option_##name() const { return option_##name; }

#define SHELL_REGISTER_OPTION(name, defval) \
	register_option(#name, &option_##name, defval)


class Shell
{
      private:
	typedef struct {
		std::string name;
		unsigned int line;
		FILE * fp;
	} source_t;

      protected:
	struct command {
		const char * name;
		int (Shell::* func) (void);
		const char * usage;
	};

#define SHELL_DEFINE_OPTION SHELL_STRUCT_OPTION
# include "shell_option_defs.h"
#undef SHELL_DEFINE_OPTION

      private:
	struct option {
		const char * name;
		int * varptr;
	};
	
      protected:
	bool xboard;

      private:
	bool flag_force;
	bool flag_ponder;
	bool flag_showthinking;
	bool flag_analyze;
	bool flag_playboth;
	
	source_t source;
	std::list<source_t> sources;
	std::list<std::vector<std::string> > atexit_commands;
	int echo;

	std::string myname;
	
	unsigned long hashsize;
	unsigned long pawnhashsize;
	unsigned long evalcachesize;

      protected:
	Game * game;
      private:
	Book * book;

#ifdef WITH_THREAD
	unsigned int parallel;
#endif

	/* print_search_info_terminal() prints partial lines that will be 
	 * overwritten by the next output of print_search_info_terminal() 
	 * or print_search_result_terminal(). We must remember the length
	 * of the last output to be able to overwrite excess characters 
	 * with spaces. */
	unsigned int last_status_line_length;

      protected:
	Search * search;

      private:
	Color myside;
	unsigned int maxdepth;

	bool quit;
	
	/* Some commands (e.g. solve) call search from within a loop.
	 * interrupt() sets this flag to abort those commands. */
	bool stop;

	std::list<struct command> commands;
	std::list<struct option> option_list;

      protected:
	/* FIXME remove, add as argument to all cmd_ functions */
	std::vector<std::string> cmd_args;

      public:
	Shell();
	virtual ~Shell();

      public:
	int main();
	void interrupt();

      public:
	void set_book(const char * bookfile);
	void set_hash_size(size_t bytes);
	size_t get_hash_size() const;
	void set_pawnhash_size(size_t bytes);
	size_t get_pawnhash_size() const;
	void set_evalcache_size(size_t bytes);
	size_t get_evalcache_size() const;
	void set_myname(const char * name);
	const char * get_myname() const;
	void set_xboard(bool x);

      public:
	void source_file(FILE * fp, const char * name);
	int exec_command(const char * s);
	int exec_command(const std::vector<std::string>& args);

      protected:
	void register_commands(const struct command* cmds);
	void register_option(const char * name, int * varptr, int defval);

      private:
	int input();
	char * get_line(const char * prompt);
	char * get_line_fgets(FILE * fp, const char * prompt);	
#ifdef HAVE_READLINE
	char * get_line_readline(const char * prompt);
#endif
	std::string get_prompt();
	bool input_move(std::string input);
	void user_move(Move mov);
	void engine_move();
	void engine_analyze();
	void engine_ponder();

      protected:
	void stop_search();

      private:
	void print_result();

      public:
	void print_search_header();
	void print_search_info(struct Search::searchinfo * si);
	void print_search_result(struct Search::searchresult * sr);
      private:
	void print_search_info_terminal(struct Search::searchinfo * si);
	void print_search_info_xboard(struct Search::searchinfo * si);
	void print_search_result_terminal(struct Search::searchresult * sr);
	void print_search_result_xboard(struct Search::searchresult * sr);
      public:
	static bool print_search_output_terminal_newline;

      private:
	static const struct command shell_commands[];

      protected:
	int cmd_null();
	int cmd_xboard();
	int cmd_protover();
	int cmd_accepted();
	int cmd_rejected();
	int cmd_new();
	int cmd_variant();
	int cmd_quit();
	int cmd_force();
	int cmd_go();
	int cmd_level();
	int cmd_st();
	int cmd_sd();
	int cmd_time();
	int cmd_otim();
	int cmd_usermove();
	int cmd_ping();
	int cmd_setboard();
	int cmd_bk();
	int cmd_undo();
	int cmd_remove();
	int cmd_hard();
	int cmd_easy();
	int cmd_post();
	int cmd_nopost();
#ifdef WITH_THREAD
	int cmd_analyze();
	int cmd_exit();
	virtual int cmd_cores();
#endif
	int cmd_option();
	
	int cmd_verbose();
	int cmd_debug();
	int cmd_help();
	int cmd_source();
	int cmd_echo();
	int cmd_noxboard();
	virtual int cmd_show();
	int cmd_solve();
	int cmd_book();
	int cmd_hash();
	int cmd_pawnhash();
	int cmd_evalcache();
	int cmd_set();
	int cmd_get();
	int cmd_playboth();
	int cmd_loadgame();
	int cmd_savegame();
	int cmd_redo();
	int cmd_options();
	int cmd_atexit();
};

#define SHELL_CMD_REQUIRE_ARGS(n) do {					\
	if (cmd_args.size() < (n)+1) {					\
		printf("Error (command requires %d argument%s): %s\n",	\
				(n), ((n) == 1 ? "" : "s"),		\
				cmd_args[0].c_str());			\
		return SHELL_CMD_FAIL;					\
	}								\
} while(0)

#define SHELL_CMD_DEPRECATED() do {			\
	printf("Warning: command %s is deprecated\n",	\
			cmd_args[0].c_str());		\
} while (0)

#define SHELL_CMD_OK 1
#define SHELL_CMD_FAIL 0


#endif // SHELL_H
