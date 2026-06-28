export module Rl.Base.Texture2;

import Rl.Base.Binding;

import <cstdint>;
import <glm/glm.hpp>;
import <string>;
import <vector>;
import <vulkan/vulkan.h>;

namespace Rl::Providers
{

export enum class Texture2Format {
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

export enum class Texture2Filter {
  NEAREST,
  LINEAR,
  NEAREST_MIPMAP_NEAREST,
  LINEAR_MIPMAP_NEAREST,
  NEAREST_MIPMAP_LINEAR,
  LINEAR_MIPMAP_LINEAR
};

export enum class Texture2Wrap {
  REPEAT,
  MIRRORED_REPEAT,
  CLAMP_TO_EDGE,
  CLAMP_TO_BORDER
};

export struct Texture2Properties
{
  Texture2Format format = Texture2Format::RGBA8;
  Texture2Filter minFilter = Texture2Filter::LINEAR_MIPMAP_LINEAR;
  Texture2Filter magFilter = Texture2Filter::LINEAR;
  Texture2Wrap   wrapS = Texture2Wrap::REPEAT;
  Texture2Wrap   wrapT = Texture2Wrap::REPEAT;
  bool           generateMipmaps = true;
  bool           sRGB = true;
  bool           anisotropicFiltering = true;
  float          maxAnisotropy = 16.0f;
};

export class Texture2
{
  public:
  /* Vulkan-specific resources */
  struct VkBinding
  {
    VkImage        vkImage = VK_NULL_HANDLE;
    VkDeviceMemory vkImageMemory = VK_NULL_HANDLE;
    VkSampler      vkSampler = VK_NULL_HANDLE;
    VkImageView    vkImageView = VK_NULL_HANDLE;
    VkBuffer       vkStagingBuffer = VK_NULL_HANDLE;
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
  bool               FromResource(const std::string& filepath);
  bool FromResource(const std::string& filepath, const Texture2Properties& properties);
  bool FromMemory(const uint8_t* data, size_t size, const Texture2Properties& properties);
  bool FromData(const uint8_t*  data,
      int                       width,
      int                       height,
      Texture2Format            format,
      const Texture2Properties& properties);
  bool FromAndroidAsset(const std::string& assetPath);
  bool FromIOSBundle(const std::string& resourcePath);
  static bool        IsMobilePlatform();
  static bool        IsDesktopPlatform();
  static std::string GetPlatformName();

  void GenMipmaps();
  void GetSampler(Main::MainBinding& context);
  void GetImageView(Main::MainBinding& context);
  void Cleanup();
  void CleanupBinding(const Main::MainBinding& context);

  private:
  void Initialize();
  bool LoadImage(const std::string& filepath);
  bool ProcessImageData(uint8_t* imageData, int width, int height, int channels);
  void CreateBindingImage(Main::MainBinding& context);
  void CreateBindingSampler(Main::MainBinding& context);
  void UploadTextureData(Main::MainBinding& context);
  [[nodiscard]]
  VkFormat GetBindingFormat() const;
  [[nodiscard]]
  VkFilter GetBindingFilter(Texture2Filter filter) const;
  [[nodiscard]]
  VkSamplerMipmapMode GetBindingMipmapMode(Texture2Filter filter) const;
  [[nodiscard]]
  VkSamplerAddressMode GetBindingWrapMode(Texture2Wrap wrap) const;

  uint8_t* data;
  size_t   dataSize;
  int      width;
  int      height;
  int      channels;
  int      mipmapLevels;

  Texture2Properties properties;
  bool               loaded;
  std::string        filepath;
};

export Texture2* GenerateLightningTexture(
    Texture2* baseTexture, const Texture2Properties& properties);
export Texture2* GenerateDirectionalLightTexture(const Texture2* baseTexture,
    const glm::vec3&                                       lightDirection,
    const Texture2Properties&                              properties);
export Texture2* GenerateNormalTexture(
    Texture2* baseTexture, const Texture2Properties& properties);

} // namespace Rl::Providers
