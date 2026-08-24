#pragma once

#ifdef R_CSTL_BUILDING_DLL
#if defined(_MSC_VER)
#define R_CSTL_API __declspec (dllexport)
#else
#define R_CSTL_API
#endif
#else
#if defined(_MSC_VER)
#define R_CSTL_API __declspec (dllimport)
#else
#define R_CSTL_API
#endif
#endif

#if defined(_WIN32) || defined(_WIN64)
#define R_CSTL_PLATFORM_WINDOWS
#elif defined(__linux__)
#define R_CSTL_PLATFORM_LINUX
#endif

#if defined(_MSC_VER)
#define R_CSTL_HEAP_COMPILER_BARRIER() MemoryBarrier ()
#elif defined(__GNUC__) || defined(__clang__)
#define R_CSTL_HEAP_COMPILER_BARRIER() asm volatile ("mfence" ::: "memory");
#else
#define R_CSTL_HEAP_COMPILER_BARRIER() ((void)0)
#endif

#if defined(_MSC_VER)
#define R_CSTL_API_ATTR __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define R_CSTL_API_ATTR inline __attribute__ ((always_inline))
#else
#define R_CSTL_API_ATTR inline
#endif

#if defined(_MSC_VER)
#define R_CSTL_LIKELY(x)   (x)
#define R_CSTL_UNLIKELY(x) (x)
#elif defined(__GNUC__) || defined(__clang__)
#define R_CSTL_LIKELY(x)   __builtin_expect ((x), 1)
#define R_CSTL_UNLIKELY(x) __builtin_expect ((x), 0)
#else
#define R_CSTL_LIKELY(x)   (x)
#define R_CSTL_UNLIKELY(x) (x)
#endif

#if defined(_MSC_VER)
#define R_CSTL_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
#define R_CSTL_RESTRICT __restrict
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define R_CSTL_RESTRICT restrict
#else
#define R_CSTL_RESTRICT
#endif

#if defined(R_DEVMODE)
#define R_LOG
#define R_CSTL_DEBUG
#endif

enum R_CSTL_Error
{
        R_CSTL_OK = 0,
        R_CSTL_ERROR_INVALID_ARGUMENT = -1,
        R_CSTL_ERROR_OUT_OF_MEMORY = -2,
        R_CSTL_ERROR_INVALID_POINTER = -3,
        R_CSTL_ERROR_BUFFER_TOO_SMALL = -4,
        R_CSTL_ERROR_INDEX_OUT_OF_BOUNDS = -5,
        R_CSTL_ERROR_HEAP_NOT_INITIALIZED = -6,
        R_CSTL_ERROR_HEAP_ALREADY_INITIALIZED = -7,
        R_CSTL_ERROR_HEAP_CORRUPTION = -8,
        R_CSTL_ERROR_STRING_OPERATION = -9,
        R_CSTL_ERROR_LEAK_DETECTED = -10,
        R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED = -11,
        R_CSTL_ERROR_EXECUTABLE_TYPE_NOT_SUPPORTED = -12,
        R_CSTL_ERROR_SYMBOL_NOT_FOUND = -13,
        R_CSTL_ERROR_THREAD_CREATE_FAILED = -14,
        R_CSTL_ERROR_THREAD_JOIN_FAILED = -15,
        R_CSTL_ERROR_MUTEX_INIT_FAILED = -16,
        R_CSTL_ERROR_MUTEX_DESTROY_FAILED = -17,
        R_CSTL_ERROR_MUTEX_LOCK_FAILED = -18,
        R_CSTL_ERROR_MUTEX_UNLOCK_FAILED = -19,
        R_CSTL_ERROR_CONDITION_INIT_FAILED = -20,
        R_CSTL_ERROR_CONDITION_DESTROY_FAILED = -21,
        R_CSTL_ERROR_CONDITION_WAIT_FAILED = -22,
        R_CSTL_ERROR_CONDITION_SIGNAL_FAILED = -23,
        R_CSTL_ERROR_UNKNOWN = -99
};

/**
 * @brief Get human-readable error message for an error code
 * @param error The error code
 * @return Static string describing the error, or "Unknown error" if not recognized
 */
const char* R_CSTL_ErrorToString (int error);
