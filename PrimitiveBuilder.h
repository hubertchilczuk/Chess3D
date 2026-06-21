/**
 * @file PrimitiveBuilder.h
 * @brief Plik zrodlowy projektu Chess3D.
 */

#pragma once

#include "Mesh.h"
#include <vector>

namespace chess3d {

// Small helpers for procedurally building primitives. They append
// vertices/indices to user-supplied buffers so several primitives
// can share one mesh.
namespace primitives {

void appendBox(std::vector<Vertex>& v, std::vector<std::uint32_t>& i,
               const glm::vec3& center, const glm::vec3& halfExtents);

} // namespace primitives
} // namespace chess3d

