#pragma once

#include "rlgame.base/cstl/cstl_platform.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @file cstl_trace.h
 * @brief Function tracing and performance profiling utilities
 *
 * This module provides macros for automatic function entry/exit tracing
 * and performance timing. Useful for debugging complex code paths and
 * identifying performance bottlenecks.
 */

/**
 * @brief Configuration for trace logging
 */
struct R_CSTL_TraceConfig
{
                uint8_t enableFunctionEntryExit : 1; /**< Enable automatic function entry/exit logging */
                uint8_t enablePerformanceTiming : 1; /**< Enable performance timing for traced functions */
                uint8_t minDurationMicroseconds : 1; /**< Minimum duration in microseconds to log performance
                                                        timing */
                uint8_t enableCallDepthIndentation : 1; /**< Enable call depth indentation in trace logs */
};

/**
 * @brief Logs environment information logging
 *
 * This function logs detailed environment
 * information including OS version, memory usage, processor count, and other
 * system-specific details useful for debugging.
 */
R_CSTL_API void R_CSTL_TraceLogEnvironmentInfo (void);

/**
 * @brief Get current trace configuration
 * @return Current trace configuration
 */
R_CSTL_API const struct R_CSTL_TraceConfig* R_CSTL_TraceGetConfig (void);

/**
 * @brief Set minimum duration for performance logging
 * @param microseconds Minimum duration in microseconds
 */
R_CSTL_API void R_CSTL_TraceSetMinDuration (uint64_t microseconds);

/**
 * @brief Get high-resolution timestamp in microseconds
 * @return Timestamp in microseconds
 */
R_CSTL_API uint64_t R_CSTL_TraceGetTimestamp (void);

/**
 * @brief Log function entry
 * @param functionName Function name
 * @param fileName Source file name
 * @param lineNumber Line number
 */
R_CSTL_API void
R_CSTL_TraceFunctionEntry (const char* functionName, const char* fileName, uint32_t lineNumber);

/**
 * @brief Log function exit
 * @param functionName Function name
 * @param fileName Source file name
 * @param lineNumber Line number
 * @param durationMicroseconds Duration in microseconds (if timing enabled)
 */
R_CSTL_API void R_CSTL_TraceFunctionExit (
    const char* functionName,
    const char* fileName,
    uint32_t    lineNumber,
    uint64_t    durationMicroseconds);

#if defined(R_CSTL_TRACE_ENABLED)

#if defined(__GNUC__) || defined(__clang__)
/* GCC/Clang: Use __attribute__((cleanup)) */
static inline void
_CSTL_TraceScopeCleanup (uint64_t* pTraceStart)
{
        uint64_t duration = R_CSTL_TraceGetTimestamp () - *pTraceStart;
        R_CSTL_TraceFunctionExit (__FUNCTION__, __FILE__, __LINE__, duration);
}

#define R_CSTL_TRACE_SCOPE()                                                                                 \
        uint64_t _trace_start __attribute__ ((cleanup (_CSTL_TraceScopeCleanup)))                            \
        = R_CSTL_TraceGetTimestamp ();                                                                       \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__)

#define R_CSTL_TRACE_SCOPE_CTX(fmt, ...)                                                                     \
        uint64_t _trace_start __attribute__ ((cleanup (_CSTL_TraceScopeCleanup)))                            \
        = R_CSTL_TraceGetTimestamp ();                                                                       \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__);                                        \
        R_CSTL_LOG_TRACE (" Context: " fmt, __VA_ARGS__)

#elif defined(_MSC_VER)
/* MSVC: Manual cleanup required */
#define R_CSTL_TRACE_SCOPE()                                                                                 \
        uint64_t _trace_start = R_CSTL_TraceGetTimestamp ();                                                 \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__);                                        \
        __pragma (warning (push)) __pragma (warning (disable : 4100)) struct _CSTL_TraceScopeGuard           \
        {                                                                                                    \
                        uint64_t    start;                                                                   \
                        const char* func;                                                                    \
                        const char* file;                                                                    \
                        uint32_t    line;                                                                    \
        } _trace_guard = {_trace_start, __FUNCTION__, __FILE__, __LINE__};                                   \
        __pragma (warning (pop)) if (0)                                                                      \
        {                                                                                                    \
                uint64_t _trace_duration = R_CSTL_TraceGetTimestamp () - _trace_guard.start;                 \
                R_CSTL_TraceFunctionExit (                                                                   \
                    _trace_guard.func,                                                                       \
                    _trace_guard.file,                                                                       \
                    _trace_guard.line,                                                                       \
                    _trace_duration);                                                                        \
        }

