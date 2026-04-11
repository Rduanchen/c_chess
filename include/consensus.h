#ifndef CONSENSUS_H
#define CONSENSUS_H

/*
Note:
1. Please mark the headfile above the code: #include "consensus.h"
2. Creating function for UI, add "UI_" before the name like "void UI_example();"
3. Creating function for rule define & recognize, add "RULE_" before the name
4. Creating function for computer caculating (AI), add "AI_" before the name
5. Creating function for edit the chess board info, add "BD_" before the name
*/

typedef struct {
	int status; //with chess_state
	int type; // with type_state
	int color; // with color_state
}board;

typedef struct {
	board grid[4][8];
	int current_player; // it will like round++, round % 2 = current_player
	int player_color[2]; // player_color[0] is for player1, fill COLOR_RED or COLOR_BLK
	int red_left; // initial 16, caculate still left on the board
	int black_left;
	int game_state; //the flag marked who wins, tie, or playing(ING)
}gameState;

enum player{
	P1, P2
};

enum color_state {
	COLOR_NONE, COLOR_RED, COLOR_BLK
};

enum type_state {
	TYPE_EMPTY = 0,
	TYPE_PAWN = 1,     // 卒 / 兵
    TYPE_CANNON = 2,   // 包 / 炮
    TYPE_HORSE = 3,    // 馬 / 傌
    TYPE_CHARIOT = 4,  // 車 / 車 
    TYPE_MINISTER = 5, // 象 / 相
    TYPE_GUARD = 6,    // 士 / 仕
    TYPE_KING = 7      // 將 / 帥 
};

enum game_state {
	STATE_ING, STATE_P1_WIN, STATE_P2_WIN, STATE_TIE 
};

enum chess_state {
	CHESS_EMPTY, CHESS_COVER, CHESS_OPEN
};

#endif
