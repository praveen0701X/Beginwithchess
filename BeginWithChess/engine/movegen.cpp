// ============================================================
//  movegen.cpp  –  BeginWithChess Engine
//
//  Implements legal move generation for all piece types.
//
//  STEPS FOR EACH PIECE:
//    1. Determine target squares
//    2. Skip squares occupied by friendly pieces
//    3. Validate board boundaries
//    4. Handle special rules (en passant, castling, promotion)
//    5. After generating all pseudo-legal moves, filter out
//       any that leave the own king in check
// ============================================================

#include "movegen.h"
#include <algorithm>

// ── Legality filter: make the move, check if own king in check ──
bool MoveGenerator::isLegal(const Board& b, const Move& m) {
    Board next = b.applyMove(m);
    // After the move it's the opponent's turn, so we check
    // if the previous side's king is now attacked
    int kr, kc;
    next.findKing(b.whiteToMove, kr, kc);
    // Is our king now attacked by the new side-to-move (opponent)?
    return !next.isSquareAttacked(kr, kc, next.whiteToMove);
}

// ── PAWN MOVES ───────────────────────────────────────────────
// White pawns move UP (decreasing row), Black pawns move DOWN.
// Pawns:
//   - Push forward 1 square (if empty)
//   - Push forward 2 squares from starting rank (if both empty)
//   - Capture diagonally (only if enemy piece present)
//   - En passant capture
//   - Promote when reaching the last rank
void MoveGenerator::genPawnMoves(const Board& b, int r, int c,
                                  std::vector<Move>& moves) {
    int8_t pawn = b.squares[r][c];
    bool   isWhite = Piece::isWhite(pawn);
    int    dir     = isWhite ? -1 : 1; // White moves up (row--), Black moves down (row++)
    int    startRow = isWhite ? 6 : 1;  // Starting row for 2-square push
    int    promoRow = isWhite ? 0 : 7;  // Row where pawn promotes

    // ── Single forward push ───────────────────────────────────
    int nr = r + dir;
    if (nr >= 0 && nr < 8 && b.squares[nr][c] == Piece::EMPTY) {
        if (nr == promoRow) {
            // PROMOTION: generate all four promotion choices
            for (int8_t promo : {Piece::type(Piece::W_QUEEN),
                                  Piece::type(Piece::W_ROOK),
                                  Piece::type(Piece::W_BISHOP),
                                  Piece::type(Piece::W_KNIGHT)}) {
                moves.push_back(Move(r, c, nr, c, pawn, 0,
                                     MoveType::PROMOTION, promo));
            }
        } else {
            moves.push_back(Move(r, c, nr, c, pawn));
        }

        // ── Double push from starting rank ───────────────────
        if (r == startRow) {
            int nr2 = r + 2 * dir;
            if (b.squares[nr2][c] == Piece::EMPTY) {
                moves.push_back(Move(r, c, nr2, c, pawn));
            }
        }
    }

    // ── Diagonal captures ─────────────────────────────────────
    for (int dc : {-1, 1}) {
        int nc = c + dc;
        if (nc < 0 || nc >= 8) continue;
        nr = r + dir;

        // Regular capture
        if (nr >= 0 && nr < 8 &&
            b.squares[nr][nc] != Piece::EMPTY &&
            Piece::isEnemy(pawn, b.squares[nr][nc])) {
            int8_t captured = b.squares[nr][nc];
            if (nr == promoRow) {
                for (int8_t promo : {Piece::type(Piece::W_QUEEN),
                                      Piece::type(Piece::W_ROOK),
                                      Piece::type(Piece::W_BISHOP),
                                      Piece::type(Piece::W_KNIGHT)}) {
                    moves.push_back(Move(r, c, nr, nc, pawn, captured,
                                         MoveType::PROMOTION, promo));
                }
            } else {
                moves.push_back(Move(r, c, nr, nc, pawn, captured));
            }
        }

        // ── En passant ────────────────────────────────────────
        if (b.enPassantCol == nc && b.enPassantRow == nr) {
            // The captured pawn is on the same row as the capturing pawn
            int8_t epPawn = b.squares[r][nc]; // The pawn being captured
            moves.push_back(Move(r, c, nr, nc, pawn, epPawn,
                                 MoveType::EN_PASSANT));
        }
    }
}

