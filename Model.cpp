/**
 * @file Model.cpp
 * @brief Plik zrodlowy projektu Chess3D.
 */

#include "Model.h"

#include <iostream>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace chess3d {

void Model::normalize() {
    if (!boundsInit_) {
        normalizationTransform_ = glm::mat4(1.0f);
        return;
    }

    // Step 1: pick an axis fix. Chess pieces are tall, so whichever axis has
    // the largest extent in the imported model is the "up" axis. We rotate it
    // to match our engine's +Y up convention. Files exported as Y-up (most
    // .obj/.fbx) need no rotation; .blend / 3ds Max files with Z-up are
    // detected automatically.
    const glm::vec3 ext = boundsMax_ - boundsMin_;
    glm::mat4 axisFix(1.0f);
    if (ext.z > ext.y && ext.z >= ext.x) {
        // Z is the tallest axis -> rotate Z-up to Y-up.
        axisFix = glm::rotate(glm::mat4(1.0f), -glm::half_pi<float>(), glm::vec3(1, 0, 0));
    } else if (ext.x > ext.y && ext.x > ext.z) {
        // X is the tallest axis (rare) -> rotate to Y-up.
        axisFix = glm::rotate(glm::mat4(1.0f),  glm::half_pi<float>(), glm::vec3(0, 0, 1));
    }
    // else: Y already up, no rotation needed.

    // Step 2: compute axis-fixed bounding box by transforming the 8 corners.
    const glm::vec3 corners[8] = {
        {boundsMin_.x, boundsMin_.y, boundsMin_.z},
        {boundsMax_.x, boundsMin_.y, boundsMin_.z},
        {boundsMin_.x, boundsMax_.y, boundsMin_.z},
        {boundsMax_.x, boundsMax_.y, boundsMin_.z},
        {boundsMin_.x, boundsMin_.y, boundsMax_.z},
        {boundsMax_.x, boundsMin_.y, boundsMax_.z},
        {boundsMin_.x, boundsMax_.y, boundsMax_.z},
        {boundsMax_.x, boundsMax_.y, boundsMax_.z},
    };
    glm::vec3 rmin( std::numeric_limits<float>::max());
    glm::vec3 rmax(-std::numeric_limits<float>::max());
    for (const auto& c : corners) {
        glm::vec3 r = glm::vec3(axisFix * glm::vec4(c, 1.0f));
        rmin = glm::min(rmin, r);
        rmax = glm::max(rmax, r);
    }
    glm::vec3 center = (rmin + rmax) * 0.5f;
    float height = std::max(rmax.y - rmin.y, 0.0001f);

    // Step 3: target height matches a typical chess piece visually.
    // Pawn-sized scenes will be small, king-sized scenes will be tall;
    // a single global per-model scale keeps relative proportions intact.
    constexpr float kTargetHeight = 1.4f;
    const float scale = kTargetHeight / height;

    // Step 4: place the centre on the XZ axes and drop the base to y = 0
    // (translation applied AFTER the axis rotation).
    const glm::vec3 translation(-center.x, -rmin.y, -center.z);

    normalizationTransform_ =
        glm::scale    (glm::mat4(1.0f), glm::vec3(scale)) *
        glm::translate(glm::mat4(1.0f), translation)      *
        axisFix;
}

void Model::draw() const {
    for (const auto& m : meshes_) m->draw();
}

// ---------------------------------------------------------------------------
// Connected-component split for combined chess-piece meshes
// ---------------------------------------------------------------------------
namespace {

// Tiny union-find over vertex indices.
struct UnionFind {
    std::vector<int> parent;
    explicit UnionFind(std::size_t n) : parent(n) {
        for (std::size_t i = 0; i < n; ++i) parent[i] = static_cast<int>(i);
    }
    int find(int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    }
    void unite(int a, int b) {
        a = find(a); b = find(b);
        if (a != b) parent[b] = a;
    }
};

} // anon

