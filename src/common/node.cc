/* Copyright (C) 2005-2012 Holger Ruckdeschel <holger@hoicher.de>
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
#include "node.h"

#include <sstream>


#ifdef USE_UNMAKE_MOVE
#error "USE_UNMAKE_MOVE currently not supported"
#endif

/*****************************************************************************
 *
 * Functions of class Node
 *
 *****************************************************************************/

Node::Node()
{
	parent = NULL;
	root = NULL;

	historytable = NULL;
	pvline.nmoves = 0;
	best_line.nmoves = 0;
}
	
Node::Node(const Board& _board)
{
	parent = NULL;
	root = NULL;

	board = _board;

	historytable = NULL;
	pvline.nmoves = 0;
	best_line.nmoves = 0;
}

void Node::init_root()
{
	root = this;

	incheck = board.in_check();
	material = board.material_difference();

	movelist.clear();
	generate_all_moves();
	ASSERT(movelist.size() > 0);
	
	set_type(Node::ROOT);
	pvline.nmoves = 0;
	set_hashmv(NO_MOVE);
	killer1 = NO_MOVE;
	killer2 = NO_MOVE;

	/* Assign one legal move as best, in case search terminates without
	 * choosing a move. */
	best_line.moves[0] = movelist[0];
	best_line.nmoves = 1;

	played_move = NO_MOVE;
}


Node * Node::make_move(Move mov, NodeAllocator * allocator) const
{
	Node * child = allocator->alloc(); /* sets this->_allocator */
	if (child == NULL) {
		BUG("Node alloc failed. MAXPLY reached?");
	}

	child->parent = this;
	child->root = this->root;

	child->board = this->board;
	child->board.make_move(mov);

	child->incheck = child->board.in_check();
	child->material = child->board.material_difference();

	child->movelist.clear();
	
	child->set_type(Node::UNKNOWN);

	/* if the PV move has been made, copy the rest of the pvline
	 * to the child */
	if (pvline.nmoves > 1 && pvline.moves[0] == mov) {
		memcpy(child->pvline.moves, pvline.moves+1,
				(pvline.nmoves-1)*sizeof(Move));
		child->pvline.nmoves = pvline.nmoves - 1;
	} else {
		child->pvline.nmoves = 0;
	}

	child->set_hashmv(NO_MOVE);
	child->killer1 = NO_MOVE;
	child->killer2 = NO_MOVE;

	child->best_line.nmoves = 0;

	child->played_move = mov;

	return child;
}

Node * Node::copy(NodeAllocator * allocator) const
{
	ASSERT(allocator != this->_allocator);

	Node * node1 = allocator->alloc(); /* sets this->_allocator */
	if (node1 == NULL) {
		BUG("Node alloc failed. MAXPLY reached?");
	}

	node1->parent = this->parent;
	node1->root = this->root;

	node1->board = this->board;
	node1->incheck = this->incheck;
	node1->material = this->material;
	node1->movelist = this->movelist;
	node1->current_move_no = this->current_move_no;
	node1->type = this->type;
	node1->pvline = this->pvline;
	node1->hashmv = this->hashmv;
	node1->killer1 = this->killer1;
	node1->killer2 = this->killer2;
	node1->historytable = NULL; /* TODO copy */
	node1->best_line = this->best_line;
	node1->played_move = this->played_move;

	return node1;
}

Move Node::first()
{
	current_move_no = -1;

	switch (type) {
	case ROOT:
		/* For the root node, all moves have been generated in
		 * Node::init_root(). Scores were assigned by search_root()
		 * using set_current_score(). So we can just start returning
		 * moves. */
		ASSERT(movelist.size() != 0);
		break;
	case FULLWIDTH:
		if (movelist.size() != 0) {
			/* Moves have already been generated
			 * due to internal iterative deepening. */
		} else if (in_check()) {
			board.generate_escapes(&movelist);
		} else {
			board.generate_moves(&movelist, false);
		}
		break;
	case QUIESCE:
		/* For some strange reason, movelist is not always 
		 * empty here...? */
		movelist.clear();

		if (in_check()) {
			board.generate_escapes(&movelist);
		} else {
			board.generate_captures(&movelist, false);
		}
		break;
	default:
		BUG("node type is bad: %d", type);
	}

	score_moves();

	return pick();
}

