#pragma once

#include <vector>
#include <cstdint>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace chess3d {

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal  {0.0f, 1.0f, 0.0f};
    glm::vec2 uv      {0.0f};
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& vertices,
                const std::vector<std::uint32_t>& indices);
    void draw() const;

    GLsizei indexCount() const { return indexCount_; }

private:
    void releaseGL();

    GLuint  vao_        = 0;
    GLuint  vbo_        = 0;
    GLuint  ebo_        = 0;
    GLsizei indexCount_ = 0;
};

} // namespace chess3d
