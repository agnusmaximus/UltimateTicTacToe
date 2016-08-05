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

int evaluate(State &s, char player) {
    int score = 0;
    for (int i = 0; i < 9; i+=3) {
	for (int j = 0; j < 9; j+=3) {
	    if (DidWin(s.board.data(), i, j, BOARD_DIM, player)) {
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
    return MIN_VALUE;
  }
  if (DidWinGame(s, s.cur_player)) {
    return MAX_VALUE;
  }
  if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
    return 0;
  }
  if (depth <= 0 || GetTimeMs()-start_time >= TIME_LIMIT) {
    //return evaluate(s, s.cur_player) - evaluate(s, Other(s.cur_player));
    return 0;
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
  int best_score = MIN_VALUE;
  vector<Move> moves;
  Move bestmove;
  GenerateValidMoves(s, moves);
  OrderMoves(s, moves, previous_best);
  for (int i = 0; i < (int)moves.size(); i++) {
    Move move = moves[i];
    PerformMove(s, move);
    int subscore = -alphabeta(s, depth-1, -b, -a, choose, top_level, start_time);
    best_score = max(best_score, subscore);
    a = max(a, subscore);
    if (best_score == subscore) {
      bestmove = move;
      if (top_level == depth) {
        choose = bestmove;
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
  int score = 0;
  auto start_start_time = GetTimeMs();
  for (int i = 1; i <= depth; i++) {
    if (GetTimeMs() - start_start_time >= TIME_LIMIT) {
       break;
    }
    nodes_searched = 0;
    auto start_time = GetTimeMs();
    score = alphabeta(s, i, MIN_VALUE, MAX_VALUE, move, i, start_start_time);
    auto end_time = GetTimeMs();
    fprintf(stderr, "Depth %d [%d nodes, %d ms, %lf nodes per second]\n", i, nodes_searched, end_time-start_time, nodes_searched / (double)(end_time-start_time) * 1000);
  }
  fprintf(stderr, "Overall time %d ms, Score: %d\n", GetTimeMs()-start_start_time, score);
  fprintf(stderr, "Move: x-%d y-%d who-%d\n", move.x, move.y, (int)move.who);
  return 0;
}

#endif
