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
  if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
    return 0;
  }
  if (depth <= 0 || GetTimeMs()-start_time >= TIME_LIMIT) {
      //d=7 P1 wins: 302 P2 wins: 80 ties: 118
      //return evaluate(s, s.cur_player) - evaluate(s, Other(s.cur_player));
      //d=7 P1 wins: 380 P2 wins: 63 ties: 57
      return 0;
  }

  TTEntry *entry = nullptr;
  Move principal_move = {EMPTY, EMPTY, EMPTY};
  if (GetTranspositionTableEntry(s, &entry) && entry->depth >= depth) {
    if (entry->type == EXACT_VALUE) {
      return entry->value;
    }
    else if (entry->type == LOWER_BOUND && entry->value > a) {
      a = entry->value;
    }
    else if (entry->type == UPPER_BOUND && entry->value < b) {
      b = entry->value;
    }
    if (a >= b) {
      return entry->value;
    }
    principal_move = entry->m;
  }
  nodes_searched += 1;

  int alpha_original = a, beta_original = b;
  int best_score = MIN_VALUE;
  Move moves[81];
  Move bestmove = {EMPTY,EMPTY,EMPTY};
  int n_moves_generated = GenerateValidMoves(s, moves);
  OrderMoves(s, moves, n_moves_generated, principal_move);
  for (int i = 0; i < n_moves_generated; i++) {
    Move &move = moves[i];
    PerformMove(s, move);
    int subscore = -alphabeta(s, depth-1, -b, -a, choose, top_level, start_time);
    if (subscore > best_score) {
	best_score = subscore;
	bestmove = move;
	if (top_level == depth) {
	    choose = bestmove;
	}
    }
    a = max(a, subscore);
    UndoMove(s, move);
    if (best_score >= b) {
      break;
    }
  }

  if (bestmove.who != EMPTY) {
      AddScore(s, bestmove, depth);
  }

  AddTranspositionTableEntry(s, bestmove, a, b, best_score, depth);

  return best_score;
}

int iterative_deepening(State &s, int depth, Move &move, bool verbose=true) {
  ResetTranspositionTable();
  int score = 0;
  auto start_start_time = GetTimeMs();\

  int i;
  for (i = 1; i <= depth; i++) {
    nodes_searched = 0;
    auto start_time = GetTimeMs();
    Move movecopy;
    score = alphabeta(s, i, MIN_VALUE, MAX_VALUE, movecopy, i, start_start_time);
    if (GetTimeMs() - start_start_time >= TIME_LIMIT) {
	break;
    }
    move = movecopy;
    auto end_time = GetTimeMs();
    if (verbose) {
	fprintf(stderr, "Depth %d [%d nodes, %d ms, %lf nodes per second]\n", i, nodes_searched, end_time-start_time, nodes_searched / (double)(end_time-start_time) * 1000);
    }
  }

  if (verbose) {
      fprintf(stderr, "Overall time %d ms, Score: %d\n", GetTimeMs()-start_start_time, score);
      fprintf(stderr, "Move: x-%d y-%d who-%d\n", move.x, move.y, (int)move.who);
  }
  return i-1;
}

int mtdf(State &s, int depth, Move &move, int f, int start_start_time) {
    int g = f;
    int upperbound = MAX_VALUE;
    int lowerbound = MIN_VALUE;
    int b = 0;
    while (lowerbound < upperbound) {
	if (g == lowerbound)
	    b = g+1;
	else
	    b = g;
	g = alphabeta(s, depth, b-1, b, move, depth, start_start_time);
	if (g < b)
	    upperbound = g;
	else
	    lowerbound = g;
    }
    return g;
}


int iterative_deepening_mtdf(State &s, int depth, Move &move) {
  ResetTranspositionTable();
  vector<int> guesses;
  guesses.push_back(0);
  auto start_start_time = GetTimeMs();

  for (int i = 1; i <= depth; i++) {
    if (GetTimeMs() - start_start_time >= TIME_LIMIT) {
       break;
    }
    nodes_searched = 0;
    auto start_time = GetTimeMs();
    int f = mtdf(s, i, move, guesses[max(0, i-3)], start_start_time);
    guesses.push_back(f);
    auto end_time = GetTimeMs();
    fprintf(stderr, "Depth %d [%d nodes, %d ms, %lf nodes per second]\n", i, nodes_searched, end_time-start_time, nodes_searched / (double)(end_time-start_time) * 1000);
  }

  fprintf(stderr, "Overall time %d ms, Score: %d\n", GetTimeMs()-start_start_time, guesses[guesses.size()-1]);
  fprintf(stderr, "Move: x-%d y-%d who-%d\n", move.x, move.y, (int)move.who);
  return 0;
}

#endif
