#ifndef _ALPHABETA_
#define _ALPHABETA_

#include "headers/utils.h"

using namespace std;


int PositionEval(const array<char, 81>& board,
    const array<char, 9>& results_board);

int alphabeta(State &s, int depth, int a, int b, Move &choose, int top_level);

int iterative_deepening(State &s, int depth, Move &move);


#endif
