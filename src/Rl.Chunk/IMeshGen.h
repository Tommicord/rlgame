#ifndef RL_CHUNK_IMESH_GEN_H
#define RL_CHUNK_IMESH_GEN_H

#include "Rl.Base/GameOpaqueBufferHandle.h"
#include "Rl.Base/GameOpaqueSyncHandle.h"

#include <cstdint>
#include <mutex>

namespace rl
{

/**
 * @brief Interface for mesh generation components
 * 
 * This interface abstracts the mesh generation functionality, allowing for both
 * GPU-accelerated and CPU-fallback implementations. Components implementing this
 * interface provide vertex and index buffers for mesh data, along with count
 * information and synchronization primitives.
 * 
 * Thread safety: Implementations must provide thread-safe access to buffers
 * through the getGenerateMutex() method.
 */
class IMeshGen
{
        public:
                virtual ~IMeshGen() = default;

                /**
                 * @brief Returns the vertex buffer containing mesh vertex data
                 * @return Reference to the vertex buffer handle
                 * 
                 * The vertex buffer contains MeshVertex structures with position,
                 * normal, and UV data. Buffer format is implementation-dependent
                 * (GPU memory vs CPU memory).
                 */
                virtual GameOpaqueBufferHandle& getVertexBuffer() = 0;

                /**
                 * @brief Returns the index buffer containing mesh index data
                 * @return Reference to the index buffer handle
                 * 
                 * The index buffer contains uint32_t indices referencing vertices.
                 * Format is implementation-dependent.
                 */
                virtual GameOpaqueBufferHandle& getIndexBuffer() = 0;

                /**
                 * @brief Returns the count buffer containing vertex and index counts
                 * @return Reference to the count buffer handle
                 * 
                 * The count buffer contains two uint32_t values:
                 * - [0]: Vertex count
                 * - [1]: Index count
                 * Format is implementation-dependent.
                 */
                virtual GameOpaqueBufferHandle& getCountBuffer() = 0;

                /**
                 * @brief Returns the subdivision level for mesh generation
                 * @return Subdivision level (0 = no subdivision, higher = more detail)
                 * 
                 * Subdivision level affects the tessellation detail of the mesh.
                 * Higher values produce more vertices but increase computational cost.
                 */
                virtual uint32_t getSubdivisions() const = 0;

                /**
                 * @brief Reads the vertex count from the count buffer
                 * @param vertexCount Output for vertex count
                 * 
                 * Reads the vertex count from the count buffer. The count buffer
                 * contains two uint32_t values: vertex count at offset 0 and index count
                 * at offset sizeof(uint32_t).
                 */
                virtual void readVertexCount(uint32_t& vertexCount) = 0;

                /**
                 * @brief Returns the generate mutex for external synchronization
                 * @return Reference to the recursive mutex
                 * 
                 * This mutex should be locked when accessing buffers or triggering
                 * generation to ensure thread safety. The mutex is recursive to allow
                 * nested locking within the same thread.
                 */
                virtual std::recursive_mutex& getGenerateMutex() = 0;

                /**
                 * @brief Returns the completion sync handle for async operations
                 * @return Reference to the completion sync handle
                 * 
                 * The sync handle is signaled when mesh generation completes.
                 * Used for GPU implementations; CPU implementations may return
                 * a dummy handle.
                 */
                virtual const GameOpaqueSyncHandle& getCompletionHandle() const = 0;

#if defined(_RL_CHUNK_VULKAN_BACKEND)
                /** Vulkan backend: opaque pointers to native buffers */
                virtual void* getVertexBufferPtr() = 0;
                virtual void* getIndexBufferPtr() = 0;
                virtual void* getCountBufferPtr() = 0;
#endif
};

} // namespace rl

#endif // RL_CHUNK_IMESH_GEN_H
