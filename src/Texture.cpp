#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <random>
#include <cmath>
#include <vector>
#include <algorithm>

// Anisotropic filtering constants are part of EXT_texture_filter_anisotropic.
// GLEW exposes them on every Desktop GL driver but we keep these fallbacks
// just so the file compiles even on minimal headers.
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

namespace chess3d {

Texture::~Texture() {
    if (texId_) {
        glDeleteTextures(1, &texId_);
        texId_ = 0;
    }
}

Texture::Texture(Texture&& other) noexcept
    : texId_(other.texId_), width_(other.width_), height_(other.height_) {
    other.texId_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (texId_) glDeleteTextures(1, &texId_);
        texId_  = other.texId_;
        width_  = other.width_;
        height_ = other.height_;
        other.texId_ = 0;
    }
    return *this;
}

bool Texture::uploadRGBA(const unsigned char* data, int w, int h, bool srgb, bool generateMips) {
    if (texId_) { glDeleteTextures(1, &texId_); texId_ = 0; }
    glGenTextures(1, &texId_);
    glBindTexture(GL_TEXTURE_2D, texId_);

    GLenum internalFmt = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    if (generateMips) glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    generateMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLfloat maxAniso = 1.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
    if (maxAniso > 1.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min(maxAniso, 8.0f));
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    width_  = w;
    height_ = h;
    return true;
}

bool Texture::loadFromFile(const std::string& path, bool srgb, bool generateMips) {
    int w = 0, h = 0, c = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
    if (!data) {
        std::cerr << "[Texture] cannot load \"" << path << "\": " << stbi_failure_reason() << '\n';
        return false;
    }
    bool ok = uploadRGBA(data, w, h, srgb, generateMips);
    stbi_image_free(data);
    return ok;
}

// ------- Simple value-noise based wood --------------------------------------
namespace {

float hash2(int x, int y, std::uint32_t seed) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393u
                    + static_cast<std::uint32_t>(y) * 668265263u
                    + seed;
    h = (h ^ (h >> 13)) * 1274126177u;
    return static_cast<float>(h & 0x00FFFFFFu) / static_cast<float>(0x01000000);
}

float smoothNoise(float x, float y, std::uint32_t seed) {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    float tx = x - x0;
    float ty = y - y0;
    auto fade = [](float t){ return t*t*(3.0f - 2.0f*t); };
    float u = fade(tx);
    float v = fade(ty);
    float a = hash2(x0,   y0,   seed);
    float b = hash2(x0+1, y0,   seed);
    float c = hash2(x0,   y0+1, seed);
    float d = hash2(x0+1, y0+1, seed);
    float ab = a + (b - a) * u;
    float cd = c + (d - c) * u;
    return ab + (cd - ab) * v;
}

float fbm(float x, float y, std::uint32_t seed, int octaves = 4) {
    float amp = 0.5f, freq = 1.0f, sum = 0.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum  += amp * smoothNoise(x * freq, y * freq, seed + i*131);
        norm += amp;
        amp  *= 0.5f;
        freq *= 2.0f;
    }
    return sum / norm;
}

} // anon

bool Texture::createProceduralWood(int size, bool lightWood) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(size) * size * 4);

    glm::vec3 base, dark;
    std::uint32_t seed;
    if (lightWood) {
        base = {0.85f, 0.70f, 0.46f};  // maple
        dark = {0.55f, 0.38f, 0.20f};
        seed = 1337u;
    } else {
        base = {0.32f, 0.20f, 0.12f};  // walnut
        dark = {0.10f, 0.06f, 0.04f};
        seed = 9001u;
    }

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = static_cast<float>(x) / size;
            float v = static_cast<float>(y) / size;

            // Tight ring pattern + low-freq fbm warp -> wood grain.
            float warp = fbm(u * 6.0f, v * 6.0f, seed, 4) * 0.4f;
            float rings = std::sin((v * 22.0f + warp * 14.0f) * 3.14159f);
            float grain = fbm(u * 64.0f, v * 12.0f, seed + 5, 3);

            float t = 0.5f + 0.5f * rings;
            t = glm::clamp(t * 0.8f + grain * 0.4f, 0.0f, 1.0f);

            glm::vec3 col = glm::mix(dark, base, t);

            int idx = (y * size + x) * 4;
            pixels[idx + 0] = static_cast<unsigned char>(glm::clamp(col.r, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 1] = static_cast<unsigned char>(glm::clamp(col.g, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 2] = static_cast<unsigned char>(glm::clamp(col.b, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 3] = 255;
        }
    }
    return uploadRGBA(pixels.data(), size, size, /*srgb*/true, /*mips*/true);
}

bool Texture::createProceduralMarble(int size, bool whiteMarble) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(size) * size * 4);

    glm::vec3 base, vein;
    std::uint32_t seed;
    if (whiteMarble) {
        base = {0.93f, 0.92f, 0.88f};
        vein = {0.55f, 0.55f, 0.55f};
        seed = 4242u;
    } else {
        base = {0.12f, 0.10f, 0.10f};
        vein = {0.35f, 0.30f, 0.28f};
        seed = 7777u;
    }

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float u = static_cast<float>(x) / size;
            float v = static_cast<float>(y) / size;

            float n  = fbm(u * 5.0f, v * 5.0f, seed, 5);
            float n2 = fbm(u * 14.0f + 3.0f, v * 14.0f + 7.0f, seed + 1, 4);
            float veins = std::abs(std::sin((u + v + n2) * 11.0f + n * 6.0f));
            veins = std::pow(1.0f - veins, 6.0f);

            glm::vec3 col = glm::mix(base, vein, veins);

            int idx = (y * size + x) * 4;
            pixels[idx + 0] = static_cast<unsigned char>(glm::clamp(col.r, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 1] = static_cast<unsigned char>(glm::clamp(col.g, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 2] = static_cast<unsigned char>(glm::clamp(col.b, 0.0f, 1.0f) * 255.0f);
            pixels[idx + 3] = 255;
        }
    }
    return uploadRGBA(pixels.data(), size, size, /*srgb*/true, /*mips*/true);
}

void Texture::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture  (GL_TEXTURE_2D, texId_);
}

void Texture::unbind(GLuint unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture  (GL_TEXTURE_2D, 0);
}

} // namespace chess3d
