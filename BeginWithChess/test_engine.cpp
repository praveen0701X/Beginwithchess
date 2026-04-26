// ============================================================
//  test_engine.cpp  –  BeginWithChess Test Suite
//
//  Compile:  g++ -std=c++17 -O2 test_engine.cpp
//            engine/board.cpp engine/movegen.cpp
//            engine/evaluation.cpp engine/minimax.cpp
//            -o test_engine
//  Run:      ./test_engine
// ============================================================

#include <iostream>
#include <cassert>
#include <string>
#include "engine/board.h"
#include "engine/movegen.h"
#include "engine/evaluation.h"
#include "engine/minimax.h"

// Simple test framework
int passed = 0, failed = 0;
#define EXPECT(cond, msg) \
    if (cond) { std::cout << "  PASS: " << msg << "\n"; passed++; } \
    else       { std::cout << "  FAIL: " << msg << "\n"; failed++; }

// ── Move count helper ─────────────────────────────────────────
int countMoves(const std::string& fen) {
    Board b; b.loadFEN(fen);
    return MoveGenerator::generateLegalMoves(b).size();
}

// ── Test 1: Starting Position ─────────────────────────────────
void testStartingPosition() {
    std::cout << "\n[Test 1] Starting Position\n";
    Board b;
    b.setStartPosition();

    // White pieces
    EXPECT(b.squares[7][4] ==  Piece::W_KING,   "White king on e1");
    EXPECT(b.squares[7][3] ==  Piece::W_QUEEN,  "White queen on d1");
    EXPECT(b.squares[7][0] ==  Piece::W_ROOK,   "White rook on a1");
    EXPECT(b.squares[6][0] ==  Piece::W_PAWN,   "White pawn on a2");

    // Black pieces
    EXPECT(b.squares[0][4] ==  Piece::B_KING,   "Black king on e8");
    EXPECT(b.squares[0][3] ==  Piece::B_QUEEN,  "Black queen on d8");
    EXPECT(b.squares[1][7] ==  Piece::B_PAWN,   "Black pawn on h7");

    // Game state
    EXPECT(b.whiteToMove,                        "White to move");
    EXPECT(b.castleWhiteKing,                    "White can castle kingside");
    EXPECT(b.castleWhiteQueen,                   "White can castle queenside");
    EXPECT(b.enPassantCol == -1,                 "No en passant initially");
}

// ── Test 2: Move Count (Perft) ────────────────────────────────
// These are known correct values for move generation accuracy
void testMoveCount() {
    std::cout << "\n[Test 2] Move Count Verification\n";

    // Starting position: 20 legal moves
    Board start;
    int moves = MoveGenerator::generateLegalMoves(start).size();
    EXPECT(moves == 20, "Start position: 20 legal moves (got " + std::to_string(moves) + ")");

    // After 1.e4: Black also has 20 moves
    Board after_e4 = start.applyMove(Move(6,4,4,4,Piece::W_PAWN));
    int black_moves = MoveGenerator::generateLegalMoves(after_e4).size();
    EXPECT(black_moves == 20, "After 1.e4: 20 legal moves for Black (got " + std::to_string(black_moves) + ")");
}

// ── Test 3: Check Detection ───────────────────────────────────
void testCheckDetection() {
    std::cout << "\n[Test 3] Check Detection\n";

    // Scholar's Mate position (Black is in checkmate)
    Board b;
    b.loadFEN("r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4");
    EXPECT(b.inCheck(),                      "King is in check after Qxf7+");
    EXPECT(MoveGenerator::isCheckmate(b),    "Position is checkmate");
    EXPECT(!MoveGenerator::isStalemate(b),   "Not stalemate");
    EXPECT(MoveGenerator::generateLegalMoves(b).size() == 0,
                                             "No legal moves in checkmate");
}

// ── Test 4: Stalemate Detection ───────────────────────────────
void testStalemateDetection() {
    std::cout << "\n[Test 4] Stalemate Detection\n";

    // Classic stalemate: Black king on a8, White queen on b6, White king on c6
    Board b;
    b.loadFEN("k7/8/1QK5/8/8/8/8/8 b - - 0 1");
    EXPECT(!b.inCheck(),                     "Black not in check");
    EXPECT(MoveGenerator::isStalemate(b),    "Position is stalemate");
    EXPECT(MoveGenerator::generateLegalMoves(b).size() == 0,
                                             "No legal moves in stalemate");
}

