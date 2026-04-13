#include "../include/consensus.h"
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include "../include/feature.h"

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

        bool turnEnded = false; // 用來標記本回合動作是否完成

        // --- 1. 事件處理 (Poll Event) ---
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) quit = true;
        }

        // --- player1: Gemini ---
        if (game.current_player == P1 && game.game_state == STATE_ING) {
            SDL_Delay(500); // 稍微停頓增加真實感
            ActionPos position2 = AI_getBestAction2(&game);

            if (position2.inst == 0 && position2.success) {
                IO_executeFlip(&game, position2.pos1.row, position2.pos1.col);
                RULE_checkFirstMove(&game, position2.pos1.row, position2.pos1.col, P1);
                printf("[gemini] flipped: (%d, %d)\n", position2.pos1.row, position2.pos1.col);

                // ==[RAY 加入]== 紀錄步數
                UI_recordMove(P1);

                turnEnded = true;
            }
            else if(position2.inst == 1 && position2.success){
                if(!RULE_isValidMove(&game, position2.pos1.row, position2.pos1.col, position2.pos2.row, position2.pos2.col)){
                    printf("[System] Gemini's movement is not valid\n");
                }
                else{
                    IO_executeMove(&game, position2.pos1.row, position2.pos1.col, position2.pos2.row, position2.pos2.col);
                    printf("[gemini] moved: (%d, %d) to (%d, %d)\n", position2.pos1.row, position2.pos1.col, position2.pos2.row, position2.pos2.col);

                    // ==[RAY 加入]== 紀錄步數
                    UI_recordMove(P1);

                    turnEnded = true;
                }
            }
            else {
                printf("[gemini] error\n");
                turnEnded = true;
            }
        }


        // --- 2. AI 回合處理 ---
        if (game.current_player == P2 && game.game_state == STATE_ING) {
            SDL_Delay(500); // 稍微停頓增加真實感
            ActionPos position = AI_getBestAction(&game);

            if (position.inst == 0 && position.success) {
                IO_executeFlip(&game, position.pos1.row, position.pos1.col);
                RULE_checkFirstMove(&game, position.pos1.row, position.pos1.col, P2);
                printf("[AI] flipped: (%d, %d)\n", position.pos1.row, position.pos1.col);

                // ==[RAY 加入]== 紀錄步數
                UI_recordMove(P2);

                turnEnded = true;
            }
            else if(position.inst == 1 && position.success){
                if(!RULE_isValidMove(&game, position.pos1.row, position.pos1.col, position.pos2.row, position.pos2.col)){
                    printf("[System] Origin AI's movement is not valid\n");
                }
                else{
                    IO_executeMove(&game, position.pos1.row, position.pos1.col, position.pos2.row, position.pos2.col);
                    printf("[AI] moved: (%d, %d) to (%d, %d)\n", position.pos1.row, position.pos1.col, position.pos2.row, position.pos2.col);

                    // ==[RAY 加入]== 紀錄步數
                    UI_recordMove(P2);

                    turnEnded = true;
                }
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
