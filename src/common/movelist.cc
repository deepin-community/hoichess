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
#include "board.h"
#include "move.h"
#include "movelist.h"

Movelist::Movelist()
{
	nextin = 0;
}

Movelist::~Movelist()
{}

void Movelist::filter_illegal(const Board & board)
{
	for (unsigned int i=0; i<size(); i++) {
		if (!move[i].is_legal(board)) {
			swap(i, size()-1);
			nextin--;
			i--;
		}
	}
}

#ifdef HOICHESS
void Movelist::filter_illegal_castling(const Board & board)
{
	for (unsigned int i=0; i<size(); i++) {
		if (move[i].is_castle() && !move[i].is_legal(board)) {
			swap(i, size()-1);
			nextin--;
			i--;
		}
	}
}
#endif

/* This is inefficient, so it should be only used for testing. */
bool Movelist::operator==(const Movelist & ml2) const
{
	const Movelist & ml1 = *this;

	if (ml1.size() != ml2.size()) {
		return false;
	}

	for (unsigned int i=0; i<ml1.size(); i++) {
		bool found = false;
		for (unsigned int j=0; j<ml2.size(); j++) {
			if (ml1[i] == ml2[j]) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}

	return true;
}

bool Movelist::operator!=(const Movelist & ml2) const
{
	return !operator==(ml2);
}
