#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <limits.h>
#include "headers/utils.h"

using namespace std;

static int nodes_searched = 0;

int alphabeta(State &s, int depth, int a, int b, Move &choose, bool at_top_level) {
  nodes_searched += 1;
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
    int subscore = alphabeta(s, depth-1, a, b, choose, false);
    if (maximizing) {
      best_score = max(best_score, subscore);
      a = max(a, best_score);
      if (at_top_level && best_score == subscore) {
        choose = move;
      }
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

int iterative_deepening(State &s, int depth, Move &move) {
  for (int i = 1; i <= depth; i++) {
    nodes_searched = 0;
    auto start_time = GetTimeMs();
    alphabeta(s, i, INT_MIN, INT_MAX, move, true);
    auto end_time = GetTimeMs();
    printf("Depth %d [%d nodes %d ms]\n", i, nodes_searched, end_time-start_time);
  }
}

int main(void) {
  State s;
  Move bestmove;
  Initialize(s);
  iterative_deepening(s, 10, bestmove);
}
