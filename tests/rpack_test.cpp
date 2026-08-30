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
    void SetUp () override { ASSERT_EQ (R_CSTL_HeapInit (kHeapSize), R_CSTL_OK); }
    void TearDown () override { R_CSTL_HeapShutdown (); }

    static R_Pack_OwnedImage MakeImage (const char* pName)
    {
        static std::array<uint8_t, 16> pixels = {
            255, 0, 0, 255,
            0, 255, 0, 255,
            0, 0, 255, 255,
            255, 255, 255, 255};
        R_Pack_OwnedImage image = {};
        EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 2, 2, 8, pName, &image), R_PACK_OK);
        return image;
    }

    static std::vector<uint8_t> Encode (R_Pack_OwnedImage* pImage, uint32_t maxTextures = 0)
    {
        R_Pack_EncoderSettings config = {};
        config.maxAtlasWidth = 16;
        config.maxAtlasHeight = 16;
        config.padding = 1;
        config.similarityThreshold = 0.0f;
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

TEST_F (RPackTest, EncodesAndExhaustivelyValidatesOneImage)
{
    R_Pack_OwnedImage image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
    EXPECT_EQ (R_Pack_DecoderValidateFile (packed.data (), packed.size ()), 1);
}

TEST_F (RPackTest, DecoderReturnsOriginalDimensionsAndPixels)
{
    R_Pack_OwnedImage image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_Decoder* decoder = R_Pack_NewDecoder (packed.data (), packed.size ());
    ASSERT_NE (decoder, nullptr);
    EXPECT_EQ (R_Pack_DecoderGetTextureCount (decoder), 1u);
    EXPECT_NE (R_Pack_DecoderFindTexture (decoder, "test-image"), nullptr);

    uint32_t width = 0, height = 0;
    ASSERT_EQ (R_Pack_DecoderGetTextureDimensions (decoder, "test-image", &width, &height), R_PACK_OK);
    EXPECT_EQ (width, 2u);
    EXPECT_EQ (height, 2u);

    std::array<uint8_t, 16> decoded = {};
    uint64_t written = 0;
    ASSERT_EQ (R_Pack_DecoderDecodeTexture (decoder, "test-image", decoded.data (), decoded.size (), &written), R_PACK_OK);
    EXPECT_EQ (written, decoded.size ());
    for (size_t i = 3; i < decoded.size (); i += 4) EXPECT_EQ (decoded[i], 255);
    R_Pack_DeleteDecoder (decoder);
}

TEST_F (RPackTest, RejectsNullAndTruncatedPackedData)
{
    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (nullptr, 0, &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ (R_Pack_ValidatePackedData (nullptr, 0, nullptr), 0);

    R_Pack_OwnedImage image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size () - 1, &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_FORMAT);
    EXPECT_EQ (R_Pack_NewDecoder (packed.data (), packed.size () - 1), nullptr);
}

TEST_F (RPackTest, RejectsInvalidHeaderMagicAndOffsets)
{
    R_Pack_OwnedImage image = MakeImage ("test-image");
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_Header* header = reinterpret_cast<R_Pack_Header*> (packed.data ());
    R_Pack_ValidationReport report = {};
    header->magicInt32 = 0;
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_FORMAT);

    header->magicInt32 = R_PACK_MAGIC;
    header->dataOffset = header->pixelIndexTableOffset;
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 0);
}

TEST_F (RPackTest, RejectsOverlappingTexturesAndDuplicateNames)
{
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 0;
    config.similarityThreshold = 0.0f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    R_Pack_OwnedImage first = MakeImage ("same");
    R_Pack_OwnedImage second = MakeImage ("same");
    ASSERT_EQ (R_Pack_EncoderAddImage (encoder, &first.image), R_PACK_OK);
    ASSERT_EQ (R_Pack_EncoderAddImage (encoder, &second.image), R_PACK_OK);
    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    std::vector<uint8_t> packed (size);
    ASSERT_EQ (R_Pack_EncoderEncode (encoder, packed.data (), packed.size (), nullptr), R_PACK_OK);
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&first);
    R_Pack_DeleteOwnedImage (&second);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 0);
}

TEST_F (RPackTest, ValidatorReportsCorruptPixelEntry)
{
    R_Pack_OwnedImage image = MakeImage ("corruptible");
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);
    R_Pack_Header* header = reinterpret_cast<R_Pack_Header*> (packed.data ());
    auto* pixels = reinterpret_cast<R_Pack_PixelIndexEntry*> (packed.data () + header->pixelIndexTableOffset);
    pixels[0].colorIndex = static_cast<uint16_t> (header->colorTableSize);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 0);
    EXPECT_EQ (report.error, R_PACK_ERROR_INVALID_DATA);
    EXPECT_EQ (report.pixelIndex, 0u);
}

