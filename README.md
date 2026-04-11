# C_Chess (象棋暗棋)

這是一個使用 C 語言與 SDL2 函式庫開發的**象棋暗棋 (Banqi / Dark Chess)** 遊戲。專案包含了完整的遊戲邏輯、圖形化使用者介面 (GUI)，以及內建 Alpha-Beta 剪枝的 Minimax AI 引擎。

## 🌟 遊戲特色 (Features)

- **人機對戰 (Human vs AI)**：玩家可以與內建的 AI 進行對弈。
- **高階 AI 引擎**：內建基於 3-Ply Minimax 演算法以及 Alpha-Beta 剪枝技術的決策核心，具備動態期望值計算、精準的捕捉判斷與步數成本優化。
- **4x8 棋盤狀態管理**：完整的暗棋規則判定（包含炮的跳吃規則、翻棋邏輯等）。
- **圖形化介面**：使用 SDL2 渲染棋盤與棋子，支援滑鼠點擊互動，並包含選單與暫停畫面。
- **綠色依賴 (Windows)**：Windows 開發環境下的 SDL2 依賴已全部內建於 `vendor` 資料夾中，無需全域安裝即可編譯。

## 📁 專案架構 (Repository Structure)

- `src/`：C 語言原始碼。
  - `main.c`：遊戲主迴圈、回合控制、輸入處理。
  - `board.c`：棋盤與遊戲狀態的初始化 (洗牌)。
  - `rule.c`：移動與吃子的合法性驗證、遊戲結束判定。
  - `IO.c`：棋盤狀態的變更操作 (翻棋/移動/吃子)。
  - `AI.c`：Minimax AI 搜尋引擎、合法移動生成、盤面局勢評估。
  - `user_interface.c`：SDL 資源載入、畫面渲染、選單與暫停邏輯。
- `include/`：標頭檔 (`consensus.h`, `feature.h`)。
- `assets/`：遊戲圖檔 (支援 `.png` 格式的棋子與背面圖)。
- `vendor/`：(Windows 專用) 包含 SDL2 及 SDL2_image 的開發標頭檔 (`include`)、靜態庫 (`lib`) 與執行期動態連結檔 (`.dll`)。
- `docs/`：環境配置與編譯的文件說明。

## 🧠 AI 核心解構 (AI Architecture)

本專案的 AI 實作於 `src/AI.c`，主要使用 **3-Ply Minimax 演算法**，特色包含：

1. **Alpha-Beta 剪枝**：大幅降低搜尋樹的分支數量，保證流暢的遊戲體驗。
2. **動態期望值計算 (`AI_evaluateFlip`)**：將「翻棋」視為終止節點，不展開機率樹，而是動態計算尚未翻開棋子的平均權重期望值。
3. **距離與步數優化**：給予移動步數成本（紅方 -1、黑方 +1），鼓勵 AI 盡可能在最短步數內尋找優勢。
4. **盤面深拷貝**：在遞迴搜尋中，藉由拷貝 `gameState` 物件來模擬走步，確保不會干擾實際的遊戲狀態。

## 🛠️ 編譯與執行 (Build and Run)

### Windows (建議使用 MinGW)

本專案在 Windows 上已經將 SDL2 相關依賴打包於 `vendor` 目錄下。

**環境需求**：安裝 MinGW，並確保 `C:\MinGW\bin` (或 `C:\msys64\ucrt64\bin`) 已加入環境變數 `PATH`。

1. **編譯** (在專案根目錄下執行 PowerShell)：
   ```powershell
   gcc src/*.c -o C_Chess -I"include" -I"vendor/include" -L"vendor/lib" -L"vendor" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
   ```
2. **執行** (執行前需將附帶的 dll 載入環境)：
   ```powershell
   $env:Path += ";$PWD\vendor"
   .\C_Chess.exe
   ```

### macOS

**環境需求**：使用 Homebrew 安裝 SDL2 工具。

```bash
brew install sdl2 sdl2_image pkg-config
```

1. **編譯**：
   ```bash
   clang src/*.c -o C_Chess -I./include $(pkg-config --cflags --libs sdl2 SDL2_image)
   ```
2. **執行**：
   ```bash
   ./C_Chess
   ```

## 🎮 控制方式

- 遊戲啟動後，第一次翻棋的角色將決定紅黑雙方。
- **滑鼠左鍵**：點擊蓋住的棋子以進行「翻棋」，或點擊己方棋子並點擊目標位置以進行「移動 / 吃子」。
