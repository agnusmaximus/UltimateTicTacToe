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
	    int mx = cur_index/BOARD_DIM;
	    int my = cur_index%BOARD_DIM;
	    s.board[cur_index++] = PLAYER_1;
	    int subgrid_index = mx/3*BOARD_DIM/3+my/3;
	    UpdatePieceCounts(s, {mx, my, PLAYER_1}, subgrid_index, 1);
	}
	else if (bstate[i] == 'o') {
	    int mx = cur_index/BOARD_DIM;
	    int my = cur_index%BOARD_DIM;
	    s.board[cur_index++] = PLAYER_2;
	    int subgrid_index = mx/3*BOARD_DIM/3+my/3;
	    UpdatePieceCounts(s, {mx, my, PLAYER_2}, subgrid_index, 1);
	}
    }
    for (int i = 0; i < 9; i += 3) {
	for (int j = 0; j < 9; j += 3) {
	    int subgrid_index = i/3*BOARD_DIM/3+j/3;
	    if (DidWin(s.board.data(), i, j, BOARD_DIM, PLAYER_1)) {
		UpdateOverallPieceCounts(s, {EMPTY, EMPTY, PLAYER_1}, subgrid_index, 1);
		s.results_board[i/3*BOARD_DIM/3+j/3] = PLAYER_1;
	    }
	    if (DidWin(s.board.data(), i, j, BOARD_DIM, PLAYER_2)) {
		UpdateOverallPieceCounts(s, {EMPTY, EMPTY, PLAYER_2}, subgrid_index, 1);
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
    iterative_deepening(s, DEPTH, &newmove);
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

void Test3() {
    string t1 =
	"------------\n"
	"|x..|...|...\n"
	"|ooo|...|.oo\n"
	"|...|ooo|...\n"
	"------------\n"
	"|x..|.x.|.xx\n"
	"|oxo|..o|xx.\n"
	"|o.o|oox|x..\n"
	"------------\n"
	"|x..|.x.|..x\n"
	"|xx.|x..|x.o\n"
	"|oxx|o..|o..\n";
    Move lastmove = (Move){8, 6, 2};
    Case(t1, lastmove);
}

void Test4() {
    string t1 =
	"------------\n"
	"|o.x|...|.o.\n"
	"|..x|x.x|..x\n"
	"|..x|xxx|.xx\n"
	"------------\n"
	"|.o.|x..|xoo\n"
	"|..x|..o|oox\n"
	"|.xx|.x.|x.o\n"
	"------------\n"
	"|.o.|ooo|o.o\n"
	"|..o|o..|oxo\n"
	"|..o|...|x..\n";
    Move lastmove = (Move){8, 6, 1};
    Case(t1, lastmove);
}


void Test5() {
    string t1 =
	"|xoo|xox|xox\n"
	"|ooo|xxx|ooo\n"
	"|...|...|...\n"
	"------------\n"
	"|xox|xox|xox\n"
	"|oxx|oxo|oxo\n"
	"|x..|ooo|x..\n"
	"------------\n"
	"|oxx|x..|x..\n"
	"|...|...|...\n"
	"|oo.|xo.|...\n";
    Move lastmove = (Move){5,4,2};
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
    TestWrapper(Test3);
    TestWrapper(Test4);
    TestWrapper(Test5);
}

#endif
