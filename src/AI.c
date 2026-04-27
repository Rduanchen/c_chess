#include "../include/consensus.h"
#include "../include/feature.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INF 999999
#define MAX_MOVES 1024

// 方向偏移 (上下左右)
static const int DIR_R[] = {-1, 1, 0, 0};
static const int DIR_C[] = {0, 0, -1, 1};

// 前向宣告
void AI_simulateMove(gameState* game, int r1, int c1, int r2, int c2);

// ====================================================================
// 1. 權重分配 — 針對暗棋策略重新調整
// ====================================================================
// 帥/將: 被吃就輸，價值最高
// 炮/砲: 暗棋中跳吃非常強力，給予高權重
// 兵/卒: 能吃將！不能太低
// 仕/士: 次高，能壓制大部分棋子
// 相/象: 中等
// 車/俥、馬/傌: 暗棋中只能走一步，價值偏低

static int get_base_weight(int type)
{
    switch (type) {
    case TYPE_KING:     return 600;
    case TYPE_GUARD:    return 270;
    case TYPE_CANNON:   return 200;
    case TYPE_MINISTER: return 120;
    case TYPE_CHARIOT:  return 100;
    case TYPE_HORSE:    return 70;
    case TYPE_PAWN:     return 40;
    default:            return 0;
    }
}

int get_piece_weight(int color, int type)
{
    int w = get_base_weight(type);
    return (color == COLOR_RED) ? w : -w;
}

// ====================================================================
// 2. 威脅分析工具
// ====================================================================

// 是否被敵方威脅
static int is_under_threat(gameState* game, int r, int c)
{
    int my_color = game->grid[r][c].color;
    for (int tr = 0; tr < 4; tr++)
        for (int tc = 0; tc < 8; tc++)
            if (game->grid[tr][tc].status == CHESS_OPEN &&
                game->grid[tr][tc].color != my_color &&
                RULE_isValidMove(game, tr, tc, r, c))
                return 1;
    return 0;
}

// 可以吃到幾個敵方棋子
static int count_attacks(gameState* game, int r, int c)
{
    int my_color = game->grid[r][c].color;
    int cnt = 0;
    for (int tr = 0; tr < 4; tr++)
        for (int tc = 0; tc < 8; tc++)
            if (game->grid[tr][tc].status == CHESS_OPEN &&
                game->grid[tr][tc].color != my_color &&
                RULE_isValidMove(game, r, c, tr, tc))
                cnt++;
    return cnt;
}

// 有幾個安全移動 (空格或可吃的敵棋)
static int count_escapes(gameState* game, int r, int c)
{
    int cnt = 0;
    // 一般棋子只看相鄰四格
    for (int d = 0; d < 4; d++) {
        int nr = r + DIR_R[d], nc = c + DIR_C[d];
        if (nr >= 0 && nr < 4 && nc >= 0 && nc < 8) {
            if (game->grid[nr][nc].status == CHESS_EMPTY)
                cnt++;
            else if (RULE_isValidMove(game, r, c, nr, nc))
                cnt++;
        }
    }
    // 炮可以跳吃遠距離目標
    if (game->grid[r][c].type == TYPE_CANNON) {
        for (int tr = 0; tr < 4; tr++)
            for (int tc = 0; tc < 8; tc++)
                if ((tr != r || tc != c) && abs(tr-r) + abs(tc-c) > 1 &&
                    RULE_isValidMove(game, r, c, tr, tc))
                    cnt++;
    }
    return cnt;
}

// 吃完後自己是否安全 (目標位置不會立刻被反吃)
static int is_safe_capture(gameState* game, int r1, int c1, int r2, int c2)
{
    gameState sim = *game;
    AI_simulateMove(&sim, r1, c1, r2, c2);
    // 檢查移動後的位置是否被威脅
    if (sim.grid[r2][c2].status != CHESS_OPEN) return 1;
    int my_color = sim.grid[r2][c2].color;
    for (int tr = 0; tr < 4; tr++)
        for (int tc = 0; tc < 8; tc++)
            if (sim.grid[tr][tc].status == CHESS_OPEN &&
                sim.grid[tr][tc].color != my_color &&
                RULE_isValidMove(&sim, tr, tc, r2, c2))
                return 0; // 會被反吃
    return 1; // 安全
}

