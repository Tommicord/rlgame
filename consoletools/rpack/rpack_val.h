#pragma once

#include "rpack_platform.h"

#include <stdint.h>

struct r_pack_validation_report
{
        enum r_pack_error error;
        uint64_t          offset;
        uint32_t          textureIndex;
        uint64_t          pixelIndex;
};

R_PACK_API int
r_pack_validate_packed_data (const uint8_t* pData, uint64_t dataSize, struct r_pack_validation_report* pReport);

R_PACK_API int r_pack_validate_packed_file (const char* pPath, struct r_pack_validation_report* pReport);
