import Rl.Base.Texture2;
import Rl.Base.Game;

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define RL_PLATFORM_WINDOWS
#elif defined(__APPLE__)
import <TargetConditionals.h>;
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#define RL_PLATFORM_IOS
#else
#define RL_PLATFORM_MACOS
#endif
#elif defined(__ANDROID__)
#define RL_PLATFORM_ANDROID
#elif defined(__linux__)
#define RL_PLATFORM_LINUX
#endif

// Platform-specific includes
#ifdef RL_PLATFORM_ANDROID
import <android/asset_manager.h>;
import <android/log.h>;
#elif defined(RL_PLATFORM_IOS)
import <CoreFoundation/CoreFoundation.h>;
#endif

#define STB_IMAGE_IMPLEMENTATION
import <stb_image.h>;

import <cmath>;
import <cstring>;
import <fstream>;
import <sstream>;

namespace Rl::Providers
{

// Platform detection implementation
bool Texture2::IsMobilePlatform()
{
#if defined(RL_PLATFORM_ANDROID) || defined(RL_PLATFORM_IOS)
  return true;
#else
  return false;
#endif
}

bool Texture2::IsDesktopPlatform()
{
#if defined(RL_PLATFORM_WINDOWS) || defined(RL_PLATFORM_MACOS) || defined(RL_PLATFORM_LINUX)
  return true;
#else
  return false;
#endif
}

std::string Texture2::GetPlatformName()
{
#if defined(RL_PLATFORM_WINDOWS)
  return "Windows";
#elif defined(RL_PLATFORM_MACOS)
  return "macOS";
#elif defined(RL_PLATFORM_LINUX)
  return "Linux";
#elif defined(RL_PLATFORM_ANDROID)
  return "Android";
#elif defined(RL_PLATFORM_IOS)
  return "iOS";
#else
  return "Unknown";
#endif
}

// Format utilities
int Texture2::GetFormatSize(Texture2Format format)
{
  switch (format)
  {
  case Texture2Format::RGB8:
    return 3;
  case Texture2Format::RGBA8:
    return 4;
  case Texture2Format::RGB16F:
    return 6;
  case Texture2Format::RGBA16F:
    return 8;
  case Texture2Format::RGB32F:
    return 12;
  case Texture2Format::RGBA32F:
    return 16;
  case Texture2Format::R8:
    return 1;
  case Texture2Format::RG8:
    return 2;
  case Texture2Format::R16F:
    return 2;
  case Texture2Format::RG16F:
    return 4;
  case Texture2Format::DEPTH16:
    return 2;
  case Texture2Format::DEPTH24:
    return 3;
  case Texture2Format::DEPTH32F:
    return 4;
  default:
    return 4;
  }
}

std::string Texture2::GetFormatName(Texture2Format format)
{
  switch (format)
  {
  case Texture2Format::RGB8:
    return "RGB8";
  case Texture2Format::RGBA8:
    return "RGBA8";
  case Texture2Format::RGB16F:
    return "RGB16F";
  case Texture2Format::RGBA16F:
    return "RGBA16F";
  case Texture2Format::RGB32F:
    return "RGB32F";
  case Texture2Format::RGBA32F:
    return "RGBA32F";
  case Texture2Format::R8:
    return "R8";
  case Texture2Format::RG8:
    return "RG8";
  case Texture2Format::R16F:
    return "R16F";
  case Texture2Format::RG16F:
    return "RG16F";
  case Texture2Format::DEPTH16:
    return "DEPTH16";
  case Texture2Format::DEPTH24:
    return "DEPTH24";
  case Texture2Format::DEPTH32F:
    return "DEPTH32F";
  default:
    return "UNKNOWN";
  }
}

bool Texture2::IsFormatSupported(Texture2Format format)
{
  // All basic formats are supported
  switch (format)
  {
  case Texture2Format::RGB8:
  case Texture2Format::RGBA8:
  case Texture2Format::R8:
  case Texture2Format::RG8:
    return true;
  case Texture2Format::RGB16F:
  case Texture2Format::RGBA16F:
  case Texture2Format::R16F:
  case Texture2Format::RG16F:
    // Float formats may not be supported on all platforms
    return true;
  case Texture2Format::RGB32F:
  case Texture2Format::RGBA32F:
    // 32-bit float formats may have limited support
    return true;
  case Texture2Format::DEPTH16:
  case Texture2Format::DEPTH24:
  case Texture2Format::DEPTH32F:
    // Depth formats are typically supported
    return true;
  default:
    return false;
  }
}

// Constructor/Destructor
Texture2::Texture2() :
    data(nullptr), dataSize(0), width(0), height(0), channels(0), mipmapLevels(1), loaded(false)
{
  Initialize();
}

Texture2::Texture2(const std::string& filepath) : Texture2()
{
  LoadFromFile(filepath);
}

Texture2::Texture2(const std::string& filepath, const Texture2Properties& properties) : Texture2()
{
  this->properties = properties;
  LoadFromFile(filepath);
}

Texture2::~Texture2()
{
  Cleanup();
}

void Texture2::Initialize()
{
  data         = nullptr;
  dataSize     = 0;
  width        = 0;
  height       = 0;
  channels     = 0;
  mipmapLevels = 1;
  loaded       = false;
}

void Texture2::Cleanup()
{
  if (data)
  {
    stbi_image_free(data);
    data = nullptr;
  }
  dataSize = 0;
  loaded   = false;
}
void Texture2::CleanupVulkan(const Game::VulkanContext& context)
{
  // Cleanup Vulkan resources
  if (binding.vkStagingBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(context.device, binding.vkStagingBuffer, nullptr);
    binding.vkStagingBuffer = VK_NULL_HANDLE;
  }
  if (binding.vkStagingBufferMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(context.device, binding.vkStagingBufferMemory, nullptr);
    binding.vkStagingBufferMemory = VK_NULL_HANDLE;
  }
  if (binding.vkImageView != VK_NULL_HANDLE)
  {
    vkDestroyImageView(context.device, binding.vkImageView, nullptr);
    binding.vkImageView = VK_NULL_HANDLE;
  }
  if (binding.vkSampler != VK_NULL_HANDLE)
  {
    vkDestroySampler(context.device, binding.vkSampler, nullptr);
    binding.vkSampler = VK_NULL_HANDLE;
  }
  if (binding.vkImage != VK_NULL_HANDLE)
  {
    vkDestroyImage(context.device, binding.vkImage, nullptr);
    binding.vkImage = VK_NULL_HANDLE;
  }
  if (binding.vkImageMemory != VK_NULL_HANDLE)
  {
    vkFreeMemory(context.device, binding.vkImageMemory, nullptr);
    binding.vkImageMemory = VK_NULL_HANDLE;
  }
}

// Loading functions
bool Texture2::LoadFromFile(const std::string& filepath)
{
  return LoadFromFile(filepath, properties);
}

bool Texture2::LoadFromFile(const std::string& filepath, const Texture2Properties& properties)
{
  this->properties = properties;
  this->filepath   = filepath;
  return LoadImage(filepath);
}

bool Texture2::LoadFromMemory(const uint8_t* data, size_t size, const Texture2Properties& properties)
{
  this->properties = properties;

  int      width, height, channels;
  stbi_uc* imageData =
      stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 0);

