#include "consensus.h"
#include "feature.h"
#include <stdio.h>

int IO_executeFlip(gameState *game, int row, int col) {
    if (row >= 0 && row < 4 && col >= 0 && col < 8) {
        if (game->grid[row][col].status == CHESS_COVER) {
            game->grid[row][col].status = CHESS_OPEN;
            printf("[Action] flip at: (%d, %d)\n", row, col);
            return 1;
            // flip successfully
        }
    }
    return 0;
    // flip unsuccessfully
}

void IO_executeMove(gameState *game, int r1, int c1, int r2, int c2){
    board *src = &game->grid[r1][c1];
    board *dst = &game->grid[r2][c2];

    // eat exection, update the left counter
    if (dst->status == CHESS_OPEN) {
        if (dst->color == COLOR_RED) {
            game->red_left--;
            printf("[Action] Red chess captured! Left: %d\n", game->red_left);
        } else if (dst->color == COLOR_BLK) {
            game->black_left--;
            printf("[Action] Black chess captured! Left: %d\n", game->black_left);
        }
    }

    // moving
    dst->type = src->type;
    dst->color = src->color;
    dst->status = CHESS_OPEN;

    // clear the source position
    src->type = TYPE_EMPTY;
    src->color = COLOR_NONE;
    src->status = CHESS_EMPTY;

    printf("[Action] Move: (%d, %d) -> (%d, %d)\n", r1, c1, r2, c2);
}