Move Node::next()
{
	return pick();
}

Move Node::pick()
{
	int score = -INFTY;
	int m = -1;
	for (unsigned int i=current_move_no+1; i<movelist.size(); i++) {
		if (movelist.get_score(i) > score) {
			score = movelist.get_score(i);
			m = i;
		}
	}

	if (m == -1) {
		return NO_MOVE;
	}

	Move mov = movelist[m];
	current_move_no++;
	if (current_move_no != m) {
		movelist.swap(current_move_no, m);
	}
	return mov;
}

/*
 * Assign scores to moves. For root node, the score are set
 * by Search::search_root() using set_current_score().
 */
void Node::score_moves()
{
	if (type == ROOT)
		return;

	for (unsigned int i=current_move_no+1; i<movelist.size(); i++) {
		int score = 0;
		Move mov = movelist[i];
		if (pvline.nmoves > 0 && mov == pvline.moves[0]) {
			/* PV move */
			score = 1000000;
		} else if (mov == hashmv) {
			/* hash move */
			score = 900000;
		} else if (mov.is_capture()
#ifdef HOICHESS
				|| mov.is_promotion()
				|| mov.is_enpassant()
#endif
				) {
			/* captures */
			unsigned int mat_atk = mat_values[mov.ptype()];
			unsigned int mat_vic = mat_values[mov.cap_ptype()];
#ifdef HOICHESS
			if (mov.is_promotion()) {
				mat_vic += mat_values[mov.promote_to()];
			}
#endif

			if (mat_vic > mat_atk) {
				/* winning capture */
				score = 800000 + mat_vic - mat_atk;
			} else if (mat_vic < mat_atk) {
				/* losing capture */
				score =  10000 + mat_vic - mat_atk;
			} else {
				/* equal capture */
				score = 500000;
			}
		} else {
			/* non-captures */
			if (mov == killer1 || mov == killer2) {
				score = 700000;
			}
			if (historytable) {
				score += MIN(historytable->get(mov), 100000);
			}
		}
		movelist.set_score(i, score);
	}
}

void Node::generate_all_moves()
{
	/* TODO This could be optimized by looking which moves
	 * have already been generated, e.g. due to IID. */
	movelist.clear();
	board.generate_moves(&movelist, false);
	movelist.filter_illegal(board);
}

std::string Node::get_best_line_str() const
{
	return pvline2str(best_line, board, false);
}

void Node::set_best(Move mov, const Node* child)
{
	best_line.moves[0] = mov;
	unsigned int n = MIN(child->best_line.nmoves, NODE_PVLINE_MAXMOVES-1);
	memcpy(best_line.moves+1, child->best_line.moves, n*sizeof(Move));
	best_line.nmoves = n + 1;
}

void Node::set_best_line(const struct pvline& best_line)
{
	this->best_line = best_line;
}

const struct Node::pvline& Node::get_pvline() const
{
	return this->pvline;
}

std::string Node::get_pvline_str() const
{
	return pvline2str(pvline, board, false);
}

void Node::set_pvline(const struct pvline & pvline)
{
	this->pvline = pvline;
}

std::string Node::pvline2str(const struct Node::pvline& pvline,
		const Board& board, bool pretty)
{
	std::ostringstream ss;
	
	Board tmpboard = board;

	for (unsigned int ply=0; ply<pvline.nmoves; ply++) {
		const Move& mov = pvline.moves[ply];

		if (ply > 0) {
			ss << " ";
		}

		if (pretty) {
			if (tmpboard.get_side() == WHITE) {
				ss << tmpboard.get_moveno() << ". ";
			} else if (ply == 0) { /* && BLACK */
				ss << tmpboard.get_moveno() << ". ... ";
			}
		}

		ss << mov.san(tmpboard);
		
		/* Limit output length to avoid xboard buffer overflow. */
		if (ply >= 30) {
			ss << " [...]";
			break;
		}

		tmpboard.make_move(mov);
	}

	return ss.str();
}


/*****************************************************************************
 *
 * Functions of class NodeAllocator
 *
 *****************************************************************************/

NodeAllocator::NodeAllocator(unsigned long count)
{
	pool = new Node[count];
	next = pool;
	end = pool + count;
}

NodeAllocator::~NodeAllocator()
{
	delete[] pool;
}

