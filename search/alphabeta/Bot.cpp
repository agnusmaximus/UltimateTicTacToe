/*******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Abdulla Gaibullaev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#define BOT 0
#define DEBUG_RUN 1
#define PLAY_DEBUG 2
#define PLAY_RANDOM 3
#define PLAY_RANDOM_MANY 4
#define RUN_TESTS 5
#define BENCHMARK 6
#define PIT_WEIGHTS 7

#ifndef METHOD
#define METHOD BOT
#endif

#define N_BENCHMARK_RUNS 20

#include <iostream>
#include <algorithm>
#include <limits.h>
#include <sstream>
#include <time.h>
#include <vector>

#include "headers/testcases.h"
#include "headers/utils.h"
#include "headers/alphabeta.h"

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    elems.clear();
    while (std::getline(ss, item, delim)) {
	elems.push_back(item);
    }
    return elems;
}


int stringToInt(const std::string &s) {
    std::istringstream ss(s);
    int result;
    ss >> result;
    return result;
}

/**
 * This class implements all IO operations.
 * Only one method must be realized:
 *
 *      > BotIO::action
 *
 */
class BotIO
{

public:

    /**
     * Initialize your bot here.
     */
    BotIO() {
	srand(static_cast<unsigned int>(time(0)));
	_field.resize(81);
	_macroboard.resize(9);
	Initialize(s);
	s.cur_player = (char)PLAYER_1;
	for (int i = 0; i < 81; i++) {
	    prev_state.push_back(0);
	}
    }


    void loop() {
	std::string line;
	std::vector<std::string> command;
	command.reserve(256);

	while (std::getline(std::cin, line)) {
	    processCommand(split(line, ' ', command));
	}
    }

private:
    State s;
    vector<int> prev_state;

    /**
     * Implement this function.
     * type is always "move"
     *
     * return value must be position in x,y presentation
     *      (use std::make_pair(x, y))
     */
    std::pair<int, int> action(const std::string &type, int time) {
	Move lastmove = {-1, -1, -1};
	for (int i = 0; i < 81; i++) {
	    if (prev_state[i] != _field[i]) {
		lastmove = (Move){i / 9, i % 9, s.cur_player};
		fprintf(stderr, "Last Move: %d %d %d\n", lastmove.x, lastmove.y, lastmove.who);
		prev_state[i] = _field[i];
	    }
	}
	if (lastmove.who != -1) {
	    PerformMove(s, lastmove);
	}
	fprintf(stderr, "Current State: \n");
	PrintBoard(s);
	Move bestmove;
	iterative_deepening(s, DEPTH, &bestmove);
	PerformMove(s, bestmove);
	prev_state[bestmove.x*9+bestmove.y] = _botId;
	fprintf(stderr, "New State after move: \n");
	PrintBoard(s);
	return std::make_pair(bestmove.y, bestmove.x);
    }

    /**
     * Returns random free cell.
     * It can be used to make your bot more immune to errors
     * Use next pattern in action method:
     *
     *      try{
     *          ... YOUR ALGORITHM ...
     *      }
     *      catch(...) {
     *          return getRandomCell();
     *      }
     *
     */
    std::pair<int, int> getRandomFreeCell() const {
	debug("Using random algorithm.");
	std::vector<int> freeCells;
	for (int i = 0; i < 81; ++i){
	    int blockId = ((i/27)*3) + (i%9)/3;
	    if (_macroboard[blockId] == -1 && _field[i] == 0){
		freeCells.push_back(i);
	    }
	}
	int randomCell = freeCells[rand()%freeCells.size()];
	return std::make_pair(randomCell%9, randomCell/9);
    }

    void processCommand(const std::vector<std::string> &command) {
	if (command[0] == "action") {
	    auto point = action(command[1], stringToInt(command[2]));
	    std::cout << "place_move " << point.first << " " << point.second << std::endl << std::flush;
	}
	else if (command[0] == "update") {
	    update(command[1], command[2], command[3]);
	}
	else if (command[0] == "settings") {
	    setting(command[1], command[2]);
	}
	else {
	    debug("Unknown command <" + command[0] + ">.");
	}
    }

