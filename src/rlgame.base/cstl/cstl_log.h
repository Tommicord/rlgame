#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

enum r_cstl_log_level
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

// Requires r_cstl_heap_init() before calling. Returns R_CSTL_OK on success.
R_CSTL_API int r_cstl_log_init (void);

// Drains pending messages, stops the consumer thread, and releases resources.
R_CSTL_API void r_cstl_log_shutdown (void);

// Blocks until all queued messages have been written.
R_CSTL_API void r_cstl_log_flush (void);

R_CSTL_API void                 r_cstl_log_set_min_level (enum r_cstl_log_level level);
R_CSTL_API enum r_cstl_log_level r_cstl_log_get_min_level (void);

// Number of messages dropped because the ring buffer was full.
R_CSTL_API uint64_t r_cstl_log_get_dropped_count (void);

R_CSTL_API const char* r_cstl_log_level_name (enum r_cstl_log_level level);

// Settingsure log flags
R_CSTL_API void     r_cstl_log_set_flags (uint32_t flags);
R_CSTL_API uint32_t r_cstl_log_get_flags (void);

R_CSTL_API void r_cstl_log_write (enum r_cstl_log_level level, const char* fmt, ...);
R_CSTL_API void r_cstl_log_writeV (enum r_cstl_log_level level, const char* fmt, va_list args);

#define R_CSTL_LOG_TRACE(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_TRACE, __VA_ARGS__)
#define R_CSTL_LOG_DEBUG(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define R_CSTL_LOG_INFO(...)  r_cstl_log_write (R_CSTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define R_CSTL_LOG_WARN(...)  r_cstl_log_write (R_CSTL_LOG_LEVEL_WARN, __VA_ARGS__)
#define R_CSTL_LOG_ERROR(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define R_CSTL_LOG_FATAL(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_FATAL, __VA_ARGS__)

#define R_CSTL_LOG_TRACE_NOTAG(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_TRACE, __VA_ARGS__)
#define R_CSTL_LOG_DEBUG_NOTAG(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define R_CSTL_LOG_INFO_NOTAG(...)  r_cstl_log_write (R_CSTL_LOG_LEVEL_INFO, __VA_ARGS__)
#define R_CSTL_LOG_WARN_NOTAG(...)  r_cstl_log_write (R_CSTL_LOG_LEVEL_WARN, __VA_ARGS__)
#define R_CSTL_LOG_ERROR_NOTAG(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_ERROR, __VA_ARGS__)
#define R_CSTL_LOG_FATAL_NOTAG(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_FATAL, __VA_ARGS__)

#define R_CSTL_LOG_TRACE_PRIVATE(...) r_cstl_log_write (R_CSTL_LOG_LEVEL_TRACE, __VA_ARGS__)