std::vector<std::unique_ptr<Model>>
Model::loadAndSplitByComponents(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "[Model] Assimp error for \"" << path << "\": "
                  << importer.GetErrorString() << '\n';
        return {};
    }

    // 1) Flatten the scene into one big vertex+index buffer. We can ignore
    //    node transforms here because chess-piece meshes typically come in
    //    a single shared frame already.
    std::vector<Vertex>            allVerts;
    std::vector<std::uint32_t>     allIdx;
    for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];
        const std::uint32_t base = static_cast<std::uint32_t>(allVerts.size());

        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            Vertex v;
            v.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
            v.normal   = mesh->HasNormals()
                ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
                : glm::vec3(0.0f, 1.0f, 0.0f);
            v.uv = mesh->HasTextureCoords(0)
                ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                : glm::vec2(v.position.x * 0.5f + 0.5f, v.position.z * 0.5f + 0.5f);
            allVerts.push_back(v);
        }
        for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            allIdx.push_back(base + face.mIndices[0]);
            allIdx.push_back(base + face.mIndices[1]);
            allIdx.push_back(base + face.mIndices[2]);
        }
    }
    if (allVerts.empty() || allIdx.size() < 3) return {};

    // 2) Union-find: join every pair of indices that appear together in a
    //    triangle. Connected components in the index space correspond to
    //    spatially disjoint sub-meshes (i.e. separate chess pieces).
    UnionFind uf(allVerts.size());
    for (std::size_t i = 0; i + 2 < allIdx.size(); i += 3) {
        uf.unite(static_cast<int>(allIdx[i]),     static_cast<int>(allIdx[i + 1]));
        uf.unite(static_cast<int>(allIdx[i + 1]), static_cast<int>(allIdx[i + 2]));
    }

    // 3) Bucket faces by their root component.
    std::unordered_map<int, std::vector<std::uint32_t>> facesByRoot;
    facesByRoot.reserve(16);
    for (std::size_t i = 0; i + 2 < allIdx.size(); i += 3) {
        int root = uf.find(static_cast<int>(allIdx[i]));
        auto& bucket = facesByRoot[root];
        bucket.push_back(allIdx[i]);
        bucket.push_back(allIdx[i + 1]);
        bucket.push_back(allIdx[i + 2]);
    }

    // 4) Build a Model per component, reindexing vertices densely.
    struct BuiltComponent {
        std::unique_ptr<Model> model;
        float height = 0.0f;
    };
    std::vector<BuiltComponent> built;
    built.reserve(facesByRoot.size());

    for (auto& kv : facesByRoot) {
        std::vector<Vertex>          cVerts;
        std::vector<std::uint32_t>   cIdx;
        std::unordered_map<std::uint32_t, std::uint32_t> remap;
        cVerts.reserve(kv.second.size());
        cIdx.reserve(kv.second.size());

        glm::vec3 bmin( std::numeric_limits<float>::max());
        glm::vec3 bmax(-std::numeric_limits<float>::max());

        for (std::uint32_t oldI : kv.second) {
            auto it = remap.find(oldI);
            std::uint32_t newI;
            if (it == remap.end()) {
                newI = static_cast<std::uint32_t>(cVerts.size());
                cVerts.push_back(allVerts[oldI]);
                remap[oldI] = newI;
                bmin = glm::min(bmin, allVerts[oldI].position);
                bmax = glm::max(bmax, allVerts[oldI].position);
            } else {
                newI = it->second;
            }
            cIdx.push_back(newI);
        }

        if (cVerts.empty() || cIdx.size() < 3) continue;

        auto model = std::make_unique<Model>();
        auto mesh  = std::make_unique<Mesh>();
        mesh->upload(cVerts, cIdx);
        model->meshes_.push_back(std::move(mesh));
        model->boundsMin_   = bmin;
        model->boundsMax_   = bmax;
        model->boundsInit_  = true;
        model->normalize();

        BuiltComponent bc;
        bc.height = bmax.y - bmin.y;
        bc.model  = std::move(model);
        built.push_back(std::move(bc));
    }

    // 5) Drop tiny floaters (e.g. helper geometry left in the scene) - keep
    //    components whose face count is at least a small fraction of the median.
    if (built.size() > 6) {
        std::sort(built.begin(), built.end(),
                  [](const BuiltComponent& a, const BuiltComponent& b) {
                      return a.model->meshes_[0]->indexCount() >
                             b.model->meshes_[0]->indexCount();
                  });
        built.resize(6);
    }

    // 6) Final sort by height ascending so the renderer can map components
    //    to pieces by their typical relative heights.
    std::sort(built.begin(), built.end(),
              [](const BuiltComponent& a, const BuiltComponent& b) {
                  return a.height < b.height;
              });

    std::vector<std::unique_ptr<Model>> out;
    out.reserve(built.size());
    for (auto& bc : built) out.push_back(std::move(bc.model));
    return out;
}

} // namespace chess3d

