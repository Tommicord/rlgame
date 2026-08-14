#ifndef RL_BASE_GAME_VULKAN_IMAGE_H
#define RL_BASE_GAME_VULKAN_IMAGE_H

#include <vulkan/vulkan.hpp>

namespace rl
{

/** @brief Create info for image creation */
struct GameVulkanImageCreateInfo
{
    VkImageCreateFlags    flags                 = 0;
    VkImageType           imageType             = VK_IMAGE_TYPE_2D;
    VkFormat              format                = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent3D            extent                = {0, 0, 0};
    uint32_t              mipLevels             = 1;
    uint32_t              arrayLayers           = 1;
    VkSampleCountFlagBits samples               = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling         tiling                = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags     usage                 = VK_IMAGE_USAGE_SAMPLED_BIT;
    VkSharingMode         sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    uint32_t              queueFamilyIndexCount = 0;
    const uint32_t*       pQueueFamilyIndices   = nullptr;
    VkImageLayout         initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    VkMemoryPropertyFlags memoryProperties      = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
};

/** @brief RAII wrapper for Vulkan image objects with device memory */
class GameVulkanImage
{
  public:
    /**
     * @brief Constructs an image (VK_NULL_HANDLE by default)
     */
    GameVulkanImage() noexcept;

    /**
     * @brief Constructs an image by createInfo
     * @param device Vulkan device
     * @param physicalDevice Physical device for memory allocation
     * @param createInfo Image creation info
     */
    GameVulkanImage(VkDevice                         device,
                    VkPhysicalDevice                 physicalDevice,
                    const GameVulkanImageCreateInfo& createInfo);

    /** @brief Destroys the image and frees device memory */
    ~GameVulkanImage();

    GameVulkanImage(GameVulkanImage& other);
    GameVulkanImage(const GameVulkanImage& other) = delete;
    GameVulkanImage(GameVulkanImage&& other) noexcept;
    GameVulkanImage& operator=(const GameVulkanImage& other) = delete;
    GameVulkanImage& operator=(GameVulkanImage&& other) noexcept;

    /** @brief Returns the image handle
     * @return Vulkan image handle */
    VkImage getImage() const;

    /** @brief Returns the device memory handle
     * @return Vulkan device memory handle */
    VkDeviceMemory getDeviceMemory() const;

    /** @brief Returns the physical device
     * @return Physical device handle */
    VkPhysicalDevice getPhysicalDevice() const;

    /** @brief Sets the image to the current state (takes ownership)
     * @param image Vulkan image handle to take ownership of
     * @param memory Device memory handle to take ownership of */
    void setImage(VkImage image, VkDeviceMemory memory);

    /** @brief Sets the image without taking ownership (non-owning reference)
     * @param image Vulkan image handle to reference without ownership
     * @param memory Device memory handle to reference without ownership */
    void setImageNonOwning(VkImage image, VkDeviceMemory memory);

    /** @brief Sets the physical device (for memory operations)
     * @param physicalDevice Physical device handle */
    void setPhysicalDevice(VkPhysicalDevice physicalDevice);

  private:
    VkDevice         device         = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkImage          image          = VK_NULL_HANDLE;
    VkDeviceMemory   deviceMemory   = VK_NULL_HANDLE;
    bool             ownsImage      = true;
};

} // namespace rl

#endif // RL_BASE_GAME_VULKAN_IMAGE_H
