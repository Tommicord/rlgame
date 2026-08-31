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
    void SetUp () override { ASSERT_EQ (r_cstl_heap_init (kHeapSize), R_CSTL_OK); }
    void TearDown () override { r_cstl_heap_shutdown (); }

    static r_pack_owned_image MakeRandomImage (const char* pName, uint32_t width, uint32_t height)
    {
        std::vector<uint8_t> pixels (width * height * 4);
        std::mt19937 gen (std::random_device{} ());
        std::uniform_int_distribution<uint8_t> dist (0, 255);
        
        for (auto& pixel : pixels)
        {
            pixel = dist (gen);
        }
        
        r_pack_owned_image image = {};
        EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), width, height, width * 4, pName, &image), R_PACK_OK);
        return image;
    }

    static r_pack_owned_image MakeGradientImage (const char* pName, uint32_t width, uint32_t height)
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
        
        r_pack_owned_image image = {};
        EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), width, height, width * 4, pName, &image), R_PACK_OK);
        return image;
    }

    static std::vector<uint8_t> Encode (r_pack_owned_image* pImage, uint32_t maxTextures = 0)
    {
        r_pack_encoder_settings config = {};
        config.maxAtlasWidth = 4096;
        config.maxAtlasHeight = 4096;
        config.padding = 1;
        config.similarityThreshold = 0.125f;
        config.maxTextures = maxTextures;
        r_pack_encoder* encoder = r_pack_new_encoder (&config);
        EXPECT_NE (encoder, nullptr);
        EXPECT_EQ (r_pack_encoder_add_image (encoder, &pImage->image), R_PACK_OK);
        uint64_t size = r_pack_encoder_get_required_size (encoder);
        EXPECT_GT (size, 0u);
        std::vector<uint8_t> output (size);
        uint64_t written = 0;
        EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), output.size (), &written), R_PACK_OK);
        EXPECT_EQ (written, size);
        r_pack_delete_encoder (encoder);
        return output;
    }
};

} // namespace

TEST_F (RPackStressTest, HandlesLargeSingleImage)
{
    r_pack_owned_image image = MakeRandomImage ("large-image", 1024, 1024);
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesManySmallImages)
{
    constexpr uint32_t kImageCount = 100;
    constexpr uint32_t kImageSize = 32;
    
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.125f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);

    for (uint32_t i = 0; i < kImageCount; ++i)
    {
        char name[32];
        snprintf (name, sizeof (name), "image-%u", i);
        r_pack_owned_image image = MakeRandomImage (name, kImageSize, kImageSize);
        EXPECT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_OK);
        r_pack_delete_owned_image (&image);
    }

    uint64_t size = r_pack_encoder_get_required_size (encoder);
    EXPECT_GT (size, 0u);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    EXPECT_EQ (written, size);
    r_pack_delete_encoder (encoder);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesHighColorVariation)
{
    r_pack_owned_image image = MakeGradientImage ("gradient", 512, 512);
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesSolidColorImages)
{
    constexpr uint32_t kSize = 256;
    std::vector<uint8_t> pixels (kSize * kSize * 4, 128); // All gray
    
    r_pack_owned_image image = {};
    EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), kSize, kSize, kSize * 4, "solid", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
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
    
    r_pack_owned_image image = {};
    EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), kSize, kSize, kSize * 4, "checkerboard", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesNonPowerOfTwoDimensions)
{
    r_pack_owned_image image = MakeRandomImage ("npot", 123, 456);
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesVeryWideImage)
{
    r_pack_owned_image image = MakeRandomImage ("wide", 2048, 32);
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesVeryTallImage)
{
    r_pack_owned_image image = MakeRandomImage ("tall", 32, 2048);
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, PerformanceLargeImageEncoding)
{
    r_pack_owned_image image = MakeRandomImage ("perf-test", 1024, 1024);
    
    auto start = std::chrono::high_resolution_clock::now ();
    
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.125f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_OK);
    
    uint64_t size = r_pack_encoder_get_required_size (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    auto end = std::chrono::high_resolution_clock::now ();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);
    
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&image);
    
    // Performance test should complete in reasonable time (< 5 seconds for 1MP image)
    EXPECT_LT (duration.count (), 5000);
}

TEST_F (RPackStressTest, PerformanceManySmallImagesEncoding)
{
    constexpr uint32_t kImageCount = 50;
    constexpr uint32_t kImageSize = 64;
    
    auto start = std::chrono::high_resolution_clock::now ();
    
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.125f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);

    for (uint32_t i = 0; i < kImageCount; ++i)
    {
        char name[32];
        snprintf (name, sizeof (name), "image-%u", i);
        r_pack_owned_image image = MakeRandomImage (name, kImageSize, kImageSize);
        EXPECT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_OK);
        r_pack_delete_owned_image (&image);
    }

    uint64_t size = r_pack_encoder_get_required_size (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    auto end = std::chrono::high_resolution_clock::now ();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);
    
    r_pack_delete_encoder (encoder);
    
    // Performance test should complete in reasonable time
    EXPECT_LT (duration.count (), 10000);
}

TEST_F (RPackStressTest, HandlesExtremeSimilarityThreshold)
{
    r_pack_owned_image image = MakeGradientImage ("gradient", 256, 256);
    
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 0.0f; // No color merging
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_OK);
    
    uint64_t size = r_pack_encoder_get_required_size (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackStressTest, HandlesVeryHighSimilarityThreshold)
{
    r_pack_owned_image image = MakeGradientImage ("gradient", 256, 256);
    
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 4096;
    config.maxAtlasHeight = 4096;
    config.padding = 1;
    config.similarityThreshold = 1.0f; // Maximum color merging
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_OK);
    
    uint64_t size = r_pack_encoder_get_required_size (encoder);
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}