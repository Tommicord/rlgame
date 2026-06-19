#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Rl::Providers {

enum class TextureFormat {
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
enum class TextureFilter {
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};

// Texture wrapping modes
enum class TextureWrap {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER
};

struct TextureProperties {
    TextureFormat format = TextureFormat::RGBA8;
    TextureFilter minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
    TextureFilter magFilter = TextureFilter::LINEAR;
    TextureWrap wrapS = TextureWrap::REPEAT;
    TextureWrap wrapT = TextureWrap::REPEAT;
    bool generateMipmaps = true;
    bool sRGB = true;
    bool anisotropicFiltering = true;
    float maxAnisotropy = 16.0f;
};

class Texture2
{
public:
    Texture2();
    explicit Texture2(const std::string& filepath);
    Texture2(const std::string& filepath, const TextureProperties& properties);
    ~Texture2();
    // Texture data access
    [[nodiscard]]
    uint8_t* GetData() const { return m_data; }
    [[nodiscard]]
    size_t GetDataSize() const { return m_dataSize; }
    // Texture properties
    [[nodiscard]]
    int GetWidth() const { return m_width; }
    [[nodiscard]]
    int GetHeight() const { return m_height; }
    [[nodiscard]]
    int GetChannels() const { return m_channels; }
    [[nodiscard]]
    TextureFormat GetFormat() const { return m_properties.format; }
    [[nodiscard]]
    int GetMipmapLevels() const { return m_mipmapLevels; }
    // Texture configuration
    void SetProperties(const TextureProperties& properties) { m_properties = properties; }
    [[nodiscard]]
    const TextureProperties& GetProperties() const { return m_properties; }
    // Texture state
    [[nodiscard]]
    bool IsLoaded() const { return m_loaded; }
    [[nodiscard]]
    bool HasMipmaps() const { return m_mipmapLevels > 1; }
    // Utility functions
    static bool IsFormatSupported(TextureFormat format);
    static int GetFormatSize(TextureFormat format);
    static std::string GetFormatName(TextureFormat format);
    // Loading functions
    bool LoadFromFile(const std::string& filepath);
    bool LoadFromFile(const std::string& filepath, const TextureProperties& properties);
    bool LoadFromMemory(const uint8_t* data, size_t size, const TextureProperties& properties);
    bool LoadFromData(const uint8_t* data, int width, int height, TextureFormat format, const TextureProperties& properties);
    // Platform-specific resource loading
    bool LoadFromAndroidAsset(const std::string& assetPath);
    bool LoadFromIOSBundle(const std::string& resourcePath);
    // Platform detection
    static bool IsMobilePlatform();
    static bool IsDesktopPlatform();
    static std::string GetPlatformName();
private:
    void Initialize();
    void Cleanup();
    bool LoadImage(const std::string& filepath);
    bool ProcessImageData(uint8_t* imageData, int width, int height, int channels);
    void GenerateMipmaps();
    
    /* Texture data */
    uint8_t* m_data;
    size_t m_dataSize;
    int m_width;
    int m_height;
    int m_channels;
    int m_mipmapLevels;
    /* Texture properties */

    TextureProperties m_properties;
    /* State */
    bool m_loaded;
    std::string m_filepath;
};

} // namespace Rl::Providers