#include "../include/consensus.h"
#include "../include/feature.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 權重與分數定義
#define AI_VAL_KING     800
#define AI_VAL_GUARD    600
#define AI_VAL_MINISTER 400
#define AI_VAL_CHARIOT  300
#define AI_VAL_HORSE    200
#define AI_VAL_CANNON   550
#define AI_VAL_PAWN     150
#define AI_INF          999999

static const int AI_SCORES[] = {0, AI_VAL_PAWN, AI_VAL_CANNON, AI_VAL_HORSE, AI_VAL_CHARIOT, AI_VAL_MINISTER, AI_VAL_GUARD, AI_VAL_KING};

// 防止重複移動紀錄
static position AI_prev_from = {-1, -1};
static position AI_prev_to = {-1, -1};

// --- 內部輔助判定：取代原本衝突的 RULE_ 命名 ---

// 判定特定移動是否符合暗棋基本邏輯
int AI_checkLegal(gameState *game, int r, int c, int tr, int tc) {
    board att = game->grid[r][c];
    board vic = game->grid[tr][tc];
    
    if (tr < 0 || tr >= 4 || tc < 0 || tc >= 8) return 0;
    if (att.status != CHESS_OPEN) return 0;
    if (vic.status == CHESS_COVER) return 0; // 不能直接移動到蓋牌（那是翻牌指令）
    if (vic.status == CHESS_OPEN && att.color == vic.color) return 0;

    int dist = abs(r - tr) + abs(c - tc);

    // 炮跳吃邏輯
    if (att.type == TYPE_CANNON) {
        if (vic.status == CHESS_EMPTY) return (dist == 1);
        if (r != tr && c != tc) return 0;
        int obs = 0;
        if (r == tr) {
            int start = (c < tc) ? c : tc, end = (c < tc) ? tc : c;
            for (int i = start + 1; i < end; i++) if (game->grid[r][i].status != CHESS_EMPTY) obs++;
        } else {
            int start = (r < tr) ? r : tr, end = (r < tr) ? tr : r;
            for (int i = start + 1; i < end; i++) if (game->grid[i][c].status != CHESS_EMPTY) obs++;
        }
        return (obs == 1);
    }

    // 一般棋子移動與階級吃法
    if (dist != 1) return 0;
    if (vic.status == CHESS_EMPTY) return 1;
    if (att.type == TYPE_KING && vic.type == TYPE_PAWN) return 0;
    if (att.type == TYPE_PAWN && vic.type == TYPE_KING) return 1;
    return (att.type >= vic.type);
}

// --- 主決策函式 ---

ActionPos AI_getBestAction2(gameState *game) {
    ActionPos res;
    res.success = 0;
    srand((unsigned int)time(NULL));

    int myColor = game->player_color[game->current_player];
    int enemyColor = (myColor == COLOR_RED) ? COLOR_BLK : COLOR_RED;

    // 1. 顏色未定時的開局 (翻角落)
    if (myColor == COLOR_NONE) {
        position corners[] = {{0,0}, {3,7}, {0,7}, {3,0}};
        for(int i=0; i<4; i++) {
            if(game->grid[corners[i].row][corners[i].col].status == CHESS_COVER) {
                res.inst = 0; res.pos1 = corners[i]; res.success = 1;
                return res;
            }
        }
    }

    // 2. 評估所有合法移動
    int bestVal = -AI_INF;
    ActionPos moveProposal;
    moveProposal.success = 0;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN && game->grid[r][c].color == myColor) {
                for (int tr = 0; tr < 4; tr++) {
                    for (int tc = 0; tc < 8; tc++) {
                        if (AI_checkLegal(game, r, c, tr, tc)) {
                            
                            // 重複移動檢查：禁止回到上一手且目標為空格的情況
                            if (r == AI_prev_to.row && c == AI_prev_to.col && tr == AI_prev_from.row && tc == AI_prev_from.col) {
                                if (game->grid[tr][tc].status == CHESS_EMPTY) continue;
                            }

                            int currentScore = 0;
                            // 吃子得分
                            if (game->grid[tr][tc].status == CHESS_OPEN) {
                                currentScore = AI_SCORES[game->grid[tr][tc].type] * 10;
                            }

                            // 簡單防守評估：檢查移動後的新位置是否會被敵人相鄰吃掉
                            int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1};
                            for(int i=0; i<4; i++) {
                                int nr = tr + dr[i], nc = tc + dc[i];
                                if(nr >= 0 && nr < 4 && nc >= 0 && nc < 8) {
                                    board enemy = game->grid[nr][nc];
                                    if(enemy.status == CHESS_OPEN && enemy.color == enemyColor) {
                                        if (enemy.type >= game->grid[r][c].type || (enemy.type == TYPE_PAWN && game->grid[r][c].type == TYPE_KING)) {
                                            currentScore -= AI_SCORES[game->grid[r][c].type] * 6;
                                        }
                                    }
                                }
                            }

                            if (currentScore > bestVal) {
                                bestVal = currentScore;
                                moveProposal.inst = 1;
                                moveProposal.pos1 = (position){r, c};
                                moveProposal.pos2 = (position){tr, tc};
                                moveProposal.success = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // 3. 翻牌決策：如果沒有好的吃子步法 (得分 <= 0) 則執行翻牌
    if (bestVal <= 0) {
        position coverPool[32];
        int count = 0;
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 8; c++) {
                if (game->grid[r][c].status == CHESS_COVER) coverPool[count++] = (position){r, c};
            }
        }
        if (count > 0) {
            res.inst = 0;
            res.pos1 = coverPool[rand() % count];
            res.success = 1;
            // 翻牌後重置路徑紀錄
            AI_prev_from = (position){-1, -1}; AI_prev_to = (position){-1, -1};
            return res;
        }
    }

    // 更新移動紀錄
    if (moveProposal.success) {
        AI_prev_from = moveProposal.pos1;
        AI_prev_to = moveProposal.pos2;
    }

    return moveProposal;
}