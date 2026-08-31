#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <vector>

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

constexpr size_t kHeapSize = 16 * 1024 * 1024;

class RPackTest : public ::testing::Test
{
protected:
    void SetUp () override { ASSERT_EQ (r_cstl_heap_init (kHeapSize), R_CSTL_OK); }
    void TearDown () override { r_cstl_heap_shutdown (); }

    static r_pack_owned_image MakeImage (const char* pName)
    {
        static std::array<uint8_t, 16> pixels = {
            255, 0, 0, 255,
            0, 255, 0, 255,
            0, 0, 255, 255,
            255, 255, 255, 255};
        r_pack_owned_image image = {};
        EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), 2, 2, 8, pName, &image), R_PACK_OK);
        return image;
    }

    static std::vector<uint8_t> Encode (r_pack_owned_image* pImage, uint32_t maxTextures = 0)
    {
        r_pack_encoder_settings config = {};
        config.maxAtlasWidth = 16;
        config.maxAtlasHeight = 16;
        config.padding = 1;
        config.similarityThreshold = 0.0f;
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

TEST_F (RPackTest, EncodesAndExhaustivelyValidatesOneImage)
{
    r_pack_owned_image image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
    EXPECT_EQ (r_pack_decoder_validate_file (packed.data (), packed.size ()), 1);
}

TEST_F (RPackTest, DecoderReturnsOriginalDimensionsAndPixels)
{
    r_pack_owned_image image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_decoder* decoder = r_pack_new_decoder (packed.data (), packed.size ());
    ASSERT_NE (decoder, nullptr);
    EXPECT_EQ (r_pack_decoder_get_texture_count (decoder), 1u);
    EXPECT_NE (r_pack_decoder_find_texture (decoder, "test-image"), nullptr);

    uint32_t width = 0, height = 0;
    ASSERT_EQ (r_pack_decoder_get_texture_dimensions (decoder, "test-image", &width, &height), R_PACK_OK);
    EXPECT_EQ (width, 2u);
    EXPECT_EQ (height, 2u);

    std::array<uint8_t, 16> decoded = {};
    uint64_t written = 0;
    ASSERT_EQ (r_pack_decoder_decode_texture (decoder, "test-image", decoded.data (), decoded.size (), &written), R_PACK_OK);
    EXPECT_EQ (written, decoded.size ());
    for (size_t i = 3; i < decoded.size (); i += 4) EXPECT_EQ (decoded[i], 255);
    r_pack_delete_decoder (decoder);
}

TEST_F (RPackTest, RejectsNullAndTruncatedPackedData)
{
    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (nullptr, 0, &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ (r_pack_validate_packed_data (nullptr, 0, nullptr), 0);

    r_pack_owned_image image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size () - 1, &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_FORMAT);
    EXPECT_EQ (r_pack_new_decoder (packed.data (), packed.size () - 1), nullptr);
}

TEST_F (RPackTest, RejectsInvalidHeaderMagicAndOffsets)
{
    r_pack_owned_image image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_header* header = reinterpret_cast<r_pack_header*> (packed.data ());
    r_pack_validation_report report = {};
    header->magicInt32 = 0;
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_FORMAT);

    header->magicInt32 = R_PACK_MAGIC;
    header->dataOffset = header->pixelIndexTableOffset;
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 0);
}

TEST_F (RPackTest, RejectsOverlappingTexturesAndDuplicateNames)
{
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 0;
    config.similarityThreshold = 0.0f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    r_pack_owned_image first = MakeImage ("same");
    r_pack_owned_image second = MakeImage ("same");
    ASSERT_EQ (r_pack_encoder_add_image (encoder, &first.image), R_PACK_OK);
    ASSERT_EQ (r_pack_encoder_add_image (encoder, &second.image), R_PACK_OK);
    uint64_t size = r_pack_encoder_get_required_size (encoder);
    std::vector<uint8_t> packed (size);
    ASSERT_EQ (r_pack_encoder_encode (encoder, packed.data (), packed.size (), nullptr), R_PACK_OK);
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&first);
    r_pack_delete_owned_image (&second);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 0);
}

TEST_F (RPackTest, ValidatorReportsCorruptPixelEntry)
{
    r_pack_owned_image image = MakeImage ("corruptible");
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);
    r_pack_header* header = reinterpret_cast<r_pack_header*> (packed.data ());
    auto* pixels = reinterpret_cast<r_pack_pixel_index_entry*> (packed.data () + header->pixelIndexTableOffset);
    pixels[0].colorIndex = static_cast<uint16_t> (header->colorTableSize);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_DATA);
    EXPECT_EQ (report.pixelIndex, 0u);
}

