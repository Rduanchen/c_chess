#ifndef FEATURE_H
#define FEATURE_H

#include "consensus.h" // 必須包含，因為我們要用到 gameState 型別
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define GRID_SIZE 80
#define OFFSET_X 80
#define OFFSET_Y 60

// 0 is for ramdom start, 1 is for human start, 2 is for AI start
#define START_MODE 0

typedef struct {
    int row;
    int col;
    int success; // 是否找到可翻的牌
} ActionPos;

// board.c
// __init
void BD_initGame(gameState *game);


// user_interface.c
// 新增：從assets資料夾匯入.png至texture陣列
// texture[0] is covered, [(color - 1) * 7 + type]
void UI_loadAssets(SDL_Renderer* renderer, SDL_Texture* textures[]);

// 更新：繪製棋盤
void UI_drawBoard(SDL_Renderer* renderer, gameState *game, SDL_Texture* textures[]);

// drow the selected position
void UI_drawSelection(SDL_Renderer* renderer, int selR, int selC);

// AI.c
// 電腦回合：自動翻牌
ActionPos AI_randomFlip(gameState *game);


// IO.c
// IO interface
int IO_executeFlip(gameState *game, int row, int col);

void IO_executeMove(gameState *game, int r1, int c1, int r2, int c2);

// rule.c
// only apply the first move
void RULE_checkFirstMove(gameState *game, int row, int col, int whoFlipped);

// ensure move or attack is valid
int RULE_isValidMove(gameState *game, int r1, int c1, int r2, int c2);

int RULE_checkGameOver(gameState *game);

#endif
