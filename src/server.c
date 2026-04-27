#include "../include/consensus.h"
#include "../include/server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

// ====================================================================
// 伺服器通訊函式
// ====================================================================

int SVR_initConnection(OnlineState *state) {
    WSADATA wsa;
    struct sockaddr_in server;

    memset(state, 0, sizeof(OnlineState));
    state->socket = INVALID_SOCKET;
    state->last_total_moves = -1;
    state->is_my_turn = 0;
    state->connected = 0;
    state->game_started = 0;
    strcpy(state->my_color, "None");
    strcpy(state->opp_color, "None");

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[SVR] WSAStartup failed: %d\n", WSAGetLastError());
        return -1;
    }

    state->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (state->socket == INVALID_SOCKET) {
        printf("[SVR] Socket creation failed: %d\n", WSAGetLastError());
        return -1;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    if (connect(state->socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("[SVR] Connection failed: %d\n", WSAGetLastError());
        closesocket(state->socket);
        state->socket = INVALID_SOCKET;
        return -1;
    }

    // 設定為非阻塞模式
    u_long mode = 1;
    ioctlsocket(state->socket, FIONBIO, &mode);

    state->connected = 1;
    printf("[SVR] Connected to Dark Chess Server at %s:%d\n", SERVER_IP, SERVER_PORT);
    return 0;
}

int SVR_joinRoom(OnlineState *state, const char *room_id) {
    if (!state->connected) return -1;

    char cmd[128];
    sprintf(cmd, "JOIN %s\n", room_id);
    strncpy(state->room_id, room_id, sizeof(state->room_id) - 1);

    // 暫時切回阻塞模式來等待加入房間的回應
    u_long blocking = 0;
    ioctlsocket(state->socket, FIONBIO, &blocking);

    send(state->socket, cmd, (int)strlen(cmd), 0);

    char response[2000];
    int size = recv(state->socket, response, 1999, 0);
    if (size > 0) {
        response[size] = '\0';
        printf("[SVR] Server response: %s\n", response);

        if (strstr(response, "SUCCESS")) {
            // 提取 ROLE
            char *role_ptr = strstr(response, "ROLE ");
            if (role_ptr) {
                sscanf(role_ptr + 5, "%s", state->assigned_role);
                printf("[SVR] Assigned Role: %s\n", state->assigned_role);
            }

            // 設定 A/B 角色
            if (strcmp(state->assigned_role, "first") == 0)
                strcpy(state->my_role_ab, "A");
            else if (strcmp(state->assigned_role, "second") == 0)
                strcpy(state->my_role_ab, "B");

            // 切回非阻塞模式
            u_long nonblocking = 1;
            ioctlsocket(state->socket, FIONBIO, &nonblocking);

            printf("[SVR] Successfully joined room %s as %s (Role %s)\n",
                   room_id, state->assigned_role, state->my_role_ab);
            return 0;
        }
    }

    // 切回非阻塞模式
    u_long nonblocking = 1;
    ioctlsocket(state->socket, FIONBIO, &nonblocking);

    printf("[SVR] Failed to join room %s\n", room_id);
    return -1;
}

void SVR_sendAction(OnlineState *state, const char *action) {
    if (!state->connected) return;
    send(state->socket, action, (int)strlen(action), 0);
    printf("[SVR] Sent action: %s", action);
}

int SVR_receiveUpdate(OnlineState *state, char *buffer, int len) {
    if (!state->connected) return 0;

    memset(buffer, 0, len);
    int size = recv(state->socket, buffer, len - 1, 0);

    if (size > 0) {
        buffer[size] = '\0';
        // 儲存最新的 JSON
        strncpy(state->raw_json, buffer, sizeof(state->raw_json) - 1);
        return 1;
    }

    return 0;
}

void SVR_closeConnection(OnlineState *state) {
    if (state->socket != INVALID_SOCKET) {
        closesocket(state->socket);
        state->socket = INVALID_SOCKET;
    }
    state->connected = 0;
    WSACleanup();
    printf("[SVR] Connection closed.\n");
}

// ====================================================================
// JSON 解析輔助函式
// ====================================================================

// 檢查棋盤是否為空 (waiting 狀態時 board 為 [])
static int SVR_isBoardEmpty(const char *json) {
    const char *p = strstr(json, "\"board\": []");
    if (p) return 1;
    p = strstr(json, "\"board\":[]");
    if (p) return 1;
    return 0;
}

void SVR_getPieceAt(const char *json, int index, char *out_piece) {
    // 先檢查是否為空棋盤
    if (SVR_isBoardEmpty(json)) {
        strcpy(out_piece, "Covered");
        return;
    }

    const char *board_start = strstr(json, "\"board\": [[");
    if (!board_start) {
        // 嘗試無空格版本
        board_start = strstr(json, "\"board\":[[");
        if (!board_start) {
            strcpy(out_piece, "Covered");
            return;
        }
        board_start += 10;
    } else {
        board_start += 11;
    }

    const char *p = board_start;
    int count = 0;

    while (count <= index && p) {
        p = strchr(p, '\"');
        if (!p) break;
        p++;
        const char *end = strchr(p, '\"');
        if (!end) break;

        if (count == index) {
            int len = (int)(end - p);
            if (len > 31) len = 31;
            strncpy(out_piece, p, len);
            out_piece[len] = '\0';
            return;
        }
        p = end + 1;
        count++;
    }
    strcpy(out_piece, "Unknown");
}

void SVR_getRoleColor(const char *json, const char *role, char *out_color) {
    char search_key[20];
    sprintf(search_key, "\"%s\": \"", role);
    const char *p = strstr(json, search_key);
    if (!p) {
        // 嘗試無空格版本
        sprintf(search_key, "\"%s\":\"", role);
        p = strstr(json, search_key);
    }
    if (p) {
        p += strlen(search_key);
        const char *end = strchr(p, '\"');
        if (end) {
            int len = (int)(end - p);
            if (len > 9) len = 9;
            strncpy(out_color, p, len);
            out_color[len] = '\0';
            return;
        }
    }
    strcpy(out_color, "None");
}

void SVR_getCurrentTurnRole(const char *json, char *out_role) {
    const char *p = strstr(json, "\"current_turn_role\": \"");
    if (!p) {
        p = strstr(json, "\"current_turn_role\":\"");
        if (!p) {
            strcpy(out_role, "");
            return;
        }
        p += 21;
    } else {
        p += 22;
    }

    // 讀取角色字元 (A 或 B)
    out_role[0] = p[0];
    out_role[1] = '\0';
}

int SVR_getTotalMoves(const char *json) {
    const char *p = strstr(json, "\"total_moves\": ");
    if (!p) {
        p = strstr(json, "\"total_moves\":");
        if (!p) return -1;
        p += 14;
    } else {
        p += 15;
    }

    int moves = -1;
    sscanf(p, "%d", &moves);
    return moves;
}

void SVR_getGameState(const char *json, char *out_state) {
    const char *p = strstr(json, "\"state\": \"");
    if (!p) {
        p = strstr(json, "\"state\":\"");
        if (!p) {
            strcpy(out_state, "unknown");
            return;
        }
        p += 9;
    } else {
        p += 10;
    }

    const char *end = strchr(p, '\"');
    if (end) {
        int len = (int)(end - p);
        if (len > 31) len = 31;
        strncpy(out_state, p, len);
        out_state[len] = '\0';
    } else {
        strcpy(out_state, "unknown");
    }
}

// ====================================================================
// 棋盤格式轉換 - 伺服器名稱 -> 系統內部值
// ====================================================================

// 伺服器棋子名稱對照表
typedef struct {
    const char *name;
    int color;
    int type;
} PieceMapping;

static const PieceMapping piece_map[] = {
    // 紅方
    {"Red_King",     COLOR_RED, TYPE_KING},
    {"Red_Guard",    COLOR_RED, TYPE_GUARD},
    {"Red_Elephant", COLOR_RED, TYPE_MINISTER},
    {"Red_Car",      COLOR_RED, TYPE_CHARIOT},
    {"Red_Horse",    COLOR_RED, TYPE_HORSE},
    {"Red_Cannon",   COLOR_RED, TYPE_CANNON},
    {"Red_Soldier",  COLOR_RED, TYPE_PAWN},
    // 黑方
    {"Black_King",     COLOR_BLK, TYPE_KING},
    {"Black_Guard",    COLOR_BLK, TYPE_GUARD},
    {"Black_Elephant", COLOR_BLK, TYPE_MINISTER},
    {"Black_Car",      COLOR_BLK, TYPE_CHARIOT},
    {"Black_Horse",    COLOR_BLK, TYPE_HORSE},
    {"Black_Cannon",   COLOR_BLK, TYPE_CANNON},
    {"Black_Soldier",  COLOR_BLK, TYPE_PAWN},
    {NULL, 0, 0}
};

static void parse_piece_name(const char *name, int *out_color, int *out_type, int *out_status) {
    if (strcmp(name, "Covered") == 0) {
        *out_color = COLOR_NONE;
        *out_type = TYPE_EMPTY;
        *out_status = CHESS_COVER;
        return;
    }
    if (strcmp(name, "Null") == 0) {
        *out_color = COLOR_NONE;
        *out_type = TYPE_EMPTY;
        *out_status = CHESS_EMPTY;
        return;
    }

    // 查對照表
    for (int i = 0; piece_map[i].name != NULL; i++) {
        if (strcmp(name, piece_map[i].name) == 0) {
            *out_color = piece_map[i].color;
            *out_type = piece_map[i].type;
            *out_status = CHESS_OPEN;
            return;
        }
    }

    // 未知棋子
    printf("[SVR] Warning: Unknown piece name '%s'\n", name);
    *out_color = COLOR_NONE;
    *out_type = TYPE_EMPTY;
    *out_status = CHESS_EMPTY;
}

void SVR_syncBoardFromJSON(const char *json, gameState *game, OnlineState *online) {
    char piece_name[32];
    int red_count = 0, black_count = 0;

    // 若棋盤為空 (waiting 狀態)，不更新棋盤格子
    if (SVR_isBoardEmpty(json)) {
        printf("[SVR] Board is empty (waiting state), skipping board sync.\n");
        // 僅更新遊戲狀態
        char state_str[32];
        SVR_getGameState(json, state_str);
        if (strcmp(state_str, "playing") == 0) {
            online->game_started = 1;
            game->game_state = STATE_ING;
        }
        return;
    }

    // 解析每一格 (4x8 = 32 格)
    for (int i = 0; i < 32; i++) {
        int row = i / 8;
        int col = i % 8;
        int color, type, status;

        SVR_getPieceAt(json, i, piece_name);
        parse_piece_name(piece_name, &color, &type, &status);

        game->grid[row][col].color = color;
        game->grid[row][col].type = type;
        game->grid[row][col].status = status;

        // 統計存活棋子
        if (status == CHESS_OPEN || status == CHESS_COVER) {
            if (color == COLOR_RED) red_count++;
            else if (color == COLOR_BLK) black_count++;
            // Covered 的棋子在伺服器端有計數，但我們不知道顏色
            // 暫且不計入，交由伺服器判定
        }
    }

    // 更新顏色資訊
    SVR_getRoleColor(json, online->my_role_ab, online->my_color);
    if (strcmp(online->my_color, "None") != 0) {
        strcpy(online->opp_color,
               strcmp(online->my_color, "Red") == 0 ? "Black" : "Red");

        // 映射到系統的 player_color
        if (strcmp(online->my_color, "Red") == 0) {
            game->player_color[P1] = COLOR_RED;
            game->player_color[P2] = COLOR_BLK;
        } else {
            game->player_color[P1] = COLOR_BLK;
            game->player_color[P2] = COLOR_RED;
        }
    }

    // 更新是否輪到我
    char turn_role[4];
    SVR_getCurrentTurnRole(json, turn_role);
    int total_moves = SVR_getTotalMoves(json);

    if (strlen(turn_role) > 0 &&
        strcmp(turn_role, online->my_role_ab) == 0 &&
        total_moves != online->last_total_moves) {
        online->is_my_turn = 1;
    } else {
        online->is_my_turn = 0;
    }

    // 更新遊戲狀態
    char state_str[32];
    SVR_getGameState(json, state_str);
    if (strcmp(state_str, "playing") == 0) {
        online->game_started = 1;
        game->game_state = STATE_ING;
    } else if (strcmp(state_str, "ended") == 0) {
        game->game_state = STATE_TIE; // 暫時設為 TIE，具體由伺服器判定
    }
}

// ====================================================================
// 動作格式轉換
// ====================================================================

void SVR_sendFlip(OnlineState *state, int row, int col) {
    char action[32];
    // 翻牌格式: "row col\n"
    sprintf(action, "%d %d\n", row, col);
    SVR_sendAction(state, action);
    state->last_total_moves = SVR_getTotalMoves(state->raw_json);
    state->is_my_turn = 0;
}

void SVR_sendMove(OnlineState *state, int r1, int c1, int r2, int c2) {
    char action[32];
    // 移動/吃牌格式: "r1 c1 r2 c2\n"
    sprintf(action, "%d %d %d %d\n", r1, c1, r2, c2);
    SVR_sendAction(state, action);
    state->last_total_moves = SVR_getTotalMoves(state->raw_json);
    state->is_my_turn = 0;
}
