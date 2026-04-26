// ============================================================
//  board.cpp  –  BeginWithChess Engine
//
//  Implements all Board methods:
//    - Standard position setup
//    - FEN parsing / export
//    - Move application (creating a new board state)
//    - Square attack detection (used for check/legal move filter)
//    - Console debug print
//
//  NOTE: Written for maximum compiler compatibility.
//        No structured bindings, no C++17-only range tricks.
//        Compiles cleanly on GCC, MinGW (Windows), MSVC, Clang.
// ============================================================

#include "board.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <cstring>
#include <cstdlib>

// ── Constructor ───────────────────────────────────────────────
Board::Board() {
    setStartPosition();
}

// ── Standard starting position ───────────────────────────────
void Board::setStartPosition() {
    // Clear the entire board first
    memset(squares, 0, sizeof(squares));

    // ── Black pieces (row 0 = rank 8) ──────────────────────
    squares[0][0] = Piece::B_ROOK;
    squares[0][1] = Piece::B_KNIGHT;
    squares[0][2] = Piece::B_BISHOP;
    squares[0][3] = Piece::B_QUEEN;
    squares[0][4] = Piece::B_KING;
    squares[0][5] = Piece::B_BISHOP;
    squares[0][6] = Piece::B_KNIGHT;
    squares[0][7] = Piece::B_ROOK;

    // Black pawns on row 1
    for (int c = 0; c < 8; c++)
        squares[1][c] = Piece::B_PAWN;

    // White pawns on row 6
    for (int c = 0; c < 8; c++)
        squares[6][c] = Piece::W_PAWN;

    // ── White pieces (row 7 = rank 1) ──────────────────────
    squares[7][0] = Piece::W_ROOK;
    squares[7][1] = Piece::W_KNIGHT;
    squares[7][2] = Piece::W_BISHOP;
    squares[7][3] = Piece::W_QUEEN;
    squares[7][4] = Piece::W_KING;
    squares[7][5] = Piece::W_BISHOP;
    squares[7][6] = Piece::W_KNIGHT;
    squares[7][7] = Piece::W_ROOK;

    // Game state flags
    whiteToMove      = true;
    castleWhiteKing  = true;
    castleWhiteQueen = true;
    castleBlackKing  = true;
    castleBlackQueen = true;
    enPassantCol     = -1;
    enPassantRow     = -1;
    halfMoveClock    = 0;
    fullMoveNumber   = 1;
}

