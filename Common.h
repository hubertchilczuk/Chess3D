/**
 * @file Common.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace chess3d {

enum class PieceType : std::uint8_t {
    None = 0,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

enum class Color : std::uint8_t {
    White = 0,
    Black = 1
};

inline Color opposite(Color c) {
    return c == Color::White ? Color::Black : Color::White;
}

struct Square {
    int file = -1; // 0..7   (a..h)
    int rank = -1; // 0..7   (1..8)

    constexpr bool valid() const noexcept {
        return file >= 0 && rank >= 0 && file < 8 && rank < 8;
    }

    friend bool operator==(const Square& a, const Square& b) {
        return a.file == b.file && a.rank == b.rank;
    }
    friend bool operator!=(const Square& a, const Square& b) { return !(a == b); }
};

struct Move {
    PieceType   piece              = PieceType::None;
    Square      from;
    Square      to;
    bool        isCapture          = false;
    bool        isCheck            = false;
    bool        isMate             = false;
    bool        isCastleKingside   = false;
    bool        isCastleQueenside  = false;
    bool        isEnPassant        = false;
    PieceType   promotion          = PieceType::None;
    Color       color              = Color::White;
    std::string san;
};

constexpr float kSquareSize = 1.0f;            // world units per square
constexpr float kBoardHalf  = 4.0f * kSquareSize;

inline glm::vec3 squareToWorld(const Square& s) {
    // Center each square; file -> X, rank -> Z (mirrored so White is at -Z)
    float x = (static_cast<float>(s.file) - 3.5f) * kSquareSize;
    float z = (3.5f - static_cast<float>(s.rank)) * kSquareSize;
    return glm::vec3(x, 0.0f, z);
}

inline const char* pieceName(PieceType t) {
    switch (t) {
        case PieceType::Pawn:   return "pawn";
        case PieceType::Knight: return "knight";
        case PieceType::Bishop: return "bishop";
        case PieceType::Rook:   return "rook";
        case PieceType::Queen:  return "queen";
        case PieceType::King:   return "king";
        default:                return "none";
    }
}

inline PieceType pieceFromLetter(char c) {
    switch (c) {
        case 'N': return PieceType::Knight;
        case 'B': return PieceType::Bishop;
        case 'R': return PieceType::Rook;
        case 'Q': return PieceType::Queen;
        case 'K': return PieceType::King;
        case 'P': return PieceType::Pawn;
        default:  return PieceType::None;
    }
}

} // namespace chess3d

