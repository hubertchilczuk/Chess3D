#pragma once

#include <glm/glm.hpp>

namespace chess3d {

struct DirectionalLight {
    glm::vec3 direction{-0.4f, -1.0f, -0.3f};
    glm::vec3 ambient  { 0.18f, 0.18f, 0.20f};
    glm::vec3 diffuse  { 0.70f, 0.70f, 0.68f};
    glm::vec3 specular { 0.90f, 0.90f, 0.90f};
};

struct PointLight {
    glm::vec3 position { 3.0f, 6.0f, 3.0f };
    glm::vec3 ambient  { 0.05f, 0.05f, 0.05f };
    glm::vec3 diffuse  { 0.95f, 0.85f, 0.65f };
    glm::vec3 specular { 1.00f, 0.95f, 0.85f };
    float     constant  = 1.0f;
    float     linear    = 0.09f;
    float     quadratic = 0.032f;
};

} // namespace chess3d