// ── Apply a move and return the resulting board ───────────────
// We never modify the current board — we always return a copy.
// This keeps the search algorithm simple (no undo-move needed).
Board Board::applyMove(const Move& m) const {
    Board next = *this;  // Copy entire board state

    int8_t movingPiece = next.squares[m.fromRow][m.fromCol];
    int8_t color = Piece::isWhite(movingPiece) ? 1 : -1;

    // ── Handle special move types ────────────────────────────

    if (m.type == MoveType::CASTLING_K) {
        // King-side castling: king moves two right, rook jumps over
        if (color == 1) {
            // White castles kingside  (e1->g1, h1 rook->f1)
            next.squares[7][4] = Piece::EMPTY;
            next.squares[7][6] = Piece::W_KING;
            next.squares[7][7] = Piece::EMPTY;
            next.squares[7][5] = Piece::W_ROOK;
            next.castleWhiteKing  = false;
            next.castleWhiteQueen = false;
        } else {
            // Black castles kingside  (e8->g8, h8 rook->f8)
            next.squares[0][4] = Piece::EMPTY;
            next.squares[0][6] = Piece::B_KING;
            next.squares[0][7] = Piece::EMPTY;
            next.squares[0][5] = Piece::B_ROOK;
            next.castleBlackKing  = false;
            next.castleBlackQueen = false;
        }
    }
    else if (m.type == MoveType::CASTLING_Q) {
        // Queen-side castling: king moves two left, rook jumps over
        if (color == 1) {
            // White castles queenside (e1->c1, a1 rook->d1)
            next.squares[7][4] = Piece::EMPTY;
            next.squares[7][2] = Piece::W_KING;
            next.squares[7][0] = Piece::EMPTY;
            next.squares[7][3] = Piece::W_ROOK;
            next.castleWhiteKing  = false;
            next.castleWhiteQueen = false;
        } else {
            // Black castles queenside (e8->c8, a8 rook->d8)
            next.squares[0][4] = Piece::EMPTY;
            next.squares[0][2] = Piece::B_KING;
            next.squares[0][0] = Piece::EMPTY;
            next.squares[0][3] = Piece::B_ROOK;
            next.castleBlackKing  = false;
            next.castleBlackQueen = false;
        }
    }
    else if (m.type == MoveType::EN_PASSANT) {
        // En passant: pawn moves diagonally but captured pawn is on
        // the same ROW as the moving pawn, not on the destination square
        next.squares[m.toRow][m.toCol]     = movingPiece;
        next.squares[m.fromRow][m.fromCol] = Piece::EMPTY;
        next.squares[m.fromRow][m.toCol]   = Piece::EMPTY; // remove captured pawn
    }
    else if (m.type == MoveType::PROMOTION) {
        // Pawn promotion: replace pawn with chosen piece
        next.squares[m.fromRow][m.fromCol] = Piece::EMPTY;
        next.squares[m.toRow][m.toCol]     = (int8_t)(m.promotion * color);
    }
    else {
        // Normal move or capture
        next.squares[m.toRow][m.toCol]     = movingPiece;
        next.squares[m.fromRow][m.fromCol] = Piece::EMPTY;
    }

    // ── Update castling rights ───────────────────────────────
    // If a rook moves from its home square, revoke that castling right
    if (m.fromRow == 7 && m.fromCol == 0) next.castleWhiteQueen = false;
    if (m.fromRow == 7 && m.fromCol == 7) next.castleWhiteKing  = false;
    if (m.fromRow == 0 && m.fromCol == 0) next.castleBlackQueen = false;
    if (m.fromRow == 0 && m.fromCol == 7) next.castleBlackKing  = false;

    // If the king moves at all, revoke both castling rights for that side
    if (Piece::type(movingPiece) == 6) {  // 6 = KING
        if (color == 1) {
            next.castleWhiteKing  = false;
            next.castleWhiteQueen = false;
        } else {
            next.castleBlackKing  = false;
            next.castleBlackQueen = false;
        }
    }

    // ── Update en passant target square ─────────────────────
    // Reset first — only set it if a pawn double-pushed this turn
    next.enPassantCol = -1;
    next.enPassantRow = -1;

    if (Piece::type(movingPiece) == 1) {  // 1 = PAWN
        int rowDiff = m.toRow - m.fromRow;
        if (rowDiff == 2 || rowDiff == -2) {
            // The pawn skipped a square — record the skipped square
            next.enPassantRow = (m.fromRow + m.toRow) / 2;
            next.enPassantCol = m.fromCol;
        }
    }

    // ── Update half-move clock (50-move rule) ────────────────
    if (m.captured != Piece::EMPTY || Piece::type(movingPiece) == 1) {
        next.halfMoveClock = 0;  // Reset on pawn move or capture
    } else {
        next.halfMoveClock++;
    }

    // ── Update full move counter ─────────────────────────────
    if (!whiteToMove) {
        next.fullMoveNumber++;  // Increments after Black's move
    }

    // Flip whose turn it is
    next.whiteToMove = !whiteToMove;

    return next;
}

// ── Find the king of a given side ────────────────────────────
bool Board::findKing(bool white, int& row, int& col) const {
    int8_t target = white ? Piece::W_KING : Piece::B_KING;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (squares[r][c] == target) {
                row = r;
                col = c;
                return true;
            }
        }
    }
    return false;
}

// ── Sliding piece attack helper ──────────────────────────────
// Walks from (row,col) in direction (dr,dc) looking for a
// friendly slider of the given type (3=bishop, 4=rook).
// Queen (type 5) is handled by calling this for both types.
bool Board::attackedBySlider(int row, int col, bool byWhite,
                              int dr, int dc, int sliderType) const {
    int r = row + dr;
    int c = col + dc;

    while (r >= 0 && r < 8 && c >= 0 && c < 8) {
        int8_t sq = squares[r][c];

        if (sq != Piece::EMPTY) {
            if (byWhite && sq > 0) {
                int t = Piece::type(sq);
                if (t == sliderType || t == 5)  // 5 = queen
                    return true;
            }
            if (!byWhite && sq < 0) {
                int t = Piece::type(sq);
                if (t == sliderType || t == 5)
                    return true;
            }
            break;  // Blocked by any piece
        }

        r += dr;
        c += dc;
    }

    return false;
}