  if (!imageData)
  {
    return false;
  }

  return ProcessImageData(imageData, width, height, channels);
}

bool Texture2::LoadFromData(const uint8_t* data,
    int                                    width,
    int                                    height,
    Texture2Format                          format,
    const Texture2Properties&               properties)
{
  this->properties        = properties;
  this->properties.format = format;
  this->width             = width;
  this->height            = height;
  this->channels          = GetFormatSize(format);

  this->dataSize = width * height * this->channels;
  this->data     = new uint8_t[this->dataSize];
  memcpy(this->data, data, this->dataSize);
  loaded = true;
  if (properties.generateMipmaps)
  {
    GenerateMipmaps();
  }

  return true;
}

bool Texture2::LoadFromAndroidAsset(const std::string& assetPath)
{
#ifdef RL_PLATFORM_ANDROID
  // Android-specific asset loading
  // This requires access to the Android AssetManager
  // For now, return false as it needs JNI integration
  return false;
#else
  return false;
#endif
}

bool Texture2::LoadFromIOSBundle(const std::string& resourcePath)
{
#ifdef RL_PLATFORM_IOS
  // iOS-specific bundle resource loading
  CFBundleRef mainBundle = CFBundleGetMainBundle();
  if (!mainBundle)
  {
    return false;
  }

  CFStringRef cfPath =
      CFStringCreateWithCString(kCFAllocatorDefault, resourcePath.c_str(), kCFStringEncodingUTF8);
  CFURLRef cfUrl = CFBundleCopyResourceURL(mainBundle, cfPath, NULL, NULL);
  CFRelease(cfPath);

  if (!cfUrl)
  {
    return false;
  }

  CFStringRef cfFilePath = CFURLCopyFileSystemPath(cfUrl, kCFURLPOSIXPathStyle);
  CFRelease(cfUrl);

  CFIndex length   = CFStringGetLength(cfFilePath);
  CFIndex maxSize  = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
  char*   filePath = new char[maxSize];

  if (CFStringGetCString(cfFilePath, filePath, maxSize, kCFStringEncodingUTF8))
  {
    bool result = LoadFromFile(std::string(filePath));
    delete[] filePath;
    CFRelease(cfFilePath);
    return result;
  }

  delete[] filePath;
  CFRelease(cfFilePath);
  return false;
#else
  return false;
#endif
}

