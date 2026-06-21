/**
 * @file Texture.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace chess3d {

/// @brief Reprezentuje teksturę OpenGL wczytaną z pliku lub wygenerowaną proceduralnie.
class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool loadFromFile(const std::string& path, bool srgb = true, bool generateMips = true);

    // Procedurally generated wood-like texture.
    // lightWood = true -> light maple; false -> dark walnut.
    bool createProceduralWood(int size, bool lightWood);

    // Procedurally generated marble pattern for pieces.
    bool createProceduralMarble(int size, bool whiteMarble);

    void bind(GLuint unit = 0) const;

    GLuint id() const { return texId_; }
    int    width()  const { return width_;  }
    int    height() const { return height_; }
    bool   valid()  const { return texId_ != 0; }

private:
    bool uploadRGBA(const unsigned char* data, int w, int h, bool srgb, bool generateMips);

    GLuint texId_  = 0;
    int    width_  = 0;
    int    height_ = 0;
};

} // namespace chess3d