TEST_F (RPackTest, ValidatorRejectsOverlappingAtlasRectangles)
{
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 1;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    R_Pack_OwnedImage first = MakeImage ("first");
    R_Pack_OwnedImage second = MakeImage ("second");
    ASSERT_EQ (R_Pack_EncoderAddImage (encoder, &first.image), R_PACK_OK);
    ASSERT_EQ (R_Pack_EncoderAddImage (encoder, &second.image), R_PACK_OK);
    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    std::vector<uint8_t> packed (size);
    ASSERT_EQ (R_Pack_EncoderEncode (encoder, packed.data (), packed.size (), nullptr), R_PACK_OK);
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&first);
    R_Pack_DeleteOwnedImage (&second);
    auto* hashes = reinterpret_cast<R_Pack_HashEntry*> (packed.data () + sizeof (R_Pack_Header));
    hashes[1].atlasOffsetX = hashes[0].atlasOffsetX;
    hashes[1].atlasOffsetY = hashes[0].atlasOffsetY;
    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 0);
}

TEST_F (RPackTest, EnforcesTextureLimitAndOutputBufferSize)
{
    R_Pack_OwnedImage image = MakeImage ("limited");
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.maxTextures = 1;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    ASSERT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_OK);
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_ERROR_INVALID_DIMENSIONS);
    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    std::vector<uint8_t> output (size);
    EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), size - 1, nullptr), R_PACK_ERROR_BUFFER_TOO_SMALL);
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&image);
}

TEST_F (RPackTest, DecoderRejectsSmallOutputBufferAndMissingTexture)
{
    R_Pack_OwnedImage image = MakeImage ("errors");
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);
    R_Pack_Decoder* decoder = R_Pack_NewDecoder (packed.data (), packed.size ());
    ASSERT_NE (decoder, nullptr);
    std::array<uint8_t, 3> output = {};
    EXPECT_EQ (R_Pack_DecoderDecodeTexture (decoder, "errors", output.data (), output.size (), nullptr),
               R_PACK_ERROR_BUFFER_TOO_SMALL);
    EXPECT_EQ (R_Pack_DecoderGetTextureSize (decoder, "missing"), 0u);
    EXPECT_EQ (R_Pack_DecoderDecodeTexture (decoder, "missing", output.data (), output.size (), nullptr),
               R_PACK_ERROR_TEXTURE_NOT_FOUND);
    R_Pack_DeleteDecoder (decoder);
}

TEST_F (RPackTest, InputModesValidateAndOwnBuffers)
{
    std::array<uint8_t, 16> pixels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    R_Pack_OwnedImage raw = {};
    ASSERT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 2, 2, 8, "raw", &raw), R_PACK_OK);
    pixels[0] = 99;
    EXPECT_EQ (raw.pPixels[0], 1);
    R_Pack_DeleteOwnedImage (&raw);
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), 3, 2, 2, 8, "bad", &raw), R_PACK_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ (R_Pack_InputFromBase64 ("!!!!", 4, "bad", &raw), R_PACK_ERROR_INVALID_FORMAT);
    EXPECT_EQ (R_Pack_InputFromBytes (pixels.data (), pixels.size (), "bad", &raw), R_PACK_ERROR_INVALID_FORMAT);
}

TEST_F (RPackTest, ColorConversionProducesBoundedChannels)
{
    uint8_t y, exponent, u, v, r, g, b;
    R_Pack_RGBAToYUV (255, 0, 0, &y, &exponent, &u, &v);
    R_Pack_YUVToRGBA (y, exponent, u, v, &r, &g, &b);
    EXPECT_LE (r, 255);
    EXPECT_LE (g, 255);
    EXPECT_LE (b, 255);
    EXPECT_LT (R_Pack_GetColorSimilarity (y, u, v, y, u, v), 0.0001f);
}

TEST_F (RPackTest, HandlesMinimalImageSize)
{
    std::array<uint8_t, 4> pixels = {255, 0, 0, 255};
    R_Pack_OwnedImage image = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 1, 1, 4, "minimal", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackTest, HandlesMaximumTextureLimit)
{
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 1;
    config.similarityThreshold = 0.0f;
    config.maxTextures = 2;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    R_Pack_OwnedImage image1 = MakeImage ("image1");
    R_Pack_OwnedImage image2 = MakeImage ("image2");
    R_Pack_OwnedImage image3 = MakeImage ("image3");
    
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image1.image), R_PACK_OK);
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image2.image), R_PACK_OK);
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image3.image), R_PACK_ERROR_INVALID_DIMENSIONS);
    
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&image1);
    R_Pack_DeleteOwnedImage (&image2);
    R_Pack_DeleteOwnedImage (&image3);
}

