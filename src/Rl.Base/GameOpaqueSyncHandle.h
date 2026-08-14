#ifndef RL_BASE_GAME_OPAQUE_SYNC_HANDLE_H
#define RL_BASE_GAME_OPAQUE_SYNC_HANDLE_H

#include <cstdint>
#include <cstddef>

namespace rl
{

/**
 * @brief Sync handle type enumeration
 */
enum class GameOpaqueSyncHandleType : uint32_t
{
  Unknown = 0,
  Semaphore,
  Fence,
  Event,
  Barrier,
};

/**
 * @brief Sync handle state enumeration
 */
enum class GameOpaqueSyncHandleState : uint32_t
{
  Unknown = 0,
  Unsignaled,
  Signaled,
};

struct GameOpaqueSyncHandle
{
    void*                     handle        = nullptr;
    GameOpaqueSyncHandleType  type          = GameOpaqueSyncHandleType::Unknown;
    GameOpaqueSyncHandleState state         = GameOpaqueSyncHandleState::Unknown;
    uint64_t                  timeout       = UINT64_MAX;
    bool                      signaled      = false;
    uint64_t                  timelineValue = 0;

    bool operator==(std::nullptr_t) const
    {
      return handle == nullptr;
    }

    bool operator!=(std::nullptr_t) const
    {
      return handle != nullptr;
    }
};

/**
 * @brief Opaque synchronization handle for graphics resources
 *
 * This opaque handle type allows implementations to store platform-specific
 * synchronization primitives without exposing graphics API details in the interface.
 * Contains useful metadata about the sync object for validation and debugging.
 */
template <class T> class GameOpaqueSync
{
  public:
    GameOpaqueSync()  = default;
    ~GameOpaqueSync() = default;

    /**
     * @brief Sets the native handle (platform-specific)
     * @param handle Native handle pointer
     */
    void setHandle(void* handle)
    {
      syncHandle.handle = handle;
    }

    /**
     * @brief Gets the native handle
     * @return Native handle pointer
     */
    void* getHandle() const
    {
      return syncHandle.handle;
    }

    T* operator->() const
    {
      return static_cast<T*>(syncHandle.handle);
    }

    T& operator*() const
    {
      return *static_cast<T*>(syncHandle.handle);
    }

    /**
     * @brief Sets the sync handle type
     * @param type Sync handle type
     */
    void setType(GameOpaqueSyncHandleType type)
    {
      syncHandle.type = type;
    }

    /**
     * @brief Gets the sync handle type
     * @return Sync handle type
     */
    GameOpaqueSyncHandleType getType() const
    {
      return syncHandle.type;
    }

    /**
     * @brief Sets the current state
     * @param state Current state
     */
    void setState(GameOpaqueSyncHandleState state)
    {
      syncHandle.state = state;
    }

    /**
     * @brief Gets the current state
     * @return Current state
     */
    GameOpaqueSyncHandleState getState() const
    {
      return syncHandle.state;
    }

    /**
     * @brief Sets the timeout value for wait operations
     * @param timeout Timeout in nanoseconds
     */
    void setTimeout(uint64_t timeout)
    {
      syncHandle.timeout = timeout;
    }

    /**
     * @brief Gets the timeout value
     * @return Timeout in nanoseconds
     */
    uint64_t getTimeout() const
    {
      return syncHandle.timeout;
    }

    /**
     * @brief Sets whether the handle is signaled
     * @param signaled True if signaled
     */
    void setSignaled(bool signaled)
    {
      syncHandle.signaled = signaled;
    }

    /**
     * @brief Checks if the handle is signaled
     * @return True if signaled
     */
    bool isSignaled() const
    {
      return syncHandle.signaled;
    }

    /**
     * @brief Sets the timeline value (for timeline semaphores)
     * @param timelineValue Timeline value
     */
    void setTimelineValue(uint64_t timelineValue)
    {
      syncHandle.timelineValue = timelineValue;
    }

    /**
     * @brief Gets the timeline value
     * @return Timeline value
     */
    uint64_t getTimelineValue() const
    {
      return syncHandle.timelineValue;
    }

    /**
     * @brief Checks if this is a timeline semaphore
     * @return True if timeline semaphore
     */
    bool isTimeline() const
    {
      return syncHandle.type == GameOpaqueSyncHandleType::Semaphore && syncHandle.timelineValue > 0;
    }

    /**
     * @brief Checks if the handle is valid
     * @return True if handle is not null
     */
    bool isValid() const
    {
      return syncHandle.handle != nullptr;
    }

    /**
     * @brief Resets the handle to unsignaled state
     */
    void reset()
    {
      syncHandle.signaled = false;
      syncHandle.state    = GameOpaqueSyncHandleState::Unsignaled;
    }

    /**
     * @brief Gets the underlying handle struct
     * @return Reference to the underlying GameOpaqueSyncHandle struct
     *
     * This allows returning the API-agnostic struct from interfaces
     * while using the template class in concrete implementations.
     */
    GameOpaqueSyncHandle& getHandleStruct()
    {
      return syncHandle;
    }

    /**
     * @brief Gets the underlying handle struct (const)
     * @return Const reference to the underlying GameOpaqueSyncHandle struct
     */
    const GameOpaqueSyncHandle& getHandleStruct() const
    {
      return syncHandle;
    }

  private:
    GameOpaqueSyncHandle syncHandle;
};

} // namespace rl

#endif // RL_BASE_GAME_OPAQUE_SYNC_HANDLE_H
