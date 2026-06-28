import Rl.Player.PlayerCamera;

import <algorithm>;
import <cmath>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/type_ptr.hpp>;

namespace Rl::Player
{

PlayerCamera::PlayerCamera()
{
  eye.x = 0.0;
  eye.y = 0.0;
  eye.z = 3.0;
  far = 1000.0, near = 0.1, fov = 45.0f;
  aspectRatio = 16.0f / 9.0f;
  zoom = 1.0f, pitch = 0.0f;
  yaw = 0.0f;
  model = glm::mat4(1.0f);
  view = glm::mat4(1.0f);
  projection = glm::mat4(1.0f);
  matrix = glm::mat4(1.0f);
  Update();
}

void PlayerCamera::Update()
{
  // Calculate front vector from pitch and yaw
  glm::vec3 front;
  front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y = sin(glm::radians(pitch));
  front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  front = glm::normalize(front);

  // Calculate right and up vectors
  right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
  up = glm::normalize(glm::cross(right, front));

  // View matrix, look at position + front
  const glm::vec3 cameraPos = glm::vec3(eye.x, eye.y, eye.z);
  view = glm::lookAt(cameraPos, cameraPos + front, up);
  // Projection matrix
  const float adjustedFov = fov / zoom;
  projection = glm::perspective(glm::radians(adjustedFov), aspectRatio,
      static_cast<float>(near), static_cast<float>(far));
  // Model matrix (identity for camera)
  model = glm::mat4(1.0f);
  // PVM matrix
  matrix = projection * view * model;
}

void PlayerCamera::SetPVMMatrix(const Matrix& mvp)
{
  glm::mat4 glmMatrix = glm::make_mat4(mvp.matrix.data());
  matrix = glmMatrix;
}

void PlayerCamera::SetRotateXYZ(const Eye& rotation)
{
  pitch = static_cast<float>(rotation.x);
  yaw = static_cast<float>(rotation.y);
  Update();
}

void PlayerCamera::SetEyePosition(const Eye& position)
{
  eye.x = position.x;
  eye.y = position.y;
  eye.z = position.z;
  Update();
}

void PlayerCamera::SetFar(const double far)
{
  this->far = far;
  Update();
}

void PlayerCamera::SetNear(const double near)
{
  this->near = near;
  Update();
}

void PlayerCamera::SetAspectRatio(const float aspectRatio)
{
  this->aspectRatio = aspectRatio;
  Update();
}

void PlayerCamera::SetFov(const float fov)
{
  this->fov = fov;
  Update();
}

void PlayerCamera::SetZoom(const float zoom)
{
  this->zoom = zoom;
  Update();
}

float PlayerCamera::GetAspectRatio() const
{
  return aspectRatio;
}

glm::mat4 PlayerCamera::GetViewMatrix() const
{
  return view;
}

glm::mat4 PlayerCamera::GetProjectionMatrix() const
{
  return projection;
}

glm::mat4 PlayerCamera::GetModelMatrix() const
{
  return model;
}

glm::mat4 PlayerCamera::GetPVMMatrix() const
{
  return matrix;
}

} // namespace Rl::Player
