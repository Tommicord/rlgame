#include "Rl.Base/GameVulkanImage.h"
#include "Rl.Base/GameError.h"

namespace rl
{

GameVulkanImage::GameVulkanImage() noexcept :
    device(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), image(VK_NULL_HANDLE),
    deviceMemory(VK_NULL_HANDLE), ownsImage(true)
{
}

GameVulkanImage::GameVulkanImage(GameVulkanImage& other) :
    device(other.device), physicalDevice(other.physicalDevice), image(other.image),
    deviceMemory(other.deviceMemory), ownsImage(other.ownsImage)
{
  other.device         = VK_NULL_HANDLE;
  other.physicalDevice = VK_NULL_HANDLE;
  other.image          = VK_NULL_HANDLE;
  other.deviceMemory   = VK_NULL_HANDLE;
  other.ownsImage      = false;
}

GameVulkanImage::GameVulkanImage(VkDevice                         device,
                                 VkPhysicalDevice                 physicalDevice,
                                 const GameVulkanImageCreateInfo& createInfo) :
    device(device), physicalDevice(physicalDevice), image(VK_NULL_HANDLE),
    deviceMemory(VK_NULL_HANDLE), ownsImage(true)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.flags                 = createInfo.flags;
  imageInfo.imageType             = createInfo.imageType;
  imageInfo.extent                = createInfo.extent;
  imageInfo.mipLevels             = createInfo.mipLevels;
  imageInfo.arrayLayers           = createInfo.arrayLayers;
  imageInfo.format                = createInfo.format;
  imageInfo.tiling                = createInfo.tiling;
  imageInfo.initialLayout         = createInfo.initialLayout;
  imageInfo.usage                 = createInfo.usage;
  imageInfo.samples               = createInfo.samples;
  imageInfo.sharingMode           = createInfo.sharingMode;
  imageInfo.queueFamilyIndexCount = createInfo.queueFamilyIndexCount;
  imageInfo.pQueueFamilyIndices   = createInfo.pQueueFamilyIndices;

  VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
  if (result != VK_SUCCESS)
  {
    GameError::exitWithError(
        "vkCreateImage",
        "Failed to create image (result = " + GameError::vulkanResultToString(result) + ")", device,
        VK_NULL_HANDLE, VK_NULL_HANDLE);
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = 0;

  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  bool found = false;
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
  {
    if ((memRequirements.memoryTypeBits & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & createInfo.memoryProperties) ==
            createInfo.memoryProperties)
    {
      allocInfo.memoryTypeIndex = i;
      found                     = true;
      break;
    }
  }

  if (!found)
  {
    vkDestroyImage(device, image, nullptr);
    GameError::exitWithError("vkGetPhysicalDeviceMemoryProperties",
                             "Failed to find suitable memory type for image", device,
                             VK_NULL_HANDLE, VK_NULL_HANDLE);
  }

  result = vkAllocateMemory(device, &allocInfo, nullptr, &deviceMemory);
  if (result != VK_SUCCESS)
  {
    vkDestroyImage(device, image, nullptr);
    GameError::exitWithError("vkAllocateMemory",
                             "Failed to allocate image memory (result = " +
                                 GameError::vulkanResultToString(result) + ")",
                             device, VK_NULL_HANDLE, VK_NULL_HANDLE);
  }

  result = vkBindImageMemory(device, image, deviceMemory, 0);
  if (result != VK_SUCCESS)
  {
    vkFreeMemory(device, deviceMemory, nullptr);
    vkDestroyImage(device, image, nullptr);
    GameError::exitWithError(
        "vkBindImageMemory",
        "Failed to bind image memory (result = " + GameError::vulkanResultToString(result) + ")",
        device, VK_NULL_HANDLE, VK_NULL_HANDLE);
  }
}

GameVulkanImage::GameVulkanImage(GameVulkanImage&& other) noexcept :
    device(other.device), physicalDevice(other.physicalDevice), image(other.image),
    deviceMemory(other.deviceMemory), ownsImage(other.ownsImage)
{
  other.device         = VK_NULL_HANDLE;
  other.physicalDevice = VK_NULL_HANDLE;
  other.image          = VK_NULL_HANDLE;
  other.deviceMemory   = VK_NULL_HANDLE;
  other.ownsImage      = false;
}

GameVulkanImage& GameVulkanImage::operator=(GameVulkanImage&& other) noexcept
{
  if (this != &other)
  {
    if (image != VK_NULL_HANDLE && ownsImage)
    {
      vkFreeMemory(device, deviceMemory, nullptr);
      vkDestroyImage(device, image, nullptr);
    }
    device               = other.device;
    physicalDevice       = other.physicalDevice;
    image                = other.image;
    deviceMemory         = other.deviceMemory;
    ownsImage            = other.ownsImage;
    other.device         = VK_NULL_HANDLE;
    other.physicalDevice = VK_NULL_HANDLE;
    other.image          = VK_NULL_HANDLE;
    other.deviceMemory   = VK_NULL_HANDLE;
    other.ownsImage      = false;
  }
  return *this;
}

GameVulkanImage::~GameVulkanImage()
{
  if (image != VK_NULL_HANDLE && ownsImage)
  {
    vkFreeMemory(device, deviceMemory, nullptr);
    vkDestroyImage(device, image, nullptr);
    image        = VK_NULL_HANDLE;
    deviceMemory = VK_NULL_HANDLE;
  }
}

VkImage GameVulkanImage::getImage() const
{
  return image;
}

VkDeviceMemory GameVulkanImage::getDeviceMemory() const
{
  return deviceMemory;
}

VkPhysicalDevice GameVulkanImage::getPhysicalDevice() const
{
  return physicalDevice;
}

void GameVulkanImage::setImage(VkImage image, VkDeviceMemory memory)
{
  if (image != VK_NULL_HANDLE && ownsImage)
  {
    vkFreeMemory(device, deviceMemory, nullptr);
    vkDestroyImage(device, image, nullptr);
  }
  this->image        = image;
  this->deviceMemory = memory;
  ownsImage          = true;
}

void GameVulkanImage::setImageNonOwning(VkImage image, VkDeviceMemory memory)
{
  if (image != VK_NULL_HANDLE && ownsImage)
  {
    vkFreeMemory(device, deviceMemory, nullptr);
    vkDestroyImage(device, image, nullptr);
  }
  this->image        = image;
  this->deviceMemory = memory;
  ownsImage          = false;
}

void GameVulkanImage::setPhysicalDevice(VkPhysicalDevice physicalDevice)
{
  this->physicalDevice = physicalDevice;
}

} // namespace rl
