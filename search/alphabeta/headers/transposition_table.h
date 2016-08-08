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

unordered_map<bitset<162>, TTEntry> transposition_table(4000000);

void ResetTranspositionTable() {
    transposition_table.clear();
}

bool GetTranspositionTableEntry(State &s, TTEntry **entry) {
    if (transposition_table.find(s.bb) != transposition_table.end()) {
	*entry = &transposition_table[s.bb];
	return true;
    }
    return false;
}


void AddTranspositionTableEntry(State &s, Move &bestmove, int alpha, int beta, int value, int depth) {
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
    transposition_table[s.bb] = entry;
}

#endif
