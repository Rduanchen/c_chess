@echo off
:: 1. Set GCC compiler path (Ensure mingw64 is located at C:\mingw64)
SET "PATH=%PATH%;C:\mingw64\bin"

:: 2. Change directory to where this script is located
cd /d "%~dp0"

gcc src/*.c -o C_Chess -I"include" -I"vendor/include" -L"vendor/lib" -L"vendor" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image

pause