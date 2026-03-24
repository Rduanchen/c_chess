#include "consensus.h"
#include "feature.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

void BD_initGame(gameState *game){

    srand(time(NULL));

    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 8; j++){
            game->grid[i][j].status = CHESS_COVER;
            game->grid[i][j].type = TYPE_EMPTY;
            game->grid[i][j].color = COLOR_NONE;
        }
    }

    //using struct board
    board pieces[32];

    int index = 0;
    int cnt[] = {0, 5, 2, 2, 2, 2, 2, 1};
    //cnt is from (PAWN) to (KING)
    //based on type_state

    for(int i = COLOR_RED; i <= COLOR_BLK; i++){
        for(int j = TYPE_PAWN; j <= TYPE_KING; j++){
            for(int n = 0; n < cnt[j]; n++){
                pieces[index].status = CHESS_COVER;
                pieces[index].type = j;
                pieces[index].color = i;
                index++;
            }
        }
    }

    //Fisher_Yates Algo.
    for(int i = 31; i >= 0; i--){
        int j = rand() % (i + 1);
        board temp = pieces[i];
        pieces[i] = pieces[j];
        pieces[j] = temp;
    }

    //fill in board
    index = 0;
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 8; j++){
            game->grid[i][j] = pieces[index];
            index++;
        }
    }

    if(START_MODE == 1){
        game->current_player = P1;
    }else if(START_MODE == 2){
        game->current_player = P2;
    }else{
        game->current_player = rand() % 2;
    }
    game->player_color[0] = COLOR_NONE;
    game->player_color[1] = COLOR_NONE;

    game->red_left = 16;
    game->black_left = 16;
    game->game_state = STATE_ING;


}