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

export enum class TextureFormat
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

// Texture filtering modes
export enum class TextureFilter
{
  NEAREST,
  LINEAR,
  NEAREST_MIPMAP_NEAREST,
  LINEAR_MIPMAP_NEAREST,
  NEAREST_MIPMAP_LINEAR,
  LINEAR_MIPMAP_LINEAR
};

// Texture wrapping modes
export enum class TextureWrap
{
  REPEAT,
  MIRRORED_REPEAT,
  CLAMP_TO_EDGE,
  CLAMP_TO_BORDER
};

export struct TextureProperties
{
  TextureFormat format               = TextureFormat::RGBA8;
  TextureFilter minFilter            = TextureFilter::LINEAR_MIPMAP_LINEAR;
  TextureFilter magFilter            = TextureFilter::LINEAR;
  TextureWrap   wrapS                = TextureWrap::REPEAT;
  TextureWrap   wrapT                = TextureWrap::REPEAT;
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
  Texture2(const std::string& filepath, const TextureProperties& properties);
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
  TextureFormat GetFormat() const
  {
    return properties.format;
  }
  [[nodiscard]]
  int GetMipmapLevels() const
  {
    return mipmapLevels;
  }
  // Texture configuration
  void SetProperties(const TextureProperties& properties)
  {
    this->properties = properties;
  }
  [[nodiscard]]
  const TextureProperties& GetProperties() const
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
  // Utility functions
  static bool        IsFormatSupported(TextureFormat format);
  static int         GetFormatSize(TextureFormat format);
  static std::string GetFormatName(TextureFormat format);
  // Loading functions
  bool LoadFromFile(const std::string& filepath);
  bool LoadFromFile(const std::string& filepath, const TextureProperties& properties);
  bool LoadFromMemory(const uint8_t* data, size_t size, const TextureProperties& properties);
  bool LoadFromData(const uint8_t* data,
      int                          width,
      int                          height,
      TextureFormat                format,
      const TextureProperties&     properties);
  // Platform-specific resource loading
  bool LoadFromAndroidAsset(const std::string& assetPath);
  bool LoadFromIOSBundle(const std::string& resourcePath);
  // Platform detection
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
  VkFormat             GetVkFormat() const;
  VkFilter             GetVkFilter(TextureFilter filter) const;
  VkSamplerMipmapMode  GetVkMipmapMode(TextureFilter filter) const;
  VkSamplerAddressMode GetVkWrapMode(TextureWrap wrap) const;

  /* Texture data */
  uint8_t* data;
  size_t   dataSize;
  int      width;
  int      height;
  int      channels;
  int      mipmapLevels;
  /* Texture properties */

  TextureProperties properties;
  /* State */
  bool        loaded;
  std::string filepath;
};

export Texture2* GenerateLightningTexture(Texture2* baseTexture, const TextureProperties& properties);
export Texture2* GenerateDirectionalLightTexture(
    Texture2* baseTexture, const glm::vec3& lightDirection, const TextureProperties& properties);
export Texture2* GenerateNormalTexture(Texture2* baseTexture, const TextureProperties& properties);

} // namespace Rl::Providers
