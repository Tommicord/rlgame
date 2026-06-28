export module Rl.Client.Render.Unit.UnitRendererFrustum;

import Rl.Client.Render.Unit.UnitRendererInfo;
import Rl.Player.PlayerCamera;

namespace Rl::Client::Render
{

/*
 * Extract frustum planes from camera view-projection matrix
 * Gets the camera View-Projection matrix and do some math
 * behind the scene
 */
export void UnitCameraToFrustumPlanes(
    UnitRenderFrustumPlanes& frustum, const Player::IPlayerCamera& cam);

} // namespace Rl::Client::Render
