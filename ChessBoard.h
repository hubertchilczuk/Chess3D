/**
 * @file ChessBoard.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include "Common.h"
#include "Mesh.h"
#include <array>
#include <memory>

namespace chess3d {

// Procedurally generated 8x8 chess board. Each square is its own
// little Mesh so that the renderer can pick which squares are
// highlighted on a per-square basis.
class ChessBoard {
public:
    ChessBoard();
    void build();

    // Mesh access:
    const Mesh& squareMesh(int file, int rank) const;
    const Mesh& frameMesh() const { return frame_; }
    const Mesh& labelsMesh() const { return labels_; }

    // Highlights - the renderer reads these every frame.
    void clearHighlights();
    void highlightSquare(Square s, float strength = 1.0f);

    float highlight(int file, int rank) const {
        return highlights_[index(file, rank)];
    }

    // Decay highlights with time so they pulse softly.
    void update(float dt);

private:
    static int index(int f, int r) { return r * 8 + f; }

    std::array<std::unique_ptr<Mesh>, 64> squares_;
    std::array<float, 64> highlights_{};
    Mesh frame_;
    Mesh labels_;
};

} // namespace chess3d

