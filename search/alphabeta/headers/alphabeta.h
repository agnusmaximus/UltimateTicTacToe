#ifndef _ALPHA_BETA_
#define _ALPHA_BETA_

#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <limits.h>
#include "utils.h"
#include "transposition_table.h"

using namespace std;

static int nodes_searched = 0;

int evaluate(State &s) {
    int score = 0;
    for (int i = 0; i < 9; i+=3) {
	for (int j = 0; j < 9; j+=3) {
	    if (DidWin(s.board.data(), i, j, BOARD_DIM, s.cur_player)) {
		score += 100;
		if (i == 3 && j == 3) {
		    score += 100;
		}
	    }
	}
    }
    return score;
}

int alphabeta(State &s, int depth, int a, int b, Move &choose, int top_level, int start_time) {
  if (DidWinGame(s, Other(s.cur_player))) {
    return INT_MIN;
  }
  if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
    return 0;
  }
  if (depth <= 0 || GetTimeMs()-start_time >= TIME_LIMIT) {
    return evaluate(s);
  }

  TTEntry *entry = nullptr;
  Move *previous_best = nullptr;
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
    previous_best = &entry->m;
  }
  nodes_searched += 1;

  int alpha_original = a, beta_original = b;
  int best_score = INT_MIN;
  vector<Move> moves;
  Move bestmove;
  GenerateValidMoves(s, moves);
  OrderMoves(s, moves, previous_best);
  for (int i = 0; i < (int)moves.size(); i++) {
    Move move = moves[i];
    PerformMove(s, move);
    int subscore = -alphabeta(s, depth-1, -b, -a, choose, top_level, start_time);
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
      break;
    }
  }

  AddScore(s, bestmove, 1);
  AddTranspositionTableEntry(s, bestmove, alpha_original, beta_original, a, depth);

  return best_score;
}

int iterative_deepening(State &s, int depth, Move &move) {
  ResetTranspositionTable();
  auto start_start_time = GetTimeMs();
  for (int i = 1; i <= depth; i++) {
    if (GetTimeMs() - start_start_time >= TIME_LIMIT) {
       break;
    }
    nodes_searched = 0;
    auto start_time = GetTimeMs();
    alphabeta(s, i, INT_MIN, INT_MAX, move, i, start_start_time);
    auto end_time = GetTimeMs();
    printf("Depth %d [%d nodes, %d ms, %lf nodes per second]\n", i, nodes_searched, end_time-start_time, nodes_searched / (double)(end_time-start_time) * 1000);
  }
  printf("Overall time %d ms\n", GetTimeMs()-start_start_time);
  return 0;
}

#endif
