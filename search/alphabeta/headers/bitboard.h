#ifndef _BITBOARD_
#define _BITBOARD_

#include <stdlib.h>
#include "defines.h"

struct BitboardHasher {
    std::size_t operator()(const Bitboard& k) const {
	/*size_t h = 0;
	for (int i = 0; i < 8; i++) {
	    h += k.p1[i][0]+k.p1[i][1]+k.p2[i][0]+k.p2[i][1];
	    }*/
	return k.hash;
    }
};

struct BitboardEquals {
    bool operator() (const Bitboard& t1, const Bitboard& t2) const {
	//return t1.p1[t1.r_index][0] == t2.p1[t2.r_index][0] && t1.p2[t1.r_index][0] == t2.p2[t2.r_index][0] &&
	//t1.p1[t1.r_index][1] == t2.p1[t2.r_index][1] && t1.p2[t1.r_index][1] == t2.p2[t2.r_index][1];
	for (int i = 0; i < 8; i++) {
	    if (t1.p1[0][0] == t2.p1[i][0] &&
		t1.p1[0][1] == t2.p1[i][1] &&
		t1.p2[0][0] == t2.p2[i][0] &&
		t1.p2[0][1] == t2.p2[i][1]) return true;
	}
	return false;
    }
};

void InitializeBitboard(Bitboard &b) {
    memset(b.p1, 0, sizeof(int64_t) * 16);
    memset(b.p2, 0, sizeof(int64_t) * 16);
    b.hash = 0;
}

void SetBitboard(Bitboard &b, int index, char c, int rot_index=0) {
    int ii = 0;
    if (index >= 64) {
	ii = 1;
	index -= 64;
    }
    if (c == PLAYER_1) {
	b.p1[rot_index][ii] |= (int64_t)1 << (63-index);
	b.hash += (int64_t)1 << (63-index);
    }
    else if (c == PLAYER_2) {
	b.p2[rot_index][ii] |= (int64_t)1 << (63-index);
	b.hash += (int64_t)1 << (63-index);
    }
    if (c == EMPTY) {
	b.p1[rot_index][ii] &= ~((int64_t)1 << (63-index));
	b.p2[rot_index][ii] &= ~((int64_t)1 << (63-index));
	b.hash -= (int64_t)1 << (63-index);
    }
}

char GetBitboardValue(Bitboard &b, int index, int rot_index=0) {
    int ii = 0;
    if (index >= 64) {
	ii = 1;
	index -= 64;
    }
    int p1_value = (b.p1[rot_index][ii] & ((int64_t)1 << (63-index))) >> (63-index);
    int p2_value = (b.p2[rot_index][ii] & ((int64_t)1 << (63-index))) >> (63-index);
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
    int index2 = (m.x*BOARD_DIM+BOARD_DIM-1-m.y);
    SetBitboard(b, index2, m.who, 1);
    int index3 = ((BOARD_DIM-1-m.x)*BOARD_DIM+m.y);
    SetBitboard(b, index3, m.who, 2);
    int index4 = ((BOARD_DIM-1-m.x)*BOARD_DIM+BOARD_DIM-1-m.y);
    SetBitboard(b, index4, m.who, 3);

    /*index = (m.y*BOARD_DIM+m.x);
    SetBitboard(b, index, m.who, 4);
    index2 = (m.y*BOARD_DIM+BOARD_DIM-1-m.x);
    SetBitboard(b, index2, m.who, 5);
    index3 = ((BOARD_DIM-1-m.y)*BOARD_DIM+m.x);
    SetBitboard(b, index3, m.who, 6);
    index4 = ((BOARD_DIM-1-m.y)*BOARD_DIM+BOARD_DIM-1-m.x);
    SetBitboard(b, index4, m.who, 7);*/
}

void RevertBitboard(Bitboard &b, const Move &m) {
    int index = (m.x*BOARD_DIM+m.y);
    SetBitboard(b, index, EMPTY);
    int index2 = (m.x*BOARD_DIM+BOARD_DIM-1-m.y);
    SetBitboard(b, index2, EMPTY, 1);
    int index3 = ((BOARD_DIM-1-m.x)*BOARD_DIM+m.y);
    SetBitboard(b, index3, EMPTY, 2);
    int index4 = ((BOARD_DIM-1-m.x)*BOARD_DIM+BOARD_DIM-1-m.y);
    SetBitboard(b, index4, EMPTY, 3);

    /*index = (m.y*BOARD_DIM+m.x);
    SetBitboard(b, index, EMPTY, 4);
    index2 = (m.y*BOARD_DIM+BOARD_DIM-1-m.x);
    SetBitboard(b, index2, EMPTY, 5);
    index3 = ((BOARD_DIM-1-m.y)*BOARD_DIM+m.x);
    SetBitboard(b, index3, EMPTY, 6);
    index4 = ((BOARD_DIM-1-m.y)*BOARD_DIM+BOARD_DIM-1-m.x);
    SetBitboard(b, index4, EMPTY, 7);*/
}

#endif
