// ============================================================
//  main.cpp  –  BeginWithChess Engine
//
//  CLI interface for the C++ engine. The Java GUI communicates
//  via stdin/stdout pipes.
//
//  COACHING FEATURES:
//    - EVAL command: static evaluation of current position
//    - EXPLAIN: plain-English move explanations embedded in
//               MOVE and AI responses
//    - FEEDBACK: blunder/mistake/inaccuracy detection embedded
//                in MOVE responses
//    - UNDO: take back last move (supports 2-ply undo for
//            undoing both player + AI moves)
//
//  ── PROTOCOL ──────────────────────────────────────────────────
//  Commands:
//    NEW                   Start a new game
//    FEN <fen_string>      Load a specific position
//    MOVE <from><to>[p]    Apply a human move (e.g., MOVE e2e4)
//    AI <depth>            Engine finds best move at depth N
//    STATUS                Report check/checkmate/stalemate
//    LEGAL <square>        List legal moves from a square
//    EVAL                  Return static evaluation
//    UNDO                  Undo the last move
//    BOARD                 Print the board as FEN
//    QUIT                  Exit the engine
//
//  Response formats:
//    OK <FEN> [EVAL n] [FEEDBACK text] [EXPLAIN text]
//    BESTMOVE <move> <FEN> SCORE n NODES n [EXPLAIN text]
//    EVAL <score>
//    LEGAL <squares...>
//    STATUS <state>
//    ERROR <message>
// ============================================================

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include <cmath>

#include "engine/board.h"
#include "engine/movegen.h"
#include "engine/minimax.h"
#include "engine/evaluation.h"

// Convert col index -> file letter (0 -> 'a')
static char colToFile(int c) { return 'a' + c; }

// Convert row index -> rank digit (0 -> '8', 7 -> '1')
static char rowToRank(int r) { return '0' + (8 - r); }

// Convert e.g. "e2" -> row=6, col=4
static bool parseSquare(const std::string& s, int& row, int& col) {
    if (s.size() < 2) return false;
    col = s[0] - 'a';
    row = 8 - (s[1] - '0');
    return col >= 0 && col < 8 && row >= 0 && row < 8;
}

// Format a move as "e2e4" style string
static std::string formatMove(const Move& m) {
    std::string s;
    s += colToFile(m.fromCol);
    s += rowToRank(m.fromRow);
    s += colToFile(m.toCol);
    s += rowToRank(m.toRow);
    if (m.type == MoveType::PROMOTION) {
        const char promoChar[] = " pnbrqk";
        s += promoChar[m.promotion];
    }
    return s;
}

// Get piece name from type
static std::string pieceName(int type) {
    switch (type) {
        case 1: return "pawn";
        case 2: return "knight";
        case 3: return "bishop";
        case 4: return "rook";
        case 5: return "queen";
        case 6: return "king";
        default: return "piece";
    }
}

// Get square name from row, col
static std::string squareName(int row, int col) {
    std::string s;
    s += colToFile(col);
    s += rowToRank(row);
    return s;
}

