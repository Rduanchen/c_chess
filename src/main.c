#include "../include/consensus.h"
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include "../include/feature.h"
#include "../include/server.h"
#include <windows.h>

int main(int argc, char* argv[]) {
    // 設定 Console 編碼為 UTF-8，避免中文亂碼
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    gameState game;
    BD_initGame(&game); // 使用團隊規範函式

    // 線上對戰狀態
    OnlineState online;
    memset(&online, 0, sizeof(OnlineState));
    online.socket = INVALID_SOCKET;

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

    // 線上模式的輸入狀態
    char room_input[32] = "";
    int room_input_len = 0;
    bool waiting_for_room = false;
    bool online_ready = false;

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

        // ==[線上連線設定]==
        if (UI_getGameMode() == GAME_MODE_ONLINE && UI_isOnlineConnecting()) {
            // 第一次進入連線模式：開啟 console 輸入
            if (!waiting_for_room && !online_ready) {
                // 重新初始化棋盤為線上模式
                BD_initOnlineGame(&game);

                printf("\n");
                printf("========================================\n");
                printf("        連 線 對 戰 模 式\n");
                printf("========================================\n");
                printf("正在連線到伺服器 %s:%d ...\n", SERVER_IP, SERVER_PORT);

                if (SVR_initConnection(&online) != 0) {
                    printf("[錯誤] 無法連線到伺服器！\n");
                    printf("按任意鍵返回...\n");
                    // 回到本機模式
                    BD_initGame(&game);
                    waiting_for_room = false;
                    online_ready = false;
                    // 不設回選單，讓它繼續當作本機模式
                    continue;
                }

                printf("連線成功！\n\n");
                printf("請輸入房間號碼: ");
                fflush(stdout);
                waiting_for_room = true;
            }

            // 等待房間號碼輸入 (非阻塞方式處理 SDL 事件)
            if (waiting_for_room) {
                // 使用 console 輸入取代 SDL 事件
                // 因為需要打字輸入房號，先用阻塞式 stdin
                char room_buf[32];
                if (fgets(room_buf, sizeof(room_buf), stdin)) {
                    // 去除換行符
                    room_buf[strcspn(room_buf, "\r\n")] = '\0';

                    if (strlen(room_buf) > 0) {
                        printf("正在加入房間 %s ...\n", room_buf);
                        if (SVR_joinRoom(&online, room_buf) == 0) {
                            printf("\n");
                            printf("========================================\n");
                            printf(" 成功加入房間: %s\n", room_buf);
                            printf(" 你的角色: %s (%s)\n", online.assigned_role, online.my_role_ab);
                            printf("========================================\n");
                            printf("等待比賽開始...\n\n");
                            waiting_for_room = false;
                            online_ready = true;
                        } else {
                            printf("加入失敗，請重新輸入房間號碼: ");
                            fflush(stdout);
                        }
                    }
                }

                // 處理 SDL 事件防止無回應
                while (SDL_PollEvent(&e) != 0) {
                    if (e.type == SDL_QUIT) {
                        quit = true;
                        break;
                    }
                }

                // 渲染等待畫面
                SDL_SetRenderDrawColor(renderer, 200, 160, 100, 255);
                SDL_RenderClear(renderer);
                UI_drawBoard(renderer, &game, chessTextures);
                UI_drawOnlineStatus(renderer, "等待輸入房間號碼...");
                SDL_RenderPresent(renderer);
                continue;
            }

            // 已連線，跳出連線設定
            if (online_ready) {
                waiting_for_room = false;
                // 不再標記為 connecting
            }
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

        // ================================================================
        // 線上對戰模式的遊戲迴圈 (使用本地 AI 自動對戰)
        // ================================================================
        if (UI_getGameMode() == GAME_MODE_ONLINE && online_ready) {
            // 處理 SDL 事件 (僅處理關閉視窗)
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) {
                    quit = true;
                    break;
                }
            }
            if (quit) break;

            // 非阻塞接收伺服器更新
            char update_buf[8192];
            if (SVR_receiveUpdate(&online, update_buf, sizeof(update_buf))) {
                printf("[Server Update] %s\n", update_buf);

                if (strstr(update_buf, "UPDATE")) {
                    // 同步棋盤狀態到本地 gameState
                    SVR_syncBoardFromJSON(update_buf, &game, &online);

                    // 檢查遊戲是否結束
                    char state_str[32];
                    SVR_getGameState(update_buf, state_str);

                    if (strcmp(state_str, "ended") == 0) {
                        printf("\n========================================\n");
                        printf("           遊戲結束！\n");
                        printf("========================================\n");
                        game.game_state = STATE_TIE;
                    }

                    // ====== 輪到我方時，使用 AI 自動下棋 ======
                    if (online.is_my_turn && game.game_state == STATE_ING) {
                        // 設定 AI 使用 P1 的身份
                        // player_color[P1] 已由 SVR_syncBoardFromJSON 設為我方顏色
                        game.current_player = P1;

                        printf("[AI-Online] 輪到我方 (角色 %s, 顏色 %s)，AI 思考中...\n",
                               online.my_role_ab, online.my_color);

                        // 呼叫選定的本地 AI 引擎
                        ActionPos bestAction;
                        if (UI_getSelectedAIVersion() == 1) {
                            bestAction = AI_getBestAction(&game);
                        } else {
                            bestAction = AI_getBestAction2(&game);
                        }

                        if (bestAction.inst == 0 && bestAction.success) {
                            // 翻牌動作
                            printf("[AI-Online] 決定翻牌: (%d, %d)\n",
                                   bestAction.pos1.row, bestAction.pos1.col);

                            // 等待 2 秒後發送 (符合伺服器要求)
                            SDL_Delay(2000);
                            SVR_sendFlip(&online, bestAction.pos1.row, bestAction.pos1.col);

                        } else if (bestAction.inst == 1 && bestAction.success) {
                            // 移動/吃子動作
                            printf("[AI-Online] 決定移動: (%d, %d) -> (%d, %d)\n",
                                   bestAction.pos1.row, bestAction.pos1.col,
                                   bestAction.pos2.row, bestAction.pos2.col);

                            // 等待 2 秒後發送
                            SDL_Delay(2000);
                            SVR_sendMove(&online, bestAction.pos1.row, bestAction.pos1.col,
                                         bestAction.pos2.row, bestAction.pos2.col);

                        } else {
                            printf("[AI-Online] AI 無法找到合法動作！\n");
                        }
                    }
                }
            }

            // 渲染
            SDL_SetRenderDrawColor(renderer, 200, 160, 100, 255);
            SDL_RenderClear(renderer);

            UI_drawBoard(renderer, &game, chessTextures);

            // 顯示連線狀態
            if (online.is_my_turn) {
                UI_drawOnlineStatus(renderer, "AI 思考中...");
            } else {
                UI_drawOnlineStatus(renderer, "等待對手...");
            }

            SDL_RenderPresent(renderer);

            // 小延遲防止 CPU 過高
            SDL_Delay(50);
            continue;
        }

        // ================================================================
        // 本機對戰模式的遊戲迴圈 (原始邏輯)
        // ================================================================

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

    // 線上模式清理
    if (online.connected) {
        SVR_closeConnection(&online);
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
