# C_Chess Project Instructions for AI Agents

## 1) Project Purpose

This repository is a C + SDL2 implementation of Chinese Dark Chess (Banqi) with:

- Human vs AI (current AI behavior: random flip)
- 4x8 board state management
- Rule validation and game-over detection
- SDL2 rendering and mouse input

When making changes, prioritize gameplay correctness and keep UI behavior stable.

## 2) Repository Structure

- `src/`: all C source files
  - `main.c`: game loop, turn flow, input handling
  - `board.c`: board and game state initialization
  - `rule.c`: move/capture validity and game-over checks
  - `IO.c`: board mutations (flip/move/capture)
  - `AI.c`: AI move decision (currently random flip)
  - `user_interface.c`: SDL asset loading and board rendering
- `include/`: project headers
  - `consensus.h`: shared structs, enums, global conventions
  - `feature.h`: public function declarations and constants
- `assets/`: required chess textures (`covered.png`, `red_1..7.png`, `blk_1..7.png`)
- `docs/compile.md`: build instructions for macOS and Windows
- `.vscode/`: editor and task configuration
- `vendor/dll/`: local Windows runtime DLLs (ignored by git)
- `bin/`: build outputs (ignored by git)

## 3) Build and Run

### macOS

Dependencies:

- `brew install sdl2 sdl2_image pkg-config`

Build from project root:

- `mkdir -p bin`
- `clang src/*.c -o bin/C_Chess -I./include $(pkg-config --cflags --libs sdl2 SDL2_image)`

Run:

- `./bin/C_Chess`

### Windows (MSYS2 UCRT64)

Dependencies:

- `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_image`

Build from project root:

- `gcc src/*.c -o bin/C_Chess.exe -I./include -I"C:/msys64/ucrt64/include" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image`

Run:

- `./bin/C_Chess.exe`

## 4) Coding Conventions (Important)

- Keep function prefix conventions from `consensus.h`:
  - `UI_` for rendering/UI functions
  - `RULE_` for rule validation
  - `AI_` for AI logic
  - `BD_` for board-state initialization/update helpers
  - `IO_` for board operation execution
- New shared declarations should go to `include/feature.h`.
- New shared data types or enums should go to `include/consensus.h`.
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
- Some comments contain mixed language and typos; prioritize behavior correctness over comment cleanup.

## 7) Preferred Change Strategy for Future AI

When implementing features or fixes:

1. Read `main.c` to understand the active turn flow.
2. Change the minimal module responsible (`rule.c`, `IO.c`, `AI.c`, etc.).
3. Update declarations in `include/feature.h` if function signatures change.
4. Rebuild with the platform command above.
5. Avoid broad refactors unless explicitly requested.

## 8) Git and Artifact Policy

- Do not commit compiled binaries or runtime DLLs.
- Keep these ignored:
  - `bin/`, `*.exe`, `*.dll`, object/library artifacts
  - `vendor/dll/`

## 9) If SDL headers cannot be found

Check in this order:

1. Required packages are installed (SDL2 + SDL2_image).
2. Build command includes correct `-I` and linker flags.
3. VS Code C/C++ config uses the correct profile for current OS.
4. Header includes remain `#include <SDL2/SDL.h>` and `#include <SDL2/SDL_image.h>`.
