#include "consensus.h"
#include "feature.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

ActionPos AI_randomFlip(gameState *game) {
    ActionPos decision = {-1, -1, 0};
    int coveredCoords[32][2];
    int count = 0;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_COVER) {
                coveredCoords[count][0] = r;
                coveredCoords[count][1] = c;
                count++;
            }
        }
    }

    if (count > 0) {
        int idx = rand() % count;
        decision.row = coveredCoords[idx][0];
        decision.col = coveredCoords[idx][1];
        decision.success = 1;
    }
    return decision;
}