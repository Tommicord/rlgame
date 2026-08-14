#ifndef RL_CHUNK_IMESH_DELAUNAY_2D_H
#define RL_CHUNK_IMESH_DELAUNAY_2D_H

#include "Rl.Base/GameOpaqueBufferHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include <cstdint>
#include <mutex>

namespace rl
{

/**
 * @brief Interface for 2D Delaunay triangulation mesh components
 *
 * This interface abstracts the 2D Delaunay triangulation functionality, which converts
 * vertex data into triangulated mesh indices. Both GPU-accelerated and CPU-fallback
 * implementations can be provided through this interface.
 *
 * The triangulation process takes vertex buffer data and generates index buffers
 * with Delaunay triangulation for 2D surfaces.
 *
 * Thread safety: Implementations must provide thread-safe access to buffers
 * through the getGenerateMutex() method.
 */
class IMeshDelaunay2D
{
        public:
                virtual ~IMeshDelaunay2D() = default;

                /**
                 * @brief Returns the index buffer containing triangulated indices
                 * @return Reference to the index buffer handle
                 *
                 * The index buffer contains the triangulated mesh indices.
                 * Buffer format is implementation-dependent (GPU memory vs CPU memory).
                 */
                virtual const GameOpaqueBufferHandle& getIndexBuffer() const = 0;

                /**
                 * @brief Returns the count buffer containing index count
                 * @return Reference to the count buffer handle
                 *
                 * The count buffer contains the number of valid indices in the index buffer.
                 */
                virtual const GameOpaqueBufferHandle& getCountBuffer() const = 0;

                /**
                 * @brief Returns the generate mutex for external synchronization
                 * @return Reference to the recursive mutex
                 *
                 * This mutex should be locked when accessing buffers or triggering
                 * triangulation to ensure thread safety. The mutex is recursive to allow
                 * nested locking within the same thread.
                 */
                virtual std::recursive_mutex& getGenerateMutex() = 0;

                /**
                 * @brief Returns the completion sync handle for async operations
                 * @return Reference to the completion sync handle
                 *
                 * The sync handle is signaled when triangulation completes.
                 * Used for GPU implementations; CPU implementations may return
                 * a dummy handle.
                 */
                virtual const GameOpaqueSyncHandle& getCompletionHandle() const = 0;

                /**
                 * @brief Reads the index buffer data
                 * @param pOutput Output pointer for index data
                 * @param outputSize Size of output buffer in bytes
                 */
                virtual void readIndices(uint32_t* pOutput, const size_t outputSize) = 0;

                /**
                 * @brief Reads the count buffer data
                 * @param pIndexCount Output pointer for index count
                 */
                virtual void readCounts(uint32_t& pIndexCount) = 0;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
                /** Vulkan backend: opaque pointer to the native index buffer (GameVulkanBuffer*)
                 */
                virtual void* getIndexBufferPtr() const = 0;

                /** Vulkan backend: opaque pointer to the native count buffer (GameVulkanBuffer*)
                 */
                virtual void* getCountBufferPtr() const = 0;
#endif
};

} // namespace rl

#endif // RL_CHUNK_IMESH_DELAUNAY_2D_H