// ====================================================================
// 3. 棋盤評估 — 材料 + 威脅 + 機動性 + 優勢判定
// ====================================================================
int AI_evaluateBoard(gameState* game)
{
    int score = 0;
    int red_max = 0, blk_max = 0;
    int red_min = TYPE_KING + 1, blk_min = TYPE_KING + 1;
    int red_cnt = 0, blk_cnt = 0;

    // Pass 1: 材料分 + 蒐集統計
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status != CHESS_OPEN) continue;
            int color = game->grid[r][c].color;
            int type  = game->grid[r][c].type;

            score += get_piece_weight(color, type);

            if (color == COLOR_RED) {
                red_cnt++;
                if (type > red_max) red_max = type;
                if (type < red_min) red_min = type;
            } else {
                blk_cnt++;
                if (type > blk_max) blk_max = type;
                if (type < blk_min) blk_min = type;
            }
        }
    }

    // Pass 2: 威脅 / 安全 / 機動性
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status != CHESS_OPEN) continue;
            int color = game->grid[r][c].color;
            int type  = game->grid[r][c].type;
            int w     = get_base_weight(type);
            int sign  = (color == COLOR_RED) ? 1 : -1;

            // 威脅加分：每可攻擊一個敵棋 +10
            int attacks = count_attacks(game, r, c);
            score += sign * attacks * 10;

            // 被威脅扣分：高價值棋子被威脅很危險
            if (is_under_threat(game, r, c)) {
                score -= sign * (w / 3);
                // 但如果能逃跑，減輕一點
                if (count_escapes(game, r, c) > 0)
                    score += sign * (w / 8);
            }

            // 機動性加分
            int mobility = count_escapes(game, r, c);
            score += sign * mobility * 5;
        }
    }

    // === 優勢判定 ===
    // 紅方場上最小等級 > 黑方場上最大等級 → 紅方完全壓制
    if (red_cnt > 0 && blk_cnt > 0) {
        // 兵 vs 將 特殊: TYPE_PAWN(1) 可吃 TYPE_KING(7)
        // 標準壓制: 我方最弱 >= 敵方最強 (同級可互吃)
        if (red_min >= blk_max && red_min != TYPE_PAWN)
            score += 300;  // 紅方壓制
        if (blk_min >= red_max && blk_min != TYPE_PAWN)
            score -= 300;  // 黑方壓制
    }

    // 吃掉將/帥 的超級獎勵
    if (red_cnt > 0 && blk_cnt == 0) score += 5000;
    if (blk_cnt > 0 && red_cnt == 0) score -= 5000;

    return score;
}

// ====================================================================
// 4. 翻棋期望值 — 全局 + 位置策略
// ====================================================================

// 全局翻棋期望值 (機率)
static int AI_evaluateFlipGlobal(gameState* game)
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

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN) {
                int ci = game->grid[r][c].color - COLOR_RED;
                int ti = game->grid[r][c].type;
                if (ci >= 0 && ci < 2 && ti >= TYPE_PAWN && ti <= TYPE_KING)
                    initial_counts[ci][ti]--;
            } else if (game->grid[r][c].status == CHESS_COVER) {
                total_hidden++;
            }
        }
    }

    if (total_hidden == 0) return 0;

    int expected_sum = 0, total_unaccounted = 0;
    for (int color = COLOR_RED; color <= COLOR_BLK; color++) {
        for (int type = TYPE_PAWN; type <= TYPE_KING; type++) {
            int count = initial_counts[color - COLOR_RED][type];
            if (count > 0) {
                expected_sum += count * get_piece_weight(color, type);
                total_unaccounted += count;
            }
        }
    }

    return (total_unaccounted == 0) ? 0 : expected_sum / total_unaccounted;
}

// 位置翻棋獎勵：分析翻某一格旁邊有什麼
static int AI_evaluateFlipPosition(gameState* game, int row, int col, int ai_color)
{
    int bonus = 0;
    int opp_color = (ai_color == COLOR_RED) ? COLOR_BLK : COLOR_RED;

    for (int d = 0; d < 4; d++) {
        int nr = row + DIR_R[d], nc = col + DIR_C[d];
        if (nr < 0 || nr >= 4 || nc < 0 || nc >= 8) continue;
        if (game->grid[nr][nc].status != CHESS_OPEN) continue;

        int adj_color = game->grid[nr][nc].color;
        int adj_type  = game->grid[nr][nc].type;
        int adj_w     = get_base_weight(adj_type);

        if (adj_color == opp_color) {
            // 敵方高價值棋子在旁邊 → 翻這格可能翻出能吃它的棋
            bonus += adj_w / 3;
            if (adj_type == TYPE_KING)  bonus += 100; // 翻到將旁邊超讚
            if (adj_type >= TYPE_GUARD) bonus += 30;
        } else if (adj_color == ai_color) {
            // 我方高價值棋子在旁邊 → 翻這格有風險 (可能翻出能吃它的敵棋)
            bonus -= adj_w / 5;
        }
    }
    return bonus;
}

