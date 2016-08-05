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

#ifndef METHOD
#define METHOD BOT
#endif

#include <iostream>
#include <algorithm>
#include <limits.h>
#include <sstream>
#include <time.h>
#include <vector>

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
        s.cur_player = char(_botId);
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
		lastmove = (Move){i / 9, i % 9, Other(s.cur_player)};
		prev_state[i] = _field[i];
	    }
	}
	if (lastmove.who != -1) {
	    PerformMove(s, lastmove);
	}
        Move bestmove;
	iterative_deepening(s, DEPTH, bestmove);
	PerformMove(s, bestmove);
	prev_state[bestmove.x*9+bestmove.y] = s.cur_player;
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


void DebugPlaySelf() {
  State s;
  Move bestmove;
  Initialize(s);
  string input = "";
  for (int i = 0; i < 1000; i++) {
      iterative_deepening(s, DEPTH, bestmove);
      PerformMove(s, bestmove);
      PrintBoard(s);
      if (DidWinGame(s, PLAYER_1) ||
          DidWinGame(s, PLAYER_2) ||
          IsFilled(s.results_board.data(), 0, 0, BOARD_DIM/3)) {
        break;
      }
      //cin >> input;
  }
}

void DebugRun() {
  State s;
  Move bestmove;
  Initialize(s);
  iterative_deepening(s, DEPTH, bestmove);
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
}
