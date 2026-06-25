import Rl.Client.State.CameraState;
import Rl.Base.Game;
import Rl.World.Camera;

import <algorithm>;
import <cmath>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/type_ptr.hpp>;

namespace Rl::Providers
{

CameraModel::CameraModel(Game::VulkanContext& context) : StateModel(context)
{
  // Create camera
  camera = std::make_unique<World::Camera>();
  // Set aspect ratio based on swap chain extent
  const float aspect = static_cast<float>(context.swapChainExtent.width) /
                       static_cast<float>(context.swapChainExtent.height);
  camera->SetAspectRatio(aspect);
  cameraDrawable = std::make_shared<CameraStateDrawable>();
  cameraResource = std::make_unique<CameraStateResource>(*camera);
  cameraVk       = std::make_unique<CameraStateDrawableVulkan>();
  cameraDrawable->OnCreate(*cameraResource, *cameraVk, context);
}

World::Camera& CameraModel::GetObject()
{
  return *camera;
}

CameraStateResource& CameraModel::GetResource()
{
  return *cameraResource;
}

CameraStateDrawable& CameraModel::GetDrawable()
{
  return *cameraDrawable;
}

CameraStateDrawableVulkan& CameraModel::GetVulkanState()
{
  return *cameraVk;
}

void CameraStateDrawable::OnPause()
{
}

void CameraStateDrawable::OnResume()
{
}

} // namespace Rl::Providers
