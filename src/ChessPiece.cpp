#include "ChessPiece.h"

namespace chess3d {

ChessPiece::ChessPiece(PieceType type, Color color, Square startingSquare)
    : type_(type), color_(color), square_(startingSquare) {
    animator_.snapTo(squareToWorld(startingSquare));
}

void ChessPiece::snapAnimatorToSquare() {
    animator_.snapTo(squareToWorld(square_));
}

} // namespace chess3d
