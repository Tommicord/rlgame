#pragma once

#include "rl/Client/Render/Unit/UnitRendererInfo.h"

#include <vector>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Rl::Client::Render
{

/* Gets the static unit vertex data */
const std::vector<UnitRenderVertex>& UnitGetTestVertices();

/* Generate cube indices dynamically for indexed drawing */
std::vector<uint32_t> UnitGenerateIndices(uint32_t verticesPerFace, uint32_t faceCount);

} // namespace Rl::Client::Render
