#ifndef RL_PLAYER_PLAYER_H
#define RL_PLAYER_PLAYER_H

#include <cstddef>
#include <cstdint>
#include <memory>

#include "Rl.Base/GameMatrix.h"

namespace rl
{

/** Structure for storing player position coordinates */
class PlayerPosStruct
{
        public:
                using type   = long long; /**< integer coordinate type */
                using typefp = double; /**< Floating-point coordinate type */

                /** Default constructor initializing coordinate to 0 */
                PlayerPosStruct() : coord(0)
                {
                }
                /** Returns the coordinate value
                 * @return Current coordinate value */
                type get() const
                {
                        return coord;
                }
                /** Sets the coordinate value
                 * @param v The value to set */
                void set(type v)
                {
                        coord = v;
                }

        private:
                type coord; /**< The coordinate value */
};

/** Represents the current state of the player */
enum class PlayerState
{
        Idle, /**< Player is standing still */
        Walking, /**< Player is walking */
        Running, /**< Player is running */
        Jumping, /**< Player is jumping */
        Falling, /**< Player is falling */
        Crouching /**< Player is crouching */
};

class IPlayerProvider;
/** interface for player controller implementations */
class IPlayerController
{
        public:
                virtual ~IPlayerController() = default;
                /** Handles player movement for the current frame
                 * @param deltaTime Time elapsed since last frame in seconds */
                virtual void handleMovement(float deltaTime) = 0;
                /** Handles camera updates for the current frame */
                virtual void handleCamera() = 0;
};

class Player;
class PlayerController;
/** interface for providing access to a Player instance */
class IPlayerProvider
{
        public:
                virtual ~IPlayerProvider() = default;

                /** Returns a reference to the player
                 * @return Reference to the player */
                virtual Player& getPlayer() = 0;
                /** Returns a const reference to the player
                 * @return Const reference to the player */
                virtual const Player& getPlayer() const = 0;
};

/** interface for player implementations */
class IPlayer
{
                friend class Player;

        protected:
                /** Default constructor */
                IPlayer() noexcept;

        public:
                virtual ~IPlayer() = default;

                PlayerPosStruct x{}; /**< X coordinate */
                PlayerPosStruct y{}; /**< Y coordinate */
                PlayerPosStruct z{}; /**< Z coordinate */
        private:
                IPlayerController* controller; /**< Pointer to the player controller */
};
/** Scale factor for coordinate conversion */
static constexpr uint32_t _coordScale = 1000;

/** Concrete player implementation */
class Player final : public IPlayer
{
                friend class PlayerController;

                friend class PlayerInstance;

        public:
                /** Constructs a new player */
                Player() noexcept;
                /** Destroys the player */
                ~Player();

                Vec3        rotation; /**< Euler angles in degrees */
                Vec3        scale; /**< Scale vector */
                Vec3        velocity; /**< Velocity vector */
                Vec3        acceleration; /**< Acceleration vector */
                PlayerState state; /**< Current player state */

                float movementSpeed; /**< Normal movement speed */
                float runSpeed; /**< Running speed */
                float jumpForce; /**< Force applied when jumping */
                float gravity; /**< Gravity force */
                float mass; /**< Player mass */
                float friction; /**< Friction coefficient */

                /** Returns the current player state
                 * @return Current player state */
                PlayerState getState() const;
                /** Returns the player controller
                 * @return Reference to the player controller */
                PlayerController& getController() const;

        private:
                /** Updates the player state based on current conditions
                 * @param deltaTime Time elapsed since last frame in seconds */
                void updateState(float deltaTime);
                /** Moves the player in a direction with a specific speed
                 * @param direction The direction to move
                 * @param speed The speed to move at */
                void moveDirAndSpeed(const Vec3& direction, float speed);
                /** Sets the player state
                 * @param newState The new state to set */
                void setState(PlayerState newState);
};

} // namespace rl

#endif
