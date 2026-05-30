#pragma once

#include "Mesh.h"
#include "Common.h"
#include <string>
#include <vector>
#include <memory>

struct aiNode;
struct aiScene;
struct aiMesh;

namespace chess3d {

// A renderable hierarchy of meshes loaded from disk through Assimp.
// All sub-meshes are pre-transformed into a single object space and
// normalized so that the model fits a unit bounding box (centered on
// origin, base at y=0). This lets us use one uniform model matrix.
class Model {
public:
    Model() = default;

    // Load every mesh in the file.
    bool loadFromFile(const std::string& path);

    // Load only meshes whose owning node's name contains `nodeNameFilter`
    // (case-insensitive substring match). Useful for multi-piece scenes
    // such as Blender's "All chess pieces" file - pass "pawn", "knight",
    // ... to extract a single piece type.
    bool loadFromFileFiltered(const std::string& path,
                              const std::string& nodeNameFilter);

    // Load a single-mesh file (typically a multi-piece export with no
    // named groups, e.g. chessPieces.obj) and split it into connected
    // components. Returns one Model per component, sorted by Y-extent
    // ascending (so the shortest piece comes first).
    static std::vector<std::unique_ptr<Model>>
    loadAndSplitByComponents(const std::string& path);

    void draw() const;

    bool empty()  const { return meshes_.empty(); }
    std::size_t meshCount() const { return meshes_.size(); }

    // Per-model transform that normalises a freshly imported mesh into our
    // engine space: rotates Blender's Z-up convention to Y-up, recenters on
    // XZ, drops the base to Y=0 and scales the bounding box height to a
    // chess-piece-sized value. The renderer multiplies this into the model
    // matrix before drawing.
    const glm::mat4& normalizationTransform() const { return normalizationTransform_; }

private:
    void processNode(const aiNode* node, const aiScene* scene,
                     const glm::mat4& parent,
                     const std::string& nodeNameFilter,
                     bool insideFilteredSubtree);
    void processMesh(const aiMesh* mesh, const glm::mat4& transform);
    void normalize();

    std::vector<std::unique_ptr<Mesh>> meshes_;
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};
    bool      boundsInit_ = false;
    glm::mat4 normalizationTransform_ {1.0f};
};

} // namespace chess3d
