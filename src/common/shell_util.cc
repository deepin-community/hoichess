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
#include "shell.h"
#include "search.h"

#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>


/*****************************************************************************
 *
 * Thinking output.
 *
 *****************************************************************************/

void Shell::print_search_header()
{
	if (!flag_showthinking) {
		return;
	}

	if (!xboard) {
		printf("Depth   Ply   Time/Alloc   Score      Nodes  Principal-Variation\n");
	}
}

/*
 * Print thinking output.
 */
void Shell::print_search_info(struct Search::searchinfo * si)
{
	if (!flag_showthinking) {
		return;
	}

	unsigned long csecs = si->csecs;

	if (xboard) {
		if (verbose >= 3 || (flag_analyze && csecs > 100)
				|| csecs > 500) {
			print_search_info_xboard(si);
		}
	} else {
		if (verbose >= 3 || (isatty(1) && csecs > 1000)) {
			print_search_info_terminal(si);
		}
	}
}

/*
 * Print search result.
 */
void Shell::print_search_result(struct Search::searchresult * sr)
{
	if (!flag_showthinking) {
		return;
	}

	unsigned long csecs = sr->csecs;
	enum Search::searchresult::resulttype type = sr->type;

	if (xboard) {
		if (type != Search::searchresult::INTERMEDIATE) {
			print_search_result_xboard(sr);
		}
	} else {
		if (verbose >= 2 || csecs > 500 || type != Search::searchresult::INTERMEDIATE) {
			print_search_result_terminal(sr);
		}
	}
}


/*
 * Print thinking output to terminal:
 *
 *  9?    4.15/10.00            1109710  1. e4 (1/20) [267k nps]
 */
void Shell::print_search_info_terminal(struct Search::searchinfo * si)
{	
	/* extract information from structure passed by search */
	unsigned int depth = si->depth;
	unsigned long csecs = si->csecs;
	unsigned long csecs_alloc = si->csecs_alloc;
	unsigned long long nodes_total = si->nodes_total;
	unsigned int i = si->i;
	unsigned int n = si->n;
	Move mov = si->mov;
	const Board & board = si->board;
	unsigned int maxplyreached_fullwidth = si->maxplyreached_fullwidth;
	unsigned int maxplyreached_quiesce = si->maxplyreached_quiesce;


	std::string s;
	char buf[128];
	
	/* depth */
	snprintf(buf, sizeof(buf), "%2u?   %2u/%2u", depth,
			maxplyreached_fullwidth+1, maxplyreached_quiesce+1);
	s += buf;
	
	/* elapsed search time */
	if (csecs >= 6000) {
		unsigned int mins = csecs / 6000;
		unsigned int secs = csecs % 6000 / 100;
		snprintf(buf, sizeof(buf), "  %2u:%02u", mins, secs);
	} else {
		snprintf(buf, sizeof(buf), "  %5.2f", (float) csecs / 100);
	}
	s += buf;

	/* allocated search time */
	if (csecs_alloc >= 6000) {
		unsigned int mins = csecs_alloc / 6000;
		unsigned int secs = csecs_alloc % 6000 / 100;
		snprintf(buf, sizeof(buf), "/%2u:%02u", mins, secs);
	} else {
		snprintf(buf, sizeof(buf), "/%5.2f", (float) csecs_alloc / 100);
	}
	s += buf;

	/* nodes */
	if (nodes_total > (int)1E9) {
		snprintf(buf, sizeof(buf), "          %8lluM  ",
				nodes_total/(int)1E6);
	} else if (nodes_total > (int)10E6) {
		snprintf(buf, sizeof(buf), "          %8lluk  ",
				nodes_total/(int)1E3);
	} else {
		snprintf(buf, sizeof(buf), "          %9llu  ", 
				nodes_total);
	}
	s += buf;

	/* current move */
	if (mov) {
		snprintf(buf, sizeof(buf), "%d. %s%s", board.get_moveno(),
				board.get_side() == WHITE ? "" : "... ",
				mov.san(board).c_str());
		s += buf;
	}

	/* number of current move / total number of moves */
	if (n != 0) {
		snprintf(buf, sizeof(buf), " (%u/%u)", i+1, n);
		s += buf;
	}

	/* nodes/s */
	unsigned int nps = (csecs > 0) ? (nodes_total / csecs * 100) : 0;
	snprintf(buf, sizeof(buf), " [%uk nps]", nps/1000);
	s += buf;

	/* fill with spaces up to length of last partial line */
	unsigned int len = s.length();
	if (len < last_status_line_length) {
		s += std::string(last_status_line_length - len, ' ');
	}

	if (print_search_output_terminal_newline) {
		atomic_printf("%s\n", s.c_str());
		last_status_line_length = 0; /* because we end this line with \n */
	} else {
		atomic_printf("\r%s", s.c_str());
		last_status_line_length = len; /* because we print a partial line that will be overwritten */
	}
	fflush(stdout);
}

/* Whether we end every status line with \n or 
 * have them overwritten by the next line. */ 
bool Shell::print_search_output_terminal_newline = false;

/*
 * In xboard mode, when analyzing, we print out the stat01 line:
 *
 * stat01: time nodes ply mvleft mvtot mvname
 * 
 * Note: The xboard protocol defines that the stat01 line is printed
 * as a response to the '.' command. However, we print the line freely,
 * but that obviously works well with xboard/winboard.
 */
