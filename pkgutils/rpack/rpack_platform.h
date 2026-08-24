#pragma once

#if defined(_WIN32)
#ifdef R_RPACK_BUILDING_DLL
#define R_RPACK_API __declspec (dllexport)
#else
#define R_RPACK_API __declspec (dllimport)
#endif
#else
#define R_RPACK_API
#endif

#if defined(R_DEVMODE)
#define R_RPACK_DEBUG
#endif

#if defined(R_RPACK_DEBUG)
#include <assert.h>
#define R_RPACK_ASSERT(condition) assert (condition)
#else
#define R_RPACK_ASSERT(condition) ((void)0)
#endif

/**
 * @brief R_RPACK wrapper error codes
 */
enum R_PackError
{
        R_RPACK_OK = 0,
        R_RPACK_ERROR_FAILED = -1,
        R_RPACK_ERROR_OUT_OF_MEMORY = -2,
        R_RPACK_ERROR_INVALID_ARGUMENT = -3,
        R_RPACK_ERROR_NULL_POINTER = -4,
        R_RPACK_ERROR_INVALID_FORMAT = -5,
        R_RPACK_ERROR_INVALID_MAGIC = -6,
        R_RPACK_ERROR_VERSION_MISMATCH = -7,
        R_RPACK_ERROR_TEXTURE_NOT_FOUND = -8,
        R_RPACK_ERROR_INVALID_DIMENSIONS = -9,
        R_RPACK_ERROR_COMPRESSION_FAILED = -10,
        R_RPACK_ERROR_DECOMPRESSION_FAILED = -11,
        R_RPACK_ERROR_INVALID_DATA = -12,
        R_RPACK_ERROR_BUFFER_TOO_SMALL = -13,
        R_RPACK_ERROR_UNKNOWN = -99
};

R_RPACK_API const char* R_PackErrorToString (enum R_PackError error);
