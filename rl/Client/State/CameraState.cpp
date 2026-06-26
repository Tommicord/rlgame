import Rl.Client.State.CameraState;
import Rl.Base.Game;
import Rl.Base.Binding;
import Rl.World.Camera;

import <algorithm>;
import <cmath>;
import <memory>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

namespace Rl::Providers
{

CameraModel::CameraModel(Game::MainBinding& context) : IStateModel(context)
{
  // Create camera
  camera = std::make_unique<World::Camera>();
  // Set aspect ratio based on swap chain extent
  const float aspect = static_cast<float>(context.swapChainExtent.width) /
                       static_cast<float>(context.swapChainExtent.height);
  camera->SetAspectRatio(aspect);
  cameraDrawable = std::make_shared<CameraStateDrawable>();
  cameraResource = std::make_unique<CameraStateResource>(*camera);
  cameraBinding = std::make_unique<CameraStateBinding>();
  cameraDrawable->OnCreate(*cameraResource, *cameraBinding, context);
}

World::Camera& CameraModel::GetObject() const
{
  return *camera;
}

CameraStateResource& CameraModel::GetResource() const
{
  return *cameraResource;
}

CameraStateDrawable& CameraModel::GetDrawable() const
{
  return *cameraDrawable;
}

CameraStateBinding& CameraModel::GetBinding() const
{
  return *cameraBinding;
}

void CameraStateDrawable::OnPause()
{
}

void CameraStateDrawable::OnResume()
{
}

} // namespace Rl::Providers
