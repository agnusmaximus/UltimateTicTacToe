#ifndef _TESTCASES_
#define _TESTCASES_

#include <string>
#include "utils.h"
#include "alphabeta.h"

using namespace std;

void LoadState(State &s, string &bstate, Move &lastmove, char self) {
    int cur_index = 0;
    for (int i = 0; i < bstate.size(); i++) {
	if (bstate[i] == '.') {
	    s.board[cur_index++] = EMPTY;
	}
	else if (bstate[i] == 'x') {
	    s.board[cur_index++] = PLAYER_1;
	}
	else if (bstate[i] == 'o') {
	    s.board[cur_index++] = PLAYER_2;
	}
    }
    for (int i = 0; i < 9; i += 3) {
	for (int j = 0; j < 9; j += 3) {
	    if (DidWin(s.board.data(), i, j, BOARD_DIM, PLAYER_1)) {
		s.results_board[i/3*BOARD_DIM/3+j/3] = PLAYER_1;
	    }
	    if (DidWin(s.board.data(), i, j, BOARD_DIM, PLAYER_2)) {
		s.results_board[i/3*BOARD_DIM/3+j/3] = PLAYER_2;
	    }
	    if (IsFilled(s.board.data(), i, j, BOARD_DIM)) {
		s.results_board[i/3*BOARD_DIM/3+j/3] = TIE;
	    }
	}
    }
    s.moves.push_back(lastmove);
    s.cur_player = self;
}

void Case(string &t1, Move &lastmove) {
    State s;
    Move newmove;
    Initialize(s);
    LoadState(s, t1, lastmove, Other(lastmove.who));
    PrintBoard(s);
    printf("Lastmove: %d %d %d\n", lastmove.x, lastmove.y, lastmove.who);
    iterative_deepening(s, DEPTH, newmove);
}

void Test1() {
    string t1 = "------------\n"
	"|...|...|...\n"
	"|...|...|...\n"
	"|.xx|xxx|.xx\n"
	"------------\n"
	"|...|...|...\n"
	"|...|...|...\n"
	"|..x|..x|..x\n"
	"------------\n"
	"|.oo|oo.|ooo\n"
	"|...|ooo|...\n"
	"|.xo|xox|.ox\n";
    Move lastmove = (Move){6, 2, 2};
    Case(t1, lastmove);
}

void Test2() {
    string t1 =
	"------------\n"
	"|...|...|...\n"
	"|...|...|...\n"
	"|..x|.xx|..x\n"
	"------------\n"
	"|...|...|...\n"
	"|..x|...|...\n"
	"|.xx|.xx|.ox\n"
	"------------\n"
	"|...|.o.|ooo\n"
	"|oo.|oo.|o.o\n"
	"|o.x|xox|.ox\n";
    Move lastmove = (Move){5, 7, 2};
    Case(t1, lastmove);
}
void TestWrapper(void (*test)(void)) {
    cout << "--------------------------------" << endl;
    test();
    cout << "--------------------------------" << endl;
}

void RunTestCases() {
    TestWrapper(Test1);
    TestWrapper(Test2);
}

#endif
