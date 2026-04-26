// ============================================================
//  evaluation.h  –  BeginWithChess Engine
//
//  The Evaluator assigns a numeric score to any board position.
//
//  Score convention:
//    + positive = GOOD for White
//    - negative = GOOD for Black
//
//  This is a STATIC evaluation (doesn't look ahead).
//  The Minimax algorithm calls this at leaf nodes.
//
//  Components:
//    1. Material count (most important)
//    2. Piece-square tables (positional bonus)
//    3. Mobility (number of legal moves available)
// ============================================================

#pragma once
#include "board.h"

class Evaluator {
public:
    // Main evaluation function
    // Returns a score in centipawns (100 = 1 pawn advantage for White)
    static int evaluate(const Board& board);

    // Individual components (for debugging)
    static int materialScore(const Board& board);
    static int positionalScore(const Board& board);

    // Piece values in centipawns
    static const int PIECE_VALUE[7]; // Index by Piece::type()
};
