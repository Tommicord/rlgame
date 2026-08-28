#pragma once

#include "rpack_platform.h"

#include <stdint.h>

struct R_Pack_ValidationReport
{
    enum R_Pack_Error error;
    uint64_t offset;
    uint32_t textureIndex;
    uint64_t pixelIndex;
};

R_RPACK_API int R_Pack_ValidatePackedData (
    const uint8_t* pData,
    uint64_t dataSize,
    struct R_Pack_ValidationReport* pReport);

R_RPACK_API int R_Pack_ValidatePackedFile (
    const char* pPath,
    struct R_Pack_ValidationReport* pReport);
