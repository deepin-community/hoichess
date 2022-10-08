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
#ifndef PGN_H
#define PGN_H

#include "common.h"
#include "board.h"
#include "move.h"

#include <map>
#include <list>
#include <string>


class PGN {
      private:
	Board opening;
	std::map<std::string, std::string> tags;
	std::list<Move> moves;

      public:
	PGN();
	~PGN() {};

      public:
	Board get_opening() const
	{ return opening; }

	std::list<Move> get_moves() const
	{ return moves; }

	bool parse(FILE * fp);

	static char * get_movetext_token(FILE * fp, char * buf, size_t bufsize);
	static std::list<PGN> parse_all(FILE * fp);
	static std::list<PGN> parse_all(const char * filename);
};

#endif // PGN_H
