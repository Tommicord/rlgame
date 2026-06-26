import Rl.Client.Render.Unit.UnitRendererVertices;
import Rl.Client.Render.Unit.UnitRendererInfo;

import <glm/glm.hpp>;

namespace Rl::Client::Render
{
// Testing vertices
// Unit ramp vertices (6 faces, 4 vertices per face for indexed drawing)
// All faces are clockwise when viewed from outside for VK_FRONT_FACE_CLOCKWISE with
// VK_CULL_MODE_BACK_BIT
static const std::vector<UnitRenderVertex> unitVertices = {
    {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 0.0f), 0, 0, 0, 0.0f, 0.95f, 0.0f, glm::vec4(-0.45f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.6f, 0.4f, 0.2f, 0.0f), glm::vec4(0.0f, 1.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(0.0f, 0.707f, -0.707f, 0.0f)},
    // Top face
    {glm::vec4(-0.5f, 0.0f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 1.0f), 0, 0, 0, 0.0f, 0.95f, 0.0f, glm::vec4(-0.45f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.6f, 0.4f, 0.2f, 0.0f), glm::vec4(0.0f, 1.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(0.0f, 0.707f, -0.707f, 0.0f)},
    {glm::vec4(0.5f, 0.0f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 1.0f), 0, 0, 0, 0.0f, 0.95f, 0.0f, glm::vec4(-0.45f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.6f, 0.4f, 0.2f, 0.0f), glm::vec4(0.0f, 1.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(0.0f, 0.707f, -0.707f, 0.0f)},
    {glm::vec4(0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 0.0f), 0, 0, 0, 0.0f, 0.95f, 0.0f, glm::vec4(-0.45f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.6f, 0.4f, 0.2f, 0.0f), glm::vec4(0.0f, 1.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(0.0f, 0.707f, -0.707f, 0.0f)},
    // Bottom face
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 0.0f), 0, 0, 1, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)},
    {glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 0.0f), 0, 0, 1, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)},
    {glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 1.0f), 0, 0, 1, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)},
    {glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 1.0f), 0, 0, 1, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, -1.0f, 0.0f, 0.0f), glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)},

    // Left face
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 1.0f), 0, 0, 2, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, 0.707f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f)},
    {glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 0.0f), 0, 0, 2, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, 0.707f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f)},
    {glm::vec4(-0.5f, 0.0f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 0.0f), 0, 0, 2, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, 0.707f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f)},
    {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 1.0f), 0, 0, 2, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, 0.707f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f)},

    // Right face
    {glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 0.0f), 0, 0, 3, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)},
    {glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 1.0f), 0, 0, 3, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)},
    {glm::vec4(0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 1.0f), 0, 0, 3, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)},
    {glm::vec4(0.5f, 0.0f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 0.0f), 0, 0, 3, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.55f, 0.38f, 0.22f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
        glm::vec4(0.0f, 0.707f, -0.707f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)},

    // Front face
    {glm::vec4(-0.5f, -0.5f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 1.0f), 0, 0, 4, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)},
    {glm::vec4(0.5f, -0.5f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 1.0f), 0, 0, 4, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)},
    {glm::vec4(0.5f, 0.0f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 0.0f), 0, 0, 4, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)},
    {glm::vec4(-0.5f, 0.0f, 0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 0.0f), 0, 0, 4, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)},

    // Back face
    {glm::vec4(0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 1.0f), 0, 0, 5, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)},
    {glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 1.0f), 0, 0, 5, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)},
    {glm::vec4(-0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(0.0f, 0.0f), 0, 0, 5, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)},
    {glm::vec4(0.5f, 0.5f, -0.5f, 1.0f), glm::vec4(0, 0, 0, 0), glm::vec4(0, 0, 0, 0),
        glm::vec2(1.0f, 0.0f), 0, 0, 5, 0.0f, 0.95f, 0.0f, glm::vec4(0.0f),
        glm::vec4(0.5f, 0.35f, 0.2f, 0.0f), glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)},
};

const std::vector<UnitRenderVertex>& UnitGetTestVertices()
{
  return unitVertices;
}

// Generate cube indices dynamically for indexed drawing
// Each face uses 4 vertices to form 2 triangles in counter-clockwise order
std::vector<uint32_t> UnitGenerateIndices(uint32_t verticesPerFace, uint32_t faceCount)
{
  std::vector<uint32_t> indices;
  indices.reserve(faceCount * 6); // 6 indices per face (2 triangles × 3 vertices)

  for (uint32_t face = 0; face < faceCount; ++face)
  {
    auto baseIndex = face * verticesPerFace;
    // Triangle 1: baseIndex, baseIndex+1, baseIndex+2
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    // Triangle 2: baseIndex, baseIndex+2, baseIndex+3
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
    indices.push_back(baseIndex);
  }

  return indices;
}

} // namespace Rl::Client::Render
