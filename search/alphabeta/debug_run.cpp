#include "headers/utils.h"
#include "headers/alphabeta.h"

using namespace std;

void DebugPlaySelf() {
  State s;
  Move bestmove;
  Initialize(s);
  string input = "";
  for (int i = 0; i < 1000; i++) {
      iterative_deepening(s, DEPTH, bestmove);
      PerformMove(s, bestmove);
      PrintBoard(s);
      SELF = Other(SELF);
      cin >> input;
  }
}

void DebugRun() {
  State s;
  Move bestmove;
  Initialize(s);
  iterative_deepening(s, DEPTH, bestmove);
}

int main(void) {
    //DebugPlaySelf();
    DebugRun();
}
