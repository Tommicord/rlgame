export module Rl.Base.Texture2;

import <cstdint>;
import <glm/glm.hpp>;
import <string>;
import <vector>;
import <vulkan/vulkan.h>;

// Forward declaration
namespace Rl::Game
{
export struct VulkanContext;
}

namespace Rl::Providers
{

export enum class Texture2Format
{
  UNKNOWN,
  RGB8,
  RGBA8,
  RGB16F,
  RGBA16F,
  RGB32F,
  RGBA32F,
  R8,
  RG8,
  R16F,
  RG16F,
  DEPTH16,
  DEPTH24,
  DEPTH32F
};

export enum class Texture2Filter
{
  NEAREST,
  LINEAR,
  NEAREST_MIPMAP_NEAREST,
  LINEAR_MIPMAP_NEAREST,
  NEAREST_MIPMAP_LINEAR,
  LINEAR_MIPMAP_LINEAR
};

export enum class Texture2Wrap
{
  REPEAT,
  MIRRORED_REPEAT,
  CLAMP_TO_EDGE,
  CLAMP_TO_BORDER
};

export struct Texture2Properties
{
  Texture2Format format               = Texture2Format::RGBA8;
  Texture2Filter minFilter            = Texture2Filter::LINEAR_MIPMAP_LINEAR;
  Texture2Filter magFilter            = Texture2Filter::LINEAR;
  Texture2Wrap   wrapS                = Texture2Wrap::REPEAT;
  Texture2Wrap   wrapT                = Texture2Wrap::REPEAT;
  bool          generateMipmaps      = true;
  bool          sRGB                 = true;
  bool          anisotropicFiltering = true;
  float         maxAnisotropy        = 16.0f;
};

export class Texture2
{
  public:
  /* Vulkan-specific resources */
  struct VkBinding
  {
    VkImage        vkImage               = VK_NULL_HANDLE;
    VkDeviceMemory vkImageMemory         = VK_NULL_HANDLE;
    VkSampler      vkSampler             = VK_NULL_HANDLE;
    VkImageView    vkImageView           = VK_NULL_HANDLE;
    VkBuffer       vkStagingBuffer       = VK_NULL_HANDLE;
    VkDeviceMemory vkStagingBufferMemory = VK_NULL_HANDLE;
  };
  VkBinding binding;
  Texture2();
  explicit Texture2(const std::string& filepath);
  Texture2(const std::string& filepath, const Texture2Properties& properties);
  ~Texture2();
  // Texture data access
  [[nodiscard]]
  uint8_t* GetData() const
  {
    return data;
  }
  [[nodiscard]]
  size_t GetDataSize() const
  {
    return dataSize;
  }
  // Texture properties
  [[nodiscard]]
  int GetWidth() const
  {
    return width;
  }
  [[nodiscard]]
  int GetHeight() const
  {
    return height;
  }
  [[nodiscard]]
  int GetChannels() const
  {
    return channels;
  }
  [[nodiscard]]
  Texture2Format GetFormat() const
  {
    return properties.format;
  }
  [[nodiscard]]
  int GetMipmapLevels() const
  {
    return mipmapLevels;
  }
  // Texture configuration
  void SetProperties(const Texture2Properties& properties)
  {
    this->properties = properties;
  }
  [[nodiscard]]
  const Texture2Properties& GetProperties() const
  {
    return properties;
  }
  // Texture state
  [[nodiscard]]
  bool IsLoaded() const
  {
    return loaded;
  }
  [[nodiscard]]
  bool HasMipmaps() const
  {
    return mipmapLevels > 1;
  }
  static bool        IsFormatSupported(Texture2Format format);
  static int         GetFormatSize(Texture2Format format);
  static std::string GetFormatName(Texture2Format format);
  bool LoadFromFile(const std::string& filepath);
  bool LoadFromFile(const std::string& filepath, const Texture2Properties& properties);
  bool LoadFromMemory(const uint8_t* data, size_t size, const Texture2Properties& properties);
  bool LoadFromData(const uint8_t* data,
      int                          width,
      int                          height,
      Texture2Format                format,
      const Texture2Properties&     properties);
  bool LoadFromAndroidAsset(const std::string& assetPath);
  bool LoadFromIOSBundle(const std::string& resourcePath);
  static bool        IsMobilePlatform();
  static bool        IsDesktopPlatform();
  static std::string GetPlatformName();

  void GenerateMipmaps();
  void GetSampler(Game::VulkanContext& context);
  void GetImageView(Game::VulkanContext& context);
  void Cleanup();
  void CleanupVulkan(const Game::VulkanContext& context);

  private:
  void                 Initialize();
  bool                 LoadImage(const std::string& filepath);
  bool                 ProcessImageData(uint8_t* imageData, int width, int height, int channels);
  void                 CreateVulkanImage(Game::VulkanContext& context);
  void                 CreateVulkanSampler(Game::VulkanContext& context);
  void                 UploadTextureData(Game::VulkanContext& context);
  [[nodiscard]]
  VkFormat             GetVkFormat() const;
  [[nodiscard]]
  VkFilter             GetVkFilter(Texture2Filter filter) const;
  [[nodiscard]]
  VkSamplerMipmapMode  GetVkMipmapMode(Texture2Filter filter) const;
  [[nodiscard]]
  VkSamplerAddressMode GetVkWrapMode(Texture2Wrap wrap) const;

  uint8_t* data;
  size_t   dataSize;
  int      width;
  int      height;
  int      channels;
  int      mipmapLevels;

  Texture2Properties properties;
  bool        loaded;
  std::string filepath;
};

export Texture2* GenerateLightningTexture(Texture2* baseTexture, const Texture2Properties& properties);
export Texture2* GenerateDirectionalLightTexture(
    Texture2* baseTexture, const glm::vec3& lightDirection, const Texture2Properties& properties);
export Texture2* GenerateNormalTexture(Texture2* baseTexture, const Texture2Properties& properties);

} // namespace Rl::Providers
