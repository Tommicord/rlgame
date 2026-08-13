#ifndef RL_PLAYER_PLAYER_CONTROLLER_H
#define RL_PLAYER_PLAYER_CONTROLLER_H

#include "Rl.Base/GameMatrix.h"
#include "Rl.Base/GameInput.h"
#include "Rl.Player/Player.h"

namespace rl
{

/** Camera matrices for player view */
struct PlayerCamera
{
                Mat4 m = Mat4::identity(); /**< Model matrix */
                Mat4 v = Mat4::identity(); /**< View matrix */
                Mat4 p = Mat4::identity(); /**< Projection matrix */
};

/** Complete camera configuration including angles and settings */
struct PlayerCameraComplete
{
                float pitch; /**< Pitch angle in degrees */
                float yaw; /**< Yaw angle in degrees */
                float roll; /**< Roll angle in degrees */
                float zoom; /**< Camera zoom level */
                float aspectRatio; /**< Aspect ratio of the viewport */
                float fov; /**< Field of view in degrees */
                float _near; /**< Near clipping plane distance */
                float _far; /**< Far clipping plane distance */
                Vec3  up; /**< Up vector */
                Vec3  front; /**< Front vector (direction camera is facing) */
};

/** Controller for handling player input and camera management */
class PlayerController final : public IGameInputReceiver,
                               public IPlayerController
{
                friend class Player;
                friend class IPlayer;
                friend class GameResources;

        private:
                PlayerCamera         camera{}; /**< Camera matrices */
                PlayerCameraComplete cameraComplete{}; /**< Complete camera configuration */

                Vec3  cameraOffset; /**< Offset of camera from player */
                Vec2  moveInput; /**< Movement input vector */
                Vec2  lookInput; /**< Look/camera input vector */
                Vec2  lastMousePos; /**< Last known mouse position */
                float cameraDistance; /**< Distance from player to camera */
                float lookSensitivity; /**< Sensitivity of camera look controls */
                bool  sprintPressed; /**< Whether sprint key is pressed */
                bool  moveForward; /**< Whether forward movement is active */
                bool  moveBackward; /**< Whether backward movement is active */
                bool  moveLeft; /**< Whether left movement is active */
                bool  moveRight; /**< Whether right movement is active */
                bool  moveUp; /**< Whether upward movement is active */
                bool  moveDown; /**< Whether downward movement is active */

                Player& player; /**< Reference to the controlled player */
                /** Constructs a controller for the specified player
                 * @param player The player to control */
                PlayerController(Player& player);

        public:
                /** Destroys the controller */
                virtual ~PlayerController();

                /** Returns the camera matrices
                 * @return Reference to the camera */
                PlayerCamera& getCamera()
                {
                        return camera;
                }
                /** Returns the complete camera configuration
                 * @return Reference to the camera configuration */
                PlayerCameraComplete& getCameraComplete()
                {
                        return cameraComplete;
                }

        protected:
                /** Handles player movement for the current frame
                 * @param deltaTime Time elapsed since last frame in seconds */
                void handleMovement(float deltaTime);
                /** Handles camera updates for the current frame */
                void handleCamera();

                /** Called when a key is pressed
                 * @param key The key code
                 * @param ctrl Whether control modifier is pressed
                 * @param shift Whether shift modifier is pressed
                 * @param alt Whether alt modifier is pressed */
                void onKeyPress(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt) override;
                /** Called when a key is released
                 * @param key The key code
                 * @param ctrl Whether control modifier is pressed
                 * @param shift Whether shift modifier is pressed
                 * @param alt Whether alt modifier is pressed */
                void
                onKeyRelease(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt) override;
                /** Called when a key is repeated (held down)
                 * @param key The key code
                 * @param ctrl Whether control modifier is pressed
                 * @param shift Whether shift modifier is pressed
                 * @param alt Whether alt modifier is pressed */
                void onKeyRepeat(GameInputEvent::KeyCode key, bool ctrl, bool shift, bool alt) override;
                /** Called when the mouse is moved
                 * @param x Mouse X position
                 * @param y Mouse Y position */
                void onMouseMove(float x, float y) override;
                /** Called when a mouse button is pressed
                 * @param button The button that was pressed
                 * @param x Mouse X position
                 * @param y Mouse Y position */
                void onMouseButtonDown(GameInputEvent::MouseButton button, float x, float y) override;
                /** Called when a mouse button is released
                 * @param button The button that was released
                 * @param x Mouse X position
                 * @param y Mouse Y position */
                void onMouseButtonUp(GameInputEvent::MouseButton button, float x, float y) override;
                /** Called when the mouse scroll wheel is used
                 * @param deltaX Horizontal scroll delta
                 * @param deltaY Vertical scroll delta
                 * @param x Mouse X position
                 * @param y Mouse Y position */
                void onMouseScroll(float deltaX, float deltaY, float x, float y) override;
};

} // namespace rl

#endif
