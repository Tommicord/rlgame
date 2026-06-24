#pragma once

#include "rl/Client/Render/Unit/UnitRendererInfo.h"
#include "rl/World/Camera.h"

namespace Rl::Client::Render
{

/*
 * Extract frustum planes from camera view-projection matrix
 * Gets the camera View-Projection matrix and do some math
 * behind the scene
 */
void UnitCameraToFrustumPlanes(UnitRenderFrustumPlanes& frustum, World::Camera& cam);

} // namespace Rl::Client::Render