// ====================================================================
// 5. 合法移動生成 — 優先排序：吃子 > 移動 > 翻牌
// ====================================================================
void AI_getAllValidMoves(gameState* game, ActionPos* moves, int* moveCount)
{
    *moveCount = 0;
    int ai_color_idx = game->current_player;
    int ai_color = game->player_color[ai_color_idx];

    if (ai_color == COLOR_NONE) {
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 8; c++)
                if (game->grid[r][c].status == CHESS_COVER) {
                    moves[*moveCount].inst = 0;
                    moves[*moveCount].pos1.row = r;
                    moves[*moveCount].pos1.col = c;
                    moves[*moveCount].success = 1;
                    (*moveCount)++;
                }
        return;
    }

    // 分兩階段：先加吃子，再加移動 (有助 alpha-beta 剪枝)
    // Phase 1: 吃子動作
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN && game->grid[r][c].color == ai_color) {
                for (int tr = 0; tr < 4; tr++) {
                    for (int tc = 0; tc < 8; tc++) {
                        if (game->grid[tr][tc].status == CHESS_OPEN &&
                            game->grid[tr][tc].color != ai_color &&
                            RULE_isValidMove(game, r, c, tr, tc)) {
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

    // Phase 2: 移動到空格
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 8; c++) {
            if (game->grid[r][c].status == CHESS_OPEN && game->grid[r][c].color == ai_color) {
                for (int d = 0; d < 4; d++) {
                    int nr = r + DIR_R[d], nc = c + DIR_C[d];
                    if (nr >= 0 && nr < 4 && nc >= 0 && nc < 8 &&
                        game->grid[nr][nc].status == CHESS_EMPTY) {
                        moves[*moveCount].inst = 1;
                        moves[*moveCount].pos1.row = r;
                        moves[*moveCount].pos1.col = c;
                        moves[*moveCount].pos2.row = nr;
                        moves[*moveCount].pos2.col = nc;
                        moves[*moveCount].success = 1;
                        (*moveCount)++;
                    }
                }
            }
        }
    }

    // Phase 3: 翻牌
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 8; c++)
            if (game->grid[r][c].status == CHESS_COVER) {
                moves[*moveCount].inst = 0;
                moves[*moveCount].pos1.row = r;
                moves[*moveCount].pos1.col = c;
                moves[*moveCount].success = 1;
                (*moveCount)++;
            }
}

// 模擬一次移動 (不觸發 print)
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

// ====================================================================
// 6. Minimax 引擎 — 移除移動懲罰，加入吃子獎勵
// ====================================================================
int AI_minimax(gameState* game, int depth, int alpha, int beta, int isMaximizingPlayer)
{
    int over = RULE_checkGameOver(game);
    if (over == STATE_P1_WIN || over == STATE_P2_WIN) {
        // 判斷贏家是什麼顏色，紅方=正、黑方=負
        int winner_color;
        if (over == STATE_P1_WIN)
            winner_color = game->player_color[P1];
        else
            winner_color = game->player_color[P2];
        return (winner_color == COLOR_RED) ? 5000 : -5000;
    }
    if (over == STATE_TIE)    return 0;
    if (depth == 0)           return AI_evaluateBoard(game);

    ActionPos moves[MAX_MOVES];
    int moveCount = 0;
    AI_getAllValidMoves(game, moves, &moveCount);

    if (moveCount == 0)
        return AI_evaluateBoard(game);

    int ai_color = game->player_color[game->current_player];

    if (isMaximizingPlayer) {
        int maxEval = -INF;
        for (int i = 0; i < moveCount; i++) {
            gameState ns = *game;
            int eval;

            if (moves[i].inst == 0) {
                // Max 方翻牌: 模擬回合交給對手 (Null Move)
                int flip_pos = AI_evaluateFlipPosition(&ns, moves[i].pos1.row, moves[i].pos1.col, ai_color);
                int flip_bonus = AI_evaluateFlipGlobal(&ns) + flip_pos;

                ns.current_player = (ns.current_player + 1) % 2;
                // 停止向下深搜，直接使用靜態評估，避免 Null Move 導致的指數級展開
                eval = AI_evaluateBoard(&ns) + flip_bonus;
            } else {
                int capture_bonus = 0;
                int tr = moves[i].pos2.row, tc = moves[i].pos2.col;
                if (ns.grid[tr][tc].status == CHESS_OPEN) {
                    int cap_w = get_base_weight(ns.grid[tr][tc].type);
                    capture_bonus = cap_w / 2;
                    // 安全吃子額外獎勵
                    if (is_safe_capture(&ns, moves[i].pos1.row, moves[i].pos1.col, tr, tc))
                        capture_bonus += cap_w / 4;
                }

                AI_simulateMove(&ns, moves[i].pos1.row, moves[i].pos1.col, tr, tc);
                ns.current_player = (ns.current_player + 1) % 2;
                eval = AI_minimax(&ns, depth - 1, alpha, beta, 0) + capture_bonus;
            }

            if (eval > maxEval) maxEval = eval;
            if (eval > alpha)   alpha = eval;
            if (beta <= alpha)  break;
        }
        return maxEval;
    } else {
        int minEval = INF;
        for (int i = 0; i < moveCount; i++) {
            gameState ns = *game;
            int eval;

            if (moves[i].inst == 0) {
                // Min 方翻牌: 模擬回合交給對手 (Null Move)
                int flip_pos = AI_evaluateFlipPosition(&ns, moves[i].pos1.row, moves[i].pos1.col, ai_color);
                int flip_bonus = AI_evaluateFlipGlobal(&ns) - flip_pos;

                ns.current_player = (ns.current_player + 1) % 2;
                // 停止向下深搜，直接使用靜態評估
                eval = AI_evaluateBoard(&ns) + flip_bonus;
            } else {
                int capture_bonus = 0;
                int tr = moves[i].pos2.row, tc = moves[i].pos2.col;
                if (ns.grid[tr][tc].status == CHESS_OPEN) {
                    int cap_w = get_base_weight(ns.grid[tr][tc].type);
                    capture_bonus = -(cap_w / 2);
                    if (is_safe_capture(&ns, moves[i].pos1.row, moves[i].pos1.col, tr, tc))
                        capture_bonus -= cap_w / 4;
                }

                AI_simulateMove(&ns, moves[i].pos1.row, moves[i].pos1.col, tr, tc);
                ns.current_player = (ns.current_player + 1) % 2;
                eval = AI_minimax(&ns, depth - 1, alpha, beta, 1) + capture_bonus;
            }

            if (eval < minEval) minEval = eval;
            if (eval < beta)    beta = eval;
            if (beta <= alpha)  break;
        }
        return minEval;
    }
}

