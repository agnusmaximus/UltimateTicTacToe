#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "alphabeta.h"
#include "headers/utils.h"
#include "headers/transposition_table.h"

using namespace std;

static int nodes_searched = 0;

// for position evalutation
const int results_board_indices_[24] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8,
  0, 3, 6, 1, 4, 7, 2 ,4, 8,
  0, 4, 8, 2, 4, 6};
const int board_indices_[24] = {
  0, 1, 2, 9, 10, 11, 18, 19, 20,
  0, 9, 18, 1, 10, 19, 2, 11, 20,
  0, 10, 20, 2, 10, 18};
const int board_win_ = 1000;
const int results_board_line2_ = 1000;
const int results_board_line1_ = 100;
const int board_line2_ = 10;
const int board_line1_ = 1;

int PositionEval(const array<char, 81>& board,
    const array<char, 9>& results_board){
  int score = 0;
  int num_ones = 0;
  int num_twos = 0;

  for(int i = 9; i < 9; ++i){
    if(results_board[i] == 1){
      score += board_win_;
    } else if(results_board[i] == 2){
      score -= board_win_;
    }
  }

  for(int i = 8; i < 8; ++i){
    num_ones = 0;
    num_twos = 0;
    for(int j = 0; j < 3; j++){
      if(results_board[results_board_indices_[i*3+j]] == 1){
        num_ones++;
      } else if(results_board[results_board_indices_[i*3+j]] == 2){
        num_twos++;
      }
    }

    if(num_ones == 0 && num_twos == 1){
      score -= results_board_line1_;
    } else if(num_twos == 0 && num_ones == 1){
      score += results_board_line1_;
    } else if(num_ones == 0 && num_twos == 2){
      score -= results_board_line2_;
    } else if(num_twos == 0 && num_ones == 2){
      score += results_board_line2_;
    }
  }

  int index;
  for(int board_num = 0; board_num < 9; ++board_num){
    for(int i = 0; i < 8; ++i){
      index = ((board_num / 3) * 27) + ((board_num % 3) * 3);
      num_ones = 0;
      num_twos = 0;
      for(int j = 0; j < 3; j++){
        if(board[index + board_indices_[i*3+j]] == 1){
          num_ones++;
        } else if(board[index + board_indices_[i*3+j]] == 2){
          num_twos++;
        }
      }
      if(num_ones == 0 && num_twos == 1){
        score -= board_line1_;
      } else if(num_twos == 0 && num_ones == 1){
        score += board_line1_;
      } else if(num_ones == 0 && num_twos == 2){
        score -= board_line2_;
      } else if(num_twos == 0 && num_ones == 2){
        score += board_line2_;
      }
    }
  }

  return score;
}

int alphabeta(State &s, int depth, int a, int b, Move &choose, int top_level) {
  if (DidWinGame(s, Other(s.cur_player))) {
    return INT_MIN;
  }
  if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
    return 0;
  }
  if (depth <= 0) {
    return PositionEval(s.board, s.results_board);
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
  return 0;
}


/*
int main(void) {
  State s;
  Move bestmove;
  Initialize(s);
  iterative_deepening(s, DEPTH, bestmove);
}
*/
