#ifndef RL_CHUNK_IUNIT_PLACEMENT_H
#define RL_CHUNK_IUNIT_PLACEMENT_H

#include "Rl.Base/GameOpaqueImageHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include <cstdint>
#include <mutex>

namespace rl
{

/**
 * @brief Interface for unit placement components
 *
 * This interface abstracts the unit and biome placement functionality, which
 * determines which units and biomes should be placed at each location in the
 * world based on environmental factors. Both GPU-accelerated and CPU-fallback
 * implementations can be provided through this interface.
 *
 * Design rationale:
 * - Enables CPU fallback for less powerful GPUs
 * - Facilitates unit testing through mock implementations
 * - Decouples unit placement from specific hardware implementation
 * - Allows runtime selection of optimal implementation
 * - Graphics API agnostic for maximum portability
 *
 * The placement process considers elevation, moisture, temperature, and other
 * environmental factors to determine appropriate unit and biome placement.
 *
 * Thread safety: Implementations must provide thread-safe access to resources
 * through the getGenerateMutex() method.
 */
class IUnitPlacement
{
  public:
    virtual ~IUnitPlacement() = default;

    /**
     * @brief Returns the unit output image
     * @return Image handle for unit placement output
     *
     * The unit output image contains unit type IDs for each texel.
     * Format is implementation-dependent (GPU texture vs CPU memory).
     */
    virtual const GameOpaqueImageHandle& getUnitOutputImage() const = 0;

    /**
     * @brief Returns the biome output image
     * @return Image handle for biome placement output
     *
     * The biome output image contains biome type IDs for each texel.
     * Format is implementation-dependent.
     */
    virtual const GameOpaqueImageHandle& getBiomeOutputImage() const = 0;

    /**
     * @brief Returns the generate mutex for external synchronization
     * @return Reference to the recursive mutex
     *
     * This mutex should be locked when accessing resources or triggering
     * placement to ensure thread safety. The mutex is recursive to allow
     * nested locking within the same thread.
     */
    virtual std::recursive_mutex& getGenerateMutex() = 0;

    /**
     * @brief Returns the completion sync handle for async operations
     * @return Reference to the completion sync handle
     *
     * The sync handle is signaled when placement completes.
     * Used for GPU implementations; CPU implementations may return
     * a dummy handle.
     */
    virtual const GameOpaqueSyncHandle& getCompletionHandle() const = 0;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
    /** Vulkan backend: opaque pointer to unit/biome image view wrappers */
    virtual void* getUnitOutputImageViewPtr() const  = 0;
    virtual void* getBiomeOutputImageViewPtr() const = 0;
#endif
};

} // namespace rl

#endif // RL_CHUNK_IUNIT_PLACEMENT_H
