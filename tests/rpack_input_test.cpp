extern "C" {
#include "rpack/rpack_input.h"
}

#include <gtest/gtest.h>
#include <array>

extern "C"
{
int R_CSTL_HeapInit (size_t heapSizeBytes);
void R_CSTL_HeapShutdown (void);
}

class RPackInputTest : public ::testing::Test
{
protected:
    void SetUp () override { ASSERT_EQ (R_CSTL_HeapInit (8 * 1024 * 1024), 0); }
    void TearDown () override { R_CSTL_HeapShutdown (); }
};

TEST_F (RPackInputTest, CopiesRawRgbaWithStride)
{
    std::array<uint8_t, 16> pixels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    R_Pack_OwnedImage output = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 1, 2, 8, "raw", &output), R_RPACK_OK);
    ASSERT_NE (output.pPixels, nullptr);
    EXPECT_EQ (output.image.width, 1u);
    EXPECT_EQ (output.image.height, 2u);
    EXPECT_EQ (output.image.stride, 8u);
    EXPECT_EQ (output.pPixels[15], 16);
    R_Pack_DeleteOwnedImage (&output);
}

TEST_F (RPackInputTest, RejectsShortRawBuffer)
{
    std::array<uint8_t, 3> pixels = {};
    R_Pack_OwnedImage output = {};
    EXPECT_EQ (R_Pack_InputFromRawRGBA (pixels.data (), pixels.size (), 1, 1, 4, "raw", &output),
               R_RPACK_ERROR_INVALID_ARGUMENT);
}

TEST_F (RPackInputTest, RejectsMalformedBase64)
{
    R_Pack_OwnedImage output = {};
    EXPECT_EQ (R_Pack_InputFromBase64 ("!!!!", 4, "bad", &output), R_RPACK_ERROR_INVALID_FORMAT);
    EXPECT_EQ (output.pPixels, nullptr);
}

TEST_F (RPackInputTest, RejectsInvalidEncodedBytes)
{
    const std::array<uint8_t, 4> bytes = {0, 1, 2, 3};
    R_Pack_OwnedImage output = {};
    EXPECT_EQ (R_Pack_InputFromBytes (bytes.data (), bytes.size (), "bad", &output), R_RPACK_ERROR_INVALID_FORMAT);
}
