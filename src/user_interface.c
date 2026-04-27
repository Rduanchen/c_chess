#include "consensus.h"
#include "feature.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
// ==[RAY 狀態變數]==
static SDL_Texture* menuTexture = NULL;
static SDL_Texture* pauseTexture = NULL;
static bool in_start_menu = true;
static bool is_paused = false;
static int p1_steps = 0;
static int p2_steps = 0;
// 最大步數

static const int MAX_STEPS = 50;

// 新增：從 assets 匯入圖檔
void UI_loadAssets(SDL_Renderer* renderer, SDL_Texture* textures[])
{
    // 0 是蓋牌
    textures[0] = IMG_LoadTexture(renderer, "assets/covered.png");
    if (!textures[0])
        printf("Failed to load covered.png: %s\n", IMG_GetError());

    char path[50];
    // 使用你的邏輯: (color - 1) * 7 + type
    // COLOR_RED 為 1, COLOR_BLK 為 2
    for (int c = COLOR_RED; c <= COLOR_BLK; c++) {
        for (int t = TYPE_PAWN; t <= TYPE_KING; t++) {
            int idx = (c - 1) * 7 + t;
            if (c == COLOR_RED) {
                sprintf(path, "assets/red_%d.png", t);
            } else {
                sprintf(path, "assets/blk_%d.png", t);
            }
            textures[idx] = IMG_LoadTexture(renderer, path);
            if (!textures[idx])
                printf("Failed to load %s\n", path);
        }
    }
}

// 實作 UI_ 前綴：繪製棋盤與棋子
void UI_drawBoard(SDL_Renderer* renderer, gameState* game, SDL_Texture* textures[])
{
    // 1. 先畫格線 (底層)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 黑色線條
    for (int i = 0; i <= 8; i++) {
        SDL_RenderDrawLine(renderer, OFFSET_X + i * GRID_SIZE, OFFSET_Y, OFFSET_X + i * GRID_SIZE, OFFSET_Y + 4 * GRID_SIZE);
    }
    for (int j = 0; j <= 4; j++) {
        SDL_RenderDrawLine(renderer, OFFSET_X, OFFSET_Y + j * GRID_SIZE, OFFSET_X + 8 * GRID_SIZE, OFFSET_Y + j * GRID_SIZE);
    }

    // 2. 再畫棋子 (上層)
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            // 讓圖片縮小一點點 (例如上下左右各縮 5 像素)，這樣才不會壓到線
            SDL_Rect rect = {
                OFFSET_X + c * GRID_SIZE + 5,
                OFFSET_Y + r * GRID_SIZE + 5,
                GRID_SIZE - 10,
                GRID_SIZE - 10
            };

            if (game->grid[r][c].status == CHESS_COVER) {
                SDL_RenderCopy(renderer, textures[0], NULL, &rect);
            } else if (game->grid[r][c].status == CHESS_OPEN) {
                // texture[0] is covered, [(color - 1) * 7 + type]
                int texIdx = (game->grid[r][c].color - 1) * 7 + game->grid[r][c].type;

                // 防呆檢查避免索引溢位
                if (texIdx >= 1 && texIdx <= 14 && textures[texIdx] != NULL) {
                    SDL_RenderCopy(renderer, textures[texIdx], NULL, &rect);
                } else {
                    // 如果圖檔載入失敗，畫個顏色當替代方案
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }
    }
}

void UI_drawSelection(SDL_Renderer* renderer, int selR, int selC)
{
    // 如果 selR 為 -1，代表沒有選取任何棋子，直接回傳
    if (selR == -1)
        return;

    // 計算選取框的矩形範圍
    // (與 UI_drawBoard 的棋子 rect 位置一致)
    SDL_Rect rect = {
        OFFSET_X + selC * GRID_SIZE + 5,
        OFFSET_Y + selR * GRID_SIZE + 5,
        GRID_SIZE - 10,
        GRID_SIZE - 10
    };

    // 1. 先畫一個帶有透明度的半透明黃色填充矩形
    // 設定顏色：黃色 (255, 255, 0)，透明度 (100)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); // 啟用混合模式
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100); // 黃色填充
    SDL_RenderFillRect(renderer, &rect);

    // 2. 再畫一個純黃色的外框
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // 純黃色外框
    SDL_RenderDrawRect(renderer, &rect);

    // 3. (選擇性) 如果想要框粗一點，可以再畫一個更內縮的框
    SDL_Rect innerRect = { rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4 };
    SDL_RenderDrawRect(renderer, &innerRect);

    // 恢復混合模式為預設，避免影響之後的繪製
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}
// ==[RAY]==
void UI_loadMenuAssets(SDL_Renderer* renderer)
{
    menuTexture = IMG_LoadTexture(renderer, "assets/start_menu.png");
    pauseTexture = IMG_LoadTexture(renderer, "assets/pause_screen.png");
}

void UI_cleanupMenuAssets()
{
    if (menuTexture)
        SDL_DestroyTexture(menuTexture);
    if (pauseTexture)
        SDL_DestroyTexture(pauseTexture);
}

bool UI_isInMenu() { return in_start_menu; }
bool UI_isPaused() { return is_paused; }

void UI_handleMenuEvent(SDL_Event* event, gameState* game)
{
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        if (event->button.x < 400) {
            game->current_player = P1;
        } else {
            game->current_player = P2;
        }
        in_start_menu = false;
    }
}

void UI_recordMove(int current_player)
{
    if (current_player == P1)
        p1_steps++;
    else if (current_player == P2)
        p2_steps++;

    printf("TotalStep -> P1: %d steps | P2: %d steps\n", p1_steps, p2_steps);

    // 滿 MAX_STEPS 步觸發暫停
    if (p1_steps >= MAX_STEPS && p2_steps >= MAX_STEPS) {
        is_paused = true;
    }
}

void UI_drawStartMenu(SDL_Renderer* renderer)
{
    if (menuTexture) {
        SDL_Rect dest = { 0, 0, 800, 450 };
        SDL_RenderCopy(renderer, menuTexture, NULL, &dest);
    } else {
        SDL_Rect left_half = { 0, 0, 400, 450 };
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        SDL_RenderFillRect(renderer, &left_half);
        SDL_Rect right_half = { 400, 0, 400, 450 };
        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderFillRect(renderer, &right_half);
    }
}

void UI_drawPauseScreen(SDL_Renderer* renderer)
{
    if (pauseTexture) {
        SDL_Rect dest = { 200, 100, 400, 250 };
        SDL_RenderCopy(renderer, pauseTexture, NULL, &dest);
    } else {
        SDL_Rect pause_box = { 250, 175, 300, 100 };
        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
        SDL_RenderFillRect(renderer, &pause_box);
    }
}
