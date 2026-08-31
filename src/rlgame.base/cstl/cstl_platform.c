#include "rlgame.base/cstl/cstl_platform.h"

const char*
r_cstl_error_to_string (int error)
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
    case R_CSTL_ERROR_THREAD_CREATE_FAILED:
        return "Thread creation failed";
    case R_CSTL_ERROR_THREAD_JOIN_FAILED:
        return "Thread join failed";
    case R_CSTL_ERROR_MUTEX_INIT_FAILED:
        return "Mutex initialization failed";
    case R_CSTL_ERROR_MUTEX_DESTROY_FAILED:
        return "Mutex destruction failed";
    case R_CSTL_ERROR_MUTEX_LOCK_FAILED:
        return "Mutex lock failed";
    case R_CSTL_ERROR_MUTEX_UNLOCK_FAILED:
        return "Mutex unlock failed";
    case R_CSTL_ERROR_CONDITION_INIT_FAILED:
        return "Condition variable initialization failed";
    case R_CSTL_ERROR_CONDITION_DESTROY_FAILED:
        return "Condition variable destruction failed";
    case R_CSTL_ERROR_CONDITION_WAIT_FAILED:
        return "Condition variable wait failed";
    case R_CSTL_ERROR_CONDITION_SIGNAL_FAILED:
        return "Condition variable signal failed";
    default:
        return "Unknown error";
    }
}
