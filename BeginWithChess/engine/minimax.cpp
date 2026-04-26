// ============================================================
//  minimax.cpp  –  BeginWithChess Engine
//
//  Implementation of Minimax with Alpha-Beta Pruning.
//
//  HOW THE ALGORITHM WORKS (step by step):
//
//  alphaBeta(board, depth, alpha, beta, maximizing):
//
//  BASE CASE (depth == 0 or game over):
//    → Return static evaluation of the board
//
//  RECURSIVE CASE:
//    If maximizing (White's turn):
//      best = -INFINITY
//      For each legal move:
//        make move → get child board
//        score = alphaBeta(child, depth-1, alpha, beta, FALSE)
//        best = max(best, score)
//        alpha = max(alpha, score)    ← update lower bound
//        if beta ≤ alpha: BREAK       ← PRUNE! Black won't allow this
//      return best
//
//    If minimizing (Black's turn):
//      best = +INFINITY
//      For each legal move:
//        make move → get child board
//        score = alphaBeta(child, depth-1, alpha, beta, TRUE)
//        best = min(best, score)
//        beta = min(beta, score)      ← update upper bound
//        if beta ≤ alpha: BREAK       ← PRUNE! White won't allow this
//      return best
// ============================================================

#include "minimax.h"
#include "movegen.h"
#include "evaluation.h"
#include <algorithm>
#include <limits>
#include <iostream>

// Large sentinel values for infinity
static constexpr int POS_INF = 1'000'000;
static constexpr int NEG_INF = -1'000'000;

// Checkmate score (high but not infinity, so shorter mates rank higher)
static constexpr int CHECKMATE_SCORE = 900'000;

// ── Move ordering score ───────────────────────────────────────
// We search captures first because they often cause cutoffs.
// MVV-LVA: Most Valuable Victim - Least Valuable Aggressor
//   (capture a queen with a pawn = very good → search first)
int Search::moveOrderScore(const Move& m) {
    if (m.captured != Piece::EMPTY) {
        int victimValue   = Evaluator::PIECE_VALUE[Piece::type(m.captured)];
        int attackerValue = Evaluator::PIECE_VALUE[Piece::type(m.piece)];
        // Bonus: victim value minus attacker value (winning capture)
        return 10 * victimValue - attackerValue;
    }
    if (m.type == MoveType::PROMOTION)
        return 5000; // Promotions are usually good
    return 0;
}

// ── Sort moves: best-first for better pruning ─────────────────
void Search::orderMoves(std::vector<Move>& moves, const Board& board) {
    std::sort(moves.begin(), moves.end(),
        [](const Move& a, const Move& b) {
            return moveOrderScore(a) > moveOrderScore(b);
        });
}

// ── Alpha-Beta Minimax (recursive) ───────────────────────────
// Returns the score of the best move available from this position.
// alpha = best score White can guarantee so far (starts at -INF)
// beta  = best score Black can guarantee so far (starts at +INF)
int Search::alphaBeta(const Board& board, int depth,
                      int alpha, int beta, bool maximizing,
                      int& nodes) {
    nodes++;

    // ── Base case: reached leaf node ─────────────────────────
    if (depth == 0) {
        return Evaluator::evaluate(board);
    }

    // ── Generate all legal moves ──────────────────────────────
    std::vector<Move> moves = MoveGenerator::generateLegalMoves(board);

    // ── Terminal positions ────────────────────────────────────
    if (moves.empty()) {
        if (board.inCheck()) {
            // Checkmate: the current side has lost.
            // Negate by depth so the engine prefers FASTER mates.
            return maximizing
                ? (NEG_INF + (1000 - depth))   // White is mated → bad for White
                : (POS_INF - (1000 - depth));   // Black is mated → bad for Black
        } else {
            return 0; // Stalemate = draw = 0
        }
    }

    // ── Order moves for better pruning ────────────────────────
    orderMoves(moves, board);

    if (maximizing) {
        // ── MAXIMIZING NODE (White's turn) ────────────────────
        int bestScore = NEG_INF;

        for (const Move& m : moves) {
            Board next = board.applyMove(m);
            int   score = alphaBeta(next, depth - 1, alpha, beta, false, nodes);

            bestScore = std::max(bestScore, score);
            alpha     = std::max(alpha, score);

            // ── Alpha-Beta CUTOFF ──────────────────────────────
            // If beta ≤ alpha: Black already has a better option elsewhere.
            // Searching further here is pointless → PRUNE (break).
            if (beta <= alpha) {
                break; // Beta cutoff
            }
        }
        return bestScore;

    } else {
        // ── MINIMIZING NODE (Black's turn) ────────────────────
        int bestScore = POS_INF;

        for (const Move& m : moves) {
            Board next = board.applyMove(m);
            int   score = alphaBeta(next, depth - 1, alpha, beta, true, nodes);

            bestScore = std::min(bestScore, score);
            beta      = std::min(beta, score);

            // ── Alpha-Beta CUTOFF ──────────────────────────────
            // If beta ≤ alpha: White already has a better option elsewhere.
            // → PRUNE (break).
            if (beta <= alpha) {
                break; // Alpha cutoff
            }
        }
        return bestScore;
    }
}

// ── Top-level search: find the best move ─────────────────────
// This is a special case of alphaBeta at the root.
// We need to track WHICH move produced the best score.
SearchResult Search::findBestMove(const Board& board, int depth) {
    SearchResult result;
    result.bestMove     = NULL_MOVE;
    result.score        = 0;
    result.nodesSearched = 0;

    std::vector<Move> moves = MoveGenerator::generateLegalMoves(board);
    if (moves.empty()) return result; // No moves (checkmate/stalemate)

    orderMoves(moves, board);

    bool  maximizing = board.whiteToMove;
    int   bestScore  = maximizing ? NEG_INF : POS_INF;
    int   alpha      = NEG_INF;
    int   beta       = POS_INF;

    for (const Move& m : moves) {
        Board next  = board.applyMove(m);
        int   score = alphaBeta(next, depth - 1, alpha, beta,
                                !maximizing, result.nodesSearched);

        // ── Pick the move with the best score ─────────────────
        if (maximizing && score > bestScore) {
            bestScore       = score;
            result.bestMove = m;
            alpha           = std::max(alpha, score);
        } else if (!maximizing && score < bestScore) {
            bestScore       = score;
            result.bestMove = m;
            beta            = std::min(beta, score);
        }
    }

    result.score = bestScore;

    // Debug output (shows search statistics)
    std::cerr << "[Search] depth=" << depth
              << " nodes=" << result.nodesSearched
              << " score=" << result.score << "\n";

    return result;
}
