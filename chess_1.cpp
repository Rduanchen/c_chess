#include <graphics.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#define ROWS 4
#define COLS 8
#define CELL_SIZE 60   // 每個棋子的大小 (60x60 像素)
#define MARGIN_X 50    // 棋盤左邊距
#define MARGIN_Y 50    // 棋盤上邊距

// 定義棋子結構
typedef struct {
    int type_id;     // 棋子種類的代號 (0~13)
    int is_revealed; // 0: 背面, 1: 正面
} Piece;

Piece board[ROWS][COLS];

const char *img_files[14] = {
    "red_1.jpg", // 0: 帥
    "red_2.jpg", // 1: 仕
    "red_3.jpg", // 2: 相
    "red_4.jpg", // 3: 俥
    "red_5.jpg", // 4: 傌
    "red_6.jpg", // 5: 炮
    "red_7.jpg", // 6: 兵
    "blk_1.jpg", // 7: 將
    "blk_2.jpg", // 8: 士
    "blk_3.jpg", // 9: 象
    "blk_4.jpg", // 10: 車
    "blk_5.jpg", // 11: 馬
    "blk_6.jpg", // 12: 包
    "blk_7.jpg"  // 13: 卒
};

int init_deck[32] = {
    0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6,
    7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 13, 13, 13
};


void init_board();
void draw_board();
void update_piece(int r, int c);
void player_turn();
void computer_turn();
int is_game_over();

int main() {
    // 初始化圖形視窗 (寬 600, 高 400)
    initwindow(600, 400, "Chinese Dark Chess (Image Version)");

    setbkcolor(BLACK);
    setcolor(WHITE);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 2);

    // 初始化亂數種子
    srand((unsigned int)time(NULL));

    init_board();

    // 遊戲開始前，先完整畫出一次棋盤
    draw_board();

    // 遊戲主迴圈
    while (!is_game_over()) {

        player_turn();

        if (is_game_over()) break;

        // 停頓 1 秒 (1000 毫秒)，讓玩家看清楚，然後換電腦
        delay(1000);

        // 電腦自動選擇一顆棋子並顯示
        computer_turn();
    }

    outtextxy(MARGIN_X, MARGIN_Y + ROWS * CELL_SIZE + 20, (char*)"Game Over!");

    getch();
    closegraph();

    return 0;
}

// 初始化並洗牌
void init_board() {
    int indices[32];
    for (int i = 0; i < 32; i++) indices[i] = i;

    // Fisher-Yates 洗牌
    for (int i = 31; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }

    int count = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            // 將洗牌後的代號放進棋盤
            board[i][j].type_id = init_deck[indices[count]];
            board[i][j].is_revealed = 0;
            count++;
        }
    }
}

void draw_board() {
    cleardevice(); // 清空畫面

    outtextxy(MARGIN_X, MARGIN_Y - 30, (char*)"Click to flip");

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int x1 = MARGIN_X + j * CELL_SIZE;
            int y1 = MARGIN_Y + i * CELL_SIZE;
            int x2 = x1 + CELL_SIZE;
            int y2 = y1 + CELL_SIZE;

            if (board[i][j].is_revealed) {
                if (board[i][j].type_id <= 6) setfillstyle(SOLID_FILL, RED);
                else setfillstyle(SOLID_FILL, DARKGRAY);
                bar(x1+1, y1+1, x2-1, y2-1);

                readimagefile(img_files[board[i][j].type_id], x1, y1, x2, y2);
            } else {
                setfillstyle(SOLID_FILL, BLUE);
                bar(x1+1, y1+1, x2-1, y2-1);

                readimagefile("back.jpg", x1, y1, x2, y2);
            }

            setcolor(WHITE);
            rectangle(x1, y1, x2, y2);
        }
    }
}

// 更新單一棋子的畫面
void update_piece(int r, int c) {
    int x1 = MARGIN_X + c * CELL_SIZE;
    int y1 = MARGIN_Y + r * CELL_SIZE;
    int x2 = x1 + CELL_SIZE;
    int y2 = y1 + CELL_SIZE;

    if (board[r][c].is_revealed) {
        if (board[r][c].type_id <= 6) setfillstyle(SOLID_FILL, RED);
        else setfillstyle(SOLID_FILL, DARKGRAY);
        bar(x1+1, y1+1, x2-1, y2-1);

        readimagefile(img_files[board[r][c].type_id], x1, y1, x2, y2);
    } else {
        setfillstyle(SOLID_FILL, BLUE);
        bar(x1+1, y1+1, x2-1, y2-1);

        readimagefile("back.jpg", x1, y1, x2, y2);
    }
    setcolor(WHITE);
    rectangle(x1, y1, x2, y2);
}

void player_turn() {
    int x, y;
    int clicked_row = -1, clicked_col = -1;

    while (1) {
        if (ismouseclick(WM_LBUTTONDOWN)) {
            // 取得點擊座標
            getmouseclick(WM_LBUTTONDOWN, x, y);

            // 判斷是否點在棋盤的有效範圍內
            if (x >= MARGIN_X && x < MARGIN_X + COLS * CELL_SIZE &&
                y >= MARGIN_Y && y < MARGIN_Y + ROWS * CELL_SIZE) {

                // 計算點擊的是哪一個格子
                clicked_col = (x - MARGIN_X) / CELL_SIZE;
                clicked_row = (y - MARGIN_Y) / CELL_SIZE;

                // 判斷該格子是否還沒翻開
                if (board[clicked_row][clicked_col].is_revealed == 0) {
                    board[clicked_row][clicked_col].is_revealed = 1;
                    update_piece(clicked_row, clicked_col);
                    break; // 翻牌
                }
            }
        }
        delay(50);
    }
}

// 電腦回合 (隨機翻牌)
void computer_turn() {
    int hidden_count = 0;
    int hidden_coords[32][2];

    // 找出所有還蓋著的棋子
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (!board[i][j].is_revealed) {
                hidden_coords[hidden_count][0] = i;
                hidden_coords[hidden_count][1] = j;
                hidden_count++;
            }
        }
    }

    if (hidden_count == 0) return;

    //隨機選一個
    int pick = rand() % hidden_count;
    int r = hidden_coords[pick][0];
    int c = hidden_coords[pick][1];

    board[r][c].is_revealed = 1;
    update_piece(r, c);
}

// 檢查遊戲是否結束
int is_game_over() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j].is_revealed == 0) return 0; // 還有蓋著的
        }
    }
    return 1; // 全部翻開
}
