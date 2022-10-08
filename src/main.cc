/* Copyright (C) 2004-2014 Holger Ruckdeschel <holger@hoicher.de>
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

#include <errno.h>
#ifndef WIN32
# include <signal.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <list>
#include <string>

#include "common.h"
#include "shell.h"
#ifdef VHDLCHESS
# include "vhdlchessshell.h"
#endif
#include "util.h"

#ifdef HAVE_GETOPT
# include <getopt.h>
#else
/* Must include this last or there will be conflicts
 * with getopt() from unistd.h. */
# include "lib/my_getopt_wrapper.h"
#endif

/* Global variables, declared in common.h. */
unsigned int debug = 0;
unsigned int verbose = 0;
bool ansicolor = false;

/* Shell must be global because we need it in the signal handler. */
static Shell * shell;

#ifndef WIN32
static void sigint_handler(int sig)
{
	(void) sig;
	shell->interrupt();
}
#endif

extern void init();
extern const char * builtin_initcmds[];

static void print_version()
{
	printf("%s %s\n", PROGNAME, version);
}

static void print_copyright()
{
	printf("Copyright (C) 2004-2017 %s %s\n", AUTHOR, AUTHOR_EMAIL);
#ifndef VHDLCHESS
	printf(
"This program is free software and comes with ABSOLUTELY NO WARRANTY.\n"
"See the GNU General Public License for more details.\n");
#endif
}

static void usage(const char * argv0)
{
	print_version();
	print_copyright();
	
	printf("\nUsage: %s [options]\n\n", argv0);
	printf("Options:\n");
	printf("  -h | --help           Display usage information\n");
	printf("  -V | --version        Display version information\n");
	printf("  -v | --verbose[=N]    Increase verbosity\n");
	printf("  -d | --debug[=N]      Increase debug level\n");
	printf("  -x | --xboard         Start in xboard mode\n");
	printf("       --source FILE    Read initial commands from FILE\n");

	printf(
"\nThese are only the most general options. See documentation for a complete\n"
"list of supported command line options, and a detailed description of them.\n"
"\nSee http://www.hoicher.de/hoichess for more information and new releases.\n"
"Please report any bugs and suggestions to %s.\n",
			AUTHOR_EMAIL);
}

struct opts {
	const char * xboard;
	std::list<const char *> source_files;
	std::list<const char *> initcmds;
	bool norc;
	const char * color;
};

static int parse_commandline(int argc, char ** argv, struct opts * opts)
{
	/* defaults */
	opts->xboard = "false";
	// opts->source_files is the empty list
	// opts->initcmds is the empty list
	opts->norc = false;
	opts->color = "auto";

	/* option definitions */
	const char * short_opts = "hVvdx::";
	struct option long_options[] = {
		{ "help", 0, 0, 'h' },
		{ "version", 0, 0, 'V' },
		{ "verbose", 2, 0, 'v' },
		{ "debug", 2, 0, 'd' },
		{ "xboard", 2, 0, 'x' },
		{ "source", 1, 0, 132 },
		{ "color", 1, 0, 133 },
		{ "initcmd", 1, 0, 136 },
		{ "norc", 0, 0, 137 },
		
		{ 0, 0, 0, 0 }
	};

	/* parse arguments */
	int c;
	while ((c = getopt_long(argc, argv, short_opts, long_options,
					NULL)) != -1) {
		switch (c) {
		case 'h': /* --help */
			usage(argv[0]);
			exit(0);
		case 'V': /* --version */
			print_version();
			exit(0);
		case 'v': /* --verbose */
			if (optarg) {
				verbose = atoi(optarg);
			} else {
				verbose++;
			}
			break;
		case 'd': /* --debug */
			if (optarg) {
				debug = atoi(optarg);
			} else {
				debug++;
			}
			break;
		case 'x': /* --xboard */
			if (optarg) {
				opts->xboard = optarg;
			} else {
				opts->xboard = "true";
			}
			break;
		case 132: /* --source */
			opts->source_files.push_back(optarg);
			break;
		case 133: /* --color */
			opts->color = optarg;
			break;
		case 136: /* --initcmd */
			opts->initcmds.push_back(optarg);
			break;
		case 137: /* --norc */
			opts->norc = true;
			break;
			
		case '?':
			usage(argv[0]);
			exit(1);
		default:
			BUG("getopt_long() returned %d", c);
		}
	}

	return 0;
}


#define IS_OPTION_TRUE(s) (strcmp((s), "yes" ) == 0 \
			|| strcmp((s), "on"  ) == 0 \
			|| strcmp((s), "true") == 0)
#define IS_OPTION_FALSE(s) (strcmp((s), "no"   ) == 0 \
			 || strcmp((s), "off"  ) == 0 \
			 || strcmp((s), "false") == 0)
#define IS_OPTION_AUTO(s) (strcmp((s), "auto"   ) == 0 \
			|| strcmp((s), "default") == 0)

