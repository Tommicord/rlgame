import Rl.Client.Render.Unit.UnitRendererFrustum;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

namespace Rl::Client::Render
{

void UnitCameraToFrustumPlanes(UnitRenderFrustumPlanes& frustum, World::Camera& cam)
{
  // Extract frustum planes from view-projection matrix (world space)
  // Negate to get inward-pointing normals (points inside have positive distance)
  glm::mat4 vp = cam.GetProjectionMatrix() * cam.GetViewMatrix();
  glm::mat4 m  = glm::transpose(vp);

  frustum.planes[0] = -(m[3] + m[0]); // Left
  frustum.planes[1] = -(m[3] - m[0]); // Right
  frustum.planes[2] = -(m[3] + m[1]); // Bottom
  frustum.planes[3] = -(m[3] - m[1]); // Top
  frustum.planes[4] = -(m[3] + m[2]); // Near
  frustum.planes[5] = -(m[3] - m[2]); // Far

  for (int i = 0; i < 6; ++i)
  {
    float length = glm::length(glm::vec3(frustum.planes[i]));
    if (length > 0.0001f)
    {
      frustum.planes[i] /= length;
    }
  }
}

} // namespace Rl::Client::Render
