#include <graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ROWS 4
#define COLS 8
#define GRID_SIZE 100

// 記憶體圖片指標
void* img_red[8];
void* img_black[8];
void* img_hidden;

typedef struct {
    int id;
    char color;
    int revealed;
} Piece;

Piece board[ROWS][COLS];

// 預載入圖片 (確保渲染瞬間完成)
void loadImagesToMemory() {
    char fileName[20];
    int size;
    readimagefile("hidden.jpg", 0, 0, GRID_SIZE-1, GRID_SIZE-1);
    size = imagesize(0, 0, GRID_SIZE-1, GRID_SIZE-1);
    img_hidden = malloc(size);
    getimage(0, 0, GRID_SIZE-1, GRID_SIZE-1, img_hidden);

    for (int i = 1; i <= 7; i++) {
        sprintf(fileName, "r%d.jpg", i);
        readimagefile(fileName, 0, 0, GRID_SIZE-1, GRID_SIZE-1);
        img_red[i] = malloc(size);
        getimage(0, 0, GRID_SIZE-1, GRID_SIZE-1, img_red[i]);

        sprintf(fileName, "b%d.jpg", i);
        readimagefile(fileName, 0, 0, GRID_SIZE-1, GRID_SIZE-1);
        img_black[i] = malloc(size);
        getimage(0, 0, GRID_SIZE-1, GRID_SIZE-1, img_black[i]);
    }
    cleardevice();
}

void initBoard() {
    Piece allPieces[32];
    int count = 0;
    char colors[] = {'r', 'b'};
    int types[] = {7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 1, 1, 1};

    for (int c = 0; c < 2; c++) {
        for (int i = 0; i < 16; i++) {
            allPieces[count].id = types[i];
            allPieces[count].color = colors[c];
            allPieces[count].revealed = 0;
            count++;
        }
    }
    srand(time(NULL));
    for (int i = 0; i < 32; i++) {
        int r = rand() % 32;
        Piece temp = allPieces[i];
        allPieces[i] = allPieces[r];
        allPieces[r] = temp;
    }
    count = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) board[i][j] = allPieces[count++];
    }
}

void render() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int x = j * GRID_SIZE;
            int y = i * GRID_SIZE;
            if (board[i][j].revealed == 0) putimage(x, y, img_hidden, COPY_PUT);
            else {
                if (board[i][j].color == 'r') putimage(x, y, img_red[board[i][j].id], COPY_PUT);
                else putimage(x, y, img_black[board[i][j].id], COPY_PUT);
            }
            setcolor(WHITE);
            rectangle(x, y, x + GRID_SIZE, y + GRID_SIZE);
        }
    }
    swapbuffers();
}

void computerMove() {
    int hiddenCoords[32][2], count = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j].revealed == 0) {
                hiddenCoords[count][0] = i; hiddenCoords[count][1] = j; count++;
            }
        }
    }
    if (count > 0) {
        int r = rand() % count;
        board[hiddenCoords[r][0]][hiddenCoords[r][1]].revealed = 1;
    }
}

int main() {
    initwindow(COLS * GRID_SIZE, ROWS * GRID_SIZE, "暗棋 - 玩家防誤觸版");
    setvisualpage(0);
    setactivepage(1);

    loadImagesToMemory();
    initBoard();
    render();

    int mx, my;
    while (1) {
        // 檢查是否有滑鼠點擊
        if (ismouseclick(WM_LBUTTONDOWN)) {
            getmouseclick(WM_LBUTTONDOWN, mx, my);
            int col = mx / GRID_SIZE;
            int row = my / GRID_SIZE;

            if (row < ROWS && col < COLS && board[row][col].revealed == 0) {
                
                // --- 1. 玩家翻牌 ---
                board[row][col].revealed = 1;
                render(); 

                // --- 2. 鎖定期間 (電腦思考) ---
                // 此時程式在 delay，雖然玩家點滑鼠，但程式不會處理 if 邏輯
                delay(1000); 

                // --- 3. 電腦翻牌 ---
                computerMove();
                render(); 

                // --- 4. 關鍵：清空剛才在 delay 期間產生的所有滑鼠點擊 ---
                // 這可以防止玩家在電腦翻牌前多點了好幾下，導致電腦翻完後自動連翻
                while(ismouseclick(WM_LBUTTONDOWN)) {
                    getmouseclick(WM_LBUTTONDOWN, mx, my); 
                }
            }
        }
        
        if (kbhit()) {
            if (getch() == 27) break;
        }
        delay(1); 
    }

    free(img_hidden);
    for(int i=1; i<=7; i++) { free(img_red[i]); free(img_black[i]); }
    closegraph();
    return 0;
}