    void update(const std::string& player, const std::string& type, const std::string& value) {
	if (player != "game" && player != _myName) {
	    // It's not my update!
	    return;
	}

	if (type == "round") {
	    _round = stringToInt(value);
	}
	else if (type == "move") {
	    _move = stringToInt(value);
	}
	else if (type == "macroboard" || type == "field") {
	    std::vector<std::string> rawValues;
	    split(value, ',', rawValues);
	    std::vector<int>::iterator choice = (type == "field" ? _field.begin() : _macroboard.begin());
	    std::transform(rawValues.begin(), rawValues.end(), choice, stringToInt);
	}
	else {
	    debug("Unknown update <" + type + ">.");
	}
    }

    void setting(const std::string& type, const std::string& value) {
        if (type == "timebank") {
            _timebank = stringToInt(value);
        }
        else if (type == "time_per_move") {
            _timePerMove = stringToInt(value);
        }
        else if (type == "player_names") {
            split(value, ',', _playerNames);
        }
        else if (type == "your_bot") {
            _myName = value;
        }
        else if (type == "your_botid") {
            _botId = stringToInt(value);
        }
        else {
            debug("Unknown setting <" + type + ">.");
        }
    }

    void debug(const std::string &s) const{
        std::cerr << s << std::endl << std::flush;
    }

private:
    // static settings
    int _timebank;
    int _timePerMove;
    int _botId;
    std::vector<std::string> _playerNames;
    std::string _myName;

    // dynamic settings
    int _round;
    int _move;
    std::vector<int> _macroboard;
    std::vector<int> _field;
};

char CheckEnd(State &s, bool verbose=true) {
    if (DidWinGame(s, PLAYER_1)) {
	if (verbose) {
	    cout << "PLAYER 1 WON" << endl;
	}
	return PLAYER_1;
    }
    if (DidWinGame(s, PLAYER_2)) {
	if (verbose) {
	    cout << "PLAYER 2 WON" << endl;
	}
	return PLAYER_2;
    }
    if (IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
	if (verbose) {
	    cout << "TIE" << endl;
	}
	return TIE;
    }
    return EMPTY;
}

void PitWeights() {
    //float w1[] = {1.586858, 2.698336, 0.528802, -0.064983, 2.015526, 1.180479, 0.691811, 0.920403, 1.087781, 0.131057, 0, 0, 0};
    float w1[] = {1.204872, 1.824167, -0.230357, -0.559060, 1.977134, 2.148259, 0.992414, 1.333402, 1.904044, 0.199132, 1.321023, 0.826519, 1.878671};
    //float w1[] = {1.481971, 1.750855, -0.857470, -1.014151, 2.028622, 0.415837, 1.018934, 0.717313, 2.228565, 0.421951, 1.379892, 0.910523, 2.003535};
    float w2[] = {1.045752, 1.242940, 0.262812, 1.189628, 4.365139, 1.484971, 1.315156, 1.113282, 0.697767, 0.018774, 1.498037, 0.475005, 1.907185};

    State s1, s2;
    InitializeWithWeights(s1, w1);
    InitializeWithWeights(s2, w2);
    int turn = 0;
    while (true) {
	Move first_player_move;
	iterative_deepening(s1, DEPTH, &first_player_move, true);
	PerformMove(s1, first_player_move);
	PerformMove(s2, first_player_move);

	PrintBoard(s1);
	if (CheckEnd(s1)) break;

	Move second_player_move;
	iterative_deepening(s2, DEPTH, &second_player_move, true);
	PerformMove(s1, second_player_move);
	PerformMove(s2, second_player_move);

	PrintBoard(s1);
	if (CheckEnd(s1)) break;
    }
}

void DebugPlaySelf() {
    State s;
    Move bestmove;
    Initialize(s);
    string input = "";
    while (true) {
	iterative_deepening(s, DEPTH, &bestmove);
	PerformMove(s, bestmove);
	PrintBoard(s);
	if (CheckEnd(s)) {
	    break;
	}
	//cin >> input;
    }
}

