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

// Branch prediction hints
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

#define R_CSTL_LOG_DEVMODE R_DEVMODE

#define R_CSTL_OK                                  0
#define R_CSTL_ERROR_INVALID_ARGUMENT              -1
#define R_CSTL_ERROR_OUT_OF_MEMORY                 -2
#define R_CSTL_ERROR_INVALID_POINTER               -3
#define R_CSTL_ERROR_BUFFER_TOO_SMALL              -4
#define R_CSTL_ERROR_INDEX_OUT_OF_BOUNDS           -5
#define R_CSTL_ERROR_HEAP_NOT_INITIALIZED          -6
#define R_CSTL_ERROR_HEAP_ALREADY_INITIALIZED      -7
#define R_CSTL_ERROR_HEAP_CORRUPTION               -8
#define R_CSTL_ERROR_STRING_OPERATION              -9
#define R_CSTL_ERROR_LEAK_DETECTED                 -10
#define R_CSTL_ERROR_ARCHITECTURE_NOT_SUPPORTED    -11
#define R_CSTL_ERROR_EXECUTABLE_TYPE_NOT_SUPPORTED -12
#define R_CSTL_ERROR_SYMBOL_NOT_FOUND              -13
#define R_CSTL_ERROR_UNKNOWN                       -99

/**
 * @brief Get human-readable error message for an error code
 * @param error The error code
 * @return Static string describing the error, or "Unknown error" if not recognized
 */
const char* R_CSTL_ErrorToString (int error);