TEST_F (RPackTest, ValidatorRejectsOverlappingAtlasRectangles)
{
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 1;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    r_pack_owned_image first = MakeImage ("first");
    r_pack_owned_image second = MakeImage ("second");
    ASSERT_EQ (r_pack_encoder_add_image (encoder, &first.image), R_PACK_OK);
    ASSERT_EQ (r_pack_encoder_add_image (encoder, &second.image), R_PACK_OK);
    uint64_t size = r_pack_encoder_get_required_size (encoder);
    std::vector<uint8_t> packed (size);
    ASSERT_EQ (r_pack_encoder_encode (encoder, packed.data (), packed.size (), nullptr), R_PACK_OK);
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&first);
    r_pack_delete_owned_image (&second);
    auto* hashes = reinterpret_cast<r_pack_hash_entry*> (packed.data () + sizeof (r_pack_header));
    hashes[1].atlasOffsetX = hashes[0].atlasOffsetX;
    hashes[1].atlasOffsetY = hashes[0].atlasOffsetY;
    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 0);
}

TEST_F (RPackTest, EnforcesTextureLimitAndOutputBufferSize)
{
    r_pack_owned_image image = MakeImage ("limited");
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.maxTextures = 1;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    ASSERT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_OK);
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_ERROR_INVALID_DIMENSIONS);
    uint64_t size = r_pack_encoder_get_required_size (encoder);
    std::vector<uint8_t> output (size);
    EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), size - 1, nullptr), R_PACK_ERROR_BUFFER_TOO_SMALL);
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&image);
}

TEST_F (RPackTest, DecoderRejectsSmallOutputBufferAndMissingTexture)
{
    r_pack_owned_image image = MakeImage ("errors");
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);
    r_pack_decoder* decoder = r_pack_new_decoder (packed.data (), packed.size ());
    ASSERT_NE (decoder, nullptr);
    std::array<uint8_t, 3> output = {};
    EXPECT_EQ (r_pack_decoder_decode_texture (decoder, "errors", output.data (), output.size (), nullptr),
               R_PACK_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ (r_pack_decoder_get_texture_size (decoder, "missing"), 0u);
    EXPECT_EQ (r_pack_decoder_decode_texture (decoder, "missing", output.data (), output.size (), nullptr),
               R_PACK_ERROR_TEXTURE_NOT_FOUND);
    r_pack_delete_decoder (decoder);
}

TEST_F (RPackTest, InputModesValidateAndOwnBuffers)
{
    std::array<uint8_t, 16> pixels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    r_pack_owned_image raw = {};
    ASSERT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), 2, 2, 8, "raw", &raw), R_PACK_OK);
    pixels[0] = 99;
    EXPECT_EQ (raw.pPixels[0], 1);
    r_pack_delete_owned_image (&raw);
    EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), 3, 2, 2, 8, "bad", &raw), R_PACK_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ (r_pack_input_from_base64 ("!!!!", 4, "bad", &raw), R_PACK_ERROR_INVALID_FORMAT);
    EXPECT_EQ (r_pack_input_from_bytes (pixels.data (), pixels.size (), "bad", &raw), R_PACK_ERROR_INVALID_FORMAT);
}

TEST_F (RPackTest, ColorConversionProducesBoundedChannels)
{
    uint8_t y, exponent, u, v, r, g, b;
    r_pack_RGBAToYUV (255, 0, 0, &y, &exponent, &u, &v);
    r_pack_YUVToRGBA (y, exponent, u, v, &r, &g, &b);
    EXPECT_LE (r, 255);
    EXPECT_LE (g, 255);
    EXPECT_LE (b, 255);
    EXPECT_LT (r_pack_get_color_similarity (y, u, v, y, u, v), 0.0001f);
}

TEST_F (RPackTest, HandlesMinimalImageSize)
{
    std::array<uint8_t, 4> pixels = {255, 0, 0, 255};
    r_pack_owned_image image = {};
    EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), 1, 1, 4, "minimal", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackTest, HandlesMaximumTextureLimit)
{
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 1;
    config.similarityThreshold = 0.0f;
    config.maxTextures = 2;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    r_pack_owned_image image1 = MakeImage ("image1");
    r_pack_owned_image image2 = MakeImage ("image2");
    r_pack_owned_image image3 = MakeImage ("image3");
    
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image1.image), R_PACK_OK);
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image2.image), R_PACK_OK);
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image3.image), R_PACK_ERROR_INVALID_DIMENSIONS);
    
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&image1);
    r_pack_delete_owned_image (&image2);
    r_pack_delete_owned_image (&image3);
}