// ── Test 5: Castling ─────────────────────────────────────────
void testCastling() {
    std::cout << "\n[Test 5] Castling\n";

    // Position where White can castle both sides
    Board b;
    b.loadFEN("rnbqk2r/pppp1ppp/5n2/2b1p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4");

    // White can castle kingside
    auto moves = MoveGenerator::generateLegalMoves(b);
    bool foundCastleK = false;
    for (auto& m : moves)
        if (m.type == MoveType::CASTLING_K) foundCastleK = true;
    EXPECT(foundCastleK, "White can castle kingside");

    // Apply castling and verify rook moved
    for (auto& m : moves) {
        if (m.type == MoveType::CASTLING_K) {
            Board after = b.applyMove(m);
            EXPECT(after.squares[7][6] == Piece::W_KING, "King on g1 after kingside castle");
            EXPECT(after.squares[7][5] == Piece::W_ROOK, "Rook on f1 after kingside castle");
            EXPECT(after.squares[7][4] == Piece::EMPTY,  "e1 empty after castling");
            EXPECT(after.squares[7][7] == Piece::EMPTY,  "h1 empty after castling");
            break;
        }
    }
}

// ── Test 6: En Passant ────────────────────────────────────────
void testEnPassant() {
    std::cout << "\n[Test 6] En Passant\n";

    // White pawn on e5, Black pawn just double-pushed to f5
    Board b;
    b.loadFEN("rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");

    auto moves = MoveGenerator::generateLegalMoves(b);
    bool foundEP = false;
    for (auto& m : moves)
        if (m.type == MoveType::EN_PASSANT) foundEP = true;
    EXPECT(foundEP, "En passant capture available");

    // Apply en passant and verify captured pawn removed
    for (auto& m : moves) {
        if (m.type == MoveType::EN_PASSANT) {
            Board after = b.applyMove(m);
            EXPECT(after.squares[2][5] == Piece::W_PAWN, "White pawn on f6 after en passant");
            EXPECT(after.squares[3][5] == Piece::EMPTY,  "Black pawn captured");
            break;
        }
    }
}

// ── Test 7: Evaluation ────────────────────────────────────────
void testEvaluation() {
    std::cout << "\n[Test 7] Evaluation Function\n";

    Board start;
    int score = Evaluator::evaluate(start);
    EXPECT(score == 0 || (score > -50 && score < 50),
                                             "Starting position score near 0");

    // White up a queen
    Board b;
    b.loadFEN("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // Black missing queen
    int s = Evaluator::evaluate(b);
    EXPECT(s > 700, "White ahead when Black missing queen (score=" + std::to_string(s) + ")");
}

// ── Test 8: AI Search ─────────────────────────────────────────
void testAISearch() {
    std::cout << "\n[Test 8] AI Search\n";

    // AI should not return a null move from starting position
    Board start;
    SearchResult r = Search::findBestMove(start, 3);
    EXPECT(r.bestMove != NULL_MOVE,          "AI finds a move at depth 3");
    EXPECT(r.nodesSearched > 0,              "AI searched some nodes");

    // White queen e1, Black queen e5 on same file → White should capture
    Board b;
    b.loadFEN("4k3/8/8/4q3/8/8/8/4QK2 w - - 0 1");
    SearchResult r2 = Search::findBestMove(b, 2);
    bool capturesQueen = (r2.bestMove.toRow == 3 && r2.bestMove.toCol == 4);
    EXPECT(capturesQueen, "AI captures free Black queen on e5 (row="
           + std::to_string(r2.bestMove.toRow) + " col="
           + std::to_string(r2.bestMove.toCol) + ")");

    // Score should be positive when White has material advantage
    Board mat;
    mat.loadFEN("4k3/8/8/8/8/8/PPPPPPPP/4K3 w - - 0 1"); // White has 8 extra pawns
    SearchResult r3 = Search::findBestMove(mat, 2);
    EXPECT(r3.score > 0, "AI score positive when White has material advantage");
}

// ── Main ──────────────────────────────────────────────────────
int main() {
    std::cout << "============================================\n";
    std::cout << "  BeginWithChess – Engine Test Suite\n";
    std::cout << "============================================\n";

    testStartingPosition();
    testMoveCount();
    testCheckDetection();
    testStalemateDetection();
    testCastling();
    testEnPassant();
    testEvaluation();
    testAISearch();

    std::cout << "\n============================================\n";
    std::cout << "  Results: " << passed << " passed, "
              << failed << " failed\n";
    std::cout << "============================================\n";

    return failed > 0 ? 1 : 0;
}
