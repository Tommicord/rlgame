#ifndef RL_BASE_GAME_OPAQUE_BUFFER_HANDLE_H
#define RL_BASE_GAME_OPAQUE_BUFFER_HANDLE_H

#include <cstdint>
#include <cstddef>

namespace rl
{

/**
 * @brief Buffer format enumeration for different data types
 */
enum class GameOpaqueBufferFormat : uint32_t
{
        Unknown = 0,
        R8_UNORM,
        R8G8_UNORM,
        R8G8B8_UNORM,
        R8G8B8A8_UNORM,
        R16_SFLOAT,
        R16G16_SFLOAT,
        R16G16B16_SFLOAT,
        R16G16B16A16_SFLOAT,
        R32_SFLOAT,
        R32G32_SFLOAT,
        R32G32B32_SFLOAT,
        R32G32B32A32_SFLOAT,
        R32_UINT,
        R32G32_UINT,
        R32G32B32_UINT,
        R32G32B32A32_UINT,
};

/**
 * @brief Buffer usage flags for describing intended usage
 */
enum class GameOpaqueBufferUsage : uint32_t
{
        None               = 0,
        TransferSrc        = 1 << 0,
        TransferDst        = 1 << 1,
        UniformTexelBuffer = 1 << 2,
        StorageTexelBuffer = 1 << 3,
        UniformBuffer      = 1 << 4,
        StorageBuffer      = 1 << 5,
        IndexBuffer        = 1 << 6,
        VertexBuffer       = 1 << 7,
        IndirectBuffer     = 1 << 8,
};

inline GameOpaqueBufferUsage operator|(GameOpaqueBufferUsage a, GameOpaqueBufferUsage b)
{
        return static_cast<GameOpaqueBufferUsage>(static_cast<uint32_t>(a) |
                                                  static_cast<uint32_t>(b));
}

inline GameOpaqueBufferUsage operator&(GameOpaqueBufferUsage a, GameOpaqueBufferUsage b)
{
        return static_cast<GameOpaqueBufferUsage>(static_cast<uint32_t>(a) &
                                                  static_cast<uint32_t>(b));
}

/**
 * @brief Buffer memory property flags for memory type selection
 */
enum class GameOpaqueBufferMemoryProperty : uint32_t
{
        None            = 0,
        DeviceLocal     = 1 << 0,
        HostVisible     = 1 << 1,
        HostCoherent    = 1 << 2,
        HostCached      = 1 << 3,
        LazilyAllocated = 1 << 4,
};

inline GameOpaqueBufferMemoryProperty operator|(GameOpaqueBufferMemoryProperty a,
                                                GameOpaqueBufferMemoryProperty b)
{
        return static_cast<GameOpaqueBufferMemoryProperty>(static_cast<uint32_t>(a) |
                                                           static_cast<uint32_t>(b));
}

inline GameOpaqueBufferMemoryProperty operator&(GameOpaqueBufferMemoryProperty a,
                                                GameOpaqueBufferMemoryProperty b)
{
        return static_cast<GameOpaqueBufferMemoryProperty>(static_cast<uint32_t>(a) &
                                                           static_cast<uint32_t>(b));
}

struct GameOpaqueBufferHandle
{
                void*                          handle = nullptr;
                uint64_t                       size   = 0;
                uint64_t                       offset = 0;
                GameOpaqueBufferFormat         format = GameOpaqueBufferFormat::Unknown;
                GameOpaqueBufferUsage          usage  = GameOpaqueBufferUsage::None;
                GameOpaqueBufferMemoryProperty memoryProperty =
                    GameOpaqueBufferMemoryProperty::None;
                uint32_t stride = 0;

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
 * @brief Opaque buffer handle for graphics resources
 *
 * This opaque handle type allows implementations to store platform-specific
 * buffer references without exposing graphics API details in the interface.
 * Contains useful metadata about the buffer for validation and debugging.
 */
template <class T> class GameOpaqueBuffer
{
        public:
                GameOpaqueBuffer()  = default;
                ~GameOpaqueBuffer() = default;

                /**
                 * @brief Sets the native handle (platform-specific)
                 * @param handle Native handle pointer
                 */
                void setHandle(void* handle)
                {
                        bufferHandle.handle = handle;
                }

                /**
                 * @brief Gets the native handle
                 * @return Native handle pointer
                 */
                void* getHandle() const
                {
                        return bufferHandle.handle;
                }

                T* operator->() const
                {
                        return static_cast<T*>(bufferHandle.handle);
                }

                T& operator*() const
                {
                        return *static_cast<T*>(bufferHandle.handle);
                }

                /**
                 * @brief Sets the buffer size in bytes
                 * @param size Buffer size
                 */
                void setSize(uint64_t size)
                {
                        bufferHandle.size = size;
                }

                /**
                 * @brief Gets the buffer size in bytes
                 * @return Buffer size
                 */
                uint64_t getSize() const
                {
                        return bufferHandle.size;
                }

                /**
                 * @brief Sets the buffer offset in bytes
                 * @param offset Buffer offset
                 */
                void setOffset(uint64_t offset)
                {
                        bufferHandle.offset = offset;
                }

                /**
                 * @brief Gets the buffer offset in bytes
                 * @return Buffer offset
                 */
                uint64_t getOffset() const
                {
                        return bufferHandle.offset;
                }

                /**
                 * @brief Sets the buffer format
                 * @param format Buffer format
                 */
                void setFormat(GameOpaqueBufferFormat format)
                {
                        bufferHandle.format = format;
                }

                /**
                 * @brief Gets the buffer format
                 * @return Buffer format
                 */
                GameOpaqueBufferFormat getFormat() const
                {
                        return bufferHandle.format;
                }

                /**
                 * @brief Returns the underlying native handle typed as T*
                 * @return Pointer to native object
                 */
                T* getNativeHandle() const
                {
                        return static_cast<T*>(bufferHandle.handle);
                }

                /**
                 * @brief Sets the buffer usage flags
                 * @param usage Buffer usage flags
                 */
                void setUsage(GameOpaqueBufferUsage usage)
                {
                        bufferHandle.usage = usage;
                }

                /**
                 * @brief Gets the buffer usage flags
                 * @return Buffer usage flags
                 */
                GameOpaqueBufferUsage getUsage() const
                {
                        return bufferHandle.usage;
                }

                /**
                 * @brief Sets the buffer memory property flags
                 * @param memoryProperty Memory property flags
                 */
                void setMemoryProperty(GameOpaqueBufferMemoryProperty memoryProperty)
                {
                        bufferHandle.memoryProperty = memoryProperty;
                }

                /**
                 * @brief Gets the buffer memory property flags
                 * @return Memory property flags
                 */
                GameOpaqueBufferMemoryProperty getMemoryProperty() const
                {
                        return bufferHandle.memoryProperty;
                }

                /**
                 * @brief Sets the stride between elements (for structured buffers)
                 * @param stride Element stride in bytes
                 */
                void setStride(uint32_t stride)
                {
                        bufferHandle.stride = stride;
                }

                /**
                 * @brief Gets the stride between elements
                 * @return Element stride in bytes
                 */
                uint32_t getStride() const
                {
                        return bufferHandle.stride;
                }

                /**
                 * @brief Checks if the handle is valid
                 * @return True if handle is not null
                 */
                bool isValid() const
                {
                        return bufferHandle.handle != nullptr;
                }

                /**
                 * @brief Gets the underlying handle struct
                 * @return Reference to the underlying GameOpaqueBufferHandle struct
                 *
                 * This allows returning the API-agnostic struct from interfaces
                 * while using the template class in concrete implementations.
                 */
                GameOpaqueBufferHandle& getHandleStruct()
                {
                        return bufferHandle;
                }

                /**
                 * @brief Gets the underlying handle struct (const)
                 * @return Const reference to the underlying GameOpaqueBufferHandle struct
                 */
                const GameOpaqueBufferHandle& getHandleStruct() const
                {
                        return bufferHandle;
                }

        private:
                GameOpaqueBufferHandle bufferHandle;
};

} // namespace rl

#endif // RL_BASE_GAME_OPAQUE_BUFFER_HANDLE_H
