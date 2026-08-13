#ifndef RL_CHUNK_ICLIMATE_COMPUTE_H
#define RL_CHUNK_ICLIMATE_COMPUTE_H

#include "Rl.Base/GameOpaqueImageHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include <cstdint>
#include <mutex>

namespace rl
{

/**
 * @brief Interface for climate computation components
 * 
 * This interface abstracts the climate computation functionality, which calculates
 * climate data including latitude, temperature, and moisture modifiers based on
 * planet position and geometry. Both GPU-accelerated and CPU-fallback implementations
 * can be provided through this interface.
 * 
 * The climate computation considers planet geometry (rotation axis, center position)
 * to calculate equatorial effects, latitude-based temperature variations, and
 * moisture distribution across the world surface.
 * 
 * Thread safety: Implementations must provide thread-safe access to resources
 * through the getGenerateMutex() method.
 */
class IClimateCompute
{
        public:
                virtual ~IClimateCompute() = default;

                /**
                 * @brief Returns the equator field image
                 * @return Image handle for equator field data
                 * 
                 * The equator field image contains climate data (latitude, temperature,
                 * moisture modifiers) for each texel. Format is implementation-dependent
                 * (GPU texture vs CPU memory).
                 */
                virtual const GameOpaqueImageHandle& getEquatorImage() const = 0;

                /**
                 * @brief Returns the generate mutex for external synchronization
                 * @return Reference to the recursive mutex
                 * 
                 * This mutex should be locked when accessing resources or triggering
                 * computation to ensure thread safety. The mutex is recursive to allow
                 * nested locking within the same thread.
                 */
                virtual std::recursive_mutex& getGenerateMutex() = 0;

                /**
                 * @brief Returns the completion sync handle for async operations
                 * @return Reference to the completion sync handle
                 * 
                 * The sync handle is signaled when climate computation completes.
                 * Used for GPU implementations; CPU implementations may return
                 * a dummy handle.
                 */
                virtual const GameOpaqueSyncHandle& getCompletionHandle() const = 0;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
                /** Vulkan backend: returns opaque pointer to equator image view wrapper */
                virtual void* getEquatorImageViewPtr() const = 0;
#endif
};

} // namespace rl

#endif // RL_CHUNK_ICLIMATE_COMPUTE_H
