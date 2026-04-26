// ============================================================
//  evaluation.cpp  –  BeginWithChess Engine
//
//  Heuristic evaluation of a board position.
//
//  PIECE VALUES (in centipawns, 100 = 1 pawn):
//    Pawn   = 100
//    Knight = 320
//    Bishop = 330
//    Rook   = 500
//    Queen  = 900
//    King   = 20000 (very high to prevent trading the king)
//
//  PIECE-SQUARE TABLES:
//    These are 8×8 tables that give a bonus or penalty based
//    on WHERE a piece is on the board. For example, a knight
//    in the center is worth more than a knight in the corner.
//    (Standard tables used by most beginner chess engines)
// ============================================================

#include "evaluation.h"
#include <cstdlib>

// ── Piece values in centipawns ────────────────────────────────
const int Evaluator::PIECE_VALUE[7] = {
    0,      // EMPTY (index 0)
    100,    // PAWN
    320,    // KNIGHT
    330,    // BISHOP
    500,    // ROOK
    900,    // QUEEN
    20000   // KING
};

// ── Piece-Square Tables ───────────────────────────────────────
// These tables represent the positional bonus/penalty for each
// square. Positive = good position. They're from White's POV;
// for Black we mirror the table vertically.

// Pawn table: encourage center control and advancement
static const int PAWN_TABLE[8][8] = {
    { 0,  0,  0,  0,  0,  0,  0,  0},
    {50, 50, 50, 50, 50, 50, 50, 50},
    {10, 10, 20, 30, 30, 20, 10, 10},
    { 5,  5, 10, 25, 25, 10,  5,  5},
    { 0,  0,  0, 20, 20,  0,  0,  0},
    { 5, -5,-10,  0,  0,-10, -5,  5},
    { 5, 10, 10,-20,-20, 10, 10,  5},
    { 0,  0,  0,  0,  0,  0,  0,  0}
};

// Knight table: knights are better in the center, bad on edges
static const int KNIGHT_TABLE[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  0,  0,  0,-20,-40},
    {-30,  0, 10, 15, 15, 10,  0,-30},
    {-30,  5, 15, 20, 20, 15,  5,-30},
    {-30,  0, 15, 20, 20, 15,  0,-30},
    {-30,  5, 10, 15, 15, 10,  5,-30},
    {-40,-20,  0,  5,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-40,-50}
};

// Bishop table: bishops prefer long diagonals
static const int BISHOP_TABLE[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5, 10, 10,  5,  0,-10},
    {-10,  5,  5, 10, 10,  5,  5,-10},
    {-10,  0, 10, 10, 10, 10,  0,-10},
    {-10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  5,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-20}
};

// Rook table: rooks like open files and the 7th rank
static const int ROOK_TABLE[8][8] = {
    { 0,  0,  0,  0,  0,  0,  0,  0},
    { 5, 10, 10, 10, 10, 10, 10,  5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    {-5,  0,  0,  0,  0,  0,  0, -5},
    { 0,  0,  0,  5,  5,  0,  0,  0}
};

// Queen table: queen prefers center, avoids early development
static const int QUEEN_TABLE[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20},
    {-10,  0,  0,  0,  0,  0,  0,-10},
    {-10,  0,  5,  5,  5,  5,  0,-10},
    { -5,  0,  5,  5,  5,  5,  0, -5},
    {  0,  0,  5,  5,  5,  5,  0, -5},
    {-10,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  5,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5,-10,-10,-20}
};

// King table (middlegame): king should stay safe and castled
static const int KING_TABLE[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-40,-40,-30},
    {-20,-30,-30,-40,-40,-30,-30,-20},
    {-10,-20,-20,-20,-20,-20,-20,-10},
    { 20, 20,  0,  0,  0,  0, 20, 20},
    { 20, 30, 10,  0,  0, 10, 30, 20}
};

// ── Get positional bonus for a piece at a given square ────────
static int getPieceSquareBonus(int8_t piece, int row, int col) {
    bool white = Piece::isWhite(piece);
    int  type  = Piece::type(piece);

    // Mirror the table for Black (Black's row 0 = White's row 7)
    int r = white ? row : (7 - row);

    switch (type) {
        case 1 /*PAWN*/:   return PAWN_TABLE[r][col];
        case 2 /*KNIGHT*/: return KNIGHT_TABLE[r][col];
        case 3 /*BISHOP*/: return BISHOP_TABLE[r][col];
        case 4 /*ROOK*/:   return ROOK_TABLE[r][col];
        case 5 /*QUEEN*/:  return QUEEN_TABLE[r][col];
        case 6 /*KING*/:   return KING_TABLE[r][col];
        default:           return 0;
    }
}

// ── Material score ────────────────────────────────────────────
// Sum up all piece values, positive for White, negative for Black
int Evaluator::materialScore(const Board& board) {
    int score = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            int8_t p = board.squares[r][c];
            if (p == Piece::EMPTY) continue;
            int val = PIECE_VALUE[Piece::type(p)];
            score += Piece::isWhite(p) ? +val : -val;
        }
    return score;
}

// ── Positional score ──────────────────────────────────────────
// Add piece-square bonuses
int Evaluator::positionalScore(const Board& board) {
    int score = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            int8_t p = board.squares[r][c];
            if (p == Piece::EMPTY) continue;
            int bonus = getPieceSquareBonus(p, r, c);
            score += Piece::isWhite(p) ? +bonus : -bonus;
        }
    return score;
}

// ── Main evaluation function ──────────────────────────────────
// Combines material + positional score.
// Returns a number in centipawns:
//   +500 = White is up a rook
//   -300 = Black is up a bishop
int Evaluator::evaluate(const Board& board) {
    // Material is the most important factor
    int score = materialScore(board);

    // Positional bonuses add strategic nuance
    score += positionalScore(board);

    return score;
}
