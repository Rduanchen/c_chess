#ifndef SERVER_H
#define SERVER_H

#include "consensus.h"
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

// ====== 伺服器連線設定 ======
#define SERVER_IP   "140.124.184.220"
#define SERVER_PORT 8888

// ====== 遊戲模式旗標 ======
#define MODE_LOCAL  0
#define MODE_ONLINE 1

// ====== 線上遊戲狀態結構 ======
typedef struct {
    SOCKET      socket;             // 連線 socket
    char        assigned_role[10];  // "first" 或 "second"
    char        my_role_ab[2];      // "A" 或 "B"
    char        my_color[10];       // "Red" 或 "Black" 或 "None"
    char        opp_color[10];      // 對方顏色
    int         last_total_moves;   // 上一次的總步數
    int         is_my_turn;         // 是否輪到我
    int         connected;          // 是否已連線
    int         game_started;       // 比賽是否已開始
    char        room_id[32];        // 房間號碼
    char        raw_json[8192];     // 最新的 JSON 資料
} OnlineState;

// ====== 伺服器通訊函式 ======

// 初始化 Winsock 並連線到伺服器
int SVR_initConnection(OnlineState *state);

// 加入房間 (以房間號碼)
int SVR_joinRoom(OnlineState *state, const char *room_id);

// 傳送動作到伺服器
void SVR_sendAction(OnlineState *state, const char *action);

// 非阻塞接收伺服器更新 (回傳 1 代表有新資料，0 代表無)
int SVR_receiveUpdate(OnlineState *state, char *buffer, int len);

// 關閉連線
void SVR_closeConnection(OnlineState *state);

// ====== JSON 解析輔助函式 ======

// 從 JSON 中提取棋盤第 index 格的棋子名稱
void SVR_getPieceAt(const char *json, int index, char *out_piece);

// 獲取指定角色 (A 或 B) 的顏色 (Red 或 Black)
void SVR_getRoleColor(const char *json, const char *role, char *out_color);

// 取得 current_turn_role
void SVR_getCurrentTurnRole(const char *json, char *out_role);

// 取得 total_moves
int SVR_getTotalMoves(const char *json);

// 取得 game state 字串 ("waiting", "playing", "ended" 等)
void SVR_getGameState(const char *json, char *out_state);

// ====== 棋盤格式轉換 ======

// 將伺服器回傳的 JSON 棋盤轉換成系統的 gameState 格式
// 注意：Covered 的棋子維持 CHESS_COVER 狀態，不標註內容
void SVR_syncBoardFromJSON(const char *json, gameState *game, OnlineState *online);

// ====== 動作格式轉換 ======

// 將翻牌動作轉換成伺服器格式並發送
void SVR_sendFlip(OnlineState *state, int row, int col);

// 將移動/吃牌動作轉換成伺服器格式並發送
void SVR_sendMove(OnlineState *state, int r1, int c1, int r2, int c2);

#endif
