#include "Camera.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace chess3d {

Camera::Camera(const glm::vec3& position, float yawDeg, float pitchDeg)
    : position_(position), yaw_(yawDeg), pitch_(pitchDeg) {
    recomputeBasis();
}

void Camera::recomputeBasis() {
    float yawR   = glm::radians(yaw_);
    float pitchR = glm::radians(pitch_);

    glm::vec3 f;
    f.x = std::cos(yawR) * std::cos(pitchR);
    f.y = std::sin(pitchR);
    f.z = std::sin(yawR) * std::cos(pitchR);

    front_ = glm::normalize(f);
    right_ = glm::normalize(glm::cross(front_, worldUp_));
    up_    = glm::normalize(glm::cross(right_, front_));
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::projection(float aspect) const {
    return glm::perspective(glm::radians(fov_), aspect, zNear_, zFar_);
}

void Camera::processKeyboard(const glm::vec3& dir, float dt) {
    float v = moveSpeed * dt;
    position_ += front_   * dir.z * v;
    position_ += right_   * dir.x * v;
    position_ += worldUp_ * dir.y * v;
}

void Camera::processMouse(float dx, float dy, bool constrainPitch) {
    yaw_   += dx * mouseSensitivity;
    pitch_ -= dy * mouseSensitivity;
    if (constrainPitch) {
        pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    }
    recomputeBasis();
}

void Camera::processScroll(float yOffset) {
    fov_ -= yOffset * 2.0f;
    fov_  = std::clamp(fov_, 20.0f, 90.0f);
}

void Camera::lookAt(const glm::vec3& target) {
    glm::vec3 d = glm::normalize(target - position_);
    pitch_ = glm::degrees(std::asin(d.y));
    yaw_   = glm::degrees(std::atan2(d.z, d.x));
    recomputeBasis();
}

} // namespace chess3d
