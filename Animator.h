/**
 * @file Animator.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include <glm/glm.hpp>
#include <cmath>

namespace chess3d {

// Small collection of easing helpers + a parameterised piece animator.
namespace easing {
    inline float linear   (float t) { return t; }
    inline float easeInOutCubic(float t) {
        return t < 0.5f
            ? 4.0f * t * t * t
            : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
    }
}

// Tracks a single piece's interpolated world position. Each piece has
// its own Animator instance so that multiple pieces (e.g. castling)
// can animate in parallel without coupling.
class PieceAnimator {
public:
    void snapTo(const glm::vec3& p);
    void moveTo(const glm::vec3& target, float durationSeconds);
    void update(float dt);

    bool isPlaying() const { return playing_; }

    glm::vec3 currentPosition() const;
    // Vertical "hop" amplitude for knights / captures (0 = none).
    void setHopAmplitude(float a) { hopAmplitude_ = a; }

private:
    glm::vec3 start_  {0.0f};
    glm::vec3 end_    {0.0f};
    float duration_   = 0.0f;
    float elapsed_    = 0.0f;
    float hopAmplitude_ = 0.0f;
    bool  playing_    = false;
};

} // namespace chess3d

