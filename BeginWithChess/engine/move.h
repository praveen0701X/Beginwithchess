// ============================================================
//  move.h  –  BeginWithChess Engine
//  Defines the Move struct: the atomic unit of chess logic.
//
//  A "Move" stores everything about one chess move:
//    - where the piece came from  (fromRow, fromCol)
//    - where it goes              (toRow,   toCol)
//    - what special type it is    (MoveType enum)
//    - what piece is being moved  (piece)
//    - what piece is captured     (captured)
//    - what piece a pawn promotes to (promotion)
// ============================================================

#pragma once
#include <cstdint>

// ── Piece constants ──────────────────────────────────────────
// We store the board as a flat int8_t array.
// Positive = White, Negative = Black, 0 = Empty
//
//   EMPTY  = 0
//   PAWN   = 1   (White = +1, Black = -1)
//   KNIGHT = 2
//   BISHOP = 3
//   ROOK   = 4
//   QUEEN  = 5
//   KING   = 6

namespace Piece {
    constexpr int8_t EMPTY  =  0;
    constexpr int8_t W_PAWN =  1;
    constexpr int8_t W_KNIGHT=  2;
    constexpr int8_t W_BISHOP=  3;
    constexpr int8_t W_ROOK =  4;
    constexpr int8_t W_QUEEN=  5;
    constexpr int8_t W_KING =  6;
    constexpr int8_t B_PAWN = -1;
    constexpr int8_t B_KNIGHT= -2;
    constexpr int8_t B_BISHOP= -3;
    constexpr int8_t B_ROOK = -4;
    constexpr int8_t B_QUEEN= -5;
    constexpr int8_t B_KING = -6;

    // Helper: get absolute piece type (color-independent)
    inline int type(int8_t p) { return p < 0 ? -p : p; }

    // Helper: is the piece white?
    inline bool isWhite(int8_t p) { return p > 0; }

    // Helper: is the piece black?
    inline bool isBlack(int8_t p) { return p < 0; }

    // Helper: are two pieces on opposite sides?
    inline bool isEnemy(int8_t a, int8_t b) {
        return (a > 0 && b < 0) || (a < 0 && b > 0);
    }
}

// ── Move type flags ──────────────────────────────────────────
enum class MoveType : uint8_t {
    NORMAL     = 0,   // Regular move or capture
    CASTLING_K = 1,   // King-side castling  (O-O)
    CASTLING_Q = 2,   // Queen-side castling (O-O-O)
    EN_PASSANT = 3,   // En passant capture
    PROMOTION  = 4    // Pawn promotion (check .promotion field)
};

// ── Move struct ──────────────────────────────────────────────
// Kept small (8 bytes) on purpose – we generate thousands of
// moves per second, so memory layout matters.
struct Move {
    int8_t  fromRow  = 0;
    int8_t  fromCol  = 0;
    int8_t  toRow    = 0;
    int8_t  toCol    = 0;
    int8_t  piece    = 0;   // which piece is moving
    int8_t  captured = 0;   // piece captured (0 = none)
    MoveType type    = MoveType::NORMAL;
    int8_t  promotion= 0;   // piece type after promotion (0 = none)

    // Convenience constructor
    Move() = default;
    Move(int fr, int fc, int tr, int tc,
         int8_t pc = 0, int8_t cap = 0,
         MoveType mt = MoveType::NORMAL,
         int8_t promo = 0)
        : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc),
          piece(pc), captured(cap), type(mt), promotion(promo) {}
    // For sorting / comparison
    bool operator==(const Move& o) const {
        return fromRow == o.fromRow && fromCol == o.fromCol &&
               toRow   == o.toRow   && toCol   == o.toCol  &&
               type    == o.type;
    }
    bool operator!=(const Move& o) const { return !(*this == o); }
};

// Null move sentinel – used to signal "no move found"
inline Move makeNullMove() { return Move(-1, -1, -1, -1); }
#define NULL_MOVE makeNullMove()
