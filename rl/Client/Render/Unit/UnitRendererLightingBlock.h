#pragma once

#include <glm/glm.hpp>

namespace Rl::Client::Render
{

/* Defines a lighting block structure that contains
 * Metadata about the sun direction, color, exposure,
 * Sky color, ground color, etc.
 */
struct UnitRenderLightingBlock
{
    alignas(16) glm::vec3 sunDirection;
    alignas(16) glm::vec3 sunColor;
    alignas(4) float ambientStrength;
    alignas(16) glm::vec3 cameraPosition;
    alignas(4) float exposure;
    alignas(4) float padding1; // Padding for 16-byte alignment
    alignas(4) float padding2; // Padding for 16-byte alignment
    alignas(4) float padding3; // Padding for 16-byte alignment
    alignas(16) glm::vec3 groundColor;
    alignas(16) glm::vec3 skyColor;
};

} // namespace Rl::Client::Render
