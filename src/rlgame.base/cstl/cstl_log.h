#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

enum R_CSTL_LogLevel
{
    R_CSTL_LOG_LEVEL_TRACE = 0,
    R_CSTL_LOG_LEVEL_DEBUG,
    R_CSTL_LOG_LEVEL_INFO,
    R_CSTL_LOG_LEVEL_WARN,
    R_CSTL_LOG_LEVEL_ERROR,
    R_CSTL_LOG_LEVEL_FATAL,
    _COUNT,
};

#define R_CSTL_LOG_FLAG_ENABLE_COLORS 0x01
#define R_CSTL_LOG_FLAG_DISABLE_TAGS  0x02
#define R_CSTL_LOG_FLAG_PRIVATE_TRACE 0x04

// Requires R_CSTL_HeapInit() before calling. Returns 0 on success.
R_CSTL_API int R_CSTL_LogInit (void);

// Drains pending messages, stops the consumer thread, and releases resources.
R_CSTL_API void R_CSTL_LogShutdown (void);

// Blocks until all queued messages have been written.
R_CSTL_API void R_CSTL_LogFlush (void);

R_CSTL_API void                 R_CSTL_LogSetMinLevel (enum R_CSTL_LogLevel level);
R_CSTL_API enum R_CSTL_LogLevel R_CSTL_LogGetMinLevel (void);

// Number of messages dropped because the ring buffer was full.
R_CSTL_API uint64_t R_CSTL_LogGetDroppedCount (void);

R_CSTL_API const char* R_CSTL_LogLevelName (enum R_CSTL_LogLevel level);

// Configure log flags
R_CSTL_API void     R_CSTL_LogSetFlags (uint32_t flags);
R_CSTL_API uint32_t R_CSTL_LogGetFlags (void);

R_CSTL_API void R_CSTL_LogWrite (enum R_CSTL_LogLevel level, const char* fmt, ...);
R_CSTL_API void R_CSTL_LogWriteV (enum R_CSTL_LogLevel level, const char* fmt, va_list args);

#define R_CSTL_LOG_TRACE(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_TRACE, __VA_ARGS__)
#define R_CSTL_LOG_DEBUG(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define R_CSTL_LOG_INFO(...)  R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define R_CSTL_LOG_WARN(...)  R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_WARN, __VA_ARGS__)
#define R_CSTL_LOG_ERROR(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define R_CSTL_LOG_FATAL(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_FATAL, __VA_ARGS__)

#define R_CSTL_LOG_TRACE_NOTAG(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_TRACE, __VA_ARGS__)
#define R_CSTL_LOG_DEBUG_NOTAG(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define R_CSTL_LOG_INFO_NOTAG(...)  R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define R_CSTL_LOG_WARN_NOTAG(...)  R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_WARN, __VA_ARGS__)
#define R_CSTL_LOG_ERROR_NOTAG(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define R_CSTL_LOG_FATAL_NOTAG(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_FATAL, __VA_ARGS__)

#define R_CSTL_LOG_TRACE_PRIVATE(...) R_CSTL_LogWrite (R_CSTL_LOG_LEVEL_TRACE, __VA_ARGS__)
