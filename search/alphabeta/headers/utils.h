#ifndef _UTILS_
#define _UTILS_

#include <array>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>

#define DEBUG 0

#define BOARD_DIM 9
#define EMPTY 0
#define PLAYER_1 1
#define PLAYER_2 2
#define SELF PLAYER_1

#define DEPTH 12

using namespace std;
using namespace std::chrono;

struct Move {
  int x, y;
  char who;
};
typedef struct Move Move;

struct State {
  // Basic info.
  array<char, BOARD_DIM> results_board;
  array<char, BOARD_DIM*BOARD_DIM> board;
  vector<Move *> moves;
  char cur_player;

  // History heuristic.
  int history[BOARD_DIM][BOARD_DIM][2][DEPTH];
};
typedef struct State State;

struct MoveSort {
  MoveSort(State *state, int rdepth) {
    this->s = state;
    this->rdepth = rdepth;
  }
  inline bool operator() (const Move& m1, const Move& m2) {
    return false;
    return s->history[m1.x][m1.y][m1.who][rdepth] >
    s->history[m2.x][m2.y][m2.who][rdepth];
  }
  State *s;
  int rdepth;
};

int GetTimeMs();

void PrintBoard(State &s);

void Initialize(State &s);

char Other(char player);

bool IsFilled(char *data, int x, int y, int ldim);

bool DidWin(char *data, int x, int y, int ldim, char who);

bool DidWinSubgrid(State &s, int subgrid_x, int subgrid_y, char who);

bool DidWinGame(State &s, char who);

void PerformMove(State &s, Move &m);

void UndoMove(State &s, Move &m);

void AddCutoff(State &s, Move &m, int rdepth);

void OrderMoves(State &s, vector<Move> &moves, int rdepth);

void GenerateValidMoves(State &s, vector<Move> &moves);

#endif
