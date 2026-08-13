#include "Rl.Player/Player.h"
#include "Rl.Base/GameTime.h"
#include "Rl.Player/PlayerController.h"

namespace rl
{

IPlayer::IPlayer() noexcept : controller(nullptr)
{
}

Player::Player() noexcept :
    rotation(0.0f, 0.0f, 0.0f), scale(1.0f, 1.0f, 1.0f), velocity(0.0f, 0.0f, 0.0f),
    acceleration(0.0f, 0.0f, 0.0f), state(PlayerState::Idle), movementSpeed(5.8f), runSpeed(6.235f),
    jumpForce(8.0f), gravity(9.8f), mass(1.0f), friction(0.80f)
{
        controller = new PlayerController(static_cast<Player&>(*this));
}

Player::~Player()
{
        delete controller;
}

PlayerState Player::getState() const
{
        return state;
}

PlayerController& Player::getController() const
{
        return static_cast<PlayerController&>(*controller);
}

void Player::moveDirAndSpeed(const Vec3& direction, float speed)
{
        Vec3  moveDir = direction;
        float length  = sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y + moveDir.z * moveDir.z);
        if (length > 0.001f)
        {
                moveDir.x /= length;
                moveDir.y /= length;
                moveDir.z /= length;
        }
        acceleration += moveDir * speed;
}

void Player::updateState(float deltaTime)
{
        velocity += acceleration * deltaTime;
        velocity *= std::pow(1.0f - friction, deltaTime);

        constexpr float positionScale = _coordScale;
        x.set(x.get() + static_cast<PlayerPosStruct::type>(velocity.x * deltaTime * positionScale));
        y.set(y.get() + static_cast<PlayerPosStruct::type>(velocity.y * deltaTime * positionScale));
        z.set(z.get() + static_cast<PlayerPosStruct::type>(velocity.z * deltaTime * positionScale));

        acceleration = Vec3(0.0f, 0.0f, 0.0f);
        if (velocity.x != 0.0f || velocity.z != 0.0f)
        {
                state = PlayerState::Walking;
        }
        else
        {
                state = PlayerState::Idle;
        }
}

void Player::setState(PlayerState newState)
{
        state = newState;
}

} // namespace rl