char DebugPlayRandom() {
    State s;
    Move bestmove;
    Initialize(s);
    string input = "";
    while (true) {
	iterative_deepening(s, DEPTH, &bestmove);
	PerformMove(s, bestmove);
	PrintBoard(s);
	if (CheckEnd(s)) {
	    return CheckEnd(s);
	}
	Move valid_moves[81];
	int n_moves = GenerateValidMoves(s, valid_moves);
	PerformMove(s, valid_moves[rand()%n_moves]);
	PrintBoard(s);
	if (CheckEnd(s)) {
	    return CheckEnd(s);
	}
	//cin >> input;
    }
    return 0;
}

void DebugPlayRandomMany() {
    int p1_wins=0, p2_wins=0, tie=0;
    for (int i = 0; i < N_BENCHMARK_RUNS; i++) {
	int win = DebugPlayRandom();
	if (win == PLAYER_1) {
	    p1_wins++;
	}
	else if (win == PLAYER_2) {
	    p2_wins++;
	}
	else {
	    tie++;
	}
    }
    fprintf(stderr, "P1 wins: %d P2 wins: %d ties: %d\n", p1_wins, p2_wins, tie);
}

void DebugRun() {
    State s;
    Move bestmove;
    Initialize(s);
    iterative_deepening(s, DEPTH, &bestmove);
}

double GetStddev(vector<int> &v){
    double avg = 0;
    for (const auto &val : v) {
	avg += val;
    }
    avg /= v.size();
    double total = 0;
    for (const auto &val : v) {
	total += (val - avg) * (val-avg);
    }
    total /= v.size();
    return sqrt(total);
}

void BenchmarkAgainstRandom() {
    int depth_sum = 0, n_counts = 0;
    vector<int> depths;
    int depth_hist[DEPTH+1];
    memset(depth_hist, 0, sizeof(int) * (DEPTH+1));
    fprintf(stderr, "Running benchmark...\n");
    for (int i = 0; i < N_BENCHMARK_RUNS; i++) {
	State s;
	Move bestmove;
	Initialize(s);
	while (true) {
	    int cur_depth = iterative_deepening(s, DEPTH, &bestmove, false);
	    // Discard those that reach max depth, since these are
	    // endgame searches.
	    if (cur_depth != DEPTH) {
		n_counts++;
		depth_sum += cur_depth;
		depths.push_back(cur_depth);
		depth_hist[cur_depth]++;
	    }
	    PerformMove(s, bestmove);
	    if (CheckEnd(s, false)) {
		break;
	    }
	    Move valid_moves[81];
	    int n_moves = GenerateValidMoves(s, valid_moves);
	    PerformMove(s, valid_moves[rand()%n_moves]);
	    if (CheckEnd(s, false)) {
		break;
	    }
	}
    }
    fprintf(stderr, "Depth histogram:\n");
    for (int i = 1; i <= DEPTH; i++) {
	fprintf(stderr, "%d: ", i);
	string line = "";
	for (int k = 0; k < depth_hist[i]; k++) {
	    line += "*";
	}
	fprintf(stderr, "%s\n", line.c_str());
    }
    int max_index = -1, max_arg = -1;
    for (int i = 1; i <= DEPTH; i++) {
	if (depth_hist[i] >= max_arg) {
	    max_arg = depth_hist[i];
	    max_index = i;
	}
    }
    fprintf(stderr, "Avg depth: %lf\nStd dev: %lf\nDepth with most counts: %d\nTime Limit %d ms\n", (double)depth_sum/(double)n_counts, GetStddev(depths), max_index, TIME_LIMIT);
}

/**
 * don't change this code.
 * See BotIO::action method.
 **/
int main() {
    if (METHOD == BOT) {
	BotIO bot;
	bot.loop();
	return 0;
    }
    if (METHOD == DEBUG_RUN) {
	DebugRun();
    }
    if (METHOD == PLAY_DEBUG) {
	DebugPlaySelf();
    }
    if (METHOD == PLAY_RANDOM) {
        DebugPlayRandom();
    }
    if (METHOD == PLAY_RANDOM_MANY) {
	DebugPlayRandomMany();
    }
    if (METHOD == RUN_TESTS) {
	RunTestCases();
    }
    if (METHOD == BENCHMARK) {
	BenchmarkAgainstRandom();
    }
    if (METHOD == PIT_WEIGHTS) {
	PitWeights();
    }
}
