#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <GL/glew.h>

namespace chess3d {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSources(const std::string& vsSrc, const std::string& fsSrc);

    void bind() const;
    static void unbind();

    GLuint id() const { return programId_; }

    void setBool (const std::string& name, bool value)        const;
    void setInt  (const std::string& name, int value)         const;
    void setFloat(const std::string& name, float value)       const;
    void setVec2 (const std::string& name, const glm::vec2& v) const;
    void setVec3 (const std::string& name, const glm::vec3& v) const;
    void setVec4 (const std::string& name, const glm::vec4& v) const;
    void setMat3 (const std::string& name, const glm::mat3& m) const;
    void setMat4 (const std::string& name, const glm::mat4& m) const;

private:
    GLint uniformLocation(const std::string& name) const;
    static GLuint compileStage(GLenum stage, const std::string& source, const std::string& tag);

    GLuint programId_ = 0;
    mutable std::unordered_map<std::string, GLint> uniformCache_;
};

} // namespace chess3d
