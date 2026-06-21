/**
 * @file Camera.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include <glm/glm.hpp>

namespace chess3d {

/// @brief Kamera swobodna używana do obserwacji planszy oraz figur.
class Camera {
public:
    Camera(const glm::vec3& position = glm::vec3(0.0f, 8.0f, 10.0f),
           float yawDeg   = -90.0f,
           float pitchDeg = -45.0f);

    glm::mat4 view()       const;
    glm::mat4 projection(float aspect) const;

    // WASD / QE input. dir components: +x right, +z forward (camera local).
    void processKeyboard(const glm::vec3& dir, float dt);
    void processMouse(float dx, float dy, bool constrainPitch = true);
    void processScroll(float yOffset);

    void setPosition(const glm::vec3& p) { position_ = p; }
    void lookAt(const glm::vec3& target);

    const glm::vec3& position() const { return position_; }
    const glm::vec3& front()    const { return front_; }
    float fov() const { return fov_; }

    float moveSpeed       = 6.0f;
    float mouseSensitivity = 0.1f;

private:
    void recomputeBasis();

    glm::vec3 position_;
    glm::vec3 front_   {0.0f, 0.0f, -1.0f};
    glm::vec3 up_      {0.0f, 1.0f,  0.0f};
    glm::vec3 right_   {1.0f, 0.0f,  0.0f};
    glm::vec3 worldUp_ {0.0f, 1.0f,  0.0f};

    float yaw_;
    float pitch_;
    float fov_   = 55.0f;
    float zNear_ = 0.1f;
    float zFar_  = 100.0f;
};

} // namespace chess3d

