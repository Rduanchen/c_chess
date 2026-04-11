@echo off
:: 1. Set GCC compiler path (Ensure mingw64 is located at C:\mingw64)
SET "PATH=%PATH%;C:\mingw64\bin"

:: 2. Change directory to where this script is located
cd /d "%~dp0"

echo [1/3] Compiling source files...
:: 3. Execute the compilation command
gcc src/*.c -o C_Chess -I"include" -I"vendor/include" -L"vendor/lib" -L"vendor" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image

:: Check if the compilation was successful
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed. Please check your source code or GCC settings.
    pause
    exit /b
)

echo [2/3] Setting up runtime environment...
:: 4. Add the vendor folder to PATH so the OS can find SDL2.dll
SET "PATH=%PATH%;%~dp0vendor"

echo [3/3] Launching the program...
echo.
:: 5. Run the executable
C_Chess.exe

pause