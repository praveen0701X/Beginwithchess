// ============================================================
//  board.h  –  BeginWithChess Engine
//
//  The Board struct holds the COMPLETE game state:
//    - 8×8 grid of pieces (int8_t squares[8][8])
//    - whose turn it is
//    - castling rights (4 flags)
//    - en passant target square
//    - half-move clock (for 50-move rule)
//    - full-move number
//
//  WHY a struct instead of a class?
//    Structs are simpler, have no hidden overhead, and are
//    easy to copy cheaply – which the search algorithm does
//    hundreds of times per second.
// ============================================================

#pragma once
#include "move.h"
#include <vector>
#include <string>
#include <cstring>

struct Board {
    // ── Board grid ───────────────────────────────────────────
    // squares[row][col]
    //   row 0 = rank 8 (Black's back rank)
    //   row 7 = rank 1 (White's back rank)
    //   col 0 = file a, col 7 = file h
    int8_t squares[8][8];

    // ── Game state flags ─────────────────────────────────────
    bool whiteToMove;          // true = White's turn

    bool castleWhiteKing;      // White can still castle kingside
    bool castleWhiteQueen;     // White can still castle queenside
    bool castleBlackKing;      // Black can still castle kingside
    bool castleBlackQueen;     // Black can still castle queenside

    int  enPassantCol;         // Column of en-passant target (-1 if none)
    int  enPassantRow;         // Row of en-passant target   (-1 if none)

    int  halfMoveClock;        // Moves since last pawn push or capture
    int  fullMoveNumber;       // Starts at 1, increments after Black moves

    // ── Constructor ──────────────────────────────────────────
    Board();                   // Sets up the standard starting position

    // ── Utility ──────────────────────────────────────────────
    void     setStartPosition();          // Reset to starting position
    bool     loadFEN(const std::string& fen); // Load a FEN string
    std::string toFEN() const;            // Export current state as FEN

    // Apply / undo a move (for the search tree)
    Board    applyMove(const Move& m) const;  // Returns new board state

    // Locate the king of a given color
    bool     findKing(bool white, int& row, int& col) const;

    // Print board to console (debug)
    void     print() const;

    // Check if a square is under attack by the given color
    bool     isSquareAttacked(int row, int col, bool byWhite) const;

    // Is the side-to-move's king in check?
    bool     inCheck() const;

private:
    // Helper for isSquareAttacked
    bool attackedBySlider(int row, int col, bool byWhite,
                          int dr, int dc, int sliderType) const;
};
