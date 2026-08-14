#ifndef RL_CHUNK_IMESH_TESS_H
#define RL_CHUNK_IMESH_TESS_H

#include "Rl.Base/GameOpaqueBufferHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include <cstdint>
#include <mutex>

namespace rl
{

/**
 * @brief Interface for mesh tessellation components
 *
 * This interface abstracts the unit tessellation functionality, which converts
 * placed units into tessellated geometry. Both GPU-accelerated and CPU-fallback
 * implementations can be provided through this interface.
 *
 * The tessellation process takes unit placement data and generates PostUnit
 * structures that contain geometric information for each unit.
 *
 * Thread safety: Implementations must provide thread-safe access to buffers
 * through the getGenerateMutex() method.
 */
class IMeshTess
{
  public:
    virtual ~IMeshTess() = default;

    /**
     * @brief Returns the output buffer containing PostUnit data
     * @return Reference to the output buffer handle
     *
     * The output buffer contains PostUnit structures with tessellated
     * geometry data for each unit. Buffer format is implementation-dependent
     * (GPU memory vs CPU memory).
     */
    virtual const GameOpaqueBufferHandle& getOutputBuffer() const = 0;

    /**
     * @brief Returns the generate mutex for external synchronization
     * @return Reference to the recursive mutex
     *
     * This mutex should be locked when accessing buffers or triggering
     * tessellation to ensure thread safety. The mutex is recursive to allow
     * nested locking within the same thread.
     */
    virtual std::recursive_mutex& getGenerateMutex() = 0;

    /**
     * @brief Returns the completion sync handle for async operations
     * @return Reference to the completion sync handle
     *
     * The sync handle is signaled when tessellation completes.
     * Used for GPU implementations; CPU implementations may return
     * a dummy handle.
     */
    virtual const GameOpaqueSyncHandle& getCompletionHandle() const = 0;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
    /** Vulkan backend: opaque pointer to the native output buffer (GameVulkanBuffer*)
     */
    virtual void* getOutputBufferPtr() const = 0;
#endif
};

} // namespace rl

#endif // RL_CHUNK_IMESH_TESS_H