// ====================================================================
// 7. 整合入口
// ====================================================================
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

    // 洗牌避免 AI 走法太僵化 (同分時隨機)
    srand(time(NULL));
    if (moveCount > 1) {
        for (int i = moveCount - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            ActionPos temp = moves[i];
            moves[i] = moves[j];
            moves[j] = temp;
        }
    }

    // 開局顏色未定 → 隨機翻
    if (ai_color == COLOR_NONE) {
        return moves[0];
    }

    int isMaximizing = (ai_color == COLOR_RED);
    int bestVal = isMaximizing ? -INF : INF;
    int bestMoveIdx = 0;

    // 依據場上棋子數量動態調整搜尋深度
    int open_count = 0;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 8; c++)
            if (game->grid[r][c].status == CHESS_OPEN) open_count++;
    int depth = (open_count <= 6) ? 9 : (open_count <= 12) ? 6 : 4;

    for (int i = 0; i < moveCount; i++) {
        gameState ns = *game;
        int eval;

        if (moves[i].inst == 0) {
                // 翻牌: 模擬回合交給對手 (Null Move)
                int flip_pos = AI_evaluateFlipPosition(&ns, moves[i].pos1.row, moves[i].pos1.col, ai_color);
                int flip_bonus = AI_evaluateFlipGlobal(&ns) + (isMaximizing ? flip_pos : -flip_pos);
                
                ns.current_player = (ns.current_player + 1) % 2;
                // 第一層的翻牌給予 depth=1 的搜尋，確保 AI 能看見對手的下一步反擊，且不會超時
                if (isMaximizing) {
                    eval = AI_minimax(&ns, 1, -INF, INF, 0) + flip_bonus;
                } else {
                    eval = AI_minimax(&ns, 1, -INF, INF, 1) + flip_bonus;
                }
        } else {
            // 移動/吃子
            int capture_bonus = 0;
            int tr = moves[i].pos2.row, tc = moves[i].pos2.col;
            if (ns.grid[tr][tc].status == CHESS_OPEN) {
                int cap_w = get_base_weight(ns.grid[tr][tc].type);
                int base_bonus = cap_w / 2;
                // 安全吃子額外獎勵
                if (is_safe_capture(&ns, moves[i].pos1.row, moves[i].pos1.col, tr, tc))
                    base_bonus += cap_w / 4;
                capture_bonus = isMaximizing ? base_bonus : -base_bonus;
            }

            AI_simulateMove(&ns, moves[i].pos1.row, moves[i].pos1.col, tr, tc);
            ns.current_player = (ns.current_player + 1) % 2;

            if (isMaximizing) {
                eval = AI_minimax(&ns, depth - 1, -INF, INF, 0) + capture_bonus;
            } else {
                eval = AI_minimax(&ns, depth - 1, -INF, INF, 1) + capture_bonus;
            }
        }

        if (isMaximizing) {
            if (eval > bestVal) { bestVal = eval; bestMoveIdx = i; }
        } else {
            if (eval < bestVal) { bestVal = eval; bestMoveIdx = i; }
        }
    }

    return moves[bestMoveIdx];
}