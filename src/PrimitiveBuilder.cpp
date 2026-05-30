#include "PrimitiveBuilder.h"

namespace chess3d::primitives {

static std::uint32_t pushVertex(std::vector<Vertex>& v,
                                const glm::vec3& p,
                                const glm::vec3& n,
                                const glm::vec2& uv) {
    v.push_back({p, n, uv});
    return static_cast<std::uint32_t>(v.size() - 1);
}

void appendBox(std::vector<Vertex>& v, std::vector<std::uint32_t>& idx,
               const glm::vec3& c, const glm::vec3& h) {
    // 6 faces, 4 vertices each, distinct normals/UV.
    const glm::vec3 p[8] = {
        c + glm::vec3(-h.x, -h.y, -h.z),
        c + glm::vec3( h.x, -h.y, -h.z),
        c + glm::vec3( h.x,  h.y, -h.z),
        c + glm::vec3(-h.x,  h.y, -h.z),
        c + glm::vec3(-h.x, -h.y,  h.z),
        c + glm::vec3( h.x, -h.y,  h.z),
        c + glm::vec3( h.x,  h.y,  h.z),
        c + glm::vec3(-h.x,  h.y,  h.z),
    };

    auto face = [&](int a, int b, int cIdx, int d, const glm::vec3& n) {
        std::uint32_t i0 = pushVertex(v, p[a], n, {0,0});
        std::uint32_t i1 = pushVertex(v, p[b], n, {1,0});
        std::uint32_t i2 = pushVertex(v, p[cIdx], n, {1,1});
        std::uint32_t i3 = pushVertex(v, p[d], n, {0,1});
        idx.insert(idx.end(), { i0, i1, i2, i0, i2, i3 });
    };

    face(4, 5, 6, 7, { 0, 0, 1});   // front +Z
    face(1, 0, 3, 2, { 0, 0,-1});   // back  -Z
    face(0, 4, 7, 3, {-1, 0, 0});   // left
    face(5, 1, 2, 6, { 1, 0, 0});   // right
    face(3, 7, 6, 2, { 0, 1, 0});   // top
    face(0, 1, 5, 4, { 0,-1, 0});   // bottom
}

} // namespace chess3d::primitives
