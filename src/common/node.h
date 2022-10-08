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
#ifndef NODE_H
#define NODE_H

#include "common.h"
#include "board.h"
#include "historytable.h"


class Node;

class NodeAllocator {
      private:
	Node* pool;
	Node* next;
	Node* end; /* points 1 beyond last Node in pool */

      public:
	NodeAllocator(unsigned long count);
	~NodeAllocator();

      public:
	inline Node* alloc();
	static void free(Node* node);
};

#define NODE_PVLINE_MAXMOVES MAXPLY

class Node {
	friend class NodeAllocator;

      public:
	enum node_type { UNKNOWN, ROOT, FULLWIDTH, QUIESCE };

	/* To collect the PV as described at
	 * http://www.brucemo.com/compchess/programming/pv.htm
	 */
	struct pvline {
		unsigned int nmoves;
		Move moves[NODE_PVLINE_MAXMOVES];
	};

      private:
	NodeAllocator * _allocator; /* managed by class NodeAllocator */

	/* tree structure */
	const Node * parent;
	const Node * root;

	/* the board */
#ifdef USE_UNMAKE_MOVE
	BoardHistory hist;
#else
	Board board;
#endif

	/* caches to avoid recomputation when needed multiple times */
	bool incheck;
	int material;

	/* movelist and move generator state */
	Movelist movelist;
	int current_move_no;

	/* node parameters, move ordering, ... */
	enum node_type type;
	struct pvline pvline;
	Move hashmv;
	Move killer1;
	Move killer2;
	HistoryTable * historytable;

	/* search result */
	struct pvline best_line;

	/* the move that led to this node, i.e. was played at parent node */
	Move played_move;

      public:
	Node();
	Node(const Board& board);
	
      public:
	void init_root();
	Node* make_move(Move mov, NodeAllocator * allocator) const;
	Node* copy(NodeAllocator * allocator) const;
	inline void free();

	Move first();
	Move next();
	Move pick();

	void score_moves();
	void generate_all_moves();

	inline const Node* get_parent() const;
	inline const Node* get_root() const;
	bool is_root() const;

	inline const Board& get_board() const;
	inline bool in_check() const;
	inline int material_balance() const;
		
	inline unsigned int get_movelist_size() const;
	inline unsigned int get_current_move_no() const;
	inline Move get_current_move() const;

	inline void set_current_score(int score);
	
	inline enum node_type get_type() const;
	inline void set_type(enum node_type t);
	
	inline Move get_best_move() const;
	inline const struct pvline& get_best_line() const;
	std::string get_best_line_str() const;
	void set_best(Move mov, const Node* child);
	void set_best_line(const struct pvline& best_line);
	
	inline Move get_hashmv() const;
	inline void set_hashmv(Move mov);

	inline Move get_pvmove() const;
	const struct pvline& get_pvline() const;
	std::string get_pvline_str() const;
	void set_pvline(const struct pvline & pvline);

	inline void set_historytable(HistoryTable * ht);
	inline void add_killer(Move mov);
	inline Move get_played_move() const;

      public:
	static std::string pvline2str(const struct Node::pvline& pvline,
			const Board& board, bool pretty);
};

/*****************************************************************************
 *
 * Inline functions of class Node
 *
 *****************************************************************************/

inline void Node::free()
{
	NodeAllocator::free(this);
}

inline const Node* Node::get_parent() const
{
	return parent;
}

inline const Node* Node::get_root() const
{
	return root;
}

inline bool Node::is_root() const
{
	return (this == root);
}

inline const Board& Node::get_board() const
{
	return board;
}

inline bool Node::in_check() const
{
	return incheck;
}

inline int Node::material_balance() const
{
	return material;
}

inline unsigned int Node::get_movelist_size() const
{
	return movelist.size();
}

inline unsigned int Node::get_current_move_no() const
{
	return current_move_no;
}

inline Move Node::get_current_move() const
{
	return movelist[current_move_no];
}

inline void Node::set_current_score(int score)
{
	movelist.set_score(current_move_no, score);
}

inline enum Node::node_type Node::get_type() const
{
	return type;
}

inline void Node::set_type(enum Node::node_type t)
{
	type = t;
}

inline Move Node::get_best_move() const
{
	if (best_line.nmoves == 0) {
		return NO_MOVE;
	} else {
		return best_line.moves[0];
	}
}

inline const struct Node::pvline& Node::get_best_line() const
{
	return best_line;
}

inline Move Node::get_hashmv() const
{
	return hashmv;
}

inline void Node::set_hashmv(Move mov)
{
	hashmv = mov;
}

inline Move Node::get_pvmove() const
{
	if (pvline.nmoves == 0) {
		return NO_MOVE;
	} else {
		return pvline.moves[0];
	}
}

inline void Node::set_historytable(HistoryTable * ht)
{
	historytable = ht;
}

inline void Node::add_killer(Move mov)
{
	if (killer1 == NO_MOVE) {
		killer1 = mov;
	} else if (killer2 != killer1) {
		killer2 = mov;
	}
}

inline Move Node::get_played_move() const
{
	return played_move;
}


/*****************************************************************************
 *
 * Inline functions of class NodeAllocator
 *
 *****************************************************************************/

inline Node* NodeAllocator::alloc()
{
	if (next == end) {
		return NULL;
	}
	Node * node = next++;
	node->_allocator = this;
	return node;
}

inline void NodeAllocator::free(Node* node) /* static */
{
	ASSERT_DEBUG(node->_allocator != NULL);
	ASSERT_DEBUG(node->_allocator->next - 1 == node);
	node->_allocator->next--;
}


#endif // NODE_H