// ── Is the given square attacked by the specified side? ──────
//
// Checks every possible attacker type one by one.
// Returns true as soon as any attacker is found.
//
// Uses plain arrays and indexed loops — no structured bindings,
// fully compatible with all MinGW / MSVC versions.
bool Board::isSquareAttacked(int row, int col, bool byWhite) const {

    // ── Pawn attacks ─────────────────────────────────────────
    // White pawns sit one row BELOW (higher index) and attack up
    // Black pawns sit one row ABOVE (lower index) and attack down
    {
        int pawnRow = byWhite ? (row + 1) : (row - 1);
        if (pawnRow >= 0 && pawnRow < 8) {
            int leftCol  = col - 1;
            int rightCol = col + 1;
            if (leftCol >= 0) {
                int8_t sq = squares[pawnRow][leftCol];
                if (byWhite  && sq == Piece::W_PAWN) return true;
                if (!byWhite && sq == Piece::B_PAWN) return true;
            }
            if (rightCol < 8) {
                int8_t sq = squares[pawnRow][rightCol];
                if (byWhite  && sq == Piece::W_PAWN) return true;
                if (!byWhite && sq == Piece::B_PAWN) return true;
            }
        }
    }

    // ── Knight attacks ───────────────────────────────────────
    {
        int knightDr[8] = {-2, -2, -1, -1,  1,  1,  2,  2};
        int knightDc[8] = {-1,  1, -2,  2, -2,  2, -1,  1};
        for (int i = 0; i < 8; i++) {
            int nr = row + knightDr[i];
            int nc = col + knightDc[i];
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                int8_t sq = squares[nr][nc];
                if (byWhite  && sq == Piece::W_KNIGHT) return true;
                if (!byWhite && sq == Piece::B_KNIGHT) return true;
            }
        }
    }

    // ── Bishop / Queen diagonal attacks ─────────────────────
    {
        int diagDr[4] = {-1, -1,  1,  1};
        int diagDc[4] = {-1,  1, -1,  1};
        for (int i = 0; i < 4; i++) {
            if (attackedBySlider(row, col, byWhite,
                                 diagDr[i], diagDc[i], 3))  // 3 = bishop
                return true;
        }
    }

    // ── Rook / Queen straight attacks ────────────────────────
    {
        int straightDr[4] = {-1,  1,  0,  0};
        int straightDc[4] = { 0,  0, -1,  1};
        for (int i = 0; i < 4; i++) {
            if (attackedBySlider(row, col, byWhite,
                                 straightDr[i], straightDc[i], 4))  // 4 = rook
                return true;
        }
    }

    // ── King adjacency ───────────────────────────────────────
    // Prevents the two kings from standing on adjacent squares
    {
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) continue;
                int nr = row + dr;
                int nc = col + dc;
                if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                    int8_t sq = squares[nr][nc];
                    if (byWhite  && sq == Piece::W_KING) return true;
                    if (!byWhite && sq == Piece::B_KING) return true;
                }
            }
        }
    }

    return false;
}

// ── Is the side-to-move's king in check? ─────────────────────
bool Board::inCheck() const {
    int kr = 0, kc = 0;
    if (!findKing(whiteToMove, kr, kc)) return false;
    return isSquareAttacked(kr, kc, !whiteToMove);
}

