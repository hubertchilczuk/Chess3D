#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace chess3d {

Shader::~Shader() {
    if (programId_ != 0) {
        glDeleteProgram(programId_);
        programId_ = 0;
    }
}

Shader::Shader(Shader&& other) noexcept
    : programId_(other.programId_),
      uniformCache_(std::move(other.uniformCache_)) {
    other.programId_ = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (programId_) glDeleteProgram(programId_);
        programId_     = other.programId_;
        uniformCache_  = std::move(other.uniformCache_);
        other.programId_ = 0;
    }
    return *this;
}

static std::string slurp(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Shader::compileStage(GLenum stage, const std::string& source, const std::string& tag) {
    GLuint sh = glCreateShader(stage);
    const char* src = source.c_str();
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);

    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<std::size_t>(std::max(len, 1)), '\0');
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        std::cerr << "[Shader] " << tag << " compile error:\n" << log << '\n';
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

bool Shader::loadFromSources(const std::string& vsSrc, const std::string& fsSrc) {
    GLuint vs = compileStage(GL_VERTEX_SHADER,   vsSrc, "vertex");
    GLuint fs = compileStage(GL_FRAGMENT_SHADER, fsSrc, "fragment");
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram (prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(static_cast<std::size_t>(std::max(len, 1)), '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "[Shader] link error:\n" << log << '\n';
        glDeleteShader(vs);
        glDeleteShader(fs);
        glDeleteProgram(prog);
        return false;
    }

    glDetachShader(prog, vs);
    glDetachShader(prog, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    if (programId_) glDeleteProgram(programId_);
    programId_ = prog;
    uniformCache_.clear();
    return true;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vs = slurp(vertexPath);
    std::string fs = slurp(fragmentPath);
    if (vs.empty()) {
        std::cerr << "[Shader] cannot read vertex shader: " << vertexPath << '\n';
        return false;
    }
    if (fs.empty()) {
        std::cerr << "[Shader] cannot read fragment shader: " << fragmentPath << '\n';
        return false;
    }
    return loadFromSources(vs, fs);
}

void Shader::bind() const { glUseProgram(programId_); }

GLint Shader::uniformLocation(const std::string& name) const {
    auto it = uniformCache_.find(name);
    if (it != uniformCache_.end()) return it->second;
    GLint loc = glGetUniformLocation(programId_, name.c_str());
    uniformCache_.emplace(name, loc);
    return loc;
}

void Shader::setBool (const std::string& n, bool  v) const { glUniform1i(uniformLocation(n), v ? 1 : 0); }
void Shader::setInt  (const std::string& n, int   v) const { glUniform1i(uniformLocation(n), v); }
void Shader::setFloat(const std::string& n, float v) const { glUniform1f(uniformLocation(n), v); }
void Shader::setVec3 (const std::string& n, const glm::vec3& v) const { glUniform3fv(uniformLocation(n), 1, glm::value_ptr(v)); }
void Shader::setMat3 (const std::string& n, const glm::mat3& m) const { glUniformMatrix3fv(uniformLocation(n), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4 (const std::string& n, const glm::mat4& m) const { glUniformMatrix4fv(uniformLocation(n), 1, GL_FALSE, glm::value_ptr(m)); }

} // namespace chess3d