static void setup(struct opts * opts)
{
	/* decide about ANSI color */
	if (opts->color) {
		if (IS_OPTION_TRUE(opts->color)) {
			ansicolor = true;
		} else if (IS_OPTION_FALSE(opts->color)) {
			ansicolor = false;
		} else if (IS_OPTION_AUTO(opts->color)) {
			/* Most Unix platforms have color terminals. But on
			 * Win32 systems, ANSI color is normally not available.
			 */
#ifdef WIN32
			ansicolor = false;
#else
			if (isatty(1)) {
				ansicolor = true;
			} else {
				ansicolor = false;
			}
#endif
		} else {
			fprintf(stderr, 
				"Illegal argument to option --color: %s\n",
				opts->color);
			exit(EXIT_FAILURE);
		}
	}

	if (ansicolor) {
		verbose && printf("Enabled ANSI color terminal.\n");
	}

}

static void setup_shell(struct opts * opts)
{
	/* decide about xboard mode */
	bool xboard_mode = false;
	if (opts->xboard) {
		if (IS_OPTION_TRUE(opts->xboard)) {
			xboard_mode = true;
		} else if (IS_OPTION_FALSE(opts->xboard)) {
			xboard_mode = false;
		} else {
			fprintf(stderr, 
				"Illegal argument to option --xboard: %s\n",
				opts->xboard);
			exit(EXIT_FAILURE);
		}
	}
	
	/* xboard mode */
	shell->set_xboard(xboard_mode);

	/* if there are no --initcmd options given, use builtin commands */
	if (opts->initcmds.size() == 0) {
		for (const char ** p = builtin_initcmds; *p != NULL; p++) {
			opts->initcmds.push_back(*p);
		}
	}

	/* read --source files given on command line; note that they are
	 * passed in reverse order to shell, because the last opened source
	 * is read as first */
	for (std::list<const char *>::reverse_iterator
			it = opts->source_files.rbegin();
			it != opts->source_files.rend();
			it++) {
		FILE * fp = fopen(*it, "r");
		if (fp == NULL) {
			fprintf(stderr, "Cannot open %s for reading: %s\n",
					*it, strerror(errno));
			exit(EXIT_FAILURE);
		}
		shell->source_file(fp, *it);
	}

	/* read hoichess.rc; note that commands here are executed _before_
	 * commands in --source files, because the last opened source is
	 * read as first */
	if (!opts->norc) {
#if defined(HOICHESS)
		const char * rcfile_base = "hoichess.rc";
#elif defined(HOIXIANGQI)
		const char * rcfile_base = "hoixiangqi.rc";
#endif
		/* we try several locations and open only the first found */
		std::list<std::string> rcfiles;

		/* current directory */
		rcfiles.push_back(std::string("./") + rcfile_base);

		/* $HOME/.hoichess */
		const char * home = getenv("HOME");
		if (home != NULL) {
			rcfiles.push_back(home + std::string("/.hoichess/")
					+ rcfile_base);
		}

#ifdef WIN32
		/* $USERPROFILE/.hoichess */
		const char * profile = getenv("USERPROFILE");
		if (profile != NULL) {
			rcfiles.push_back(profile + std::string("/.hoichess/")
					+ rcfile_base);
		}
#endif

#ifdef DATA_DIR
		/* installation dependent default location */
		rcfiles.push_back(DATA_DIR + std::string("/") + rcfile_base);
#endif

		/* read the first file that was found */
		for (std::list<std::string>::iterator it = rcfiles.begin();
				it != rcfiles.end(); it++) {
			FILE * fp = fopen(it->c_str(), "r");
			if (fp != NULL) {
				shell->source_file(fp, it->c_str());
				break;
			}
		}
	}
}

static int run_shell(struct opts * opts)
{
	/* execute initial commands */
	for (std::list<const char *>::iterator
			it = opts->initcmds.begin();
			it != opts->initcmds.end();
			it++) {
		int ret = shell->exec_command(*it);
		if (ret != SHELL_CMD_OK) {
			return  EXIT_FAILURE;
		}
	}

	/* run the shell (starting with commands from sourced files) */
	return shell->main();
}

int main(int argc, char ** argv)
{
	struct opts opts;
	parse_commandline(argc, argv, &opts);

	if (IS_OPTION_TRUE(opts.xboard)) {
		/* xboard has a timeout of 2 seconds after 'protover', and 
		 * falls back to protocol version 1 if it does not receive the
		 * 'feature' reply within that time.
		 * To prevent this in case our initialization takes too long,
		 * we send a 'feature' line now if we're running under xboard,
		 * as suggested by the xboard protocol spec. */
		printf("feature done=0\n");
	}
	
	print_version();
	print_copyright();
	printf("\n");

	if (verbose || debug) {
		printf("Verbosity set to %u.\n", verbose);
		printf("Debug level set to %u.\n", debug);
		printf("\n");
	}

	setup(&opts);
	init();

#ifdef VHDLCHESS
	shell = new VHDLChessShell();
#else
	shell = new Shell();
#endif

	setup_shell(&opts);

#ifndef WIN32
	signal(SIGINT, sigint_handler);	
#endif

	int ret = run_shell(&opts);

#ifndef WIN32
	signal(SIGINT, SIG_IGN);	
#endif

	delete shell;
	return ret;
}