// ── Generate a plain-English explanation for a move ──────────
static std::string explainMove(const Board& boardBefore, const Move& m,
                                int scoreBefore, int scoreAfter) {
    int moverType   = Piece::type(m.piece);
    bool isWhite    = Piece::isWhite(m.piece);
    std::string side      = isWhite ? "White" : "Black";
    std::string moverName = pieceName(moverType);
    std::string toSq      = squareName(m.toRow, m.toCol);

    // Special moves
    if (m.type == MoveType::CASTLING_K)
        return side + " castled kingside -- tucking the king to safety and activating the rook.";
    if (m.type == MoveType::CASTLING_Q)
        return side + " castled queenside -- securing the king and connecting the rooks.";
    if (m.type == MoveType::EN_PASSANT)
        return side + "'s pawn captured en passant on " + toSq +
               " -- a special capture after the opponent's pawn double-pushed.";
    if (m.type == MoveType::PROMOTION) {
        std::string promoName = pieceName(m.promotion);
        std::string expl = side + "'s pawn promoted to a " + promoName + " on " + toSq + "!";
        if (m.captured != Piece::EMPTY)
            expl += " It also captured " + pieceName(Piece::type(m.captured)) + ".";
        return expl;
    }

    // Captures
    if (m.captured != Piece::EMPTY) {
        int capturedType = Piece::type(m.captured);
        int capturedVal  = Evaluator::PIECE_VALUE[capturedType];
        int moverVal     = Evaluator::PIECE_VALUE[moverType];
        std::string expl = side + "'s " + moverName + " captured " +
                          pieceName(capturedType) + " on " + toSq + ". ";
        if (capturedVal > moverVal + 50)
            expl += "Great trade -- winning material!";
        else if (capturedVal < moverVal - 50)
            expl += "Trading a higher-value piece for a lower one.";
        else
            expl += "An equal exchange.";
        return expl;
    }

    // Check if the move delivers check
    Board after = boardBefore.applyMove(m);
    if (after.inCheck())
        return side + "'s " + moverName + " moves to " + toSq + " -- CHECK! The opponent's king is under attack.";

    // Pawn moves
    if (moverType == 1) {
        int rowDiff = std::abs(m.toRow - m.fromRow);
        if (rowDiff == 2)
            return side + " advanced a pawn two squares to " + toSq + " -- fighting for center control.";
        int promoRow = isWhite ? 0 : 7;
        int dist = std::abs(m.toRow - promoRow);
        if (dist <= 2)
            return side + "'s pawn pushes to " + toSq + " -- getting close to promotion!";
        return side + " pushed a pawn to " + toSq + " -- gaining space.";
    }

    // Knight/Bishop development
    if (moverType == 2 || moverType == 3) {
        int backRank = isWhite ? 7 : 0;
        if (m.fromRow == backRank)
            return side + " developed their " + moverName + " to " + toSq + " -- piece development is key!";
        bool toCenter = (m.toRow >= 2 && m.toRow <= 5 && m.toCol >= 2 && m.toCol <= 5);
        if (toCenter)
            return side + " moved their " + moverName + " to the center (" + toSq + ") -- pieces are stronger here.";
        return side + " repositioned their " + moverName + " to " + toSq + ".";
    }

    // Rook
    if (moverType == 4) {
        int seventhRank = isWhite ? 1 : 6;
        if (m.toRow == seventhRank)
            return side + "'s rook invaded the 7th rank at " + toSq + " -- a powerful position!";
        return side + " moved their rook to " + toSq + ".";
    }

    // Queen
    if (moverType == 5)
        return side + "'s queen moved to " + toSq + ".";

    // King
    if (moverType == 6)
        return side + "'s king moved to " + toSq + ".";

    // Fallback
    return side + " played " + moverName + " to " + toSq + ".";
}

// ── Generate blunder feedback ─────────────────────────────────
static std::string blunderCheck(int scoreBefore, int scoreAfter, bool moverIsWhite) {
    // Score is from White's perspective.
    int delta;
    if (moverIsWhite)
        delta = scoreBefore - scoreAfter; // positive = White lost advantage
    else
        delta = scoreAfter - scoreBefore; // positive = Black lost advantage

    if (delta >= 300)
        return "BLUNDER! That move lost significant material or position. Consider taking it back!";
    if (delta >= 150)
        return "Mistake -- that move weakened your position noticeably.";
    if (delta >= 80)
        return "Inaccuracy -- there was a better move available.";
    if (delta <= -150)
        return "Excellent move! That significantly improved your position.";
    if (delta <= -50)
        return "Good move -- solid improvement.";
    return "";
}

