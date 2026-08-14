#ifndef RL_BASE_GAME_OPAQUE_IMAGE_HANDLE_H
#define RL_BASE_GAME_OPAQUE_IMAGE_HANDLE_H

#include <cstdint>
#include <cstddef>

namespace rl
{

/**
 * @brief Image format enumeration for different data types
 */
enum class GameOpaqueImageFormat : uint32_t
{
        Unknown = 0,
        R8_UNORM,
        R8G8_UNORM,
        R8G8B8_UNORM,
        R8G8B8A8_UNORM,
        R8_SRGB,
        R8G8_SRGB,
        R8G8B8_SRGB,
        R8G8B8A8_SRGB,
        R16_SFLOAT,
        R16G16_SFLOAT,
        R16G16B16_SFLOAT,
        R16G16B16A16_SFLOAT,
        R32_SFLOAT,
        R32G32_SFLOAT,
        R32G32B32_SFLOAT,
        R32G32B32A32_SFLOAT,
        D16_UNORM,
        D32_SFLOAT,
        D24_UNORM_S8_UINT,
        D32_SFLOAT_S8_UINT,
        BC1_RGB_UNORM_BLOCK,
        BC1_RGBA_UNORM_BLOCK,
        BC3_UNORM_BLOCK,
        BC4_UNORM_BLOCK,
        BC5_UNORM_BLOCK,
        BC7_UNORM_BLOCK,
};

/**
 * @brief Image type enumeration
 */
enum class GameOpaqueImageType : uint32_t
{
        Unknown = 0,
        Image1D,
        Image2D,
        Image3D,
        Cube,
        Image1DArray,
        Image2DArray,
        CubeArray,
};

/**
 * @brief Image tiling mode
 */
enum class GameOpaqueImageTiling : uint32_t
{
        Unknown = 0,
        Optimal,
        Linear,
};

/**
 * @brief Image usage flags
 */
enum class GameOpaqueImageUsage : uint32_t
{
        None                   = 0,
        TransferSrc            = 1 << 0,
        TransferDst            = 1 << 1,
        Sampled                = 1 << 2,
        Storage                = 1 << 3,
        ColorAttachment        = 1 << 4,
        DepthStencilAttachment = 1 << 5,
        InputAttachment        = 1 << 6,
};

/**
 * @brief Image aspect flags
 */
enum class GameOpaqueImageAspect : uint32_t
{
        None     = 0,
        Color    = 1 << 0,
        Depth    = 1 << 1,
        Stencil  = 1 << 2,
        Metadata = 1 << 3,
};

/**
 * @brief Image layout enumeration
 */
enum class GameOpaqueImageLayout : uint32_t
{
        Unknown = 0,
        Undefined,
        General,
        ColorAttachmentOptimal,
        DepthStencilAttachmentOptimal,
        DepthStencilReadOnlyOptimal,
        ShaderReadOnlyOptimal,
        TransferSrcOptimal,
        TransferDstOptimal,
        Preinitialized,
        DepthReadOnlyStencilAttachmentOptimal,
        DepthAttachmentStencilReadOnlyOptimal,
};

/**
 * @brief Image memory property flags
 */
enum class GameOpaqueImageMemoryProperty : uint32_t
{
        None            = 0,
        DeviceLocal     = 1 << 0,
        HostVisible     = 1 << 1,
        HostCoherent    = 1 << 2,
        HostCached      = 1 << 3,
        LazilyAllocated = 1 << 4,
};

/**
 * @brief Image sample count flags
 */
enum class GameOpaqueImageSampleCount : uint32_t
{
        None    = 0,
        Count1  = 1 << 0,
        Count2  = 1 << 1,
        Count4  = 1 << 2,
        Count8  = 1 << 3,
        Count16 = 1 << 4,
        Count32 = 1 << 5,
        Count64 = 1 << 6,
};

struct GameOpaqueImageHandle
{
                void*                         handle         = nullptr;
                uint64_t                      width          = 0;
                uint64_t                      height         = 0;
                uint64_t                      depth          = 0;
                GameOpaqueImageFormat         format         = GameOpaqueImageFormat::Unknown;
                GameOpaqueImageType           type           = GameOpaqueImageType::Unknown;
                GameOpaqueImageTiling         tiling         = GameOpaqueImageTiling::Unknown;
                GameOpaqueImageUsage          usage          = GameOpaqueImageUsage::None;
                GameOpaqueImageAspect         aspect         = GameOpaqueImageAspect::None;
                GameOpaqueImageLayout         layout         = GameOpaqueImageLayout::Undefined;
                GameOpaqueImageMemoryProperty memoryProperty = GameOpaqueImageMemoryProperty::None;
                GameOpaqueImageSampleCount    sampleCount    = GameOpaqueImageSampleCount::None;
                uint32_t                      mipLevels      = 1;
                uint32_t                      arrayLayers    = 1;

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
 * @brief Opaque image handle for graphics resources
 *
 * This opaque handle type allows implementations to store platform-specific
 * image references without exposing graphics API details in the interface.
 * Contains useful metadata about the image for validation and debugging.
 */
template <class T> class GameOpaqueImage
{
        public:
                GameOpaqueImage()  = default;
                ~GameOpaqueImage() = default;

                /**
                 * @brief Sets the native handle
                 * @param handle Native handle pointer
                 */
                void setHandle(void* handle)
                {
                        imageHandle.handle = handle;
                }

                /**
                 * @brief Gets the native handle
                 * @return Native handle pointer
                 */
                void* getHandle() const
                {
                        return imageHandle.handle;
                }

                T* operator->() const
                {
                        return static_cast<T*>(imageHandle.handle);
                }

                T& operator*() const
                {
                        return *static_cast<T*>(imageHandle.handle);
                }

                /**
                 * @brief Sets the image width
                 * @param width Image width in pixels
                 */
                void setWidth(uint64_t width)
                {
                        imageHandle.width = width;
                }

                /**
                 * @brief Gets the image width
                 * @return Image width in pixels
                 */
                uint64_t getWidth() const
                {
                        return imageHandle.width;
                }

                /**
                 * @brief Sets the image height
                 * @param height Image height in pixels
                 */
                void setHeight(uint64_t height)
                {
                        imageHandle.height = height;
                }

                /**
                 * @brief Gets the image height
                 * @return Image height in pixels
                 */
                uint64_t getHeight() const
                {
                        return imageHandle.height;
                }

                /**
                 * @brief Sets the image depth
                 * @param depth Image depth (for 3D images)
                 */
                void setDepth(uint64_t depth)
                {
                        imageHandle.depth = depth;
                }

                /**
                 * @brief Gets the image depth
                 * @return Image depth
                 */
                uint64_t getDepth() const
                {
                        return imageHandle.depth;
                }

                /**
                 * @brief Sets the image format
                 * @param format Image format
                 */
                void setFormat(GameOpaqueImageFormat format)
                {
                        imageHandle.format = format;
                }

                /**
                 * @brief Gets the image format
                 * @return Image format
                 */
                GameOpaqueImageFormat getFormat() const
                {
                        return imageHandle.format;
                }

                /**
                 * @brief Sets the image type
                 * @param type Image type
                 */
                void setType(GameOpaqueImageType type)
                {
                        imageHandle.type = type;
                }

                /**
                 * @brief Gets the image type
                 * @return Image type
                 */
                GameOpaqueImageType getType() const
                {
                        return imageHandle.type;
                }

                /**
                 * @brief Sets the image tiling mode
                 * @param tiling Image tiling mode
                 */
                void setTiling(GameOpaqueImageTiling tiling)
                {
                        imageHandle.tiling = tiling;
                }

                /**
                 * @brief Gets the image tiling mode
                 * @return Image tiling mode
                 */
                GameOpaqueImageTiling getTiling() const
                {
                        return imageHandle.tiling;
                }

                /**
                 * @brief Sets the image usage flags
                 * @param usage Image usage flags
                 */
                void setUsage(GameOpaqueImageUsage usage)
                {
                        imageHandle.usage = usage;
                }

                /**
                 * @brief Gets the image usage flags
                 * @return Image usage flags
                 */
                GameOpaqueImageUsage getUsage() const
                {
                        return imageHandle.usage;
                }

                /**
                 * @brief Sets the image aspect flags
                 * @param aspect Image aspect flags
                 */
                void setAspect(GameOpaqueImageAspect aspect)
                {
                        imageHandle.aspect = aspect;
                }

                /**
                 * @brief Gets the image aspect flags
                 * @return Image aspect flags
                 */
                GameOpaqueImageAspect getAspect() const
                {
                        return imageHandle.aspect;
                }

                /**
                 * @brief Sets the current image layout
                 * @param layout Image layout
                 */
                void setLayout(GameOpaqueImageLayout layout)
                {
                        imageHandle.layout = layout;
                }

                /**
                 * @brief Gets the current image layout
                 * @return Image layout
                 */
                GameOpaqueImageLayout getLayout() const
                {
                        return imageHandle.layout;
                }

                /**
                 * @brief Sets the image memory property flags
                 * @param memoryProperty Memory property flags
                 */
                void setMemoryProperty(GameOpaqueImageMemoryProperty memoryProperty)
                {
                        imageHandle.memoryProperty = memoryProperty;
                }

                /**
                 * @brief Gets the image memory property flags
                 * @return Memory property flags
                 */
                GameOpaqueImageMemoryProperty getMemoryProperty() const
                {
                        return imageHandle.memoryProperty;
                }

                /**
                 * @brief Sets the image sample count
                 * @param sampleCount Sample count flags
                 */
                void setSampleCount(GameOpaqueImageSampleCount sampleCount)
                {
                        imageHandle.sampleCount = sampleCount;
                }

                /**
                 * @brief Gets the image sample count
                 * @return Sample count flags
                 */
                GameOpaqueImageSampleCount getSampleCount() const
                {
                        return imageHandle.sampleCount;
                }

                /**
                 * @brief Sets the mip level count
                 * @param mipLevels Number of mip levels
                 */
                void setMipLevels(uint32_t mipLevels)
                {
                        imageHandle.mipLevels = mipLevels;
                }

                /**
                 * @brief Gets the mip level count
                 * @return Number of mip levels
                 */
                uint32_t getMipLevels() const
                {
                        return imageHandle.mipLevels;
                }

                /**
                 * @brief Sets the array layer count
                 * @param arrayLayers Number of array layers
                 */
                void setArrayLayers(uint32_t arrayLayers)
                {
                        imageHandle.arrayLayers = arrayLayers;
                }

                /**
                 * @brief Gets the array layer count
                 * @return Number of array layers
                 */
                uint32_t getArrayLayers() const
                {
                        return imageHandle.arrayLayers;
                }

                /**
                 * @brief Checks if the handle is valid
                 * @return True if handle is not null
                 */
                bool isValid() const
                {
                        return imageHandle.handle != nullptr;
                }

                /**
                 * @brief Gets the underlying handle struct
                 * @return Reference to the underlying GameOpaqueImageHandle struct
                 *
                 * This allows returning the API-agnostic struct from interfaces
                 * while using the template class in concrete implementations.
                 */
                GameOpaqueImageHandle& getHandleStruct()
                {
                        return imageHandle;
                }

                /**
                 * @brief Gets the underlying handle struct (const)
                 * @return Const reference to the underlying GameOpaqueImageHandle struct
                 */
                const GameOpaqueImageHandle& getHandleStruct() const
                {
                        return imageHandle;
                }

                /**
                 * @brief Returns the underlying native handle typed as T*
                 * @return Pointer to native object
                 */
                T* getNativeHandle() const
                {
                        return static_cast<T*>(imageHandle.handle);
                }

        private:
                GameOpaqueImageHandle imageHandle;
};

} // namespace rl

#endif // RL_BASE_GAME_OPAQUE_IMAGE_HANDLE_H
