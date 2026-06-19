#include "rl/Base/Texture2.h"

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define RL_PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
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
    #include <android/asset_manager.h>
    #include <android/log.h>
#elif defined(RL_PLATFORM_IOS)
    #include <CoreFoundation/CoreFoundation.h>
#endif

// stb_image for cross-platform image loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>

namespace Rl::Providers {

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
int Texture2::GetFormatSize(TextureFormat format)
{
    switch (format) {
        case TextureFormat::RGB8: return 3;
        case TextureFormat::RGBA8: return 4;
        case TextureFormat::RGB16F: return 6;
        case TextureFormat::RGBA16F: return 8;
        case TextureFormat::RGB32F: return 12;
        case TextureFormat::RGBA32F: return 16;
        case TextureFormat::R8: return 1;
        case TextureFormat::RG8: return 2;
        case TextureFormat::R16F: return 2;
        case TextureFormat::RG16F: return 4;
        case TextureFormat::DEPTH16: return 2;
        case TextureFormat::DEPTH24: return 3;
        case TextureFormat::DEPTH32F: return 4;
        default: return 4;
    }
}

std::string Texture2::GetFormatName(TextureFormat format)
{
    switch (format) {
        case TextureFormat::RGB8: return "RGB8";
        case TextureFormat::RGBA8: return "RGBA8";
        case TextureFormat::RGB16F: return "RGB16F";
        case TextureFormat::RGBA16F: return "RGBA16F";
        case TextureFormat::RGB32F: return "RGB32F";
        case TextureFormat::RGBA32F: return "RGBA32F";
        case TextureFormat::R8: return "R8";
        case TextureFormat::RG8: return "RG8";
        case TextureFormat::R16F: return "R16F";
        case TextureFormat::RG16F: return "RG16F";
        case TextureFormat::DEPTH16: return "DEPTH16";
        case TextureFormat::DEPTH24: return "DEPTH24";
        case TextureFormat::DEPTH32F: return "DEPTH32F";
        default: return "UNKNOWN";
    }
}

bool Texture2::IsFormatSupported(TextureFormat format)
{
    // All basic formats are supported
    switch (format) {
        case TextureFormat::RGB8:
        case TextureFormat::RGBA8:
        case TextureFormat::R8:
        case TextureFormat::RG8:
            return true;
        case TextureFormat::RGB16F:
        case TextureFormat::RGBA16F:
        case TextureFormat::R16F:
        case TextureFormat::RG16F:
            // Float formats may not be supported on all platforms
            return true;
        case TextureFormat::RGB32F:
        case TextureFormat::RGBA32F:
            // 32-bit float formats may have limited support
            return true;
        case TextureFormat::DEPTH16:
        case TextureFormat::DEPTH24:
        case TextureFormat::DEPTH32F:
            // Depth formats are typically supported
            return true;
        default:
            return false;
    }
}

// Constructor/Destructor
Texture2::Texture2()
    : m_data(nullptr)
    , m_dataSize(0)
    , m_width(0)
    , m_height(0)
    , m_channels(0)
    , m_mipmapLevels(1)
    , m_loaded(false)
{
    Initialize();
}

Texture2::Texture2(const std::string& filepath)
    : Texture2()
{
    LoadFromFile(filepath);
}

Texture2::Texture2(const std::string& filepath, const TextureProperties& properties)
    : Texture2()
{
    m_properties = properties;
    LoadFromFile(filepath);
}

Texture2::~Texture2()
{
    Cleanup();
}

void Texture2::Initialize()
{
    m_data = nullptr;
    m_dataSize = 0;
    m_width = 0;
    m_height = 0;
    m_channels = 0;
    m_mipmapLevels = 1;
    m_loaded = false;
}

void Texture2::Cleanup()
{
    if (m_data) {
        stbi_image_free(m_data);
        m_data = nullptr;
    }
    m_dataSize = 0;
    m_loaded = false;
}

// Loading functions
bool Texture2::LoadFromFile(const std::string& filepath)
{
    return LoadFromFile(filepath, m_properties);
}

bool Texture2::LoadFromFile(const std::string& filepath, const TextureProperties& properties)
{
    m_properties = properties;
    m_filepath = filepath;
    
    return LoadImage(filepath);
}

bool Texture2::LoadFromMemory(const uint8_t* data, size_t size, const TextureProperties& properties)
{
    m_properties = properties;
    
    int width, height, channels;
    stbi_uc* imageData = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 0);
    
    if (!imageData) {
        return false;
    }
    
    return ProcessImageData(imageData, width, height, channels);
}

bool Texture2::LoadFromData(const uint8_t* data, int width, int height, TextureFormat format, const TextureProperties& properties)
{
    m_properties = properties;
    m_properties.format = format;
    m_width = width;
    m_height = height;
    m_channels = GetFormatSize(format);
    
    m_dataSize = width * height * m_channels;
    m_data = new uint8_t[m_dataSize];
    memcpy(m_data, data, m_dataSize);
    
    m_loaded = true;
    
    if (m_properties.generateMipmaps) {
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
    if (!mainBundle) {
        return false;
    }
    
    CFStringRef cfPath = CFStringCreateWithCString(kCFAllocatorDefault, resourcePath.c_str(), kCFStringEncodingUTF8);
    CFURLRef cfUrl = CFBundleCopyResourceURL(mainBundle, cfPath, NULL, NULL);
    CFRelease(cfPath);
    
    if (!cfUrl) {
        return false;
    }
    
    CFStringRef cfFilePath = CFURLCopyFileSystemPath(cfUrl, kCFURLPOSIXPathStyle);
    CFRelease(cfUrl);
    
    CFIndex length = CFStringGetLength(cfFilePath);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    char* filePath = new char[maxSize];
    
    if (CFStringGetCString(cfFilePath, filePath, maxSize, kCFStringEncodingUTF8)) {
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
    stbi_uc* imageData = stbi_load(
        filepath.c_str(),
        &width, &height,
        &channels,
        0
        );
    
    if (!imageData) {
        // Try platform-specific loading if standard loading fails
        if (IsMobilePlatform()) {
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
    m_width = width;
    m_height = height;
    m_channels = channels;
    
    // Determine format based on channels
    switch (channels) {
        case 1:
            m_properties.format = TextureFormat::R8;
            break;
        case 2:
            m_properties.format = TextureFormat::RG8;
            break;
        case 3:
            m_properties.format = TextureFormat::RGB8;
            break;
        case 4:
            m_properties.format = TextureFormat::RGBA8;
            break;
        default:
            stbi_image_free(imageData);
            return false;
    }
    
    // Calculate data size
    m_dataSize = width * height * channels;
    m_data = new uint8_t[m_dataSize];
    memcpy(m_data, imageData, m_dataSize);
    // Free stb_image data
    stbi_image_free(imageData);
    m_loaded = true;
    // Generate mipmaps if requested
    if (m_properties.generateMipmaps) {
        GenerateMipmaps();
    }
    
    return true;
}

void Texture2::GenerateMipmaps()
{
    if (!m_loaded || m_width == 0 || m_height == 0) {
        return;
    }
    
    // Calculate number of mipmap levels
    int maxDimension = std::max(m_width, m_height);
    m_mipmapLevels = static_cast<int>(std::floor(std::log2(maxDimension))) + 1;
}

} // namespace Rl