// ── KNIGHT MOVES ─────────────────────────────────────────────
// Knights jump in an L-shape: 8 possible target squares.
// They can jump over pieces. Just check boundary + friendly piece.
void MoveGenerator::genKnightMoves(const Board& b, int r, int c,
                                    std::vector<Move>& moves) {
    int8_t knight = b.squares[r][c];
    static const int KN[8][2] = {
        {-2,-1},{-2,1},{-1,-2},{-1,2},
        { 1,-2},{ 1,2},{ 2,-1},{ 2, 1}
    };
    for (auto& d : KN) {
        int nr = r + d[0], nc = c + d[1];
        if (nr < 0 || nr >= 8 || nc < 0 || nc >= 8) continue;
        int8_t target = b.squares[nr][nc];
        // Can't capture own piece
        if (target != Piece::EMPTY && !Piece::isEnemy(knight, target)) continue;
        moves.push_back(Move(r, c, nr, nc, knight, target));
    }
}

// ── SLIDING PIECE HELPER ──────────────────────────────────────
// Used by bishop, rook, and queen.
// Walk in each direction until hitting a piece or the board edge.
void MoveGenerator::genSlidingMoves(const Board& b, int r, int c,
                                     int8_t piece,
                                     const int dirs[][2], int numDirs,
                                     std::vector<Move>& moves) {
    for (int d = 0; d < numDirs; d++) {
        int nr = r + dirs[d][0];
        int nc = c + dirs[d][1];
        while (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
            int8_t target = b.squares[nr][nc];
            if (target == Piece::EMPTY) {
                moves.push_back(Move(r, c, nr, nc, piece));
            } else {
                if (Piece::isEnemy(piece, target))
                    moves.push_back(Move(r, c, nr, nc, piece, target));
                break; // Blocked: stop sliding this direction
            }
            nr += dirs[d][0];
            nc += dirs[d][1];
        }
    }
}

