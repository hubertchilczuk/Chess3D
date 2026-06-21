/**
 * @file Animator.cpp
 * @brief Plik zrodlowy projektu Chess3D.
 */

#include "Animator.h"
#include <algorithm>
#include <cmath>

namespace chess3d {

void PieceAnimator::snapTo(const glm::vec3& p) {
    start_   = p;
    end_     = p;
    duration_ = 0.0f;
    elapsed_  = 0.0f;
    playing_  = false;
}

void PieceAnimator::moveTo(const glm::vec3& target, float durationSeconds) {
    start_    = currentPosition();
    end_      = target;
    duration_ = std::max(0.0001f, durationSeconds);
    elapsed_  = 0.0f;
    playing_  = true;
}

void PieceAnimator::update(float dt) {
    if (!playing_) return;
    elapsed_ += dt;
    if (elapsed_ >= duration_) {
        elapsed_ = duration_;
        playing_ = false;
        start_   = end_;     // collapse so currentPosition() returns end
    }
}

glm::vec3 PieceAnimator::currentPosition() const {
    if (duration_ <= 0.0f) return end_;
    float t = std::clamp(elapsed_ / duration_, 0.0f, 1.0f);
    float te = easing::easeInOutCubic(t);
    glm::vec3 p = start_ + (end_ - start_) * te;
    if (hopAmplitude_ > 0.0f) {
        // sinusoidal hop: 0 at start/end, peak in middle.
        p.y += hopAmplitude_ * std::sin(t * 3.14159265f);
    }
    return p;
}

} // namespace chess3d