TEST_F (RPackTest, HandlesZeroPadding)
{
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 0;
    config.similarityThreshold = 0.0f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    R_Pack_OwnedImage image = MakeImage ("no-padding");
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

TEST_F (RPackTest, HandlesLargePadding)
{
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 32;
    config.maxAtlasHeight = 32;
    config.padding = 10;
    config.similarityThreshold = 0.0f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    R_Pack_OwnedImage image = MakeImage ("large-padding");
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

TEST_F (RPackTest, RejectsImageExceedingAtlasSize)
{
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 8;
    config.maxAtlasHeight = 8;
    config.padding = 0;
    config.similarityThreshold = 0.0f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    std::array<uint8_t, 16> pixels = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    R_Pack_OwnedImage image = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 4, 4, 16, "too-large", &image), R_PACK_OK);
    
    EXPECT_EQ (R_Pack_EncoderAddImage (encoder, &image.image), R_PACK_ERROR_INVALID_DIMENSIONS);
    
    R_Pack_DeleteEncoder (encoder);
    R_Pack_DeleteOwnedImage (&image);
}

TEST_F (RPackTest, HandlesSingleColorImage)
{
    std::array<uint8_t, 16> pixels = {128, 128, 128, 255, 128, 128, 128, 255, 128, 128, 128, 255, 128, 128, 128, 255};
    R_Pack_OwnedImage image = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 2, 2, 8, "single-color", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
    
    // Verify color table is minimal for single color
    R_Pack_Decoder* decoder = R_Pack_NewDecoder (packed.data (), packed.size ());
    ASSERT_NE (decoder, nullptr);
    EXPECT_EQ (decoder->pHeader->colorTableSize, 1u);
    R_Pack_DeleteDecoder (decoder);
}

TEST_F (RPackTest, HandlesBlackAndWhiteImage)
{
    std::array<uint8_t, 16> pixels = {0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255};
    R_Pack_OwnedImage image = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 2, 2, 8, "bw", &image), R_PACK_OK);
    
    std::vector<uint8_t> packed = Encode (&image);
    R_Pack_DeleteOwnedImage (&image);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (packed.data (), packed.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
}

TEST_F (RPackTest, HashFunctionProducesUniqueValues)
{
    const char* strings[] = {"test1", "test2", "test3", "different", "strings"};
    uint64_t hashes[5];
    
    for (int i = 0; i < 5; ++i)
    {
        hashes[i] = R_Pack_Hash64String (strings[i], 0);
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
    uint64_t hash1 = R_Pack_Hash64String (testStr, 0);
    uint64_t hash2 = R_Pack_Hash64String (testStr, 42);
    
    EXPECT_NE (hash1, hash2);
}

TEST_F (RPackTest, ColorSimilarityIsSymmetric)
{
    uint8_t y1 = 128, u1 = 8, v1 = 8;
    uint8_t y2 = 64, u2 = 4, v2 = 4;
    
    float sim1 = R_Pack_GetColorSimilarity (y1, u1, v1, y2, u2, v2);
    float sim2 = R_Pack_GetColorSimilarity (y2, u2, v2, y1, u1, v1);
    
    EXPECT_FLOAT_EQ (sim1, sim2);
}

TEST_F (RPackTest, ColorSimilarityZeroForIdenticalColors)
{
    uint8_t y = 128, u = 8, v = 8;
    
    float sim = R_Pack_GetColorSimilarity (y, u, v, y, u, v);
    
    EXPECT_FLOAT_EQ (sim, 0.0f);
}

TEST_F (RPackTest, HandlesEmptyEncoder)
{
    R_Pack_EncoderSettings config = {};
    config.maxAtlasWidth = 16;
    config.maxAtlasHeight = 16;
    config.padding = 1;
    config.similarityThreshold = 0.0f;
    R_Pack_Encoder* encoder = R_Pack_NewEncoder (&config);
    ASSERT_NE (encoder, nullptr);
    
    uint64_t size = R_Pack_EncoderGetRequiredSize (encoder);
    EXPECT_GT (size, 0u);
    
    std::vector<uint8_t> output (size);
    uint64_t written = 0;
    EXPECT_EQ (R_Pack_EncoderEncode (encoder, output.data (), output.size (), &written), R_PACK_OK);
    
    R_Pack_DeleteEncoder (encoder);

    R_Pack_ValidationReport report = {};
    EXPECT_EQ (R_Pack_ValidatePackedData (output.data (), output.size (), &report), 1);
    EXPECT_EQ (report.error, R_PACK_OK);
    
    R_Pack_Decoder* decoder = R_Pack_NewDecoder (output.data (), output.size ());
    ASSERT_NE (decoder, nullptr);
    EXPECT_EQ (R_Pack_DecoderGetTextureCount (decoder), 0u);
    R_Pack_DeleteDecoder (decoder);
}
