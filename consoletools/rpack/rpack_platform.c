#include "rpack/rpack_platform.h"

const char*
R_Pack_ErrorToString (enum R_Pack_Error error)
{
    switch (error)
    {
    case R_PACK_OK:
        return "Success";
    case R_PACK_ERROR_FAILED:
        return "General failure";
    case R_PACK_ERROR_OUT_OF_MEMORY:
        return "Out of memory";
    case R_PACK_ERROR_INVALID_ARGUMENT:
        return "Invalid argument";
    case R_PACK_ERROR_NULL_POINTER:
        return "Null pointer";
    case R_PACK_ERROR_INVALID_FORMAT:
        return "Invalid format";
    case R_PACK_ERROR_INVALID_MAGIC:
        return "Invalid magic number";
    case R_PACK_ERROR_VERSION_MISMATCH:
        return "Version mismatch";
    case R_PACK_ERROR_TEXTURE_NOT_FOUND:
        return "Texture not found";
    case R_PACK_ERROR_INVALID_DIMENSIONS:
        return "Invalid dimensions";
    case R_PACK_ERROR_COMPRESSION_FAILED:
        return "Compression failed";
    case R_PACK_ERROR_DECOMPRESSION_FAILED:
        return "Decompression failed";
    case R_PACK_ERROR_INVALID_DATA:
        return "Invalid data";
    case R_PACK_ERROR_BUFFER_TOO_SMALL:
        return "Buffer too small";
    case R_PACK_ERROR_UNSUPPORTED_FORMAT:
        return "Unsupported image format or JPEG mode";
    case R_PACK_ERROR_UNKNOWN:
        return "Unknown error";
    default:
        return "Invalid error code";
    }
}
