#include "consensus.h"
#include <stdio.h>
#include <stdlib.h>

void RULE_checkFirstMove(gameState *game, int row, int col, int whoFlipped) {
    // 如果玩家顏色尚未決定 (COLOR_NONE)
    if (game->player_color[0] == COLOR_NONE) {
        int flippedColor = game->grid[row][col].color;
        
        printf("[Rule] P%d flipped the first\n", (whoFlipped + 1));
        // flipper
        game->player_color[whoFlipped] = flippedColor;
        
        // opposite
        game->player_color[(whoFlipped + 1) % 2] = (flippedColor == COLOR_RED) ? COLOR_BLK : COLOR_RED;
        
        printf("[Rule] P%d is %s, P%d is %s\n", 
                (whoFlipped + 1),
                (flippedColor == COLOR_RED ? "RED" : "BLACK"),
                (whoFlipped + 2) % 2,
                (flippedColor == COLOR_RED ? "BLACK" : "RED"));
    }
}


/*
1. 距離與方向：除了「炮」之外，所有棋子只能移動到相鄰的上下左右一格（距離為 1）。
2. 目標限制：目標格不能是自己的棋子。
3. 等級制度 (Rank)：大吃小，但有特殊例外（兵吃將）。
4. 炮的跳台規則：炮必須隔一個棋子才能吃，且移動與吃牌的邏輯不同。
*/

// 輔助函式：判斷等級大小 (1 是最小, 7 是最大)
// 回傳 1 代表 attacker 可以吃 target，0 代表不行
int can_rank_capture(int attacker_type, int target_type) {
    // 特殊規則：兵(1) 可以吃 將(7)
    if (attacker_type == TYPE_PAWN && target_type == TYPE_KING) return 1;
    // 特殊規則：將(7) 不能吃 兵(1)
    if (attacker_type == TYPE_KING && target_type == TYPE_PAWN) return 0;
    
    // 一般規則：大吃小或同級互吃
    return attacker_type >= target_type;
}

int RULE_isValidMove(gameState *game, int r1, int c1, int r2, int c2) {
    board *src = &game->grid[r1][c1];
    board *dst = &game->grid[r2][c2];

    // 基礎檢查：不能原地踏步
    if (r1 == r2 && c1 == c2) return 0;

    // 計算曼哈頓距離
    int dist = abs(r1 - r2) + abs(c1 - c2);

    // --- 狀況 A：目標是空地 ---
    if (dst->status == CHESS_EMPTY) {
        // 除了炮以外，所有棋子移動距離必須為 1
        return (dist == 1);
    }

    // --- 狀況 B：目標是敵方棋子 (CHESS_OPEN) ---
    if (dst->status == CHESS_OPEN && dst->color != src->color) {
        
        // 1. 炮 (TYPE_CANNON) 的特殊跳台規則
        if (src->type == TYPE_CANNON) {
            // 必須在同一直線，且距離大於 1
            if (r1 != r2 && c1 != c2) return 0;
            
            // 計算路徑上有幾顆棋子 (不論翻開與否)
            int count = 0;
            if (r1 == r2) { // 水平掃描
                int start = (c1 < c2) ? c1 : c2;
                int end = (c1 < c2) ? c2 : c1;
                for (int i = start + 1; i < end; i++) {
                    if (game->grid[r1][i].status != CHESS_EMPTY) count++;
                }
            } else { // 垂直掃描
                int start = (r1 < r2) ? r1 : r2;
                int end = (r1 < r2) ? r2 : r1;
                for (int i = start + 1; i < end; i++) {
                    if (game->grid[i][c1].status != CHESS_EMPTY) count++;
                }
            }
            // 炮必須剛好隔一顆棋子才能吃
            return (count == 1);
        }

        // 2. 一般棋子吃牌
        if (dist == 1) {
            return can_rank_capture(src->type, dst->type);
        }
    }

    return 0; // 其他狀況(如目標是蓋著的、或是隊友)皆不准移動
}

int RULE_checkGameOver(gameState *game){
    // check if red left = 0
    if (game->red_left == 0) {
        // find who is red
        return (game->player_color[P1] == COLOR_RED) ? STATE_P2_WIN : STATE_P1_WIN;
    }

    // opposite
    if (game->black_left == 0) {
        return (game->player_color[P1] == COLOR_BLK) ? STATE_P2_WIN : STATE_P1_WIN;
    }

    // continue
    return STATE_ING;

    // we are going to write the tie rule(not processing)
}