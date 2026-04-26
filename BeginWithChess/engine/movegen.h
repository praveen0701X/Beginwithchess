// ============================================================
//  movegen.h  –  BeginWithChess Engine
//
//  MoveGenerator: produces all legal moves for a position.
//
//  Two-phase approach (standard in chess engines):
//    1. Generate pseudo-legal moves (fast, ignores check)
//    2. Filter out moves that leave own king in check
//
//  The final result is a list of fully legal moves.
// ============================================================

#pragma once
#include "board.h"
#include "move.h"
#include <vector>

class MoveGenerator {
public:
    // ── Main entry point ──────────────────────────────────────
    // Returns all legal moves for the side-to-move
    static std::vector<Move> generateLegalMoves(const Board& board);

    // ── Check / stalemate / checkmate detection ───────────────
    // Returns true if the side-to-move has NO legal moves
    static bool hasNoMoves(const Board& board);

    // Is this position checkmate for side-to-move?
    static bool isCheckmate(const Board& board);

    // Is this position stalemate for side-to-move?
    static bool isStalemate(const Board& board);

private:
    // ── Pseudo-legal generators (don't check for own king safety) ──
    static void genPawnMoves   (const Board& b, int r, int c,
                                std::vector<Move>& moves);
    static void genKnightMoves (const Board& b, int r, int c,
                                std::vector<Move>& moves);
    static void genBishopMoves (const Board& b, int r, int c,
                                std::vector<Move>& moves);
    static void genRookMoves   (const Board& b, int r, int c,
                                std::vector<Move>& moves);
    static void genQueenMoves  (const Board& b, int r, int c,
                                std::vector<Move>& moves);
    static void genKingMoves   (const Board& b, int r, int c,
                                std::vector<Move>& moves);
    static void genCastlingMoves(const Board& b, int r, int c,
                                 std::vector<Move>& moves);

    // Sliding piece helper (bishop/rook/queen share this)
    static void genSlidingMoves(const Board& b, int r, int c,
                                int8_t piece,
                                const int dirs[][2], int numDirs,
                                std::vector<Move>& moves);

    // Is a move legal? (does it leave own king in check?)
    static bool isLegal(const Board& b, const Move& m);
};
