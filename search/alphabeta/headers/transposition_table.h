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
      hash += b[i];
    }
    return hash;
  }
};

unordered_map<array<char, BOARD_DIM*BOARD_DIM>, TTEntry, BoardHasher> transposition_table;

bool GetTranspositionTableEntry(State &s, TTEntry **entry) {
  if (transposition_table.find(s.board) != transposition_table.end()) {
    *entry = &transposition_table[s.board];
    return true;
  }
  return false;
}

void AddTranspositionTableEntry(State &s, Move &bestmove, int alpha, int beta, int value, int depth) {
  TTEntry new_entry = {bestmove, value, depth, 0};
  if (value <= alpha) {
    new_entry.type = UPPER_BOUND;
  }
  else if (value >= beta) {
    new_entry.type = LOWER_BOUND;
  }
  else {
    new_entry.type = EXACT_VALUE;
  }
  transposition_table[s.board] = new_entry;
}


#endif
