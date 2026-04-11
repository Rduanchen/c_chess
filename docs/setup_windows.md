# Windows 環境建置與執行指南

## 前置需求

SDL2 所有標頭檔與執行時 DLL 已內建於 `vendor/`，開發者**只需安裝 MinGW (gcc)**。

---

## 編譯專案

在專案根目錄（`c_chess\`）執行：

```powershell
gcc src/*.c -o C_Chess -I"include" -I"vendor/include" -L"vendor/lib" -L"vendor" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
```

編譯成功後會在根目錄產生 `C_Chess.exe`。

---

## 執行專案

執行時需要讓系統找得到 `vendor\` 資料夾裡的 DLL，每次開新終端機都需要執行一次：

```powershell
$env:Path += ";$PWD\vendor"
.\C_Chess.exe
```

或直接把 `vendor\` 的絕對路徑加入系統 PATH（一勞永逸）：

- 搜尋「編輯系統環境變數」→「環境變數」→ 找到 `Path` → 新增 `C:\path\to\c_chess\vendor`

---

## 專案結構說明

```
c_chess/
├── assets/          # 遊戲圖片資源 (PNG)
├── docs/            # 說明文件
├── include/         # 專案標頭檔 (consensus.h, feature.h)
├── src/             # 原始碼 (.c 檔)
├── vendor/
│   ├── include/     # SDL2 開發用標頭檔 (已內建，無需另外安裝)
│   ├── lib/         # SDL2 連結庫 (已內建，無需另外安裝)
│   └── *.dll        # 執行時期 DLL 依賴
└── C_Chess.exe      # 編譯後產生的執行檔
```
