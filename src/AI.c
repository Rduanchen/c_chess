#include "../include/consensus.h"
#include "../include/feature.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INF 999999
#define MAX_MOVES 1024

// --- 3.1 權重分配 ---
int get_piece_weight(int color, int type)
{
    if (color == COLOR_RED) {
        switch (type) {
        case TYPE_KING:
            return 100;
        case TYPE_GUARD:
            return 90;
        case TYPE_MINISTER:
            return 50;
        case TYPE_CHARIOT:
            return 30;
        case TYPE_HORSE:
            return 10;
        case TYPE_CANNON:
            return 70;
        case TYPE_PAWN:
            return 5;
        default:
            return 0;
        }
    } else if (color == COLOR_BLK) {
        switch (type) {
        case TYPE_KING:
            return -100;
        case TYPE_GUARD:
            return -90;
        case TYPE_MINISTER:
            return -50;
        case TYPE_CHARIOT:
            return -30;
        case TYPE_HORSE:
            return -10;
        case TYPE_CANNON:
            return -70;
        case TYPE_PAWN:
            return -5;
        default:
            return 0;
        }
    }
    return 0;
}

// --- 3.3 總分計算公式 ---
// 根據場上的已知紅黑棋子總計權重
int AI_evaluateBoard(gameState* game)
{
    int score = 0;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN) {
                score += get_piece_weight(game->grid[r][c].color, game->grid[r][c].type);
            }
        }
    }
    return score;
}
// --- 5. 翻棋動態期望值 (已修正作弊問題) ---
int AI_evaluateFlip(gameState* game)
{
    int total_hidden = 0;

    int initial_counts[2][8] = { 0 };
    for (int i = COLOR_RED; i <= COLOR_BLK; i++) {
        initial_counts[i - COLOR_RED][TYPE_KING] = 1;
        initial_counts[i - COLOR_RED][TYPE_GUARD] = 2;
        initial_counts[i - COLOR_RED][TYPE_MINISTER] = 2;
        initial_counts[i - COLOR_RED][TYPE_CHARIOT] = 2;
        initial_counts[i - COLOR_RED][TYPE_HORSE] = 2;
        initial_counts[i - COLOR_RED][TYPE_CANNON] = 2;
        initial_counts[i - COLOR_RED][TYPE_PAWN] = 5;
    }

    // 找出所有已經翻開或被吃掉的棋子（場上不再是 CHESS_COVER）
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN) {
                int c_idx = game->grid[r][c].color - COLOR_RED;
                int t_idx = game->grid[r][c].type;
                if (c_idx >= 0 && c_idx < 2 && t_idx >= TYPE_PAWN && t_idx <= TYPE_KING) {
                    initial_counts[c_idx][t_idx]--;
                }
            } else if (game->grid[r][c].status == CHESS_COVER) {
                total_hidden++;
            }
        }
    }

    // 如果沒有暗棋了，期望值為 0
    if (total_hidden == 0)
        return 0;

    int expected_sum = 0;

    for (int color = COLOR_RED; color <= COLOR_BLK; color++) {
        for (int type = TYPE_PAWN; type <= TYPE_KING; type++) {
            int count = initial_counts[color - COLOR_RED][type];
            if (count > 0) {
                expected_sum += count * get_piece_weight(color, type);
            }
        }
    }

    return expected_sum / total_hidden;
}

// --- 4.1 合法移動生成 ---
void AI_getAllValidMoves(gameState* game, ActionPos* moves, int* moveCount)
{
    *moveCount = 0;
    int ai_color_idx = game->current_player;
    int ai_color = game->player_color[ai_color_idx];

    // 如果第一步，顏色未定，只能翻棋
    if (ai_color == COLOR_NONE) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 8; c++) {
                if (game->grid[r][c].status == CHESS_COVER) {
                    moves[*moveCount].inst = 0;
                    moves[*moveCount].pos1.row = r;
                    moves[*moveCount].pos1.col = c;
                    moves[*moveCount].success = 1;
                    (*moveCount)++;
                }
            }
        }
        return;
    }

    // 1. Gen all valid moves/captures for AI's opened pieces
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN && game->grid[r][c].color == ai_color) {
                // 找出十字方向與跳吃
                for (int tr = 0; tr < 4; tr++) {
                    for (int tc = 0; tc < 8; tc++) {
                        // 使用原有的合法判斷函數
                        if (RULE_isValidMove(game, r, c, tr, tc)) {
                            moves[*moveCount].inst = 1;
                            moves[*moveCount].pos1.row = r;
                            moves[*moveCount].pos1.col = c;
                            moves[*moveCount].pos2.row = tr;
                            moves[*moveCount].pos2.col = tc;
                            moves[*moveCount].success = 1;
                            (*moveCount)++;
                        }
                    }
                }
            }
        }
    }

    // 2. Gen all flip actions
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_COVER) {
                moves[*moveCount].inst = 0;
                moves[*moveCount].pos1.row = r;
                moves[*moveCount].pos1.col = c;
                moves[*moveCount].success = 1;
                (*moveCount)++;
            }
        }
    }
}

