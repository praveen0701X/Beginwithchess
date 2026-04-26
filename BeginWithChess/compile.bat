@echo off
setlocal

echo ===========================================
echo  BeginWithChess - Build Script (Windows)
echo ===========================================

REM ── Check g++ is available ──────────────────────────────────
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: g++ not found!
    echo Please install MinGW-w64 and add it to PATH.
    echo Download from: https://winlibs.com
    echo.
    pause
    exit /b 1
)

REM ── Check javac is available ────────────────────────────────
where javac >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: javac not found!
    echo Please install JDK 11 or later.
    echo Download from: https://adoptium.net
    echo.
    pause
    exit /b 1
)

REM ── Step 1: Compile C++ engine (all on one line) ────────────
echo.
echo [1/3] Compiling C++ chess engine...
g++ -std=c++17 -O2 -o chess_engine.exe main.cpp engine\board.cpp engine\movegen.cpp engine\evaluation.cpp engine\minimax.cpp

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: C++ compilation failed!
    echo Check the error messages above.
    echo.
    pause
    exit /b 1
)
echo    OK - chess_engine.exe created

REM ── Step 2: Compile Java GUI ────────────────────────────────
echo.
echo [2/3] Compiling Java GUI...
javac -d gui gui\ChessGUI.java

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Java compilation failed!
    echo Check the error messages above.
    echo.
    pause
    exit /b 1
)
echo    OK - Java classes compiled

REM ── Step 3: Copy engine into gui folder ─────────────────────
echo.
echo [3/3] Copying engine to gui folder...
copy /Y chess_engine.exe gui\chess_engine.exe >nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Could not copy chess_engine.exe to gui folder.
    pause
    exit /b 1
)
echo    OK - chess_engine.exe copied to gui\

REM ── Launch the game ─────────────────────────────────────────
echo.
echo ===========================================
echo  Build successful! Launching game...
echo ===========================================
echo.

cd gui
java -cp . ChessGUI

endlocal
