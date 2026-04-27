#ifndef FEATURE_H
#define FEATURE_H

#include "consensus.h" // 必須包含，因為我們要用到 gameState 型別
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define GRID_SIZE 80
#define OFFSET_X 80
#define OFFSET_Y 60

// 0 is for ramdom start, 1 is for player1 start, 2 is for player2 start
#define START_MODE 0

#include <stdbool.h>

// ====== 遊戲模式 ======
enum game_mode {
    GAME_MODE_LOCAL,    // 本機對戰 (人 vs AI)
    GAME_MODE_ONLINE    // 連線對戰
};

typedef struct {
    int row;
    int col;
} position;

typedef struct {
    int inst; // 0 for flipping, 1 for moving(eating)
    position pos1; // {row, col}
    position pos2; // {row, col}
    int success; // 是否找到可翻的牌
} ActionPos;

// board.c
// __init
void BD_initGame(gameState *game);

// 線上模式棋盤初始化 (不隨機放棋子，等伺服器同步)
void BD_initOnlineGame(gameState *game);


// user_interface.c
// 新增：從assets資料夾匯入.png至texture陣列
// texture[0] is covered, [(color - 1) * 7 + type]
void UI_loadAssets(SDL_Renderer* renderer, SDL_Texture* textures[]);

// 更新：繪製棋盤
void UI_drawBoard(SDL_Renderer* renderer, gameState *game, SDL_Texture* textures[]);

// drow the selected position
void UI_drawSelection(SDL_Renderer* renderer, int selR, int selC);

// AI.c
// 電腦回合：自動翻牌/移動
ActionPos AI_getBestAction(gameState *game);

// AI2.c
ActionPos AI_getBestAction2(gameState *game);

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

// ==[RAY 擴充功能]==
void UI_loadMenuAssets(SDL_Renderer* renderer);
void UI_cleanupMenuAssets();
bool UI_isInMenu();
bool UI_isPaused();
void UI_handleMenuEvent(SDL_Event* event, gameState* game);
void UI_recordMove(int current_player);
void UI_drawStartMenu(SDL_Renderer* renderer);
void UI_drawPauseScreen(SDL_Renderer* renderer);

// ==[線上對戰 UI]==
int UI_getGameMode();           // 回傳當前遊戲模式
bool UI_isOnlineConnecting();   // 是否正在連線設定中
void UI_drawOnlineStatus(SDL_Renderer* renderer, const char* status_text);

#endif
