# BeginWithChess — Chess Coaching Engine for Beginners

## Project Overview

**BeginWithChess** is a chess game where a human plays against an AI, with a built-in coaching system. The AI explains every move in plain English, detects blunders, and lets you undo mistakes to learn.

### Core DAA Algorithms
- **Minimax** game tree search
- **Alpha-Beta pruning** optimization
- **Heuristic evaluation** with piece-square tables
- **Move ordering** via MVV-LVA

### Coaching Features (NEW)
- **Move Explanations** — Every move comes with a plain-English explanation
- **Blunder Detection** — Detects blunders, mistakes, and inaccuracies
- **Undo / Takeback** — Take back moves to try a different approach
- **Evaluation Bar** — Visual bar showing who is winning
- **Coach Panel** — Dedicated panel with tips and feedback

| Component | Tech |
|-----------|------|
| Engine | C++17 |
| GUI | Java Swing |
| Communication | stdin/stdout pipes |

---

## Project Structure

```
BeginWithChess/
├── main.cpp                    ← Engine CLI + coaching logic
├── engine/
│   ├── move.h                  ← Move struct & Piece constants
│   ├── board.h / board.cpp     ← Board state & FEN
│   ├── movegen.h / movegen.cpp ← Legal move generation
│   ├── evaluation.h / .cpp     ← Static evaluation
│   └── minimax.h / minimax.cpp ← Alpha-Beta search
├── gui/
│   └── ChessGUI.java           ← Redesigned GUI with coaching
├── compile.sh / compile.bat    ← Build scripts
└── README.md
```

---

## How to Compile and Run

### Prerequisites
- g++ (GCC 9+) with C++17
- JDK 11+

### Linux / macOS
```bash
g++ -std=c++17 -O2 -o chess_engine \
    main.cpp engine/board.cpp engine/movegen.cpp \
    engine/evaluation.cpp engine/minimax.cpp
javac -d gui/ gui/ChessGUI.java
cp chess_engine gui/
cd gui && java -cp . ChessGUI
```

### Windows
```batch
compile.bat
```

---

## Engine Protocol

| Command | Response |
|---------|----------|
| `NEW` | `OK <FEN>` |
| `MOVE e2e4` | `OK <FEN> EVAL <n> FEEDBACK <text> EXPLAIN <text>` |
| `AI 3` | `BESTMOVE <move> <FEN> SCORE <n> NODES <n> EXPLAIN <text>` |
| `EVAL` | `EVAL <score>` |
| `UNDO` | `OK <FEN>` or `ERROR Nothing to undo` |
| `STATUS` | `STATUS NORMAL/CHECK/CHECKMATE/STALEMATE` |
| `LEGAL e2` | `LEGAL e3 e4` |
| `QUIT` | *(process ends)* |

---

## Blunder Detection Algorithm

```
delta = eval_before - eval_after  (adjusted for color)

delta >= 300  →  BLUNDER
delta >= 150  →  Mistake
delta >= 80   →  Inaccuracy
delta <= -150 →  Excellent move
delta <= -50  →  Good move
```

---

## Architecture

```
┌──────────────────────────────────────────────────┐
│              Java GUI (ChessGUI.java)             │
│  ┌────────┐ ┌──────────┐ ┌───────┐ ┌──────────┐ │
│  │EvalBar │ │BoardPanel│ │ Coach │ │ Controls │ │
│  └────────┘ └──────────┘ └───┬───┘ └──────────┘ │
└──────────────────────────────┼────────────────────┘
                               │ stdin/stdout
┌──────────────────────────────┼────────────────────┐
│            C++ Engine (main.cpp)                   │
│  ┌──────┐ ┌───────┐ ┌──────┴─────┐ ┌──────────┐ │
│  │Board │ │MoveGen│ │  Search    │ │ Explain  │ │
│  │      │ │       │ │ (Minimax + │ │ & Coach  │ │
│  │      │ │       │ │ AlphaBeta) │ │          │ │
│  └──────┘ └───────┘ └─────┬──────┘ └──────────┘ │
│              ┌─────────────┴──────────────────┐   │
│              │  Evaluator (material + PST)    │   │
│              └────────────────────────────────┘   │
└───────────────────────────────────────────────────┘
```
