#ifndef RL_CHUNK_IHEIGHTMAP_H
#define RL_CHUNK_IHEIGHTMAP_H

#include "Rl.Base/GameOpaqueImageHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include <cstdint>
#include <mutex>

namespace rl
{

/**
 * @brief Interface for heightmap generation components
 *
 * This interface abstracts the heightmap generation functionality, which creates
 * terrain elevation, temperature, and moisture data for world generation. Both
 * GPU-accelerated and CPU-fallback implementations can be provided through this
 * interface.
 *
 * The heightmap generation uses noise algorithms (e.g., Simplex noise) to create
 * realistic terrain with elevation, temperature, and moisture variations. It
 * produces both basemap (surface) and deepmap (underground) data.
 *
 * Thread safety: Implementations must provide thread-safe access to resources
 * through the getGenerateMutex() method.
 */
class IHeightmap
{
  public:
    virtual ~IHeightmap() = default;

    /**
     * @brief Returns the basemap elevation image
     * @return Image handle for basemap elevation
     *
     * The basemap elevation image contains surface terrain elevation data.
     * Format is implementation-dependent.
     */
    virtual const GameOpaqueImageHandle& getBasemapElevationImage() const = 0;

    /**
     * @brief Returns the basemap temperature image
     * @return Image handle for basemap temperature
     *
     * The basemap temperature image contains surface temperature data.
     * Format is implementation-dependent.
     */
    virtual const GameOpaqueImageHandle& getBasemapTemperatureImage() const = 0;

    /**
     * @brief Returns the basemap moisture image
     * @return Image handle for basemap moisture
     *
     * The basemap moisture image contains surface moisture data.
     * Format is implementation-dependent.
     */
    virtual const GameOpaqueImageHandle& getBasemapMoistureImage() const = 0;

    /**
     * @brief Returns the deepmap elevation image
     * @return Image handle for deepmap elevation
     *
     * The deepmap elevation image contains underground terrain elevation data.
     * Format is implementation-dependent.
     */
    virtual const GameOpaqueImageHandle& getDeepmapElevationImage() const = 0;

    /**
     * @brief Returns the deepmap temperature image
     * @return Image handle for deepmap temperature
     *
     * The deepmap temperature image contains underground temperature data.
     * Format is implementation-dependent.
     */
    virtual const GameOpaqueImageHandle& getDeepmapTemperatureImage() const = 0;

    /**
     * @brief Returns the deepmap moisture image
     * @return Image handle for deepmap moisture
     *
     * The deepmap moisture image contains underground moisture data.
     * Format is implementation-dependent.
     */
    virtual const GameOpaqueImageHandle& getDeepmapMoistureImage() const = 0;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
    /**
     * @brief Vulkan backend: returns an opaque pointer to the image view wrapper
     * This is intentionally a void* to avoid exposing Vulkan types in the
     * cross-platform interface. When the Vulkan backend is enabled, the
     * pointer will point to a GameVulkanImageView instance owned by the
     * concrete implementation.
     */
    virtual void* getBasemapElevationImageViewPtr() const   = 0;
    virtual void* getBasemapTemperatureImageViewPtr() const = 0;
    virtual void* getBasemapMoistureImageViewPtr() const    = 0;
    virtual void* getDeepmapElevationImageViewPtr() const   = 0;
    virtual void* getDeepmapTemperatureImageViewPtr() const = 0;
    virtual void* getDeepmapMoistureImageViewPtr() const    = 0;
#endif

    /**
     * @brief Returns the generate mutex for external synchronization
     * @return Reference to the recursive mutex
     *
     * This mutex should be locked when accessing resources or triggering
     * generation to ensure thread safety. The mutex is recursive to allow
     * nested locking within the same thread.
     */
    virtual std::recursive_mutex& getGenerateMutex() = 0;

    /**
     * @brief Returns the completion sync handle for async operations
     * @return Reference to the completion sync handle
     *
     * The sync handle is signaled when heightmap generation completes.
     * Used for GPU implementations; CPU implementations may return
     * a dummy handle.
     */
    virtual const GameOpaqueSyncHandle& getCompletionHandle() const = 0;
};

} // namespace rl

#endif // RL_CHUNK_IHEIGHTMAP_H