#define R_CSTL_TRACE_SCOPE_CTX(fmt, ...)                                                                     \
        uint64_t _trace_start = R_CSTL_TraceGetTimestamp ();                                                 \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__);                                        \
        R_CSTL_LOG_TRACE (" Context: " fmt, __VA_ARGS__);                                                    \
        __pragma (warning (push)) __pragma (warning (disable : 4100)) struct _CSTL_TraceScopeGuard           \
        {                                                                                                    \
                        uint64_t    start;                                                                   \
                        const char* func;                                                                    \
                        const char* file;                                                                    \
                        uint32_t    line;                                                                    \
        } _trace_guard = {_trace_start, __FUNCTION__, __FILE__, __LINE__};                                   \
        __pragma (warning (pop)) if (0)                                                                      \
        {                                                                                                    \
                uint64_t _trace_duration = R_CSTL_TraceGetTimestamp () - _trace_guard.start;                 \
                R_CSTL_TraceFunctionExit (                                                                   \
                    _trace_guard.func,                                                                       \
                    _trace_guard.file,                                                                       \
                    _trace_guard.line,                                                                       \
                    _trace_duration);                                                                        \
        }

#define R_CSTL_TRACE_SCOPE_EXIT()                                                                            \
        do                                                                                                   \
        {                                                                                                    \
                uint64_t _trace_duration = R_CSTL_TraceGetTimestamp () - _trace_start;                       \
                R_CSTL_TraceFunctionExit (__FUNCTION__, __FILE__, __LINE__, _trace_duration);                \
        } while (0)

#else

#define R_CSTL_TRACE_SCOPE()                                                                                 \
        uint64_t _trace_start = R_CSTL_TraceGetTimestamp ();                                                 \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__)

#define R_CSTL_TRACE_SCOPE_CTX(fmt, ...)                                                                     \
        uint64_t _trace_start = R_CSTL_TraceGetTimestamp ();                                                 \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__);                                        \
        R_CSTL_LOG_TRACE (" Context: " fmt, __VA_ARGS__)

#define R_CSTL_TRACE_SCOPE_EXIT()                                                                            \
        do                                                                                                   \
        {                                                                                                    \
                uint64_t _trace_duration = R_CSTL_TraceGetTimestamp () - _trace_start;                       \
                R_CSTL_TraceFunctionExit (__FUNCTION__, __FILE__, __LINE__, _trace_duration);                \
        } while (0)

#endif

/**
 * @brief Trace function entry with manual timing
 * Usage: R_CSTL_TRACE_FUNCTION(); at function start
 * Must pair with R_CSTL_TRACE_RETURN() before each return
 */
#define R_CSTL_TRACE_FUNCTION()                                                                              \
        uint64_t _trace_start = R_CSTL_TraceGetTimestamp ();                                                 \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__)

/**
 * @brief Trace function entry with context
 * Usage: R_CSTL_TRACE_FUNCTION_CTX("processing user %d", userId);
 * Must pair with R_CSTL_TRACE_RETURN() before each return
 */
#define R_CSTL_TRACE_FUNCTION_CTX(fmt, ...)                                                                  \
        uint64_t _trace_start = R_CSTL_TraceGetTimestamp ();                                                 \
        R_CSTL_TraceFunctionEntry (__FUNCTION__, __FILE__, __LINE__);                                        \
        R_CSTL_LOG_TRACE (" Context: " fmt, __VA_ARGS__)

/**
 * @brief Trace function exit with manual timing
 * Usage: R_CSTL_TRACE_RETURN(); before each return statement
 */
