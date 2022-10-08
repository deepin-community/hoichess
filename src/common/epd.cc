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
#include "epd.h"
#include "game.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>


/*
 * TODO We need some error handling if parsing failes.
 */
EPD::EPD(const std::string & _s)
{
	char * s = new char[_s.size()+1];
	strcpy(s, _s.c_str());
	if (s[strlen(s)-1] == '\n')
		s[strlen(s)-1] = '\0';
	const char * p = s;
	parse(p);	
	delete s;
}

/*
 * Return an FEN representation for this EPD. If halfmove or fullmove
 * numbers are not specified, we just use 0 resp. 1.
 */
std::string EPD::get_fen() const
{
	std::string fmvn, hmvn;
	if ((hmvn = get1("hmvn")) == "")
		hmvn = "0";
	if ((fmvn = get1("fmvn")) == "")
		fmvn = "1";
	
	std::stringstream ss;
	ss << fen_position << " " << fen_color << " " << fen_castling << " "
		<< fen_ep << " " << hmvn << " " << fmvn;
	return ss.str();
}

/*
 * Get a list of all operands for an the opcode.
 */
std::list<std::string> EPD::get(const std::string & opcode) const
{
	std::list<std::string> ret;
	
	/* ops[opcode] does not work here, because operator[] is not const. */
	std::map<std::string, std::list<std::string> >::const_iterator it
		= ops.find(opcode);
	
	if (it != ops.end())
		ret = it->second;
	
	return ret;
}

/*
 * Get only one operand. To make things simpler, use this method for
 * opcodes that normally have only one operand, like id or hmvn.
 */
std::string EPD::get1(const std::string & opcode) const
{
	std::list<std::string> tmp = get(opcode);

	if (tmp.size() == 0) {
#ifdef DEBUG
		WARN("get1(): no operand for '%s', returning empty string",
				opcode.c_str());
#endif
		return std::string("");
	} else if (tmp.size() > 1) {		
		WARN("get1(): returning only 1st operand out of %d for '%s'",
				tmp.size(), opcode.c_str());
	}

	return tmp.front();
}

/*
 * Get a list of all operands for an the 'bm' opcode converted to Moves.
 */
std::list<Move> EPD::get_bm() const
{
	std::list<std::string> bms_str = get("bm");
	Board board(get_fen().c_str());
	std::list<Move> bms;

	for (std::list<std::string>::iterator it = bms_str.begin();
			it != bms_str.end(); it++) {
		Move m = board.parse_move(*it);
		if (!m) {
			WARN("get_bm(): illegal move: %s", it->c_str());
			continue;
		}
		bms.push_back(m);
	}

	return bms;
}


/*****************************************************************************
 *
 * EPD parsing functions.
 *
 *****************************************************************************/

void EPD::parse(const char * p)
{
	/* First the FEN */
	enum foo { S_POSITION, S_COLOR, S_CASTLING, S_EP, S_DONE };
	int state = S_POSITION;
	while (*p) {
		if (*p == ' ') {
			state++;
			p++;
			if (state == S_DONE)
				break;
			continue;
		}
		
		switch (state) {
		case S_POSITION:
			fen_position += *p;
			break;
		case S_COLOR:
			fen_color += *p;
			break;
		case S_CASTLING:
			fen_castling += *p;
			break;
		case S_EP:
			fen_ep += *p;
			break;
		default:
			BUG("illegal state");
		}
		
		p++;
	}

	/* Now the opcodes with their operands */
	while (*p) {
		switch (*p) {
		case ' ':
			break;
		default:
			p = parse_opcode(p);
		}
		p++;
	}
}

const char * EPD::parse_opcode(const char * p)
{
	std::string opcode;
	std::list<std::string> operands;
	
	while (*p) {
		switch (*p) {
		case ';':
#ifdef DEBUG
			printf("opcode = '%s', no operands\n",
					opcode.c_str());
#endif
			ops[opcode] = operands;
			return p;
		case ' ':
#ifdef DEBUG
			printf("opcode = '%s', operands follow\n",
					opcode.c_str());
#endif
			ops[opcode] = operands;
			p++;
			while (*p) {
				p = parse_operand(p, opcode);
				if (*p != ' ')
					break;
				p++;
			}
#ifdef DEBUG
			printf("opcode '%s' finished, had %d operands\n",
					opcode.c_str(), ops[opcode].size());
#endif
			return p;			
		default:
			opcode += *p;
		}
		p++;
	}

	return p;
}

const char * EPD::parse_operand(const char * p, const std::string & opcode)
{
	std::string operand;

	while (*p) {
		switch (*p) {
		case '"':
			/* A quoted string. Put everything up to
			 * the next `"' into operand. */
			p++;
			while (*p) {
				if (*p == '"') {
					break;
				} else {
					operand += *p++;
				}
			}
			break;			
		case ';':
#ifdef DEBUG
			printf("\toperand = '%s' (last)\n", operand.c_str());
#endif
			ops[opcode].push_back(operand);
			return p;
		case ' ':
#ifdef DEBUG
			printf("\toperand = '%s'\n", operand.c_str());
#endif
			ops[opcode].push_back(operand);
			return p;
		default:
			operand += *p;
		}
		p++;
	}

	return p;
}

