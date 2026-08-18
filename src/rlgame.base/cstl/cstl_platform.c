#include "rlgame.base/cstl/cstl_platform.h"

const char*
R_CSTL_ErrorToString (int error)
{
        switch (error)
        {
        case R_CSTL_OK:
                return "Success";
        case R_CSTL_ERROR_INVALID_ARGUMENT:
                return "Invalid argument";
        case R_CSTL_ERROR_OUT_OF_MEMORY:
                return "Out of memory";
        case R_CSTL_ERROR_INVALID_POINTER:
                return "Invalid pointer";
        case R_CSTL_ERROR_BUFFER_TOO_SMALL:
                return "Buffer too small";
        case R_CSTL_ERROR_INDEX_OUT_OF_BOUNDS:
                return "Index out of bounds";
        case R_CSTL_ERROR_HEAP_NOT_INITIALIZED:
                return "Heap not initialized";
        case R_CSTL_ERROR_HEAP_ALREADY_INITIALIZED:
                return "Heap already initialized";
        case R_CSTL_ERROR_HEAP_CORRUPTION:
                return "Heap corruption detected";
        case R_CSTL_ERROR_STRING_OPERATION:
                return "String operation error";
        case R_CSTL_ERROR_LEAK_DETECTED:
                return "Allocation leak detected";
        default:
                return "Unknown error";
        }
}