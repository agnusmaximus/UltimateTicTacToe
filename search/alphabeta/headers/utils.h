#ifndef _UTILS_
#define _UTILS_

#include <array>
#include <algorithm>
#include <iostream>
#include <string.h>
#include <chrono>
#include <stdlib.h>
#include <map>
#include <unordered_map>
#include <vector>

#define DEBUG 0

#define BOARD_DIM 9
#define EMPTY 0
#define PLAYER_1 1
#define PLAYER_2 2
int SELF = PLAYER_1;

#define DEPTH 12
int TIME_LIMIT = 50000000;

using namespace std;
using namespace std::chrono;

struct Move {
  int x, y;
  char who;
};
typedef struct Move Move;

bool MoveEquals(const Move &m1, const Move &m2) {
    return m1.x==m2.x && m1.y==m2.y && m1.who==m2.who;
}

struct State {
  // Basic info.
  array<char, BOARD_DIM> results_board;
  array<char, BOARD_DIM*BOARD_DIM> board;
  vector<Move> moves;
  char cur_player;

  // History heuristic.
  int history[BOARD_DIM][BOARD_DIM][2];
};
typedef struct State State;

struct MoveSort {
  MoveSort(State *state, Move *previous) {
    this->s = state;
    this->m = previous;
  }
  inline bool operator() (const Move& m1, const Move& m2) {
      if (this->m != nullptr) {
	  if (MoveEquals(m1, *this->m)) return true;
	  if (MoveEquals(m2, *this->m)) return false;
      }
      return s->history[m1.x][m1.y][m1.who-1] >
	  s->history[m2.x][m2.y][m2.who-1];
  }
  State *s;
  Move *m;
};

int GetTimeMs() {
  milliseconds ms = duration_cast< milliseconds >(
      system_clock::now().time_since_epoch());
  return ms.count();
}

void PrintBoard(State &s) {
  for (int i = 0; i < BOARD_DIM; i++) {
    string line = "";
    if (i % 3 == 0) {
      for (int j = 0; j < BOARD_DIM+BOARD_DIM/3; j++ ){
        line += "-";
      }
      line += "\n";
    }
    for (int j = 0; j < BOARD_DIM; j++) {
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
  for (int i = 0; i < BOARD_DIM/3; i++) {
    for (int j = 0; j < BOARD_DIM/3; j++) {
      cout << (int)s.results_board[i*BOARD_DIM/3+j];
    }
    cout << endl;
  }
}

void Initialize(State &s) {
  memset(s.results_board.data(), 0, sizeof(char) * BOARD_DIM);
  memset(s.board.data(), 0, sizeof(char) * BOARD_DIM * BOARD_DIM);
  memset(s.history, 0, sizeof(int) * BOARD_DIM * BOARD_DIM * 2);
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
      data[(x+2)*ldim+(y+2)] == who;
  bool diag2 = data[x*ldim+y+2] == who &&
      data[(x+1)*ldim+(y+1)] == who &&
      data[(x+2)*ldim+y] == who;
  return diag1 || diag2;
}

bool DidWinSubgrid(State &s, int subgrid_x, int subgrid_y, char who) {
  /*if (DidWin(s.board.data(), subgrid_x, subgrid_y, BOARD_DIM, who)) {
    cout << "YOOOOOOOOOOO:" << (int)who << " " << subgrid_x << " " << subgrid_y << endl;
    PrintBoard(s);
    }*/

  return DidWin(s.board.data(), subgrid_x, subgrid_y, BOARD_DIM, who);
}

bool DidWinGame(State &s, char who) {
  return DidWin(s.results_board.data(), 0, 0, BOARD_DIM/3, who);
}

void PerformMove(State &s, Move &m) {
  int index = m.x * BOARD_DIM + m.y;
  s.board[index] = m.who;
  if (DidWinSubgrid(s, m.x/3 * 3, m.y/3 * 3, m.who)) {
    int results_index = (m.x / 3) * (BOARD_DIM / 3) + (m.y / 3);
    s.results_board[results_index] = m.who;
  }
  s.moves.push_back(m);
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

void AddScore(State &s, Move &m, int value) {
  s.history[m.x][m.y][m.who-1] += value;
}

void OrderMoves(State &s, vector<Move> &moves, Move *previous_best) {
    sort(moves.begin(), moves.end(), MoveSort(&s, previous_best));
}

void GenerateValidMoves(State &s, vector<Move> &moves) {
  Move *lastmove = NULL;
  if (s.moves.size() > 0) {
    lastmove = &s.moves[s.moves.size()-1];
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

  if (can_move_anywhere) {
      for (int i = 0; i < BOARD_DIM; i++) {
	  for (int j = 0; j < BOARD_DIM; j++) {
            int subgrid_x = i/3;
            int subgrid_y = j/3;
	      if (s.board[i*BOARD_DIM+j] == EMPTY && s.results_board[subgrid_x*BOARD_DIM/3+subgrid_y] == EMPTY) {
		  moves.push_back((Move){i, j, current_player});
	      }
	  }
      }
  }
  else {
      for (int i = lastmove_subgrid_x; i < lastmove_subgrid_x + 3; i++) {
	  for (int j = lastmove_subgrid_y; j < lastmove_subgrid_y  + 3; j++) {
	      if (s.board[i*BOARD_DIM+j] == EMPTY) {
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
