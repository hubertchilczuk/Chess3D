#pragma once

#include "Mesh.h"
#include "Common.h"
#include <string>
#include <vector>
#include <memory>

namespace chess3d {

// Renderable model loaded from a combined chess-pieces file via Assimp.
// Meshes are normalized into engine space: Y-up, base at y=0, height ~1.4 units.
class Model {
public:
    Model() = default;

    // Load a file with no named groups and split it into connected components.
    // Returns one Model per component, sorted by Y-extent ascending
    // (shortest piece first).
    static std::vector<std::unique_ptr<Model>>
    loadAndSplitByComponents(const std::string& path);

    void draw() const;

    bool empty() const { return meshes_.empty(); }

    // Applied by the renderer as part of the model matrix before drawing.
    const glm::mat4& normalizationTransform() const { return normalizationTransform_; }

private:
    void normalize();

    std::vector<std::unique_ptr<Mesh>> meshes_;
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};
    bool      boundsInit_ = false;
    glm::mat4 normalizationTransform_ {1.0f};
};

} // namespace chess3d