// ── Main loop ────────────────────────────────────────────────
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Board board;
    board.setStartPosition();

    // Undo stack: store previous board states
    std::stack<Board> undoStack;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        // ── NEW ──────────────────────────────────────────────
        if (cmd == "NEW") {
            board.setStartPosition();
            while (!undoStack.empty()) undoStack.pop();
            std::cout << "OK " << board.toFEN() << "\n";
        }

        // ── FEN ──────────────────────────────────────────────
        else if (cmd == "FEN") {
            std::string fen;
            std::getline(ss, fen);
            if (!fen.empty() && fen[0] == ' ') fen = fen.substr(1);
            if (board.loadFEN(fen)) {
                while (!undoStack.empty()) undoStack.pop();
                std::cout << "OK " << board.toFEN() << "\n";
            } else
                std::cout << "ERROR Invalid FEN\n";
        }

        // ── MOVE ─────────────────────────────────────────────
        else if (cmd == "MOVE") {
            std::string moveStr;
            ss >> moveStr;

            if (moveStr.size() < 4) {
                std::cout << "ERROR Bad move format\n";
                continue;
            }

            int fr, fc, tr, tc;
            if (!parseSquare(moveStr.substr(0,2), fr, fc) ||
                !parseSquare(moveStr.substr(2,2), tr, tc)) {
                std::cout << "ERROR Bad square\n";
                continue;
            }

            int8_t promo = 0;
            if (moveStr.size() >= 5) {
                switch (moveStr[4]) {
                    case 'q': promo = 5; break;
                    case 'r': promo = 4; break;
                    case 'b': promo = 3; break;
                    case 'n': promo = 2; break;
                }
            }

            auto legalMoves = MoveGenerator::generateLegalMoves(board);
            Move found = NULL_MOVE;
            for (const Move& m : legalMoves) {
                if (m.fromRow == fr && m.fromCol == fc &&
                    m.toRow   == tr && m.toCol   == tc) {
                    if (m.type == MoveType::PROMOTION) {
                        if (promo == 0 || m.promotion == promo) {
                            found = m;
                            break;
                        }
                    } else {
                        found = m;
                        break;
                    }
                }
            }

            if (found == NULL_MOVE) {
                std::cout << "ERROR Illegal move\n";
            } else {
                // Save state for undo
                undoStack.push(board);
                // Evaluate before and after for coaching
                int evalBefore = Evaluator::evaluate(board);
                Board boardBefore = board;
                board = board.applyMove(found);
                int evalAfter = Evaluator::evaluate(board);
                bool moverIsWhite = Piece::isWhite(found.piece);
                std::string feedback = blunderCheck(evalBefore, evalAfter, moverIsWhite);
                std::string explanation = explainMove(boardBefore, found, evalBefore, evalAfter);
                std::cout << "OK " << board.toFEN()
                          << " EVAL " << evalAfter
                          << " FEEDBACK " << feedback
                          << " EXPLAIN " << explanation << "\n";
            }
        }

        // ── AI ───────────────────────────────────────────────
        else if (cmd == "AI") {
            int depth = 3;
            ss >> depth;
            if (depth < 1) depth = 1;
            if (depth > 7) depth = 7;

            int evalBefore = Evaluator::evaluate(board);
            SearchResult result = Search::findBestMove(board, depth);

            if (result.bestMove == NULL_MOVE) {
                std::cout << "ERROR No moves available\n";
            } else {
                undoStack.push(board);
                std::string explanation = explainMove(board, result.bestMove, evalBefore, result.score);
                board = board.applyMove(result.bestMove);
                std::cout << "BESTMOVE " << formatMove(result.bestMove)
                          << " " << board.toFEN()
                          << " SCORE " << result.score
                          << " NODES " << result.nodesSearched
                          << " EXPLAIN " << explanation << "\n";
            }
        }

        // ── EVAL ─────────────────────────────────────────────
        else if (cmd == "EVAL") {
            int score = Evaluator::evaluate(board);
            std::cout << "EVAL " << score << "\n";
        }

        // ── UNDO ─────────────────────────────────────────────
        else if (cmd == "UNDO") {
            if (undoStack.empty()) {
                std::cout << "ERROR Nothing to undo\n";
            } else {
                board = undoStack.top();
                undoStack.pop();
                std::cout << "OK " << board.toFEN() << "\n";
            }
        }

        // ── STATUS ───────────────────────────────────────────
        else if (cmd == "STATUS") {
            std::string state;
            if (MoveGenerator::isCheckmate(board))
                state = "CHECKMATE";
            else if (MoveGenerator::isStalemate(board))
                state = "STALEMATE";
            else if (board.inCheck())
                state = "CHECK";
            else
                state = "NORMAL";
            std::cout << "STATUS " << state << "\n";
        }

        // ── LEGAL ────────────────────────────────────────────
        else if (cmd == "LEGAL") {
            std::string sqStr;
            ss >> sqStr;
            int r, c;
            if (!parseSquare(sqStr, r, c)) {
                std::cout << "ERROR Bad square\n";
                continue;
            }
            auto legalMoves = MoveGenerator::generateLegalMoves(board);
            std::cout << "LEGAL";
            for (const Move& m : legalMoves) {
                if (m.fromRow == r && m.fromCol == c)
                    std::cout << " " << colToFile(m.toCol) << rowToRank(m.toRow);
            }
            std::cout << "\n";
        }

        // ── BOARD ────────────────────────────────────────────
        else if (cmd == "BOARD") {
            std::cout << "BOARD " << board.toFEN() << "\n";
        }

        // ── QUIT ─────────────────────────────────────────────
        else if (cmd == "QUIT") {
            break;
        }

        else {
            std::cout << "ERROR Unknown command: " << cmd << "\n";
        }

        std::cout.flush();
    }

    return 0;
}
