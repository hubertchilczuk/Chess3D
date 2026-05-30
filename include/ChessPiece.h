#pragma once

#include "Common.h"
#include "Animator.h"

namespace chess3d {

// A logical piece in the scene. Holds its current logical square,
// its animation state, and a "captured" flag (so the renderer can
// fade it out or skip drawing).
class ChessPiece {
public:
    ChessPiece(PieceType type, Color color, Square startingSquare);

    PieceType type()  const { return type_;  }
    Color     color() const { return color_; }
    Square    square() const { return square_; }

    void setSquare(Square s) { square_ = s; }

    bool captured() const { return captured_; }
    void setCaptured(bool c) { captured_ = c; }

    // Promotion handling (e.g. pawn -> queen).
    void setType(PieceType newType) { type_ = newType; }

    PieceAnimator& animator()       { return animator_; }
    const PieceAnimator& animator() const { return animator_; }

    // Convenience: place piece visually at its current square instantly.
    void snapAnimatorToSquare();

private:
    PieceType    type_;
    Color        color_;
    Square       square_;
    bool         captured_ = false;
    PieceAnimator animator_;
};

} // namespace chess3d
