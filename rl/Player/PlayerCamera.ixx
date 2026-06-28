export module Rl.Player.PlayerCamera;

import Rl.Base.IUpdatable;
import Rl.Base.UserInput;

import <array>;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/type_ptr.hpp>;

namespace Rl::Player
{

using namespace Rl::Providers;

export class IPlayerCamera
{
  public:
  struct Eye
  {
    double x, y, z;
  };
  struct Matrix
  {
    std::array<float, 16> matrix;
  };
  double far, near;
  Eye    eye;
  float  fov;
  float  aspectRatio;
  float  zoom;
  float  pitch, yaw;
  virtual ~IPlayerCamera() = default;
  IPlayerCamera() = default;
  IPlayerCamera(const IPlayerCamera& other) = delete;
  virtual void SetPVMMatrix(const Matrix& mvp) = 0;
  virtual void SetRotateXYZ(const Eye& eye) = 0;
  virtual void SetEyePosition(const Eye& eye) = 0;
  virtual void SetFar(double far) = 0;
  virtual void SetNear(double near) = 0;
  virtual void SetAspectRatio(float aspectRatio) = 0;
  virtual void SetFov(float fov) = 0;
  virtual void SetZoom(float zoom) = 0;
  [[nodiscard]]
  virtual float GetAspectRatio() const = 0;
  [[nodiscard]]
  virtual glm::mat4 GetViewMatrix() const = 0;
  [[nodiscard]]
  virtual glm::mat4 GetProjectionMatrix() const = 0;
  [[nodiscard]]
  virtual glm::mat4 GetModelMatrix() const = 0;
  [[nodiscard]]
  virtual glm::mat4 GetPVMMatrix() const = 0;
};

export struct PlayerCameraInput : Input::IInputObserver
{
  PlayerCameraInput() : IInputObserver(*this)
  {
  }
  void OnKeyEvent(const Input::KeyEvent& event) override = 0;
  void OnMouseButtonEvent(const Input::MouseButtonEvent& event) override = 0;
  void OnMouseMoveEvent(const Input::MouseMoveEvent& event) override = 0;
  void OnMouseScrollEvent(const Input::MouseScrollEvent& event) override = 0;
};

export class PlayerCamera final : public IPlayerCamera,
                                  public IUpdatable
{
  public:
  PlayerCamera();
  ~PlayerCamera() override = default;
  void Update() override;
  void SetPVMMatrix(const Matrix& mvp) override;
  void SetRotateXYZ(const Eye& eye) override;
  void SetEyePosition(const Eye& eye) override;
  void SetFar(double far) override;
  void SetNear(double near) override;
  void SetAspectRatio(float aspectRatio) override;
  void SetFov(float fov) override;
  void SetZoom(float zoom) override;
  [[nodiscard]]
  float GetAspectRatio() const override;
  [[nodiscard]]
  glm::mat4 GetViewMatrix() const override;
  [[nodiscard]]
  glm::mat4 GetProjectionMatrix() const override;
  [[nodiscard]]
  glm::mat4 GetModelMatrix() const override;
  [[nodiscard]]
  glm::mat4 GetPVMMatrix() const override;

  private:
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  glm::mat4 matrix;
  glm::vec3 front;
  glm::vec3 right;
  glm::vec3 up;
};

} // namespace Rl::Player
