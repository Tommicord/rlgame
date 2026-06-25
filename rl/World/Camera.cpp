import Rl.World.Camera;
import Rl.Client.State.CameraState;

import <algorithm>;
import <cmath>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/type_ptr.hpp>;

namespace Rl::World
{

Camera::Camera()
{
  eye.x = 0.0;
  eye.y = 0.0;
  eye.z = 3.0;
  far = 1000.0, near = 0.1, fov = 45.0f;
  aspectRatio = 16.0f / 9.0f;
  zoom = 1.0f, pitch = 0.0f;
  yaw              = 0.0f;
  modelMatrix      = glm::mat4(1.0f);
  viewMatrix       = glm::mat4(1.0f);
  projectionMatrix = glm::mat4(1.0f);
  pvmMatrix        = glm::mat4(1.0f);
  UpdateMatrices();
}

Camera::~Camera()
{
}

void Camera::Update()
{
  constexpr float moveSpeed = 0.025f;
  // Check pressed keys and apply movement in camera direction
  if (pressedKeys.count(Input::Key::W))
  {
    eye.x += cameraFront.x * moveSpeed;
    eye.y += cameraFront.y * moveSpeed;
    eye.z += cameraFront.z * moveSpeed;
  }
  if (pressedKeys.count(Input::Key::S))
  {
    eye.x -= cameraFront.x * moveSpeed;
    eye.y -= cameraFront.y * moveSpeed;
    eye.z -= cameraFront.z * moveSpeed;
  }
  if (pressedKeys.count(Input::Key::A))
  {
    eye.x -= cameraRight.x * moveSpeed;
    eye.y -= cameraRight.y * moveSpeed;
    eye.z -= cameraRight.z * moveSpeed;
  }
  if (pressedKeys.count(Input::Key::D))
  {
    eye.x += cameraRight.x * moveSpeed;
    eye.y += cameraRight.y * moveSpeed;
    eye.z += cameraRight.z * moveSpeed;
  }
  if (pressedKeys.count(Input::Key::Space))
    eye.y -= moveSpeed;
  if (pressedKeys.count(Input::Key::LeftShift))
    eye.y += moveSpeed;
  UpdateMatrices();
}

void Camera::UpdateMatrices()
{
  // Calculate front vector from pitch and yaw
  glm::vec3 front;
  front.x     = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  front.y     = sin(glm::radians(pitch));
  front.z     = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraFront = glm::normalize(front);

  // Calculate right and up vectors
  cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
  cameraUp    = glm::normalize(glm::cross(cameraRight, cameraFront));

  // View matrix, look at position + front
  glm::vec3 cameraPos = glm::vec3(eye.x, eye.y, eye.z);
  viewMatrix          = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
  // Projection matrix
  float adjustedFov = fov / zoom;
  projectionMatrix  = glm::perspective(
      glm::radians(adjustedFov), aspectRatio, static_cast<float>(near), static_cast<float>(far));
  // Model matrix (identity for camera)
  modelMatrix = glm::mat4(1.0f);
  // PVM matrix
  pvmMatrix = projectionMatrix * viewMatrix * modelMatrix;
}

void Camera::SetPVMMatrix(const PVMMatrix& mvp)
{
  glm::mat4 glmMatrix = glm::make_mat4(mvp.matrix.data());
  pvmMatrix           = glmMatrix;
}

void Camera::SetRotateXYZ(const Eye& rotation)
{
  pitch = static_cast<float>(rotation.x);
  yaw   = static_cast<float>(rotation.y);
  UpdateMatrices();
}

void Camera::SetEyePosition(const Eye& position)
{
  eye.x = position.x;
  eye.y = position.y;
  eye.z = position.z;
  UpdateMatrices();
}

void Camera::SetFar(const double far)
{
  this->far = far;
  UpdateMatrices();
}

void Camera::SetNear(const double near)
{
  this->near = near;
  UpdateMatrices();
}

void Camera::SetAspectRatio(const float aspectRatio)
{
  this->aspectRatio = aspectRatio;
  UpdateMatrices();
}

void Camera::SetFov(const float fov)
{
  this->fov = fov;
  UpdateMatrices();
}

void Camera::SetZoom(const float zoom)
{
  this->zoom = zoom;
  UpdateMatrices();
}

float Camera::GetAspectRatio() const
{
  return aspectRatio;
}

glm::mat4 Camera::GetViewMatrix() const
{
  return viewMatrix;
}

glm::mat4 Camera::GetProjectionMatrix() const
{
  return projectionMatrix;
}

glm::mat4 Camera::GetModelMatrix() const
{
  return modelMatrix;
}

glm::mat4 Camera::GetPVMMatrix() const
{
  return pvmMatrix;
}

void Camera::OnKeyEvent(const Input::KeyEvent& event)
{
  if (event.action == Input::Action::Press)
  {
    pressedKeys.insert(event.key);
  }
  else if (event.action == Input::Action::Release)
  {
    pressedKeys.erase(event.key);
  }
}

void Camera::OnMouseButtonEvent(const Input::MouseButtonEvent& event)
{
  // Handle mouse button input for camera control
  if (event.action == Input::Action::Press)
  {
    // Could start mouse look mode
  }
  else if (event.action == Input::Action::Release)
  {
    // Could end mouse look mode
  }
}

void Camera::OnMouseMoveEvent(const Input::MouseMoveEvent& event)
{
  // Handle mouse movement for camera rotation
  static double lastX      = 0.0;
  static double lastY      = 0.0;
  static bool   firstMouse = true;

  if (firstMouse)
  {
    lastX      = event.x;
    lastY      = event.y;
    firstMouse = false;
    return;
  }
  // Calculate offset from last position
  const double xOffset = event.x - lastX;
  const double yOffset = event.y - lastY;

  lastX = event.x;
  lastY = event.y;
  // Apply sensitivity
  constexpr float sensitivity = 0.1f;

  // Update yaw (horizontal rotation)
  yaw += static_cast<float>(xOffset) * sensitivity;

  // Update pitch (vertical rotation)
  pitch += static_cast<float>(yOffset) * sensitivity;

  // Clamp pitch to prevent gimbal lock
  pitch = std::clamp(pitch, -89.0f, 89.0f);

  UpdateMatrices();
}

void Camera::OnMouseScrollEvent(const Input::MouseScrollEvent& event)
{
  zoom -= static_cast<float>(event.yoffset);
  zoom = std::clamp(zoom, 0.1f, 10.0f);
  UpdateMatrices();
}

} // namespace Rl::World
