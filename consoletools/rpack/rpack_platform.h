#pragma once

#if defined(_WIN32)
#ifdef R_PACK_BUILDING_DLL
#define R_PACK_API __declspec (dllexport)
#else
#define R_PACK_API __declspec (dllimport)
#endif
#else
#define R_PACK_API
#endif

#if defined(R_DEVMODE)
#define R_PACK_DEBUG
#endif

#if defined(R_PACK_DEBUG)
#include <assert.h>
#define R_PACK_ASSERT(condition) assert (condition)
#else
#define R_PACK_ASSERT(condition) ((void)0)
#endif

/**
 * @brief R_RPACK wrapper error codes
 */
enum R_Pack_Error
{
    R_PACK_OK = 0,
    R_PACK_ERROR_FAILED = -1,
    R_PACK_ERROR_OUT_OF_MEMORY = -2,
    R_PACK_ERROR_INVALID_ARGUMENT = -3,
    R_PACK_ERROR_NULL_POINTER = -4,
    R_PACK_ERROR_INVALID_FORMAT = -5,
    R_PACK_ERROR_INVALID_MAGIC = -6,
    R_PACK_ERROR_VERSION_MISMATCH = -7,
    R_PACK_ERROR_TEXTURE_NOT_FOUND = -8,
    R_PACK_ERROR_INVALID_DIMENSIONS = -9,
    R_PACK_ERROR_COMPRESSION_FAILED = -10,
    R_PACK_ERROR_DECOMPRESSION_FAILED = -11,
    R_PACK_ERROR_INVALID_DATA = -12,
    R_PACK_ERROR_BUFFER_TOO_SMALL = -13,
    R_PACK_ERROR_UNSUPPORTED_FORMAT = -14,
    R_PACK_ERROR_UNKNOWN = -99
};

R_PACK_API const char* R_Pack_ErrorToString (enum R_Pack_Error error);
