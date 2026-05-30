#pragma once

#include "Common.h"
#include "ChessPiece.h"
#include <vector>
#include <memory>
#include <array>
#include <string>

namespace chess3d {

// Owns the list of pieces, the move list (parsed from PGN), the
// current move index and the playback state. Provides logical
// move-application (with PGN disambiguation) and drives animations.
class ChessGame {
public:
    ChessGame();

    void resetToStartingPosition();
    bool loadPGNFile(const std::string& path);

    // Playback ---------------------------------------------------------
    void play()      { playing_ = true;  }
    void pause()     { playing_ = false; }
    void togglePlay(){ playing_ = !playing_; }
    bool isPlaying() const { return playing_; }

    bool stepForward();   // advance currentMoveIndex_, animate
    bool stepBackward();  // rebuild state from move list up to index - 1

    void update(float dt);

    // Speed: moves per second.
    void  setSpeed(float movesPerSecond) { speed_ = std::max(0.05f, movesPerSecond); }
    float speed() const { return speed_; }

    // Inspection -------------------------------------------------------
    const std::vector<Move>&                          moves()   const { return moves_; }
    const std::vector<std::unique_ptr<ChessPiece>>&   pieces()  const { return pieces_; }
    int                                               currentMoveIndex() const { return currentMoveIndex_; }
    int                                               totalMoves() const { return static_cast<int>(moves_.size()); }
    const std::string&                                pgnHeader(const std::string& key) const;

    // For renderer to know what to highlight.
    bool        hasLastMove() const { return currentMoveIndex_ > 0; }
    const Move& lastMove()    const { return moves_[currentMoveIndex_ - 1]; }

private:
    // ----- logical board (for PGN move resolution) -----
    struct BoardSlot {
        ChessPiece* piece = nullptr;
    };
    std::array<BoardSlot, 64> board_{};

    static int idx(int f, int r) { return r * 8 + f; }

    ChessPiece* pieceAt(Square s) const;
    void placePiece(ChessPiece* p, Square s);
    void removeAt(Square s);

    // Move resolution / application ------------------------------------
    bool resolveMove(Move& m, Color toMove) const;
    bool canPieceReach(const ChessPiece& p, Square to, bool isCapture) const;
    void applyMoveAnimated(const Move& m);
    void applyMoveLogical (const Move& m);

    // ----- members -----
    std::vector<std::unique_ptr<ChessPiece>> pieces_;
    std::vector<Move>                        moves_;
    std::vector<std::pair<std::string, std::string>> headers_;
    int   currentMoveIndex_ = 0;   // = number of moves already played
    bool  playing_          = false;
    float speed_            = 1.0f;
    float autoPlayAccum_    = 0.0f;
};

} // namespace chess3d
