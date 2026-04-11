# C_Chess Project Instructions for AI Agents

## 1) Project Purpose

This repository is a C + SDL2 implementation of Chinese Dark Chess (Banqi) with:

- Human vs AI (current AI behavior: random flip)
- 4x8 board state management
- Rule validation and game-over detection
- SDL2 rendering and mouse input
- Menu and pause screen (RAY extension, currently partially implemented)

When making changes, prioritize gameplay correctness and keep UI behavior stable.

## 2) Repository Structure

- `src/`: all C source files
  - `main.c`: game loop, turn flow, input handling
  - `board.c`: board and game state initialization
  - `rule.c`: move/capture validity and game-over checks
  - `IO.c`: board mutations (flip/move/capture)
  - `AI.c`: AI move decision (currently random flip)
  - `user_interface.c`: SDL asset loading, board rendering, menu/pause logic (RAY)
- `include/`: project headers
  - `consensus.h`: shared structs, enums, global conventions
  - `feature.h`: public function declarations and constants
- `assets/`: required chess textures (`covered.png`, `red_1..7.png`, `blk_1..7.png`)
- `docs/`: documentation
  - `compile.md`: legacy build notes
  - `setup_windows.md`: full Windows setup guide (MinGW only, SDL2 vendored)
- `vendor/`: all runtime and build-time SDL2 dependencies (no system install needed)
  - `include/SDL2/`: SDL2 + SDL2_image development headers
  - `lib/`: SDL2 + SDL2_image `.a` static link libraries
  - `*.dll`: runtime DLLs required at execution time

## 3) Build and Run

### Windows (MinGW — recommended, SDL2 fully vendored)

**Only prerequisite:** Install MinGW and add `C:\MinGW\bin` to PATH.

Build from project root:

```powershell
gcc src/*.c -o C_Chess -I"include" -I"vendor/include" -L"vendor/lib" -L"vendor" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
```

Run (add vendor DLLs to PATH first):

```powershell
$env:Path += ";$PWD\vendor"
.\C_Chess.exe
```

See `docs/setup_windows.md` for full setup instructions.

### macOS

Dependencies:

- `brew install sdl2 sdl2_image pkg-config`

Build from project root:

```bash
clang src/*.c -o C_Chess -I./include $(pkg-config --cflags --libs sdl2 SDL2_image)
```

Run:

```bash
./C_Chess
```

## 4) Coding Conventions (Important)

- Keep function prefix conventions from `consensus.h`:
  - `UI_` for rendering/UI functions
  - `RULE_` for rule validation
  - `AI_` for AI logic
  - `BD_` for board-state initialization/update helpers
  - `IO_` for board operation execution
- New shared declarations go in `include/feature.h`.
- New shared data types or enums go in `include/consensus.h`.
- All `src/*.c` files must use `#include "../include/consensus.h"` and `#include "../include/feature.h"` (relative paths, not bare names).
- Always include `<stdio.h>` in any `.c` file that uses `printf` or `sprintf`.
- Do not break existing texture index contract:
  - `textures[0]` is covered chess
  - piece texture index = `(color - 1) * 7 + type`

## 5) Gameplay Invariants to Preserve

- Board size is fixed to `4 x 8`.
- `CHESS_COVER` means piece not flipped yet.
- `RULE_checkFirstMove` must assign player colors only once.
- Turn alternates with `current_player = (current_player + 1) % 2` only after turn action ends.
- Capture counter updates must stay consistent with board mutations (`red_left`, `black_left`).

## 6) Current Known Limitations

- AI currently only performs random flip and does not perform strategic move/capture.
- Tie logic is not fully implemented in `RULE_checkGameOver`.
- RAY's menu/pause extension (`UI_loadMenuAssets`, `UI_cleanupMenuAssets`, `UI_isInMenu`, `UI_isPaused`, `UI_handleMenuEvent`, `UI_recordMove`, `UI_drawStartMenu`, `UI_drawPauseScreen`) is declared in `feature.h` and called in `main.c`, but the implementation resides in `user_interface.c`. Ensure all declared functions always have a corresponding implementation or linker errors will occur.
- Some comments contain mixed language and typos; prioritize behavior correctness over comment cleanup.

## 7) Preferred Change Strategy for Future AI

When implementing features or fixes:

1. Read `main.c` to understand the active turn flow.
2. Change the minimal module responsible (`rule.c`, `IO.c`, `AI.c`, etc.).
3. Update declarations in `include/feature.h` if function signatures change.
4. Rebuild with the Windows command above from the project root.
5. Avoid broad refactors unless explicitly requested.

## 8) Git and Artifact Policy

- Do not commit compiled binaries or runtime DLLs.
- `.gitignore` excludes `*.exe` and should also exclude `*.dll` artifacts if added.
- `vendor/include/` and `vendor/lib/` **should be committed** — they are the vendored SDL2 dev files that replace the need for system-wide installation.

## 9) If SDL headers cannot be found

Check in this order:

1. Build command includes `-I"vendor/include"` and `-L"vendor/lib"`.
2. `vendor/include/SDL2/SDL.h` and `vendor/include/SDL2/SDL_image.h` exist in the repo.
3. VS Code C/C++ config uses `vendor/include` in `includePath`.
4. Header includes remain `#include <SDL2/SDL.h>` and `#include <SDL2/SDL_image.h>`.
