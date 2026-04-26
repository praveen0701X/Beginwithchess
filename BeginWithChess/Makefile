# ============================================================
#  Makefile  –  BeginWithChess
#  Usage:
#    make         → Build C++ engine + Java GUI
#    make engine  → Build C++ engine only
#    make gui     → Compile Java GUI only
#    make run     → Run the game
#    make clean   → Remove all build artifacts
# ============================================================

# ── C++ Settings ─────────────────────────────────────────────
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra

# Source files for the engine
ENGINE_SRCS := main.cpp \
               engine/board.cpp \
               engine/movegen.cpp \
               engine/evaluation.cpp \
               engine/minimax.cpp

ENGINE_OUT := chess_engine

# ── Java Settings ─────────────────────────────────────────────
JC       := javac
JAVA     := java
JAVA_SRC := gui/ChessGUI.java
JAVA_OUT := gui/

# ── Default target: build everything ─────────────────────────
all: engine gui
	@echo ""
	@echo "✅  Build complete!"
	@echo "   Run:  make run"
	@echo ""

# ── Build C++ engine ─────────────────────────────────────────
engine:
	@echo "🔨  Compiling C++ engine..."
	$(CXX) $(CXXFLAGS) -o $(ENGINE_OUT) $(ENGINE_SRCS)
	@echo "✅  Engine built: ./$(ENGINE_OUT)"

# ── Compile Java GUI ─────────────────────────────────────────
gui:
	@echo "☕  Compiling Java GUI..."
	$(JC) -d $(JAVA_OUT) $(JAVA_SRC)
	@echo "✅  GUI compiled to gui/"

# ── Run the game ─────────────────────────────────────────────
run: all
	@echo "🚀  Starting BeginWithChess..."
	cd gui && $(JAVA) -cp . ChessGUI

# ── Clean build artifacts ─────────────────────────────────────
clean:
	@echo "🧹  Cleaning..."
	rm -f $(ENGINE_OUT) $(ENGINE_OUT).exe
	rm -f gui/*.class
	@echo "✅  Clean complete"

# ── Windows helper (compile.bat is created separately) ────────
.PHONY: all engine gui run clean
