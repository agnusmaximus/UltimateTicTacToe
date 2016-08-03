#ifndef _UTILS_
#define _UTILS_

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <unordered_map>
#include <vector>

#define DEBUG 0

#define BOARD_DIM 9
#define EMPTY 0
#define PLAYER_1 1
#define PLAYER_2 2
#define SELF PLAYER_1

using namespace std;

struct Move {
  int x, y;
  char who;
};
typedef struct Move Move;

struct State {
  array<char, 9> results_board;
  array<char, 81> board;
  vector<Move *> moves;
  char cur_player;
};

typedef struct State State;

void PrintBoard(State &s) {
  for (int i = 0; i < 9; i++) {
    string line = "";
    if (i % 3 == 0) {
      for (int j = 0; j < 12; j++ ){
        line += "-";
      }
      line += "\n";
    }
    for (int j = 0; j < 9; j++) {
      if (j % 3 == 0) {
        line += "|";
      }
      if (s.board[i*BOARD_DIM+j] == PLAYER_1) {
        line += "x";
      }
      else if (s.board[i*BOARD_DIM+j] == PLAYER_2) {
        line += "o";
      }
      else {
        line += ".";
      }
    }
    cout << line << endl;
  }
}

void Initialize(State &s) {
  memset(s.results_board.data(), 0, sizeof(char) * 9);
  memset(s.board.data(), 0, sizeof(char) * 9 * 9);
  s.cur_player = PLAYER_1;
}

char Other(char player) {
  if (player == PLAYER_1) return PLAYER_2;
  return PLAYER_1;
}

bool IsFilled(char *data, int x, int y, int ldim) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (data[(x+i)*ldim + (y+j)] == EMPTY) return false;
    }
  }
  return true;
}

bool DidWin(char *data, int x, int y, int ldim, char who) {
  for (int i = 0; i < 3; i++) {
    if (data[x*ldim+i+y] == who &&
        data[(x+1)*ldim+i+y] == who &&
        data[(x+2)*ldim+i+y] == who) {
      return true;
    }
    if (data[(x+i)*ldim+y] == who &&
        data[(x+i)*ldim+(y+1)] == who &&
        data[(x+i)*ldim+(y+2)] == who) {
      return true;
    }
  }
  bool diag1 = data[x*ldim+y] == who &&
      data[(x+1)*ldim+(y+1)] == who &&
      data[(x+2)*ldim+(y+2)];
  bool diag2 = data[x*ldim+y+2] == who &&
      data[(x+1)*ldim+(y+1)] == who &&
      data[(x+2)*ldim+y] == who;
  return diag1 || diag2;
}

bool DidWinSubgrid(State &s, int subgrid_x, int subgrid_y, char who) {
  return DidWin(s.board.data(), subgrid_x, subgrid_y, BOARD_DIM, who);
}

bool DidWinGame(State &s, char who) {
  return DidWin(s.results_board.data(), 0, 0, BOARD_DIM/3, who);
}

void PerformMove(State &s, Move &m) {
  int index = m.x * BOARD_DIM + m.y;
  s.board[index] = m.who;
  if (DidWinSubgrid(s, m.x/3, m.y/3, m.who)) {
    int results_index = (m.x / 3) * (BOARD_DIM / 3) + (m.y / 3);
    s.results_board[results_index] = m.who;
  }
  s.moves.push_back(&m);
  s.cur_player = Other(s.cur_player);
}

void UndoMove(State &s, Move &m) {
  int index = m.x * BOARD_DIM + m.y;
  s.board[index] = EMPTY;
  int results_index = (m.x / 3) * (BOARD_DIM / 3) + (m.y / 3);
  s.results_board[results_index] = EMPTY;
  s.cur_player = Other(s.cur_player);
  s.moves.pop_back();
}

void GenerateValidMoves(State &s, vector<Move> &moves) {
  Move *lastmove = NULL;
  if (s.moves.size() > 0) {
    lastmove = s.moves[s.moves.size()-1];
  }
  char current_player = s.cur_player;
  int lastmove_subgrid_x = -1, lastmove_subgrid_y = -1;
  if (lastmove != NULL) {
    lastmove_subgrid_x = (lastmove->x % (BOARD_DIM/3))*3;
    lastmove_subgrid_y = (lastmove->y % (BOARD_DIM/3))*3;
  }
  int can_move_anywhere = lastmove == NULL ||
      DidWinSubgrid(s, lastmove_subgrid_x, lastmove_subgrid_y, lastmove->who) ||
      DidWinSubgrid(s, lastmove_subgrid_x, lastmove_subgrid_y, current_player) ||
      IsFilled(s.board.data(), lastmove_subgrid_x, lastmove_subgrid_y, BOARD_DIM);

  for (int i = 0; i < BOARD_DIM; i++) {
    for (int j = 0; j < BOARD_DIM; j++) {
      bool empty = s.board[i*BOARD_DIM+j] == EMPTY;
      if (empty) {
        int cur_subgrid_x = i / (BOARD_DIM/3) * 3;
        int cur_subgrid_y = j / (BOARD_DIM/3) * 3;
        bool is_in_target_subgrid = (cur_subgrid_x == lastmove_subgrid_x) && (cur_subgrid_y == lastmove_subgrid_y);
        if (can_move_anywhere || is_in_target_subgrid) {
          moves.push_back((Move){i, j, current_player});
        }
      }
    }
  }

  if (DEBUG) {
    PrintBoard(s);
    cout << "Moves:";
    for (auto &move : moves) {
      cout << "{" << move.x << " " << move.y << " " << (int)move.who << "} ";
    }
    cout << endl;
  }
}

#endif
