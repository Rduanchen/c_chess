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

    bool quit = false;
    SDL_Event e;

    static int selR = -1;
    static int selC = -1;

    while (!quit) {
        if(game.current_player == P1 && game.game_state == STATE_ING){
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT) quit = true;

                // checking game state
                game.game_state = RULE_checkGameOver(&game);

                if (game.game_state != STATE_ING) {
                    printf("state code: %d\n", game.game_state);
                    if (game.game_state == STATE_P1_WIN) printf("--- GAME OVER: PLAYER 1 WINS! ---\n");
                    else if (game.game_state == STATE_P2_WIN) printf("--- GAME OVER: PLAYER 2 WINS! ---\n");
                    SDL_Delay(10000);
                    quit = 1;
                }

                // game running
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if(e.button.x >= OFFSET_X && e.button.y >= OFFSET_Y){
                        int col = (e.button.x - OFFSET_X) / GRID_SIZE;
                        int row = (e.button.y - OFFSET_Y) / GRID_SIZE;

                        printf("[Player] touch detected at: (%d, %d)\n", row, col);
                        if(row < 0 || row > 3 || col < 0 || col > 7){
                            printf("[System] player touch unaccessible\n");
                        }

                        // flipping
                        if(IO_executeFlip(&game, row, col)){
                            selR = -1, selC = -1;
                            // check if the first move, and get player's color
                            RULE_checkFirstMove(&game, row, col, P1);
                            
                            game.current_player = (game.current_player + 1)% 2;
                        }
                        // selecting, eating
                        else if(game.grid[row][col].status == CHESS_OPEN){
                            // check if player selected a legal position
                            if(selR == -1){
                                // check if player selected his oun color
                                if(game.grid[row][col].color == game.player_color[game.current_player]){
                                    selR = row;
                                    selC = col;
                                    printf("[Player] select at: (%d, %d)\n", selR, selC);
                                }else{
                                    printf("[Player] you can't select opposite color\n");
                                }
                            }
                            // selected legal position
                            // the second select is what to move or eat
                            else{
                                printf("[System] player 1 trying to move (%d, %d) to eat (%d, %d)\n", selR, selC, row, col);
                                if(RULE_isValidMove(&game, selR, selC, row, col)){
                                    IO_executeMove(&game, selR, selC, row, col);
                                    // next player
                                    game.current_player = (game.current_player + 1)% 2;
                                }else{
                                    printf("[System] execution error\n");
                                    selR = -1, selC = -1;
                                }

                                // set to origin
                                selR = -1, selC = -1;

                                
                            }
                        }
                        // moveing
                        else if(game.grid[row][col].status == CHESS_EMPTY && selR != -1){
                            printf("[System] player 1 trying to move (%d, %d) to (%d, %d)\n", selR, selC, row, col);
                            if(RULE_isValidMove(&game, selR, selC, row, col)){
                                IO_executeMove(&game, selR, selC, row, col);
                            }

                            // set to origin
                            selR = -1, selC = -1;

                            // next player
                            game.current_player = (game.current_player + 1)% 2;
                        }
                    }else{
                        selR = -1, selC = -1;
                        printf("[System] player touch unaccessible\n");
                    }
                }
            }
        }
        else if(game.current_player == P2 && game.game_state == STATE_ING){
            SDL_Delay(200);

            // get the position where AI want to flip
            ActionPos position = AI_randomFlip(&game);

            // write the data
            if(position.success == 1){
                IO_executeFlip(&game, position.row, position.col);
                printf("[AI] touch detected at: (%d, %d)\n", position.row, position.col);
                RULE_checkFirstMove(&game, position.row, position.col, P2);
            }else{
                printf("[AI] flip unsuccessfully\n");
            }

            game.current_player = (game.current_player + 1)% 2;
        }

        SDL_SetRenderDrawColor(renderer, 200, 160, 100, 255);
        SDL_RenderClear(renderer);
        
        UI_drawBoard(renderer, &game, chessTextures); // 使用 UI_ 前綴函式

        UI_drawSelection(renderer, selR, selC);

        SDL_RenderPresent(renderer);
    }

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