#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <limits.h>
#include "headers/utils.h"

using namespace std;

int alphabeta(State &s, int depth, int a, int b) {
  if (DidWinGame(s, Other(s.cur_player))) {
    if (s.cur_player == SELF) {
      return INT_MIN;
    }
    return INT_MAX;
  }
  if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
    return 0;
  }
  if (depth <= 0) {
    return 0;
  }
  int maximizing = s.cur_player == SELF;
  int best_score = maximizing ? INT_MIN : INT_MAX;
  vector<Move> moves;
  GenerateValidMoves(s, moves);
  for (auto &move : moves) {
    PerformMove(s, move);
    int subscore = alphabeta(s, depth-1, a, b);
    if (maximizing) {
      best_score = max(best_score, subscore);
      a = max(a, best_score);
    }
    else {
      best_score = min(best_score, subscore);
      b = min(b, best_score);
    }
    UndoMove(s, move);
    if (a >= b) {
      break;
    }
  }
  return best_score;
}

int main(void) {
  State s;
  Initialize(s);
  alphabeta(s, 10, INT_MIN, INT_MAX);
}
