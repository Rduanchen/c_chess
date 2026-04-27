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

static const int MAX_STEPS = 40;

// ==[線上對戰狀態]==
static int selected_game_mode = -1;       // -1 = 尚未選擇, 0 = LOCAL, 1 = ONLINE
static bool online_connecting = false;    // 正在進行連線設定中
static int selected_ai_version = 1;       // 1 = AI1, 2 = AI2

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
        int x = event->button.x;
        int win_w = 800;
        int third = win_w / 3; // ~266

        if (x < third) {
            // 左邊 1/3：先手 (本機對戰)
            selected_game_mode = GAME_MODE_LOCAL;
            game->current_player = P1;
            in_start_menu = false;
            printf("[UI] Selected: Local game, Player first\n");
        } else if (x < third * 2) {
            // 中間 1/3：連線對戰
            selected_game_mode = GAME_MODE_ONLINE;
            online_connecting = true;
            in_start_menu = false;
            if (event->button.y < 225) {
                selected_ai_version = 1;
                printf("[UI] Selected: Online battle mode (Original AI 1)\n");
            } else {
                selected_ai_version = 2;
                printf("[UI] Selected: Online battle mode (Gemini AI 2)\n");
            }
        } else {
            // 右邊 1/3：後手 (本機對戰)
            selected_game_mode = GAME_MODE_LOCAL;
            game->current_player = P2;
            in_start_menu = false;
            printf("[UI] Selected: Local game, AI first\n");
        }
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

// 輔助函式：畫粗線
static void UI_drawThickLine(SDL_Renderer* r, int x1, int y1, int x2, int y2) {
    for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
            SDL_RenderDrawLine(r, x1+i, y1+j, x2+i, y2+j);
        }
    }
}

// 輔助函式：畫 "AI 1" 或 "AI 2" 的文字幾何圖形
static void UI_drawAIText(SDL_Renderer* r, int version, int x, int y) {
    int s = 2; // 放大倍率
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255); // 白色字

    // 畫 'A' (20x30)
    UI_drawThickLine(r, x + 0*s, y + 30*s, x + 10*s, y + 0*s);
    UI_drawThickLine(r, x + 10*s, y + 0*s,  x + 20*s, y + 30*s);
    UI_drawThickLine(r, x + 5*s,  y + 15*s, x + 15*s, y + 15*s);
    
    // 畫 'I' (20x30), 位移 35*s
    x += 35*s;
    UI_drawThickLine(r, x + 0*s, y + 0*s, x + 20*s, y + 0*s);
    UI_drawThickLine(r, x + 10*s,y + 0*s, x + 10*s, y + 30*s);
    UI_drawThickLine(r, x + 0*s, y + 30*s,x + 20*s, y + 30*s);

    // 畫數字, 位移 35*s
    x += 35*s;
    if (version == 1) {
        // '1'
        UI_drawThickLine(r, x + 0*s, y + 10*s, x + 10*s, y + 0*s);
        UI_drawThickLine(r, x + 10*s,y + 0*s,  x + 10*s, y + 30*s);
        UI_drawThickLine(r, x + 0*s, y + 30*s, x + 20*s, y + 30*s);
    } else {
        // '2'
        UI_drawThickLine(r, x + 0*s, y + 0*s,  x + 20*s, y + 0*s);
        UI_drawThickLine(r, x + 20*s,y + 0*s,  x + 20*s, y + 15*s);
        UI_drawThickLine(r, x + 20*s,y + 15*s, x + 0*s,  y + 15*s);
        UI_drawThickLine(r, x + 0*s, y + 15*s, x + 0*s,  y + 30*s);
        UI_drawThickLine(r, x + 0*s, y + 30*s, x + 20*s, y + 30*s);
    }
}

void UI_drawStartMenu(SDL_Renderer* renderer)
{
    int third = 800 / 3;

    if (menuTexture) {
        SDL_Rect dest = { 0, 0, 800, 450 };
        SDL_RenderCopy(renderer, menuTexture, NULL, &dest);
        
        // 疊加 AI 選擇提示框 (中間區域)
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // 上半部 AI 1
        SDL_Rect ai1_box = { third + 20, 50, third - 40, 150 };
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 150); // 半透明紅
        SDL_RenderFillRect(renderer, &ai1_box);
        
        // 下半部 AI 2
        SDL_Rect ai2_box = { third + 20, 250, third - 40, 150 };
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 150); // 半透明藍
        SDL_RenderFillRect(renderer, &ai2_box);
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        
        // 加上文字圖形
        UI_drawAIText(renderer, 1, third + 50, 100);
        UI_drawAIText(renderer, 2, third + 50, 290);
    } else {
        // 三分區 fallback：左=先手、中=連線對戰、右=後手
        SDL_Rect left_area = { 0, 0, third, 450 };
        SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
        SDL_RenderFillRect(renderer, &left_area);

        SDL_Rect mid_top = { third, 0, third, 225 };
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255); // AI 1
        SDL_RenderFillRect(renderer, &mid_top);

        SDL_Rect mid_bot = { third, 225, third, 225 };
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255); // AI 2
        SDL_RenderFillRect(renderer, &mid_bot);
        
        // 加上文字圖形
        UI_drawAIText(renderer, 1, third + 50, 80);
        UI_drawAIText(renderer, 2, third + 50, 305);

        SDL_Rect right_area = { third * 2, 0, 800 - third * 2, 450 };
        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderFillRect(renderer, &right_area);

        // 分隔線
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawLine(renderer, third, 0, third, 450);
        SDL_RenderDrawLine(renderer, third * 2, 0, third * 2, 450);
        SDL_RenderDrawLine(renderer, third, 225, third * 2, 225);
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

// ==[線上對戰 UI 函式]==

int UI_getGameMode() {
    return selected_game_mode;
}

int UI_getSelectedAIVersion() {
    return selected_ai_version;
}

bool UI_isOnlineConnecting() {
    return online_connecting;
}

void UI_drawOnlineStatus(SDL_Renderer* renderer, const char* status_text) {
    // 在畫面上方顯示連線狀態的半透明橫條
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // 背景條
    SDL_Rect bar = { 0, 0, 800, 30 };
    SDL_SetRenderDrawColor(renderer, 30, 30, 80, 200);
    SDL_RenderFillRect(renderer, &bar);

    // 綠色指示燈
    SDL_Rect indicator = { 10, 8, 14, 14 };
    SDL_SetRenderDrawColor(renderer, 0, 220, 80, 255);
    SDL_RenderFillRect(renderer, &indicator);

    // 恢復混合模式
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // 注意：不使用 SDL_ttf 的情況下，我們只在 console 輸出狀態
    // 若需要在畫面上顯示文字，需要引入 SDL_ttf
    // 目前僅在 console 顯示
    static const char* last_status = NULL;
    if (last_status != status_text) {
        printf("[Online] %s\n", status_text);
        last_status = status_text;
    }
}