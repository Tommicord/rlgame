#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <vector>
#include <random>
#include <chrono>
#include <cstdio>

extern "C"
{
#include "rpack/rpack_decoder.h"
#include "rpack/rpack_encoder.h"
#include "rpack/rpack_input.h"
#include "rpack/rpack_val.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace
{

constexpr size_t kHeapSize = 64 * 1024 * 1024; // Larger heap for stress tests

class RPackStressTest : public ::testing::Test
{
protected:
    void SetUp () override { ASSERT_EQ (R_CSTL_HeapInit (kHeapSize), R_CSTL_OK); }
    void TearDown () override { R_CSTL_HeapShutdown (); }

    static R_Pack_OwnedImage MakeRandomImage (const char* pName, uint32_t width, uint32_t height)
    {
        std::vector<uint8_t> pixels (width * height * 4);
        std::mt19937 gen (std::random_device{} ());
        std::uniform_int_distribution<uint8_t> dist (0, 255);
        
        for (auto& pixel : pixels)
        {
            pixel = dist (gen);
        }
        
        R_Pack_OwnedImage image = {};
        EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), width, height, width * 4, pName, &image), R_PACK_OK);
        return image;
    }

    static R_Pack_OwnedImage MakeGradientImage (const char* pName, uint32_t width, uint32_t height)
    {
        std::vector<uint8_t> pixels (width * height * 4);
        
        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                uint32_t idx = (y * width + x) * 4;
                pixels[idx] = static_cast<uint8_t>((x * 255) / width);     // R
                pixels[idx + 1] = static_cast<uint8_t>((y * 255) / height); // G
                pixels[idx + 2] = static_cast<uint8_t>(((x + y) * 255) / (width + height)); // B
                pixels[idx + 3] = 255; // A
            }
        }
        
        R_Pack_OwnedImage image = {};
        EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), width, height, width * 4, pName, &image), R_PACK_OK);
        return image;
    }

    static std::vector<uint8_t> Encode (R_Pack_OwnedImage* pImage, uint32_t maxTextures = 0)
    {
        R_Pack_EncoderSettings config = {};
        config.maxAtlasWidth = 4096;
        config.maxAtlasHeight = 4096;
        config.padding = 1;
        config.similarityThreshold = 0.125f;
        config.maxTextures = maxTextures;
        R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
        EXPECT_NE (encoder, nullptr);
        EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &pImage->image), R_PACK_OK);
        uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
        EXPECT_GT (size, 0u);
        std::vector<uint8_t> output (size);
        uint64_t written = 0;
        EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), output.size (), &written), R_PACK_OK);
        EXPECT_EQ (written, size);
        R_Pack_DeleteEncoder (encoder);
        return output;
    }
};

} // namespace

TEST_F (RPackStressTest, HandlesLargeSingleImage)
{
    R_Pack_OwnedImage image = MakeRandomImage ("large-image", 1024, 1024);
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesManySmallImages)
{
    constexpr uint32_t kImageCount = 100;
    constexpr uint32_t kImageSize = 32;
    
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.125f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);

    for (uint32_t i = 0; i < kImageCount; ++i)
    {
        char name[32];
        snprintf (name, sizeof (name), "image-%u", i);
        R_Pack_OwnedImage image = MakeRandomImage (name, kImageSize, kImageSize);
        EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_OK);
        R_Pack_DeleteOwnedImage (&image);
    }

    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    EXPECT_GT (size, 0u);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    EXPECT_EQ (written, size);
    R_Pack_DeleteEncoder (encoder);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesHighColorVariation)
{
    R_Pack_OwnedImage image = MakeGradientImage ("gradient", 512, 512);
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesSolidColorImages)
{
    constexpr uint32_t kSize = 256;
    std::vector<uint8_t> pixels (kSize * kSize * 4, 128); // All gray
    
    R_Pack_OwnedImage image = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), kSize, kSize, kSize * 4, "solid", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesCheckerboardPattern)
{
    constexpr uint32_t kSize = 128;
    std::vector<uint8_t> pixels (kSize * kSize * 4);
    
    for (uint32_t y = 0; y < kSize; ++y)
    {
        for (uint32_t x = 0; x < kSize; ++x)
        {
            uint32_t idx = (y * kSize + x) * 4;
            uint8_t value = ((x + y) % 2 == 0) ? 255 : 0;
            pixels[idx] = value;     // R
            pixels[idx + 1] = value; // G
            pixels[idx + 2] = value; // B
            pixels[idx + 3] = 255;   // A
        }
    }
    
    R_Pack_OwnedImage image = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), kSize, kSize, kSize * 4, "checkerboard", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesNonPowerOfTwoDimensions)
{
    R_Pack_OwnedImage image = MakeRandomImage ("npot", 123, 456);
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesVeryWideImage)
{
    R_Pack_OwnedImage image = MakeRandomImage ("wide", 2048, 32);
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesVeryTallImage)
{
    R_Pack_OwnedImage image = MakeRandomImage ("tall", 32, 2048);
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, PerformanceLargeImageEncoding)
{
    R_Pack_OwnedImage image = MakeRandomImage ("perf-test", 1024, 1024);
    
    auto start = std::chrono::high_resolution_clock::now ();
    
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.125f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_OK);
    
    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    auto end = std::chrono::high_resolution_clock::now ();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);
    
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&image);
    
    // Performance test should complete in reasonable time (< 5 seconds for 1MP image)
    EXPECT_LT (duration.count (), 5000);
}

TEST_F (RPackStressTest, PerformanceManySmallImagesEncoding)
{
    constexpr uint32_t kImageCount = 50;
    constexpr uint32_t kImageSize = 64;
    
    auto start = std::chrono::high_resolution_clock::now ();
    
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.125f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);

    for (uint32_t i = 0; i < kImageCount; ++i)
    {
        char name[32];
        snprintf (name, sizeof (name), "image-%u", i);
        R_Pack_OwnedImage image = MakeRandomImage (name, kImageSize, kImageSize);
        EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_OK);
        R_Pack_DeleteOwnedImage (&image);
    }

    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    auto end = std::chrono::high_resolution_clock::now ();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);
    
    R_Pack_DeleteEncoder (encoder);
    
    // Performance test should complete in reasonable time
    EXPECT_LT (duration.count (), 10000);
}

TEST_F (RPackStressTest, HandlesExtremeSimilarityThreshold)
{
    R_Pack_OwnedImage image = MakeGradientImage ("gradient", 256, 256);
    
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.0f; // No color merging
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_OK);
    
    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesVeryHighSimilarityThreshold)
{
    R_Pack_OwnedImage image = MakeGradientImage ("gradient", 256, 256);
    
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 1.0f; // Maximum color merging
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_OK);
    
    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}