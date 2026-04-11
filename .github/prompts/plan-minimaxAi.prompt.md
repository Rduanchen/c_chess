## Plan: 象棋暗棋 Minimax AI 實作計畫

**TL;DR**：使用帶有 Alpha-Beta 剪枝的 3-Ply Minimax 搜尋，並結合動態查表機率評分「翻棋期望值」，取代現有的隨機 AI。此計畫高度可行，且與既有系統相容。

**Steps**

1. **實作狀態模擬與走法生成 (Move Generation)**
   - 實作 `AI_cloneGameState()` 快速拷貝目前的 `gameState`。
   - 實作 `AI_getAllValidMoves()`，找出當前玩家所有合法移動 (上下左右、炮跳吃)。同時處理 `CHESS_COVER` 作為唯一的合法「翻棋動作」。
   - **特殊處理**：若 AI 代表色尚未決定 (即雙方皆 `COLOR_NONE`)，強制只生成並回傳「翻棋」動作。
2. **實作審局函數 (Heuristic Evaluation)**
   - 實作 `AI_evaluateBoard()`，依照圖片定義的確切權重加總計算：
     - **紅方（防守）**：帥(100)、仕(90)、相(50)、俥(30)、傌(10)、炮(70)、兵(5)。移動成本：每步 -1。
     - **黑方（進攻）**：將(-100)、士(-90)、象(-50)、車(-30)、馬(-10)、包(-70)、卒(-5)。移動成本：每步 +1。
   - 實作 `AI_evaluateFlip()`：計算盤面上**還沒翻出**的剩餘棋子權重平均，乘上剩餘棋子總數，得出該次「翻棋」的動態期望分數。
3. **實作 Minimax 搜尋引擎 (加入 Alpha-Beta 剪枝)**
   - 實作遞迴函式 `AI_minimax()`，包含深度控制 (Depth = 3) 與極大極小層運算。
   - 取消之前單獨提及的移動成本計數（改合併至上面統一處理）。
   - **Alpha-Beta 剪枝**：根據 Alpha 與 Beta 閥值直接截斷無效分枝，確保遊戲效能。
4. **整合與替換原 AI**
   - 建立 `AI_getBestAction()` 主入口，傳入到 `main.c` 替代原先的隨機翻棋。
   - 解析 Minimax 回傳的最佳走步，轉化為相容遊戲引擎的 `ActionPos` 結構。

**Relevant files**

- `src/AI.c` — 新增所有的演算法與移動生成邏輯 (`AI_evaluateBoard`, `AI_minimax`, `AI_getAllValidMoves`, `AI_getBestAction`)
- `include/feature.h` — 宣告 AI 原型函式 `AI_getBestAction` 以供全域使用。

**Verification**

1. 編譯確保無語法與連結錯誤。
2. 開啟遊戲確認 AI 在開局時能正確翻開棋子並決定顏色。
3. 觀察 AI 在 3 步內是否會主動選擇高價值捕獲（如以俥吃卒）而非無意義翻棋或移動。
4. 核驗 AI 遇到必殺局或危險局時，有加入步數懲罰使其盡可能躲避。

**Decisions**

- AI 首步未定顏色前維持 100% 翻棋策略。
- 翻棋的評估採「動態未翻開棋子平均權重」取代您原本設定的固定中值。
- Minimax 的走法生成優先調用既有的 `RULE_isValidMove` 以保證遊戲邏輯無瑕疵。
