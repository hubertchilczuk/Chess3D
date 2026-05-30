#include "ChessGame.h"
#include "PGNParser.h"

#include <iostream>
#include <cmath>
#include <algorithm>

namespace chess3d {

ChessGame::ChessGame() {
    resetToStartingPosition();
}

void ChessGame::resetToStartingPosition() {
    pieces_.clear();
    board_.fill({});
    currentMoveIndex_ = 0;
    autoPlayAccum_    = 0.0f;

    auto add = [this](PieceType t, Color c, int file, int rank) {
        auto p = std::make_unique<ChessPiece>(t, c, Square{file, rank});
        placePiece(p.get(), {file, rank});
        pieces_.push_back(std::move(p));
    };

    // Pawns
    for (int f = 0; f < 8; ++f) {
        add(PieceType::Pawn, Color::White, f, 1);
        add(PieceType::Pawn, Color::Black, f, 6);
    }
    // Major pieces
    const PieceType backRank[8] = {
        PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen,
        PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook
    };
    for (int f = 0; f < 8; ++f) {
        add(backRank[f], Color::White, f, 0);
        add(backRank[f], Color::Black, f, 7);
    }
}

ChessPiece* ChessGame::pieceAt(Square s) const {
    if (!s.valid()) return nullptr;
    return board_[idx(s.file, s.rank)].piece;
}

void ChessGame::placePiece(ChessPiece* p, Square s) {
    board_[idx(s.file, s.rank)].piece = p;
    p->setSquare(s);
}

void ChessGame::removeAt(Square s) {
    board_[idx(s.file, s.rank)].piece = nullptr;
}

bool ChessGame::canPieceReach(const ChessPiece& p, Square to, bool isCapture) const {
    if (!to.valid()) return false;
    Square from = p.square();
    if (from == to) return false;
    int df  = to.file - from.file;
    int dr  = to.rank - from.rank;
    int adf = std::abs(df);
    int adr = std::abs(dr);

    auto rayClear = [&](int sf, int sr) {
        Square cur{ from.file + sf, from.rank + sr };
        while (cur != to) {
            if (pieceAt(cur)) return false;
            cur.file += sf;
            cur.rank += sr;
        }
        return true;
    };

    switch (p.type()) {
    case PieceType::Knight:
        return (adf == 1 && adr == 2) || (adf == 2 && adr == 1);
    case PieceType::King:
        return adf <= 1 && adr <= 1 && (adf + adr > 0);
    case PieceType::Pawn: {
        int dir = (p.color() == Color::White) ? 1 : -1;
        if (isCapture) {
            return adf == 1 && dr == dir;
        }
        if (df != 0) return false;
        if (dr == dir) return pieceAt(to) == nullptr;
        if (dr == 2 * dir) {
            int startRank = (p.color() == Color::White) ? 1 : 6;
            if (from.rank != startRank) return false;
            Square mid{ from.file, from.rank + dir };
            return pieceAt(mid) == nullptr && pieceAt(to) == nullptr;
        }
        return false;
    }
    case PieceType::Bishop:
        if (adf != adr || adf == 0) return false;
        return rayClear((df > 0) - (df < 0), (dr > 0) - (dr < 0));
    case PieceType::Rook:
        if (df != 0 && dr != 0) return false;
        if (adf + adr == 0) return false;
        return rayClear((df > 0) - (df < 0), (dr > 0) - (dr < 0));
    case PieceType::Queen:
        if (adf == adr && adf > 0)
            return rayClear((df > 0) - (df < 0), (dr > 0) - (dr < 0));
        if ((df == 0 || dr == 0) && (adf + adr > 0))
            return rayClear((df > 0) - (df < 0), (dr > 0) - (dr < 0));
        return false;
    default:
        return false;
    }
}

bool ChessGame::resolveMove(Move& m, Color toMove) const {
    if (m.isCastleKingside) {
        int rank = (toMove == Color::White) ? 0 : 7;
        m.from = {4, rank};
        m.to   = {6, rank};
        return true;
    }
    if (m.isCastleQueenside) {
        int rank = (toMove == Color::White) ? 0 : 7;
        m.from = {4, rank};
        m.to   = {2, rank};
        return true;
    }

    // Collect candidate pieces of matching type+color that can reach the destination.
    std::vector<ChessPiece*> candidates;
    for (auto& up : pieces_) {
        ChessPiece* p = up.get();
        if (p->captured())            continue;
        if (p->color() != toMove)     continue;
        if (p->type()  != m.piece)    continue;
        if (m.from.file >= 0 && p->square().file != m.from.file) continue;
        if (m.from.rank >= 0 && p->square().rank != m.from.rank) continue;
        if (!canPieceReach(*p, m.to, m.isCapture)) continue;
        candidates.push_back(p);
    }

    if (candidates.empty()) {
        // En passant fallback: pawn capture where target is empty.
        if (m.piece == PieceType::Pawn && m.isCapture) {
            for (auto& up : pieces_) {
                ChessPiece* p = up.get();
                if (p->captured() || p->color() != toMove || p->type() != PieceType::Pawn) continue;
                if (m.from.file >= 0 && p->square().file != m.from.file) continue;
                int dir = (toMove == Color::White) ? 1 : -1;
                if (p->square().rank + dir != m.to.rank) continue;
                if (std::abs(p->square().file - m.to.file) != 1) continue;
                candidates.push_back(p);
            }
        }
    }

    if (candidates.empty()) return false;
    m.from = candidates.front()->square();
    return true;
}

void ChessGame::applyMoveLogical(const Move& m) {
    if (m.isCastleKingside || m.isCastleQueenside) {
        int rank = m.from.rank;
        int rookFromFile = m.isCastleKingside ? 7 : 0;
        int rookToFile   = m.isCastleKingside ? 5 : 3;

        ChessPiece* king = pieceAt(m.from);
        ChessPiece* rook = pieceAt({rookFromFile, rank});
        if (king) {
            removeAt(m.from);
            placePiece(king, m.to);
        }
        if (rook) {
            removeAt({rookFromFile, rank});
            placePiece(rook, {rookToFile, rank});
        }
        return;
    }

    ChessPiece* mover = pieceAt(m.from);
    if (!mover) return;

    // Capture
    ChessPiece* target = pieceAt(m.to);
    if (m.isCapture && !target && mover->type() == PieceType::Pawn) {
        // en passant - captured pawn sits on (m.to.file, m.from.rank)
        Square epSq{ m.to.file, m.from.rank };
        if (auto* ep = pieceAt(epSq)) {
            ep->setCaptured(true);
            removeAt(epSq);
        }
    }
    if (target) {
        target->setCaptured(true);
        removeAt(m.to);
    }

    removeAt(m.from);
    placePiece(mover, m.to);

    // Promotion
    if (m.promotion != PieceType::None) {
        mover->setType(m.promotion);
    }
}

void ChessGame::applyMoveAnimated(const Move& m) {
    const float duration = std::max(0.15f, 1.0f / speed_ * 0.7f);

    if (m.isCastleKingside || m.isCastleQueenside) {
        int rank = m.from.rank;
        int rookFromFile = m.isCastleKingside ? 7 : 0;
        int rookToFile   = m.isCastleKingside ? 5 : 3;

        if (auto* king = pieceAt(m.from)) {
            king->animator().moveTo(squareToWorld(m.to), duration);
        }
        if (auto* rook = pieceAt({rookFromFile, rank})) {
            rook->animator().moveTo(squareToWorld({rookToFile, rank}), duration);
        }
    } else {
        ChessPiece* mover = pieceAt(m.from);
        if (mover) {
            // Tiny "hop" for knights (and slightly for captures).
            float hop = 0.0f;
            if (mover->type() == PieceType::Knight) hop = 0.45f;
            else if (m.isCapture)                   hop = 0.15f;
            mover->animator().setHopAmplitude(hop);
            mover->animator().moveTo(squareToWorld(m.to), duration);
        }
    }
    applyMoveLogical(m);
}

bool ChessGame::stepForward() {
    if (currentMoveIndex_ >= static_cast<int>(moves_.size())) return false;
    Move m = moves_[currentMoveIndex_];
    Color toMove = (currentMoveIndex_ % 2 == 0) ? Color::White : Color::Black;
    if (!resolveMove(m, toMove)) {
        std::cerr << "[Chess3D] Failed to resolve move " << currentMoveIndex_ + 1
                  << ": " << m.san << '\n';
        return false;
    }
    moves_[currentMoveIndex_] = m;  // store resolved version (with from set)
    applyMoveAnimated(m);
    ++currentMoveIndex_;
    return true;
}

bool ChessGame::stepBackward() {
    if (currentMoveIndex_ <= 0) return false;
    int target = currentMoveIndex_ - 1;

    // Rebuild from scratch up to target moves applied, then snap visuals.
    resetToStartingPosition();
    for (int i = 0; i < target; ++i) {
        Move m = moves_[i];
        Color toMove = (i % 2 == 0) ? Color::White : Color::Black;
        if (!resolveMove(m, toMove)) {
            std::cerr << "[Chess3D] Rebuild failed at move " << i + 1 << '\n';
            break;
        }
        moves_[i] = m;
        applyMoveLogical(m);
    }
    for (auto& p : pieces_) {
        if (!p->captured()) p->snapAnimatorToSquare();
    }
    currentMoveIndex_ = target;
    return true;
}

void ChessGame::update(float dt) {
    for (auto& p : pieces_) p->animator().update(dt);

    if (playing_) {
        autoPlayAccum_ += dt * speed_;
        const float threshold = 1.0f;
        if (autoPlayAccum_ >= threshold) {
            autoPlayAccum_ -= threshold;
            if (!stepForward()) {
                playing_ = false;
                autoPlayAccum_ = 0.0f;
            }
        }
    } else {
        autoPlayAccum_ = 0.0f;
    }
}

bool ChessGame::loadPGNFile(const std::string& path) {
    auto pr = PGNParser::parseFile(path);
    if (!pr.error.empty()) {
        std::cerr << "[Chess3D] PGN error: " << pr.error << '\n';
        return false;
    }
    moves_   = std::move(pr.moves);
    headers_ = std::move(pr.headers);
    resetToStartingPosition();
    for (auto& p : pieces_) p->snapAnimatorToSquare();
    return true;
}

const std::string& ChessGame::pgnHeader(const std::string& key) const {
    static const std::string empty;
    for (auto& kv : headers_) if (kv.first == key) return kv.second;
    return empty;
}

} // namespace chess3d
