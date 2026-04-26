#!/bin/bash
# ============================================================
#  compile.sh  –  BeginWithChess  (Linux / macOS)
#  Usage:  chmod +x compile.sh && ./compile.sh
# ============================================================

set -e  # Exit on any error

echo "==========================================="
echo " BeginWithChess - Build Script (Linux/Mac)"
echo "==========================================="

# ── Step 1: Compile C++ engine ──────────────────────────────
echo ""
echo "[1/3] Compiling C++ chess engine..."
g++ -std=c++17 -O2 -Wall \
    main.cpp \
    engine/board.cpp \
    engine/movegen.cpp \
    engine/evaluation.cpp \
    engine/minimax.cpp \
    -o chess_engine

chmod +x chess_engine
echo "   OK - ./chess_engine built"

# ── Step 2: Compile Java GUI ─────────────────────────────────
echo ""
echo "[2/3] Compiling Java GUI..."
javac -d gui/ gui/ChessGUI.java
echo "   OK - Java classes compiled to gui/"

# ── Step 3: Copy engine to gui folder ────────────────────────
echo ""
echo "[3/3] Setting up..."
cp chess_engine gui/chess_engine
echo "   OK"

echo ""
echo "==========================================="
echo " Build complete! Launching game..."
echo "==========================================="
echo ""

# Launch the GUI from the gui/ folder
cd gui
java -cp . ChessGUI
