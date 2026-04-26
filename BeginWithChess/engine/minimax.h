// ============================================================
//  minimax.h  –  BeginWithChess Engine
//
//  The Search engine: finds the best move using Minimax
//  with Alpha-Beta pruning.
//
//  ── What is Minimax? ─────────────────────────────────────────
//  Minimax is a recursive algorithm that explores a game tree.
//  At each node, the "maximizing player" (White) picks the
//  move that gives the HIGHEST score, while the "minimizing
//  player" (Black) picks the move with the LOWEST score.
//
//  Game tree example (depth 2):
//
//         [ROOT: White to move]
//          +-------+-------+
//         [A]     [B]     [C]     <- Black responses
//        +--+    +--+    +--+
//       +3  +1  -2  +4  +0  +5   <- Static evaluations
//
//  White picks the move with best (max) score:
//    A → Black picks min(3,1) = 1
//    B → Black picks min(-2,4) = -2
//    C → Black picks min(0,5) = 0
//  White picks max(1,-2,0) = 1 → move A
//
//  ── What is Alpha-Beta? ──────────────────────────────────────
//  Alpha-Beta pruning skips subtrees that CANNOT affect the
//  result. It maintains two values:
//    alpha = best score White is GUARANTEED (lower bound)
//    beta  = best score Black is GUARANTEED (upper bound)
//
//  When beta ≤ alpha, we "prune" (skip the remaining siblings)
//  because neither player would choose this branch.
//
//  Alpha-Beta achieves the SAME result as Minimax but can
//  skip ~50% of nodes in practice, allowing deeper search.
// ============================================================

#pragma once
#include "board.h"
#include "move.h"

struct SearchResult {
    Move  bestMove;
    int   score;
    int   nodesSearched;
};

class Search {
public:
    // ── Find the best move for the current side ───────────────
    // depth: how many half-moves (plies) to look ahead
    //   depth 1 = look at all replies to current moves
    //   depth 3 = typical easy AI
    //   depth 5 = moderate AI (may take a few seconds)
    static SearchResult findBestMove(const Board& board, int depth);

private:
    // ── Core Alpha-Beta Minimax ───────────────────────────────
    // alpha: best score the maximizer (White) can guarantee
    // beta:  best score the minimizer (Black) can guarantee
    // maximizing: true if it's White's turn (we want max score)
    static int alphaBeta(const Board& board, int depth,
                         int alpha, int beta, bool maximizing,
                         int& nodes);

    // ── Move ordering ─────────────────────────────────────────
    // Order moves to improve pruning efficiency:
    //   1. Captures (especially winning captures) first
    //   2. Other moves after
    // Better move ordering → more pruning → deeper effective search
    static void orderMoves(std::vector<Move>& moves, const Board& board);

    // Score a move for ordering purposes (higher = search first)
    static int moveOrderScore(const Move& m);
};
