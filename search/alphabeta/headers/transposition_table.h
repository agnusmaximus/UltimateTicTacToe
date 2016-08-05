#ifndef _TRANSPOSITION_TABLE_
#define _TRANSPOSITION_TABLE_

#include <unordered_map>
#include "utils.h"

using namespace std;

#define EXACT_VALUE 0
#define LOWER_BOUND 1
#define UPPER_BOUND 2

struct TTEntry {
  Move m;
  int value, depth;
  short type;
};

typedef struct TTEntry TTEntry;

struct BoardHasher {
    size_t operator() (const array<char, BOARD_DIM*BOARD_DIM> &b) const {
	size_t hash = 0;
	for (int i = 0; i < BOARD_DIM*BOARD_DIM; i++) {
	    hash *= 3;
	    hash += (size_t)b[i];
	}
	return hash;
    }
};

unordered_map<array<char, BOARD_DIM*BOARD_DIM>, TTEntry, BoardHasher> transposition_table;

void ResetTranspositionTable() {
    transposition_table.clear();
}

void RotateBoard(array<char, BOARD_DIM*BOARD_DIM> &b) {
  for (int i = 0; i < BOARD_DIM/2; i++) {
    for (int j = 0; j < BOARD_DIM/2+1; j++) {
      char t = b[i*BOARD_DIM+j];
      b[i*BOARD_DIM+j] = b[j*BOARD_DIM+BOARD_DIM-1-i];
      b[j*BOARD_DIM+BOARD_DIM-1-i] = b[(BOARD_DIM-1-i)*BOARD_DIM+BOARD_DIM-1-j];
      b[(BOARD_DIM-1-i)*BOARD_DIM+BOARD_DIM-1-j] = b[(BOARD_DIM-1-j)*BOARD_DIM+i];
      b[(BOARD_DIM-1-j)*BOARD_DIM+i] = t;
    }
  }
}

bool GetTranspositionTableEntry(State &s, TTEntry **entry) {
  if (transposition_table.find(s.board) != transposition_table.end()) {
    *entry = &transposition_table[s.board];
    return true;
  }
  return false;
}

void AddTranspositionTableEntry(State &s, Move &bestmove, int alpha, int beta, int value, int depth) {
    array<char, BOARD_DIM*BOARD_DIM> bb = s.board;
    for (int i = 0; i < 1; i++) {
	TTEntry entry = {bestmove, value, depth, 0};
	entry = {bestmove, value, depth, 0};
	if (value <= alpha) {
	    entry.type = LOWER_BOUND;
	}
	else if (value >= beta) {
	    entry.type = UPPER_BOUND;
	}
	else {
	    entry.type = EXACT_VALUE;
	}
	transposition_table[bb] = entry;
	RotateBoard(bb);
    }
}


#endif
