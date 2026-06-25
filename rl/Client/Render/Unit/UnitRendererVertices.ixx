export module Rl.Client.Render.Unit.UnitRendererVertices;

import Rl.Client.Render.Unit.UnitRendererInfo;
import <glm/glm.hpp>;
import <vector>;

namespace Rl::Client::Render
{

/* Gets the static unit vertex data */
export const std::vector<UnitRenderVertex>& UnitGetTestVertices();

/* Generate cube indices dynamically for indexed drawing */
export std::vector<uint32_t> UnitGenerateIndices(uint32_t verticesPerFace, uint32_t faceCount);

} // namespace Rl::Client::Render
