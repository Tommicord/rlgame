#include "Rl.Player/PlayerController.h"
#include <cmath>
#include "Rl.Base/GameInput.h"
#include "Rl.Base/GameTime.h"
#include "Rl.Log/Log.h"

namespace rl
{

PlayerController::PlayerController(Player& player) :
    cameraOffset(0.0f, 0.0f, 0.0f), cameraDistance(5.0f), lookSensitivity(0.1f),
    moveInput(0.0f, 0.0f), lookInput(0.0f, 0.0f), lastMousePos(0.0f, 0.0f), sprintPressed(false),
    moveForward(false), moveBackward(false), moveLeft(false), moveRight(false), moveDown(false),
    moveUp(false), player(player)
{
  GameInputInstance::registerReceiver(static_cast<IGameInputReceiver*>(this));

  cameraComplete.pitch       = 45.0f;
  cameraComplete.yaw         = 0.0f;
  cameraComplete.roll        = 0.0f;
  cameraComplete.zoom        = 1.0f;
  cameraComplete.aspectRatio = 16.0f / 9.0f;
  cameraComplete.fov         = 45.0f;
  cameraComplete._near       = 0.01f;
  cameraComplete._far        = 100.0f;
  cameraComplete.up          = Vec3(0.0f, 1.0f, 0.0f);
  cameraComplete.front       = Vec3(0.0f, 0.0f, -1.0f);

  camera.p = Mat4::infinitePerspective(45.0f * 3.14159265358979323846f / 180.0f,
                                       cameraComplete.aspectRatio, cameraComplete._near);
}

PlayerController::~PlayerController()
{
  GameInputInstance::unregisterReceiver(static_cast<IGameInputReceiver*>(this));
}

void PlayerController::handleCamera()
{
  cameraComplete.pitch += lookInput.y * lookSensitivity;
  cameraComplete.yaw += lookInput.x * lookSensitivity;

  // Avoid gimbal lock
  const float maxPitch = 89.0f;
  if (cameraComplete.pitch > maxPitch)
    cameraComplete.pitch = maxPitch;
  if (cameraComplete.pitch < -maxPitch)
    cameraComplete.pitch = -maxPitch;

  constexpr float piDiv        = 3.14159265358979323846f / 180.0f;
  float           pitchRad     = cameraComplete.pitch * piDiv;
  float           yawRad       = cameraComplete.yaw * piDiv;
  float           rollRad      = cameraComplete.roll * piDiv;
  float           effectiveFov = cameraComplete.fov / cameraComplete.zoom;
  float           fovRad       = effectiveFov * piDiv;

  cameraComplete.front.x = std::sin(yawRad) * std::cos(pitchRad);
  cameraComplete.front.y = std::sin(pitchRad);
  cameraComplete.front.z = -std::cos(yawRad) * std::cos(pitchRad);
  cameraComplete.front   = cameraComplete.front.normalized();

  Vec3 right = cameraComplete.front.cross(cameraComplete.up).normalized();
  Vec3 up    = right.cross(cameraComplete.front).normalized();

  if (std::abs(rollRad) > 0.0001f)
  {
    float cosRoll     = std::cos(rollRad);
    float sinRoll     = std::sin(rollRad);
    Vec3  rolledRight = right * cosRoll + up * sinRoll;
    Vec3  rolledUp    = up * cosRoll - right * sinRoll;
    right             = rolledRight.normalized();
    up                = rolledUp.normalized();
  }

  constexpr float positionScale = _coordScale;
  Vec3            playerPos(static_cast<float>(player.x.get()) / positionScale,
                            static_cast<float>(player.y.get()) / positionScale,
                            static_cast<float>(player.z.get()) / positionScale);
  Vec3            cameraPos = playerPos + cameraOffset;

  Mat4 view = Mat4();
  view.m[0] = right.x;
  view.m[1] = up.x;
  view.m[2] = -cameraComplete.front.x;
  view.m[3] = 0.0f;

  view.m[4] = right.y;
  view.m[5] = up.y;
  view.m[6] = -cameraComplete.front.y;
  view.m[7] = 0.0f;

  view.m[8]  = right.z;
  view.m[9]  = up.z;
  view.m[10] = -cameraComplete.front.z;
  view.m[11] = 0.0f;

  view.m[12] = -right.dot(cameraPos);
  view.m[13] = -up.dot(cameraPos);
  view.m[14] = cameraComplete.front.dot(cameraPos);
  view.m[15] = 1.0f;

  camera.v = view;

  camera.p = Mat4::infinitePerspective(fovRad, cameraComplete.aspectRatio, cameraComplete._near);
  camera.m = Mat4::identity();
}

void PlayerController::handleMovement(float deltaTime)
{
  Vec3 moveDirection(0.0f, 0.0f, 0.0f);
  Vec3 forward = cameraComplete.front;
  forward.y    = 0.0f;
  forward      = forward.normalized();

  Vec3 crossedUp = cameraComplete.front.cross(cameraComplete.up);
  Vec3 right     = crossedUp.normalized();
  right.y        = 0.0f;
  right          = right.normalized();

  if (moveForward)
    moveDirection += forward;
  if (moveBackward)
    moveDirection -= forward;
  if (moveLeft)
    moveDirection -= right;
  if (moveRight)
    moveDirection += right;

  // TODO: Fix the current implementation for moving up and down
  Vec3 up = Vec3(0.0f, 1.0f, 0.0f);
  if (moveUp)
  {
    moveDirection -= up;
    // Avoid wasting CPU cycles
    up.y = 0.0f;
  }
  if (moveDown && !!up.y)
  {
    moveDirection += up;
  }

  if (moveDirection.length() > 0.0f)
  {
    moveDirection = moveDirection.normalized();

    float speed;
    if (sprintPressed)
    {
      speed = player.runSpeed;
    }
    else
    {
      speed = player.movementSpeed;
    }
    player.moveDirAndSpeed(moveDirection, speed);
    player.updateState(GameTime::getDeltaTime());
    handleCamera();
  }
}

void PlayerController::onKeyPress(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt)
{
  if (ctrl)
  {
    sprintPressed = true;
  }
  if (key == GameInputEvent::KeyCode::W)
  {
    moveForward = true;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::S)
  {
    moveBackward = true;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::A)
  {
    moveLeft = true;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::D)
  {
    moveRight = true;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::Space)
  {
    moveUp = true;
    handleMovement(GameTime::getDeltaTime());
  }
  if (shift)
  {
    moveDown = true;
    handleMovement(GameTime::getDeltaTime());
  }
}

void PlayerController::onKeyRelease(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt)
{
  if (ctrl)
  {
    sprintPressed = false;
  }
  if (key == GameInputEvent::KeyCode::W)
  {
    moveForward = false;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::S)
  {
    moveBackward = false;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::A)
  {
    moveLeft = false;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::D)
  {
    moveRight = false;
    handleMovement(GameTime::getDeltaTime());
  }
  if (key == GameInputEvent::KeyCode::Space)
  {
    moveUp = false;
    handleMovement(GameTime::getDeltaTime());
  }
  if (shift)
  {
    moveDown = false;
    handleMovement(GameTime::getDeltaTime());
  }
}

void PlayerController::onKeyRepeat(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt)
{
  if (key == GameInputEvent::KeyCode::W || key == GameInputEvent::KeyCode::S ||
      key == GameInputEvent::KeyCode::A || key == GameInputEvent::KeyCode::D)
  {
    handleMovement(GameTime::getDeltaTime());
  }
}

void PlayerController::onMouseMove(float x, float y)
{
  lookInput.x = x;
  lookInput.y = y;
  handleCamera();
}

void PlayerController::onMouseButtonDown(GameInputEvent::MouseButton button, float x, float y)
{
}

void PlayerController::onMouseButtonUp(GameInputEvent::MouseButton button, float x, float y)
{
}

void PlayerController::onMouseScroll(float deltaX, float deltaY, float x, float y)
{
  cameraComplete.zoom += deltaY;
  if (cameraComplete.zoom < 0.5f)
    cameraComplete.zoom = 0.5f;
  if (cameraComplete.zoom > 4.0f)
    cameraComplete.zoom = 4.0f;
}

} // namespace rl
