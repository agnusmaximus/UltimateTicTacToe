#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <limits.h>
#include "headers/utils.h"
#include "headers/transposition_table.h"

using namespace std;

static int nodes_searched = 0;

int alphabeta(State &s, int depth, int a, int b, Move &choose, int top_level) {
  if (DidWinGame(s, Other(s.cur_player))) {
    return INT_MIN;
  }
  if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
    return 0;
  }
  if (depth <= 0) {
    return 0;
  }

  TTEntry *entry = NULL;
  if (GetTranspositionTableEntry(s, &entry) && entry->depth >= depth) {
    if (entry->type == EXACT_VALUE) {
      return entry->value;
    }
    else if (entry->type == LOWER_BOUND) {
      a = entry->value;
    }
    else if (entry->type == UPPER_BOUND) {
      b = entry->value;
    }
    if (a >= b) {
      return entry->value;
    }
  }
  nodes_searched += 1;

  int alpha_original = a, beta_original = b;
  int best_score = INT_MIN;
  Move bestmove;
  vector<Move> moves;
  GenerateValidMoves(s, moves);
  OrderMoves(s, moves, top_level-depth);
  for (auto &move : moves) {
    PerformMove(s, move);
    int subscore = -alphabeta(s, depth-1, -b, -a, choose, false);
    best_score = max(best_score, subscore);
    a = max(a, best_score);
    if (best_score == subscore) {
      bestmove = move;
      if (top_level == depth) {
        choose = move;
      }
    }
    UndoMove(s, move);
    if (a >= b) {
      AddCutoff(s, move, top_level-depth);
      break;
    }
  }

  AddTranspositionTableEntry(s, bestmove, alpha_original, beta_original, a, depth);

  return best_score;
}

int iterative_deepening(State &s, int depth, Move &move) {
  for (int i = 1; i <= depth; i++) {
    nodes_searched = 0;
    auto start_time = GetTimeMs();
    alphabeta(s, i, INT_MIN, INT_MAX, move, i);
    auto end_time = GetTimeMs();
    printf("Depth %d [%d nodes, %d ms, %lf nodes per second]\n", i, nodes_searched, end_time-start_time, nodes_searched / (double)(end_time-start_time) * 1000);
  }
}

int main(void) {
  State s;
  Move bestmove;
  Initialize(s);
  iterative_deepening(s, DEPTH, bestmove);
}
