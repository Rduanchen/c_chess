macOS setup

1. Install dependencies:

```
brew install sdl2 sdl2_image pkg-config
```

2. Compile from project root:

```
mkdir -p bin
clang src/*.c -o bin/C_Chess -I./include $(pkg-config --cflags --libs sdl2 SDL2_image)
```

3. Run:

```
./bin/C_Chess
```

Windows (MSYS2 UCRT64) setup

1. Install dependencies in MSYS2 UCRT64 shell:

```
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-SDL2_image
```

2. In PowerShell, add MSYS2 toolchain to PATH:

```
$env:Path += ";C:\msys64\ucrt64\bin"
```

3. Compile from project root:

```
mkdir bin
gcc src/*.c -o bin/C_Chess.exe -I./include -I"C:/msys64/ucrt64/include" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
```

4. Run:

```
./bin/C_Chess.exe
```