// ── ASCII debug print ─────────────────────────────────────────
// White pieces = UPPERCASE, Black pieces = lowercase, empty = dot
void Board::print() const {
    std::cout << "\n  a b c d e f g h\n";
    for (int r = 0; r < 8; r++) {
        std::cout << (8 - r) << " ";
        for (int c = 0; c < 8; c++) {
            int8_t p = squares[r][c];
            char sym;
            if (p == 0) {
                sym = '.';
            } else if (p > 0) {
                sym = "PNBRQK"[p - 1];
            } else {
                sym = "pnbrqk"[-p - 1];
            }
            std::cout << sym << " ";
        }
        std::cout << (8 - r) << "\n";
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << (whiteToMove ? "White" : "Black") << " to move\n\n";
}

// ── FEN string loader ────────────────────────────────────────
// Example FEN: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
bool Board::loadFEN(const std::string& fen) {
    memset(squares, 0, sizeof(squares));

    std::istringstream ss(fen);
    std::string piecePlacement, activeColor, castling, enPassant;
    int halfMove = 0, fullMove = 1;

    ss >> piecePlacement >> activeColor >> castling >> enPassant
       >> halfMove >> fullMove;

    // ── Piece placement ──────────────────────────────────────
    int row = 0, col = 0;
    for (size_t i = 0; i < piecePlacement.size(); i++) {
        char ch = piecePlacement[i];
        if (ch == '/') {
            row++;
            col = 0;
        } else if (ch >= '1' && ch <= '8') {
            col += (ch - '0');
        } else {
            int8_t p = 0;
            char upper = (char)toupper((unsigned char)ch);
            if      (upper == 'P') p = 1;
            else if (upper == 'N') p = 2;
            else if (upper == 'B') p = 3;
            else if (upper == 'R') p = 4;
            else if (upper == 'Q') p = 5;
            else if (upper == 'K') p = 6;

            // Lowercase FEN letter = Black piece = negative
            if (ch >= 'a' && ch <= 'z') p = (int8_t)(-p);

            if (row < 8 && col < 8) {
                squares[row][col] = p;
            }
            col++;
        }
    }

    // ── Active color ─────────────────────────────────────────
    whiteToMove = (activeColor == "w");

    // ── Castling rights ──────────────────────────────────────
    castleWhiteKing  = (castling.find('K') != std::string::npos);
    castleWhiteQueen = (castling.find('Q') != std::string::npos);
    castleBlackKing  = (castling.find('k') != std::string::npos);
    castleBlackQueen = (castling.find('q') != std::string::npos);

    // ── En passant square ────────────────────────────────────
    if (enPassant != "-" && enPassant.size() >= 2) {
        enPassantCol = enPassant[0] - 'a';
        enPassantRow = 8 - (enPassant[1] - '0');
    } else {
        enPassantCol = -1;
        enPassantRow = -1;
    }

    // ── Move counters ────────────────────────────────────────
    halfMoveClock  = halfMove;
    fullMoveNumber = fullMove;

    return true;
}

// ── FEN string exporter ──────────────────────────────────────
std::string Board::toFEN() const {
    std::string fen;

    // Part 1: Piece placement
    for (int r = 0; r < 8; r++) {
        int emptyCount = 0;
        for (int c = 0; c < 8; c++) {
            int8_t p = squares[r][c];
            if (p == 0) {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen += (char)('0' + emptyCount);
                    emptyCount = 0;
                }
                int t = Piece::type(p);
                char sym = "PNBRQK"[t - 1];
                if (Piece::isBlack(p)) {
                    sym = (char)tolower((unsigned char)sym);
                }
                fen += sym;
            }
        }
        if (emptyCount > 0) {
            fen += (char)('0' + emptyCount);
        }
        if (r < 7) fen += '/';
    }

    // Part 2: Active color
    fen += ' ';
    fen += (whiteToMove ? 'w' : 'b');

    // Part 3: Castling rights
    fen += ' ';
    std::string castleStr;
    if (castleWhiteKing)  castleStr += 'K';
    if (castleWhiteQueen) castleStr += 'Q';
    if (castleBlackKing)  castleStr += 'k';
    if (castleBlackQueen) castleStr += 'q';
    fen += castleStr.empty() ? "-" : castleStr;

    // Part 4: En passant target
    fen += ' ';
    if (enPassantCol >= 0 && enPassantRow >= 0) {
        fen += (char)('a' + enPassantCol);
        fen += (char)('0' + (8 - enPassantRow));
    } else {
        fen += '-';
    }

    // Parts 5 & 6: Move counters
    fen += ' ';
    fen += std::to_string(halfMoveClock);
    fen += ' ';
    fen += std::to_string(fullMoveNumber);

    return fen;
}
