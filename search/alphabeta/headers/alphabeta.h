#ifndef _ALPHA_BETA_
#define _ALPHA_BETA_

#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <limits.h>
#include "utils.h"
#include "transposition_table.h"

using namespace std;

static int nodes_searched = 0;
static int n_leaf_nodes = 0;

float evaluate(State &s) {
    return s.score[s.cur_player-1] - s.score[Other(s.cur_player)-1];
}

float alphabeta(State &s, int depth, float a, float b, Move &choose, int top_level, int start_time) {

    if (s.did_win_overall) {
	return MIN_VALUE / (top_level-depth+1);
    }
    if (s.n_subgrids_won == 9) {
	return 0;
    }
    if (depth <= 0 || GetTimeMs()-start_time >= TIME_LIMIT) {
	n_leaf_nodes++;
	return evaluate(s);
    }

    TTEntry *entry = nullptr;
    if (GetTranspositionTableEntry(s, &entry) && entry->depth >= depth) {
	if (entry->type == EXACT_VALUE) {
	    return entry->value;
	}
	else if (entry->type == LOWER_BOUND && entry->value > a) {
	    a = entry->value;
	}
	else if (entry->type == UPPER_BOUND && entry->value < b) {
	    b = entry->value;
	}
	if (a >= b) {
	    return entry->value;
	}
    }
    nodes_searched += 1;

    float alpha_original = a, beta_original = b;
    float best_score = MIN_VALUE;
    Move moves[81];
    Move bestmove = {EMPTY,EMPTY,EMPTY};
    int n_moves_generated = GenerateValidMoves(s, moves);
    OrderMoves(s, moves, n_moves_generated);

    for (int i = 0; i < n_moves_generated; i++) {
	Move &move = moves[i];
	PerformMove(s, move);
	float subscore = -alphabeta(s, depth-1, -b, -a, choose, top_level, start_time);
	if (subscore > best_score) {
	    best_score = subscore;
	    bestmove = move;
	    if (top_level == depth) {
		choose = bestmove;
	    }
	}
	a = max(a, subscore);
	UndoMove(s, move);
	if (best_score >= b) {
	    break;
	}
    }

    if (bestmove.who != EMPTY) {
	AddScore(s, bestmove, depth);
    }
    AddTranspositionTableEntry(s, bestmove, a, b, best_score, depth);

    return best_score;
}

int iterative_deepening(State &s, int depth, Move *move, bool verbose=true) {
    ResetTranspositionTable();
    auto start_start_time = GetTimeMs();

    int will_win = false, i;
    float score = 0;
    float previous_score = 0;
    Move movecopy;
    for (i = 1; i <= depth; i++) {
	n_leaf_nodes = 0;
	nodes_searched = 0;
	auto start_time = GetTimeMs();
	previous_score = alphabeta(s, i, MIN_VALUE, MAX_VALUE, movecopy, i, start_start_time);
	if (GetTimeMs() - start_start_time >= TIME_LIMIT) {
	    break;
	}
	auto end_time = GetTimeMs();
	if (verbose) {
	    fprintf(stderr, "Depth %d [%d nodes, %d ms, %lf nodes per second]\n", i, nodes_searched, end_time-start_time, nodes_searched / (double)(end_time-start_time) * 1000);
	}
	score = previous_score;
	memcpy(move, &movecopy, sizeof(Move));
    }

    if (verbose) {
	fprintf(stderr, "Overall time %d ms, Score: %f\n", GetTimeMs()-start_start_time, score);
	fprintf(stderr, "Move: x-%d y-%d who-%d\n", move->x, move->y, (int)move->who);
	fprintf(stderr, "%d leaf nodes at depth=%d\n", n_leaf_nodes, i-1);
    }

    return i-1;
}

// Seems buggy.
int mtdf(State &s, int depth, Move &move, int f, int start_start_time) {
    int g = f;
    int upperbound = MAX_VALUE;
    int lowerbound = MIN_VALUE;
    int b = 0;
    while (lowerbound < upperbound) {
	b = max(g, lowerbound+1);
	g = alphabeta(s, depth, b-1, b, move, depth, start_start_time);
	if (g < b)
	    upperbound = g;
	else
	    lowerbound = g;
    }
    return g;
}

// Seems buggy.
int iterative_deepening_mtdf(State &s, int depth, Move *move, bool verbose=true) {
    ResetTranspositionTable();
    vector<int> guesses;
    guesses.push_back(0);
    auto start_start_time = GetTimeMs();

    int i, score=0;
    Move movecopy;
    for (i = 1; i <= depth; i++) {
	if (GetTimeMs() - start_start_time >= TIME_LIMIT) {
	    break;
	}
	nodes_searched = 0;
	auto start_time = GetTimeMs();
	int f = mtdf(s, i, movecopy, guesses[max(0, i-1)], start_start_time);
	if (GetTimeMs() - start_start_time >= TIME_LIMIT) {
	    break;
	}
	score = f;
	memcpy(move, &movecopy, sizeof(Move));
	guesses.push_back(f);
	auto end_time = GetTimeMs();
	if (verbose) {
	    fprintf(stderr, "Depth %d [%d nodes, %d ms, %lf nodes per second]\n", i, nodes_searched, end_time-start_time, nodes_searched / (double)(end_time-start_time) * 1000);
	}
    }

    if (verbose) {
	fprintf(stderr, "Overall time %d ms, Score: %d\n", GetTimeMs()-start_start_time, score);
	fprintf(stderr, "Move: x-%d y-%d who-%d\n", move->x, move->y, (int)move->who);
    }
    return i-1;
}

#endif