// 模擬一次 IO_executeMove 而不觸發 print
void AI_simulateMove(gameState* game, int r1, int c1, int r2, int c2)
{
    board* src = &game->grid[r1][c1];
    board* dst = &game->grid[r2][c2];

    if (dst->status == CHESS_OPEN) {
        if (dst->color == COLOR_RED)
            game->red_left--;
        else if (dst->color == COLOR_BLK)
            game->black_left--;
    }

    dst->type = src->type;
    dst->color = src->color;
    dst->status = CHESS_OPEN;

    src->type = TYPE_EMPTY;
    src->color = COLOR_NONE;
    src->status = CHESS_EMPTY;
}

// --- 4.2 Minimax 引擎 ---
int AI_minimax(gameState* game, int depth, int alpha, int beta, int isMaximizingPlayer)
{
    // 抵達葉子節點或結束
    int over = RULE_checkGameOver(game);
    if (depth == 0 || over == STATE_P1_WIN || over == STATE_P2_WIN || over == STATE_TIE) {
        return AI_evaluateBoard(game);
    }

    ActionPos moves[MAX_MOVES];
    int moveCount = 0;
    AI_getAllValidMoves(game, moves, &moveCount);

    if (moveCount == 0)
        return AI_evaluateBoard(game);

    if (isMaximizingPlayer) {
        int maxEval = -INF;
        for (int i = 0; i < moveCount; i++) {
            gameState nextState = *game;
            int eval = 0;

            if (moves[i].inst == 0) { // 翻牌 (不再展開)，加入動態期望值
                eval = AI_evaluateBoard(&nextState) + AI_evaluateFlip(&nextState);
            } else { // 移動 (展開)
                AI_simulateMove(&nextState, moves[i].pos1.row, moves[i].pos1.col, moves[i].pos2.row, moves[i].pos2.col);
                nextState.current_player = (nextState.current_player + 1) % 2;
                eval = AI_minimax(&nextState, depth - 1, alpha, beta, 0) - 1; // 紅方(Max)移動，分數 - 1
            }

            if (eval > maxEval)
                maxEval = eval;
            if (eval > alpha)
                alpha = eval;
            if (beta <= alpha)
                break; // Beta cutoff
        }
        return maxEval;
    } else {
        int minEval = INF;
        for (int i = 0; i < moveCount; i++) {
            gameState nextState = *game;
            int eval = 0;

            if (moves[i].inst == 0) { // 翻牌
                eval = AI_evaluateBoard(&nextState) + AI_evaluateFlip(&nextState);
            } else { // 移動
                AI_simulateMove(&nextState, moves[i].pos1.row, moves[i].pos1.col, moves[i].pos2.row, moves[i].pos2.col);
                nextState.current_player = (nextState.current_player + 1) % 2;
                eval = AI_minimax(&nextState, depth - 1, alpha, beta, 1) + 1; // 黑方(Min)移動，分數 + 1
            }

            if (eval < minEval)
                minEval = eval;
            if (eval < beta)
                beta = eval;
            if (beta <= alpha)
                break; // Alpha cutoff
        }
        return minEval;
    }
}

// --- 5. 整合入口 ---
ActionPos AI_getBestAction(gameState* game)
{
    ActionPos bestMove;
    bestMove.success = 0;

    ActionPos moves[MAX_MOVES];
    int moveCount = 0;
    AI_getAllValidMoves(game, moves, &moveCount);

    if (moveCount == 0)
        return bestMove;

    int ai_color_idx = game->current_player;
    int ai_color = game->player_color[ai_color_idx];

    // 洗牌避免 AI 走法太僵化
    srand(time(NULL));
    if (moveCount > 1) {
        for (int i = moveCount - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            ActionPos temp = moves[i];
            moves[i] = moves[j];
            moves[j] = temp;
        }
    }

    // 如果是開局翻牌，隨機翻一張不需要跑 Minimax
    if (ai_color == COLOR_NONE) {
        return moves[0];
    }

    int isMaximizing = (ai_color == COLOR_RED);
    int bestVal = isMaximizing ? -INF : INF;
    int bestMoveIdx = 0;

    int depth = 7; // 搜尋深度

    for (int i = 0; i < moveCount; i++) {
        gameState nextState = *game;
        int eval = 0;

        if (moves[i].inst == 0) { // 翻牌
            eval = AI_evaluateBoard(&nextState) + AI_evaluateFlip(&nextState);
        } else { // 移動
            AI_simulateMove(&nextState, moves[i].pos1.row, moves[i].pos1.col, moves[i].pos2.row, moves[i].pos2.col);
            nextState.current_player = (nextState.current_player + 1) % 2;

            if (isMaximizing) {
                eval = AI_minimax(&nextState, depth - 1, -INF, INF, 0) - 1;
            } else {
                eval = AI_minimax(&nextState, depth - 1, -INF, INF, 1) + 1;
            }
        }

        if (isMaximizing) {
            if (eval > bestVal) {
                bestVal = eval;
                bestMoveIdx = i;
            }
        } else {
            if (eval < bestVal) {
                bestVal = eval;
                bestMoveIdx = i;
            }
        }
    }

    return moves[bestMoveIdx];
}
#include "../include/consensus.h"
#include "../include/feature.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

ActionPos AI_randomFlip(gameState* game)
{
    ActionPos decision = { 0, { -1, -1 }, { -1, -1 }, 0 };
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
        decision.pos1.row = coveredCoords[idx][0];
        decision.pos1.col = coveredCoords[idx][1];
        decision.success = 1;
    }
    return decision;
}