TEST_F (RPackTest, HandlesZeroPadding)
{
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 0;
    config.similarityThreshold = 0.0f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    r_pack_owned_image image = MakeImage ("no-padding");
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

TEST_F (RPackTest, HandlesLargePadding)
{
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 32;
    config.maxAtlasHeight = 32;
    config.padding = 10;
    config.similarityThreshold = 0.0f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    r_pack_owned_image image = MakeImage ("large-padding");
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

TEST_F (RPackTest, RejectsImageExceedingAtlasSize)
{
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 8;
    config.maxAtlasHeight = 8;
    config.padding = 0;
    config.similarityThreshold = 0.0f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    std::array<uint8_t, 16> pixels = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    r_pack_owned_image image = {};
    EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), 4, 4, 16, "too-large", &image), R_PACK_OK);
    
    EXPECT_EQ (r_pack_encoder_add_image (encoder, &image.image), R_PACK_ERROR_INVALID_DIMENSIONS);
    
    r_pack_delete_encoder (encoder);
    r_pack_delete_owned_image (&image);
}

TEST_F (RPackTest, HandlesSingleColorImage)
{
    std::array<uint8_t, 16> pixels = {128, 128, 128, 255, 128, 128, 128, 255, 128, 128, 128, 255, 128, 128, 128, 255};
    r_pack_owned_image image = {};
    EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), 2, 2, 8, "single-color", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
    
    // Verify color table is minimal for single color
    r_pack_decoder* decoder = r_pack_new_decoder (packed.data (), packed.size ());
    ASSERT_NE (decoder, nullptr);
    EXPECT_EQ (decoder->pHeader->colorTableSize, 1u);
    r_pack_delete_decoder (decoder);
}

TEST_F (RPackTest, HandlesBlackAndWhiteImage)
{
    std::array<uint8_t, 16> pixels = {0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255};
    r_pack_owned_image image = {};
    EXPECT_EQ (r_pack_input_from_rawRGBA (pixels.data (), pixels.size (), 2, 2, 8, "bw", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    r_pack_delete_owned_image (&image);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackTest, HashFunctionProducesUniqueValues)
{
    const char* strings[] = {"test1", "test2", "test3", "different", "strings"};
    uint64_t hashes[5];
    
    for (int i = 0; i < 5; ++i)
    {
        hashes[i] = r_pack_hash64_string (strings[i], 0);
    }
    
    // Check that all hashes are different
    for (int i = 0; i < 5; ++i)
    {
        for (int j = i + 1; j < 5; ++j)
        {
            EXPECT_NE (hashes[i], hashes[j]) << "Hash collision between " << strings[i] << " and " << strings[j];
        }
    }
}

TEST_F (RPackTest, HashFunctionWithSeedProducesDifferentValues)
{
    const char* testStr = "test";
    uint64_t hash1 = r_pack_hash64_string (testStr, 0);
    uint64_t hash2 = r_pack_hash64_string (testStr, 42);
    
    EXPECT_NE (hash1, hash2);
}

TEST_F (RPackTest, ColorSimilarityIsSymmetric)
{
    uint8_t y1 = 128, u1 = 8, v1 = 8;
    uint8_t y2 = 64, u2 = 4, v2 = 4;
    
    float sim1 = r_pack_get_color_similarity (y1, u1, v1, y2, u2, v2);
    float sim2 = r_pack_get_color_similarity (y2, u2, v2, y1, u1, v1);
    
    EXPECT_FLOAT_EQ (sim1, sim2);
}

TEST_F (RPackTest, ColorSimilarityZeroForIdenticalColors)
{
    uint8_t y = 128, u = 8, v = 8;
    
    float sim = r_pack_get_color_similarity (y, u, v, y, u, v);
    
    EXPECT_FLOAT_EQ (sim, 0.0f);
}

TEST_F (RPackTest, HandlesEmptyEncoder)
{
    r_pack_encoder_settings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 1;
    config.similarityThreshold = 0.0f;
    r_pack_encoder* encoder = r_pack_new_encoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    uint64_t size = r_pack_encoder_get_required_size (encoder);
    EXPECT_GT (size, 0u);
    
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (r_pack_encoder_encode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    r_pack_delete_encoder (encoder);

    r_pack_validation_report report = {};
    EXPECT_EQ (r_pack_validate_packed_data (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
    
    r_pack_decoder* decoder = r_pack_new_decoder (output.data (), output.size ());
    ASSERT_NE (decoder, nullptr);
    EXPECT_EQ (r_pack_decoder_get_texture_count (decoder), 0u);
    r_pack_delete_decoder (decoder);
}
