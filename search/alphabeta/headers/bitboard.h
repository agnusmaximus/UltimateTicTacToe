#ifndef _BITBOARD_
#define _BITBOARD_

#include "defines.h"

struct BitboardHasher {
    std::size_t operator()(const Bitboard& k) const {
	return k.p1[0]+k.p1[1]+k.p2[0]+k.p2[1];
    }
};

struct BitboardEquals {
    bool operator() (const Bitboard& t1, const Bitboard& t2) const {
	return t1.p1[0] == t2.p1[0] && t1.p2[0] == t2.p2[0] &&
	    t1.p1[1] == t2.p1[1] && t1.p2[1] == t2.p2[1];
    }
};

void SetBitboard(Bitboard &b, int index, char c) {
    int ii = 0;
    if (index >= 64) {
	ii = 1;
	index -= 64;
    }
    if (c == PLAYER_1) {
	b.p1[ii] |= (int64_t)1 << (63-index);
    }
    else if (c == PLAYER_2) {
	b.p2[ii] |= (int64_t)1 << (63-index);
    }
    if (c == EMPTY) {
	b.p1[ii] &= ~((int64_t)1 << (63-index));
	b.p2[ii] &= ~((int64_t)1 << (63-index));
    }
}

char GetBitboardValue(Bitboard &b, int index) {
    int ii = 0;
    if (index >= 64) {
	ii = 1;
	index -= 64;
    }
    int p1_value = (b.p1[ii] & ((int64_t)1 << (63-index))) >> (63-index);
    int p2_value = (b.p2[ii] & ((int64_t)1 << (63-index))) >> (63-index);
    if (p1_value) {
	return PLAYER_1;
    }
    if (p2_value) {
	return PLAYER_2;
    }
    return EMPTY;
}

void PrintBitboard(State &s) {
    cout << bitset<64>(s.bb.p1[0]) << " " << bitset<64>(s.bb.p1[1]) << endl;
    cout << bitset<64>(s.bb.p2[0]) << " " << bitset<64>(s.bb.p2[1]) << endl;
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
	    char value = GetBitboardValue(s.bb, i*BOARD_DIM+j);
	    if (value == PLAYER_1) {
		line += "x";
	    }
	    else if (value == PLAYER_2) {
		line += "o";
	    }
	    else {
		line += ".";
	    }
	}
	cerr << line << endl;
    }
    int halt;
    cin >> halt;
}

void UpdateBitboard(Bitboard &b, const Move &m) {
    int index = (m.x*BOARD_DIM+m.y);
    SetBitboard(b, index, m.who);
}

void RevertBitboard(Bitboard &b, const Move &m) {
    int index = (m.x*BOARD_DIM+m.y);
    SetBitboard(b, index, EMPTY);
}

#endif
