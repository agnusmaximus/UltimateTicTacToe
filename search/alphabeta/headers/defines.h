#ifndef _DEFINES_
#define _DEFINES_

#define DEBUG 0

#define BOARD_DIM 9
#define EMPTY 0
#define PLAYER_1 1
#define PLAYER_2 2
#define TIE 3
#define DEPTH 20

#define MIN_VALUE (-10000000)
#define MAX_VALUE (10000000)

int TIME_LIMIT = 500;

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

struct Bitboard {
    int64_t p1[8][2];
    int64_t p2[8][2];
    int64_t hash;
};

typedef struct Bitboard Bitboard;

struct State {
  // Basic info.
  array<char, BOARD_DIM> results_board;
  array<char, BOARD_DIM*BOARD_DIM> board;
  Bitboard bb;
  vector<Move> moves;
  char cur_player;

  // History heuristic.
  int history[BOARD_DIM][BOARD_DIM][2];
};
typedef struct State State;

#endif