// ── BISHOP MOVES ─────────────────────────────────────────────
void MoveGenerator::genBishopMoves(const Board& b, int r, int c,
                                    std::vector<Move>& moves) {
    static const int DIAG[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
    genSlidingMoves(b, r, c, b.squares[r][c], DIAG, 4, moves);
}

// ── ROOK MOVES ────────────────────────────────────────────────
void MoveGenerator::genRookMoves(const Board& b, int r, int c,
                                  std::vector<Move>& moves) {
    static const int STRAIGHT[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    genSlidingMoves(b, r, c, b.squares[r][c], STRAIGHT, 4, moves);
}

// ── QUEEN MOVES ───────────────────────────────────────────────
// Queen = Bishop + Rook combined
void MoveGenerator::genQueenMoves(const Board& b, int r, int c,
                                   std::vector<Move>& moves) {
    static const int ALL[8][2] = {
        {-1,-1},{-1,0},{-1,1},{0,-1},
        { 0, 1},{ 1,-1},{ 1,0},{ 1,1}
    };
    genSlidingMoves(b, r, c, b.squares[r][c], ALL, 8, moves);
}

// ── KING MOVES ────────────────────────────────────────────────
void MoveGenerator::genKingMoves(const Board& b, int r, int c,
                                  std::vector<Move>& moves) {
    int8_t king = b.squares[r][c];
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr, nc = c + dc;
            if (nr < 0 || nr >= 8 || nc < 0 || nc >= 8) continue;
            int8_t target = b.squares[nr][nc];
            if (target != Piece::EMPTY && !Piece::isEnemy(king, target)) continue;
            moves.push_back(Move(r, c, nr, nc, king, target));
        }
    // Castling is handled separately
    genCastlingMoves(b, r, c, moves);
}

// ── CASTLING ──────────────────────────────────────────────────
// Conditions for castling:
//   1. King has not moved (castling right flag still set)
//   2. Rook has not moved
//   3. Squares between king and rook are empty
//   4. King is not currently in check
//   5. King does not pass through a checked square
//   6. King does not end up in check
void MoveGenerator::genCastlingMoves(const Board& b, int r, int c,
                                      std::vector<Move>& moves) {
    bool white = Piece::isWhite(b.squares[r][c]);
    int  backRank = white ? 7 : 0;

    // Must be king on its home square
    if (r != backRank || c != 4) return;
    // King must not be in check
    if (b.isSquareAttacked(r, c, !white)) return;

    // ── King-side castling (O-O) ──────────────────────────────
    bool canK = white ? b.castleWhiteKing : b.castleBlackKing;
    if (canK) {
        // f1 and g1 (or f8 and g8) must be empty
        if (b.squares[r][5] == Piece::EMPTY &&
            b.squares[r][6] == Piece::EMPTY) {
            // f-file must not be under attack
            if (!b.isSquareAttacked(r, 5, !white) &&
                !b.isSquareAttacked(r, 6, !white)) {
                moves.push_back(Move(r, c, r, 6,
                    b.squares[r][c], 0, MoveType::CASTLING_K));
            }
        }
    }

    // ── Queen-side castling (O-O-O) ───────────────────────────
    bool canQ = white ? b.castleWhiteQueen : b.castleBlackQueen;
    if (canQ) {
        // b1, c1, d1 (or b8, c8, d8) must be empty
        if (b.squares[r][3] == Piece::EMPTY &&
            b.squares[r][2] == Piece::EMPTY &&
            b.squares[r][1] == Piece::EMPTY) {
            if (!b.isSquareAttacked(r, 3, !white) &&
                !b.isSquareAttacked(r, 2, !white)) {
                moves.push_back(Move(r, c, r, 2,
                    b.squares[r][c], 0, MoveType::CASTLING_Q));
            }
        }
    }
}

// ── MAIN: Generate all legal moves ───────────────────────────
// For each square: if it holds a friendly piece, generate its moves.
// Then filter to keep only moves that don't leave the king in check.
std::vector<Move> MoveGenerator::generateLegalMoves(const Board& board) {
    std::vector<Move> pseudoLegal;
    pseudoLegal.reserve(64); // Most positions have ~30 moves; 64 is safe

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int8_t p = board.squares[r][c];
            if (p == Piece::EMPTY) continue;
            // Only generate moves for the side-to-move
            if (board.whiteToMove  && Piece::isBlack(p)) continue;
            if (!board.whiteToMove && Piece::isWhite(p)) continue;

            switch (Piece::type(p)) {
                case 1: genPawnMoves  (board, r, c, pseudoLegal); break; // PAWN
                case 2: genKnightMoves(board, r, c, pseudoLegal); break; // KNIGHT
                case 3: genBishopMoves(board, r, c, pseudoLegal); break; // BISHOP
                case 4: genRookMoves  (board, r, c, pseudoLegal); break; // ROOK
                case 5: genQueenMoves (board, r, c, pseudoLegal); break; // QUEEN
                case 6: genKingMoves  (board, r, c, pseudoLegal); break; // KING
            }
        }
    }

    // Filter: keep only moves where own king is NOT in check afterward
    std::vector<Move> legal;
    legal.reserve(pseudoLegal.size());
    for (const Move& m : pseudoLegal)
        if (isLegal(board, m))
            legal.push_back(m);

    return legal;
}

// ── Terminal state detection ──────────────────────────────────
bool MoveGenerator::hasNoMoves(const Board& board) {
    return generateLegalMoves(board).empty();
}

bool MoveGenerator::isCheckmate(const Board& board) {
    return board.inCheck() && hasNoMoves(board);
}

bool MoveGenerator::isStalemate(const Board& board) {
    return !board.inCheck() && hasNoMoves(board);
}
