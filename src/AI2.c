#include "../include/consensus.h"
#include "../include/feature.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define AI_VAL_KING     800
#define AI_VAL_GUARD    600
#define AI_VAL_MINISTER 400
#define AI_VAL_CHARIOT  300
#define AI_VAL_HORSE    200
#define AI_VAL_CANNON   550 
#define AI_VAL_PAWN     150
#define AI_INF          999999

static position AI_h_pos[4] = {{-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}};
static int AI_h_idx = 0;
static int AI_steps = 0;

// --- 規則檢查 ---
int AI_checkLegal(gameState *game, int r, int c, int tr, int tc) {
    if (tr < 0 || tr >= 4 || tc < 0 || tc >= 8) return 0;
    board att = game->grid[r][c];
    board vic = game->grid[tr][tc];
    if (att.status != CHESS_OPEN) return 0;
    if (vic.status == CHESS_COVER || (vic.status == CHESS_OPEN && att.color == vic.color)) return 0;

    int dist = abs(r - tr) + abs(c - tc);
    if (att.type == TYPE_CANNON) {
        if (vic.status == CHESS_EMPTY) return (dist == 1);
        if (r != tr && c != tc) return 0;
        int obs = 0;
        if (r == tr) {
            int s = (c < tc) ? c : tc, e = (c < tc) ? tc : c;
            for (int i = s + 1; i < e; i++) if (game->grid[r][i].status != CHESS_EMPTY) obs++;
        } else {
            int s = (r < tr) ? r : tr, e = (r < tr) ? tr : r;
            for (int i = s + 1; i < e; i++) if (game->grid[i][c].status != CHESS_EMPTY) obs++;
        }
        return (obs == 1);
    }
    if (dist != 1) return 0;
    if (vic.status == CHESS_EMPTY) return 1;
    if (att.type == TYPE_KING && vic.type == TYPE_PAWN) return 0;
    if (att.type == TYPE_PAWN && vic.type == TYPE_KING) return 1;
    return (att.type >= vic.type);
}

// --- 預測反擊 ---
int AI_isUnderAttackNextTurn(gameState *game, int tr, int tc, int myCol, int myType) {
    int opCol = (myCol == COLOR_RED) ? COLOR_BLK : COLOR_RED;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            board op = game->grid[r][c];
            if (op.status == CHESS_OPEN && op.color == opCol) {
                if (op.type == TYPE_CANNON) {
                    if (r == tr || c == tc) {
                        int obs = 0;
                        if (r == tr) {
                            int s = (c < tc) ? c : tc, e = (c < tc) ? tc : c;
                            for (int i = s + 1; i < e; i++) if (game->grid[r][i].status != CHESS_EMPTY) obs++;
                        } else {
                            int s = (r < tr) ? r : tr, e = (r < tr) ? tr : r;
                            for (int i = s + 1; i < e; i++) if (game->grid[i][c].status != CHESS_EMPTY) obs++;
                        }
                        if (obs == 1) return 1;
                    }
                } else if (abs(r - tr) + abs(c - tc) == 1) {
                    if (op.type == TYPE_KING && myType == TYPE_PAWN) continue;
                    if (op.type == TYPE_PAWN && myType == TYPE_KING) return 1;
                    if (op.type >= myType) return 1;
                }
            }
        }
    }
    return 0;
}

// --- 核心決策 ---
ActionPos AI_getBestAction2(gameState *game) {
    ActionPos res; res.success = 0;
    AI_steps++;
    srand((unsigned int)time(NULL) + AI_steps);

    int myCol = game->player_color[game->current_player];
    int opCol = (myCol == COLOR_RED) ? COLOR_BLK : COLOR_RED;

    int my_n = 0, cv_n = 0;
    for(int i=0; i<4; i++) for(int j=0; j<8; j++) {
        if(game->grid[i][j].status == CHESS_OPEN && game->grid[i][j].color == myCol) my_n++;
        else if(game->grid[i][j].status == CHESS_COVER) cv_n++;
    }

    int scores[] = {0, 150, 200, 200, 300, 400, 600, 800};
    int bestV = -AI_INF;
    ActionPos moves[256]; int mCount = 0;

    // A. 掃描所有移動
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN && game->grid[r][c].color == myCol) {
                for (int tr = 0; tr < 4; tr++) {
                    for (int tc = 0; tc < 8; tc++) {
                        if (AI_checkLegal(game, r, c, tr, tc)) {
                            int s = 0;
                            board cur = game->grid[r][c], tar = game->grid[tr][tc];
                            if (tar.status == CHESS_OPEN) {
                                if (scores[tar.type] >= scores[cur.type]) s += 10000;
                                else s += scores[tar.type] * 20;
                            }
                            if (AI_isUnderAttackNextTurn(game, tr, tc, myCol, cur.type)) {
                                s -= (cur.type == TYPE_GUARD) ? 15000 : (scores[cur.type] * 30);
                            }
                            for(int er=0; er<4; er++) for(int ec=0; ec<8; ec++) {
                                if(game->grid[er][ec].status == CHESS_OPEN && game->grid[er][ec].color == opCol)
                                    s += (10 - (abs(tr-er)+abs(tc-ec))) * (my_n <= 3 ? 50 : 5);
                            }
                            if (s > bestV) { bestV = s; mCount = 0; moves[mCount++] = (ActionPos){1, {r, c}, {tr, tc}, 1}; }
                            else if (abs(s - bestV) < 20 && mCount < 256) moves[mCount++] = (ActionPos){1, {r, c}, {tr, tc}, 1};
                        }
                    }
                }
            }
        }
    }

    // B. 強化翻牌決策 (翻牌伏擊)
    position bestFlip = {-1, -1};
    int maxFlipVal = -1;

    if (cv_n > 0) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 8; c++) {
                if (game->grid[r][c].status == CHESS_COVER) {
                    int flipVal = rand() % 10; // 基礎隨機分
                    // [戰術]：檢查周圍是否有敵方的兵或強子
                    int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1};
                    for(int i=0; i<4; i++) {
                        int nr = r + dr[i], nc = c + dc[i];
                        if(nr >= 0 && nr < 4 && nc >= 0 && nc < 8) {
                            board nearby = game->grid[nr][nc];
                            if(nearby.status == CHESS_OPEN && nearby.color == opCol) {
                                if(nearby.type == TYPE_PAWN) flipVal += 50; // 伏擊敵方的兵
                                else if(nearby.type >= TYPE_MINISTER) flipVal += 30; // 伏擊敵方強子
                            }
                        }
                    }
                    if(flipVal > maxFlipVal) { maxFlipVal = flipVal; bestFlip = (position){r, c}; }
                }
            }
        }
    }

    // 如果沒有足以吃子的好移動 (bestV < 1000)，或者為了戰術伏擊，執行翻牌
    if (cv_n > 0 && (mCount == 0 || bestV < 500 || (maxFlipVal > 40 && bestV < 2000))) {
        return (ActionPos){0, bestFlip, {0,0}, 1};
    }

    if (mCount > 0) {
        res = moves[rand() % mCount];
        AI_h_pos[AI_h_idx] = res.pos2; AI_h_idx = (AI_h_idx + 1) % 4;
        return res;
    }

    return (ActionPos){0, bestFlip, {0,0}, (bestFlip.row != -1)}; 
}