#define R_CSTL_TRACE_RETURN()                                                                                \
        do                                                                                                   \
        {                                                                                                    \
                uint64_t _trace_duration = R_CSTL_TraceGetTimestamp () - _trace_start;                       \
                R_CSTL_TraceFunctionExit (__FUNCTION__, __FILE__, __LINE__, _trace_duration);                \
        } while (0)

/**
 * @brief Manual trace point
 * Usage: R_CSTL_TRACE_POINT("checkpoint_name");
 */
#define R_CSTL_TRACE_POINT(name) R_CSTL_LOG_TRACE ("%s: %s", __FUNCTION__, name)

/**
 * @brief Trace variable value with automatic type detection
 * Usage: R_CSTL_TRACE_VAR("myVar", myVar);
 */
#define R_CSTL_TRACE_VAR(name, value)                                                                        \
        _Generic (                                                                                           \
            (value),                                                                                         \
            int: R_CSTL_LOG_TRACE ("%s: %s = %d", __FUNCTION__, name, (int)(value)),                         \
            unsigned int: R_CSTL_LOG_TRACE ("%s: %s = %u", __FUNCTION__, name, (unsigned int)(value)),       \
            long: R_CSTL_LOG_TRACE ("%s: %s = %ld", __FUNCTION__, name, (long)(value)),                      \
            unsigned long: R_CSTL_LOG_TRACE ("%s: %s = %lu", __FUNCTION__, name, (unsigned long)(value)),    \
            long long: R_CSTL_LOG_TRACE ("%s: %s = %lld", __FUNCTION__, name, (long long)(value)),           \
            unsigned long long: R_CSTL_LOG_TRACE (                                                           \
                "%s: %s = %llu",                                                                             \
                __FUNCTION__,                                                                                \
                name,                                                                                        \
                (unsigned long long)(value)),                                                                \
            float: R_CSTL_LOG_TRACE ("%s: %s = %f", __FUNCTION__, name, (float)(value)),                     \
            double: R_CSTL_LOG_TRACE ("%s: %s = %f", __FUNCTION__, name, (double)(value)),                   \
            char*: R_CSTL_LOG_TRACE ("%s: %s = %s", __FUNCTION__, name, (char*)(value)),                     \
            const char*: R_CSTL_LOG_TRACE ("%s: %s = %s", __FUNCTION__, name, (const char*)(value)),         \
            default: R_CSTL_LOG_TRACE ("%s: %s = %p", __FUNCTION__, name, (const void*)(value)))

/**
 * @brief Trace variable value with custom format specifier
 * Usage: R_CSTL_TRACE_VAR_FMT("myVar", myVar, "%.2f");
 */
#define R_CSTL_TRACE_VAR_FMT(name, value, fmt) R_CSTL_LOG_TRACE ("%s: %s = " fmt, __FUNCTION__, name, (value))

/**
 * @brief Trace pointer value
 * Usage: R_CSTL_TRACE_PTR("myPtr", myPtr);
 */
#define R_CSTL_TRACE_PTR(name, ptr) R_CSTL_LOG_TRACE ("%s: %s = %p", __FUNCTION__, name, (const void*)(ptr))

#else

#define R_CSTL_TRACE_FUNCTION()             ((void)0)
#define R_CSTL_TRACE_FUNCTION_CTX(fmt, ...) ((void)0)
#define R_CSTL_TRACE_RETURN()               ((void)0)
#define R_CSTL_TRACE_SCOPE()                ((void)0)
#define R_CSTL_TRACE_SCOPE_CTX(fmt, ...)    ((void)0)
#if defined(_MSC_VER) || (!defined(__GNUC__) && !defined(__clang__))
#define R_CSTL_TRACE_SCOPE_EXIT() ((void)0)
#endif
#define R_CSTL_TRACE_POINT(name)               ((void)0)
#define R_CSTL_TRACE_VAR(name, value)          ((void)0)
#define R_CSTL_TRACE_VAR_FMT(name, value, fmt) ((void)0)
#define R_CSTL_TRACE_PTR(name, ptr)            ((void)0)

#endif
