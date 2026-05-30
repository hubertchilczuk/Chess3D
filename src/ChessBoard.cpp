#include "ChessBoard.h"
#include "PrimitiveBuilder.h"

#include <algorithm>
#include <glm/gtc/constants.hpp>

#define STB_EASY_FONT_IMPLEMENTATION
#include <stb_easy_font.h>

namespace chess3d {

ChessBoard::ChessBoard() = default;

void ChessBoard::build() {
    // Each square -> its own little Mesh -> 0.5 unit cube of height 0.05.
    const float thickness = 0.06f;

    for (int r = 0; r < 8; ++r) {
        for (int f = 0; f < 8; ++f) {
            std::vector<Vertex>          v;
            std::vector<std::uint32_t>   i;
            glm::vec3 center = squareToWorld({f, r});
            center.y = -thickness;
            primitives::appendBox(v, i, center,
                glm::vec3(kSquareSize * 0.5f, thickness, kSquareSize * 0.5f));

            auto mesh = std::make_unique<Mesh>();
            mesh->upload(v, i);
            squares_[index(f, r)] = std::move(mesh);
        }
    }

    // Decorative outer frame around the whole board.
    {
        std::vector<Vertex>        v;
        std::vector<std::uint32_t> i;
        const float frameW   = 0.6f;
        const float frameH   = 0.18f;
        const float halfFull = kBoardHalf + frameW * 0.5f;
        const float yC       = -frameH * 0.5f;

        // top  (+Z) and bottom (-Z) strips
        primitives::appendBox(v, i, {0, yC,  kBoardHalf + frameW * 0.5f},
                              {halfFull, frameH * 0.5f, frameW * 0.5f});
        primitives::appendBox(v, i, {0, yC, -kBoardHalf - frameW * 0.5f},
                              {halfFull, frameH * 0.5f, frameW * 0.5f});
        // left  (-X) and right (+X) strips
        primitives::appendBox(v, i, { kBoardHalf + frameW * 0.5f, yC, 0},
                              {frameW * 0.5f, frameH * 0.5f, kBoardHalf});
        primitives::appendBox(v, i, {-kBoardHalf - frameW * 0.5f, yC, 0},
                              {frameW * 0.5f, frameH * 0.5f, kBoardHalf});

        frame_.upload(v, i);
    }

    // ---- File / rank labels engraved onto the frame top ----
    // stb_easy_font emits a quad mesh in 2D font coords (origin top-left,
    // +X right, +Y down). We map each label's quads onto the world-space
    // frame surface so they rotate naturally with the camera.
    {
        struct EFV { float x, y, z; unsigned char c[4]; };
        std::vector<Vertex>          v;
        std::vector<std::uint32_t>   i;

        const float scale  = 0.045f;                // ~0.45 unit tall glyph
        const float labelY = 0.001f;                // sits flush on frame top
        const float edge   = kBoardHalf + 0.30f;    // mid of 0.6-wide frame strip

        auto emit = [&](const char* text,
                        const glm::vec3& origin,
                        const glm::vec3& axisX,
                        const glm::vec3& axisZ) {
            EFV buf[512];
            int quads = stb_easy_font_print(0.0f, 0.0f,
                                            const_cast<char*>(text),
                                            nullptr, buf, sizeof(buf));
            if (quads <= 0) return;
            // Center on bounding box so the origin is the glyph center.
            float minX = buf[0].x, maxX = buf[0].x;
            float minY = buf[0].y, maxY = buf[0].y;
            for (int q = 0; q < quads * 4; ++q) {
                minX = std::min(minX, buf[q].x);
                maxX = std::max(maxX, buf[q].x);
                minY = std::min(minY, buf[q].y);
                maxY = std::max(maxY, buf[q].y);
            }
            const float cx = (minX + maxX) * 0.5f;
            const float cy = (minY + maxY) * 0.5f;

            for (int q = 0; q < quads; ++q) {
                std::uint32_t base = static_cast<std::uint32_t>(v.size());
                for (int k = 0; k < 4; ++k) {
                    const EFV& fv = buf[q * 4 + k];
                    glm::vec3 p = origin
                                + axisX * ((fv.x - cx) * scale)
                                + axisZ * ((fv.y - cy) * scale);
                    v.push_back({ p, glm::vec3(0, 1, 0), glm::vec2(0) });
                }
                i.insert(i.end(), { base, base + 1, base + 2,
                                    base, base + 2, base + 3 });
            }
        };

        char t[2] = { 0, 0 };
        // Files a..h on both Z edges, oriented to read from each side.
        for (int f = 0; f < 8; ++f) {
            const float x = (static_cast<float>(f) - 3.5f) * kSquareSize;
            t[0] = static_cast<char>('a' + f);
            emit(t, { x, labelY,  edge }, { 1, 0, 0}, {0, 0,  1});
            emit(t, { x, labelY, -edge }, {-1, 0, 0}, {0, 0, -1});
        }
        // Ranks 1..8 on both X edges.
        for (int r = 0; r < 8; ++r) {
            const float z = (3.5f - static_cast<float>(r)) * kSquareSize;
            t[0] = static_cast<char>('1' + r);
            emit(t, {  edge, labelY, z }, {0, 0, -1}, { 1, 0, 0});
            emit(t, { -edge, labelY, z }, {0, 0,  1}, {-1, 0, 0});
        }

        if (!v.empty()) labels_.upload(v, i);
    }

    clearHighlights();
}

const Mesh& ChessBoard::squareMesh(int f, int r) const {
    return *squares_[index(f, r)];
}

void ChessBoard::clearHighlights() {
    highlights_.fill(0.0f);
}

void ChessBoard::highlightSquare(Square s, float strength) {
    if (!s.valid()) return;
    highlights_[index(s.file, s.rank)] = std::max(highlights_[index(s.file, s.rank)], strength);
}

void ChessBoard::update(float dt) {
    // Gentle exponential decay so highlights fade out softly.
    const float decay = std::exp(-dt * 0.8f);
    for (auto& h : highlights_) {
        h *= decay;
        if (h < 0.01f) h = 0.0f;
    }
}

} // namespace chess3d