bool Texture2::LoadImage(const std::string& filepath)
{
  int width, height, channels;
  // Load image with stb_image
  stbi_uc* imageData = stbi_load(filepath.c_str(), &width, &height, &channels, 0);

  if (!imageData)
  {
    // Try platform-specific loading if standard loading fails
    if (IsMobilePlatform())
    {
#ifdef RL_PLATFORM_ANDROID
      return LoadFromAndroidAsset(filepath);
#elif defined(RL_PLATFORM_IOS)
      return LoadFromIOSBundle(filepath);
#endif
    }
    return false;
  }

  return ProcessImageData(imageData, width, height, channels);
}

bool Texture2::ProcessImageData(uint8_t* imageData, int width, int height, int channels)
{
  this->width    = width;
  this->height   = height;
  this->channels = channels;

  // Determine format based on channels
  switch (channels)
  {
  case 1:
    properties.format = Texture2Format::R8;
    break;
  case 2:
    properties.format = Texture2Format::RG8;
    break;
  case 3:
    properties.format = Texture2Format::RGB8;
    break;
  case 4:
    properties.format = Texture2Format::RGBA8;
    break;
  default:
    stbi_image_free(imageData);
    return false;
  }
  // Calculate data size
  dataSize = width * height * channels;
  data     = new uint8_t[dataSize];
  memcpy(data, imageData, dataSize);
  // Free stb_image data
  stbi_image_free(imageData);
  loaded = true;
  // Generate mipmaps if requested
  if (properties.generateMipmaps)
  {
    GenerateMipmaps();
  }

  return true;
}

void Texture2::GenerateMipmaps()
{
  if (!loaded || width == 0 || height == 0)
  {
    return;
  }
  // Calculate number of mipmap levels
  int maxDimension = std::max(width, height);
  mipmapLevels     = static_cast<int>(std::floor(std::log2(maxDimension))) + 1;

  // Generate mipmap data from RGBA array
  if (mipmapLevels > 1 && data)
  {
    // Calculate total size needed for all mipmaps
    size_t totalMipmapSize = 0;
    int    mipWidth        = width;
    int    mipHeight       = height;

    for (int level = 0; level < mipmapLevels; ++level)
    {
      size_t levelSize = mipWidth * mipHeight * channels;
      totalMipmapSize += levelSize;
      if (mipWidth > 1)
        mipWidth /= 2;
      if (mipHeight > 1)
        mipHeight /= 2;
    }

    // Allocate new buffer for mipmapped data
    uint8_t* mipmappedData = new uint8_t[totalMipmapSize];
    uint8_t* dst           = mipmappedData;

    // Copy base level
    mipWidth        = width;
    mipHeight       = height;
    size_t baseSize = width * height * channels;
    memcpy(dst, data, baseSize);
    dst += baseSize;

    // Generate each subsequent mipmap level
    for (int level = 1; level < mipmapLevels; ++level)
    {
      int prevWidth  = mipWidth;
      int prevHeight = mipHeight;

      if (mipWidth > 1)
        mipWidth /= 2;
      if (mipHeight > 1)
        mipHeight /= 2;

      // Downsample using box filter
      for (int y = 0; y < mipHeight; ++y)
      {
        for (int x = 0; x < mipWidth; ++x)
        {
          // Sample 2x2 pixels from previous level and average
          for (int c = 0; c < channels; ++c)
          {
            int sum   = 0;
            int count = 0;

            for (int dy = 0; dy < 2; ++dy)
            {
              for (int dx = 0; dx < 2; ++dx)
              {
                int srcX = (x * 2 + dx) < prevWidth ? (x * 2 + dx) : prevWidth - 1;
                int srcY = (y * 2 + dy) < prevHeight ? (y * 2 + dy) : prevHeight - 1;

                size_t srcOffset = (srcY * prevWidth + srcX) * channels + c;
                // Find the offset in the previous level's data
                size_t prevLevelOffset = 0;
                int    pw              = width;
                int    ph              = height;
                for (int l = 0; l < level - 1; ++l)
                {
                  size_t levelSize = pw * ph * channels;
                  prevLevelOffset += levelSize;
                  if (pw > 1)
                    pw /= 2;
                  if (ph > 1)
                    ph /= 2;
                }
                prevLevelOffset += srcOffset + c;

                sum += data[prevLevelOffset];
                count++;
              }
            }
            dst[(y * mipWidth + x) * channels + c] = static_cast<uint8_t>(sum / count);
          }
        }
      }

      dst += mipWidth * mipHeight * channels;
    }

    // Replace old data with mipmapped data
    delete[] data;
    data     = mipmappedData;
    dataSize = totalMipmapSize;
  }
}

} // namespace Rl::Providers
