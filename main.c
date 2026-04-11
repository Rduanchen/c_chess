#include "consensus.h"
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include "feature.h"

int main(int argc, char* argv[]) {
    gameState game;
    BD_initGame(&game); // 使用團隊規範函式

    // 1. 初始化 SDL2 與 SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // 初始化 PNG 支援
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("C_chess - Team Mode", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 450, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // import assets to textures
    // texture[0] is covered, [(color - 1) * 7 + type]
    SDL_Texture* chessTextures[15] = { NULL };
    UI_loadAssets(renderer, chessTextures);

    // ==[RAY 加入]== 讀取選單與暫停圖片
    UI_loadMenuAssets(renderer);

    bool quit = false;
    SDL_Event e;

    static int selR = -1;
    static int selC = -1;

    while (!quit) {
        // ==[RAY 加入]== 1. 選單畫面攔截
        if (UI_isInMenu()) {
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) quit = true;
                UI_handleMenuEvent(&e, &game);
            }
            SDL_SetRenderDrawColor(renderer, 200, 160, 100, 255);
            SDL_RenderClear(renderer);
            UI_drawStartMenu(renderer);
            SDL_RenderPresent(renderer);
            continue; // 卡在選單，不執行後面的遊戲邏輯
        }

        // ==[RAY 加入]== 2. 暫停畫面攔截
        if (UI_isPaused()) {
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) quit = true;
            }
            SDL_SetRenderDrawColor(renderer, 200, 160, 100, 255);
            SDL_RenderClear(renderer);
            UI_drawBoard(renderer, &game, chessTextures);
            UI_drawSelection(renderer, selR, selC);
            UI_drawPauseScreen(renderer); // 疊加畫上暫停圖示
            SDL_RenderPresent(renderer);
            continue; // 卡在暫停，不執行後面的遊戲邏輯
        }

        bool turnEnded = false; // 用來標記本回合動作是否完成

        // --- 1. 事件處理 (Poll Event) ---
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;

            // 人類玩家回合邏輯
            if (game.current_player == P1 && game.game_state == STATE_ING && e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.x >= OFFSET_X && e.button.y >= OFFSET_Y) {
                    int col = (e.button.x - OFFSET_X) / GRID_SIZE;
                    int row = (e.button.y - OFFSET_Y) / GRID_SIZE;

                    if (row >= 0 && row <= 3 && col >= 0 && col <= 7) {
                        // A. 翻牌動作
                        if (IO_executeFlip(&game, row, col)) {
                            RULE_checkFirstMove(&game, row, col, P1);
                            selR = -1; selC = -1;

                            // ==[RAY 加入]== 紀錄步數
                            UI_recordMove(P1);

                            turnEnded = true;
                        }
                        // B. 選取或移動/吃牌動作
                        else {
                            if (selR == -1) { // 尚未選取，嘗試選取
                                if (game.grid[row][col].status == CHESS_OPEN &&
                                    game.grid[row][col].color == game.player_color[P1]) {
                                    selR = row; selC = col;
                                    printf("[Player] selected: (%d, %d)\n", selR, selC);
                                }
                            } else { // 已選取，嘗試移動或吃牌
                                if (RULE_isValidMove(&game, selR, selC, row, col)) {
                                    IO_executeMove(&game, selR, selC, row, col);

                                    // ==[RAY 加入]== 紀錄步數
                                    UI_recordMove(P1);

                                    turnEnded = true;
                                }
                                selR = -1; selC = -1; // 只要進行第二次點擊，不論成功與否都重置
                            }
                        }
                    }
                } else { // 點擊棋盤外
                    selR = -1; selC = -1;
                }
            }
        }

        // --- 2. AI 回合處理 ---
        if (game.current_player == P2 && game.game_state == STATE_ING) {
            SDL_Delay(500); // 稍微停頓增加真實感
            ActionPos position = AI_randomFlip(&game);

            if (position.inst == 0 && position.success) {
                IO_executeFlip(&game, position.pos1.row, position.pos1.col);
                RULE_checkFirstMove(&game, position.pos1.row, position.pos1.col, P2);
                printf("[AI] flipped: (%d, %d)\n", position.pos1.row, position.pos1.col);

                // ==[RAY 加入]== 紀錄步數
                UI_recordMove(P2);

                turnEnded = true;
            }
            else if(position.inst == 1 && position.success){
                IO_executeMove(&game, position.pos1.row, position.pos1.col, position.pos2.row, position.pos2.col);
                printf("[AI] moved: (%d, %d) to (%d, %d)\n", position.pos1.row, position.pos1.col, position.pos2.row, position.pos2.col);

                // ==[RAY 加入]== 紀錄步數
                UI_recordMove(P2);

                turnEnded = true;
            }
            else {
                printf("[AI] error\n");
                turnEnded = true;
            }
        }

        // --- 3. 狀態更新 (裁判判定) ---
        if (turnEnded) {
            game.game_state = RULE_checkGameOver(&game);
            if (game.game_state != STATE_ING) {
                printf("--- GAME OVER! Result Code: %d ---\n", game.game_state);
            }
            game.current_player = (game.current_player + 1) % 2; // 正確切換回合
        }

        // --- 4. 渲染 (Rendering) ---
        SDL_SetRenderDrawColor(renderer, 200, 160, 100, 255);
        SDL_RenderClear(renderer);

        UI_drawBoard(renderer, &game, chessTextures);
        UI_drawSelection(renderer, selR, selC); // 畫框

        SDL_RenderPresent(renderer);
    }

    // ==[RAY]== 清理擴充圖片記憶體
    UI_cleanupMenuAssets();

    // release MEM.
    for (int i = 0; i < 15; i++) {
        if (chessTextures[i] != NULL) {
            SDL_DestroyTexture(chessTextures[i]);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