void Shell::print_search_info_xboard(struct Search::searchinfo * si)
{
	/* extract information from structure passed by search */
	unsigned int depth = si->depth;
	unsigned long csecs = si->csecs;
	unsigned long long nodes_total = si->nodes_total;
	unsigned int i = si->i;
	unsigned int n = si->n;
	Move mov = si->mov;
	const Board & board = si->board;
	//unsigned int maxplyreached_fullwidth = si->maxplyreached_fullwidth;
	//unsigned int maxplyreached_quiesce = si->maxplyreached_quiesce;


	atomic_printf("stat01: %lu %llu %u %u %u %s\n",
			csecs,
			nodes_total,
			depth,
			n - i - 1,
			n,
			mov ? (mov.san(board).c_str()) : "(no move)");
}

/*
 * Print search result to terminal.
 * 
 *  7.    1.16/10.00    0.25     261225  1. e4 e5 2. Nf3 Nf6 3. Nxe5 Bd6 4. d4
 */
void Shell::print_search_result_terminal(struct Search::searchresult * sr)
{
	/* extract information from structure passed by search */
	enum Search::searchresult::resulttype type = sr->type;
	unsigned int depth = sr->depth;
	int score = sr->score;
	unsigned long csecs = sr->csecs;
	unsigned long csecs_alloc = sr->csecs_alloc;
	unsigned long long nodes_total = sr->nodes_total;
	const std::string& best_line = sr->best_line;
	unsigned int maxplyreached_fullwidth = sr->maxplyreached_fullwidth;
	unsigned int maxplyreached_quiesce = sr->maxplyreached_quiesce;
	

	std::string s;
	char buf[128];
	
	/* depth and result type */
	char c;
	switch (type) {
		case Search::searchresult::INTERMEDIATE:	c = ' '; break;
		case Search::searchresult::DEPTH:		c = '.'; break;
		case Search::searchresult::FAILLOW:		c = '-'; break;
		case Search::searchresult::FAILHIGH:		c = '+'; break;
		case Search::searchresult::FINAL:		c = ':'; break;
		default:
			BUG("illegal type: ", type);
	}
	snprintf(buf, sizeof(buf), "%2u%c   %2u/%2u", depth, c,
			maxplyreached_fullwidth+1, maxplyreached_quiesce+1);
	s += buf;
	
	/* elapsed search time */
	if (csecs >= 6000) {
		unsigned int mins = csecs / 6000;
		unsigned int secs = csecs % 6000 / 100;
		snprintf(buf, sizeof(buf), "  %2u:%02u", mins, secs);
	} else {
		snprintf(buf, sizeof(buf), "  %5.2f", (float) csecs / 100);
	}
	s += buf;

	/* allocated search time */
	if (csecs_alloc >= 6000) {
		unsigned int mins = csecs_alloc / 6000;
		unsigned int secs = csecs_alloc % 6000 / 100;
		snprintf(buf, sizeof(buf), "/%2u:%02u", mins, secs);
	} else {
		snprintf(buf, sizeof(buf), "/%5.2f", (float) csecs_alloc / 100);
	}
	s += buf;


	/* score */
	if (score >= MATE) {
		snprintf(buf, sizeof(buf), "   Mat%2d",
				INFTY-score);
	} else if (score <= -MATE) {
		snprintf(buf, sizeof(buf), "  -Mat%2d",
				score+INFTY);
	} else {
		snprintf(buf, sizeof(buf), " %7.2f",
				(float) score / 100);

	}
	s += buf;

	/* nodes */
	if (nodes_total > (int)1E9) {
		snprintf(buf, sizeof(buf), "  %8lluM  ",
				nodes_total/(int)1E6);
	} else if (nodes_total > (int)10E6) {
		snprintf(buf, sizeof(buf), "  %8lluk  ",
				nodes_total/(int)1E3);
	} else {
		snprintf(buf, sizeof(buf), "  %9llu  ",
				nodes_total);
	}
	s += buf;

	/* pv line */
	s += best_line;
	
	/* fill with spaces up to length of last partial line */
	unsigned int len = s.length();
	if (len < last_status_line_length) {
		s += std::string(last_status_line_length - len, ' ');
	}

	last_status_line_length = 0; /* because we end this line with \n */

	if (print_search_output_terminal_newline) {
		atomic_printf("%s\n", s.c_str());
	} else {
		atomic_printf("\r%s\n", s.c_str());
	}
	fflush(stdout);
}

/*
 * Print search result in xboard mode:
 * 
 * ply score time nodes pv
 */
void Shell::print_search_result_xboard(struct Search::searchresult * sr)
{
	/* extract information from structure passed by search */
	//enum Search::searchresult::resulttype type = sr->type;
	unsigned int depth = sr->depth;
	int score = sr->score;
	unsigned long csecs = sr->csecs;
	unsigned long long nodes_total = sr->nodes_total;
	const std::string& best_line = sr->best_line;
	//unsigned int maxplyreached_fullwidth = sr->maxplyreached_fullwidth;
	//unsigned int maxplyreached_quiesce = sr->maxplyreached_quiesce;

	const char * note = "";
	switch (sr->type) {
		case Search::searchresult::FAILLOW:
			note = "(-) ";
			break;
		case Search::searchresult::FAILHIGH:
			note = "(+) ";
			break;
		case Search::searchresult::INTERMEDIATE:
			note = "(.) ";
			break;
		default:
			note = "";
			break;
	}

	atomic_printf("%u %d %lu %llu %s%s\n",
			depth,
			score,
			csecs,
			nodes_total,
			note,
			best_line.c_str());
}

