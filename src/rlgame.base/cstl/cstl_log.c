#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <errno.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#else
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#include <stdio.h>
#include <time.h>
#endif
#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#endif
#endif

#ifndef R_CSTL_LOG_RING_CAPACITY
#define R_CSTL_LOG_RING_CAPACITY 2048u
#endif

#ifndef R_CSTL_LOG_BACKTRACE_MAX_FRAMES
#define R_CSTL_LOG_BACKTRACE_MAX_FRAMES 48u
#endif

#ifndef R_CSTL_LOG_MESSAGE_SIZE
#define R_CSTL_LOG_MESSAGE_SIZE 4096
#endif

#ifndef R_CSTL_LOG_BACKTRACE_SIZE
#define R_CSTL_LOG_BACKTRACE_SIZE 8192
#endif

typedef struct r_cstl_log_entry
{
        enum r_cstl_log_level level;
        char                 timestamp[32];
        uint32_t             threadId;
        char                 message[R_CSTL_LOG_MESSAGE_SIZE];
        char                 backtrace[R_CSTL_LOG_BACKTRACE_SIZE];
} r_cstl_log_entry;

#if defined(_WIN32)
typedef CRITICAL_SECTION   r_cstl_log_mutex;
typedef CONDITION_VARIABLE r_cstl_log_cond;

typedef struct r_cstl_log_atomic
{
        volatile LONG   running;
        volatile LONG   minLevel;
        volatile LONG64 dropped;
} r_cstl_log_atomics;

static void
r_cstl_log_new_mutex (r_cstl_log_mutex* m)
{
    if (m) InitializeCriticalSection (m);
}

static void
r_cstl_log_mutex_lock (r_cstl_log_mutex* m)
{
    if (m) EnterCriticalSection (m);
}

static void
r_cstl_log_mutex_unlock (r_cstl_log_mutex* m)
{
    if (m) LeaveCriticalSection (m);
}

static void
r_cstl_log_mutex_destroy (r_cstl_log_mutex* m)
{
    if (m) DeleteCriticalSection (m);
}

static void
r_cstl_log_cond_init (r_cstl_log_cond* c)
{
    if (c) InitializeConditionVariable (c);
}

static void
r_cstl_log_cond_wait (r_cstl_log_cond* c, r_cstl_log_mutex* m)
{
    if (c && m) SleepConditionVariableCS (c, m, INFINITE);
}

static void
r_cstl_log_cond_signal (r_cstl_log_cond* c)
{
    if (c) WakeConditionVariable (c);
}

static void
r_cstl_log_cond_broadcast (r_cstl_log_cond* c)
{
    if (c) WakeAllConditionVariable (c);
}

static void
r_cstl_log_format_timestamp (char* buffer, size_t bufferSize)
{
#if defined(_WIN32)
    SYSTEMTIME st;
    GetLocalTime (&st);
    snprintf (
        buffer,
        bufferSize,
        "%04u-%02u-%02u %02u:%02u:%02u.%03u",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds);
#else
    struct timespec ts;
    struct tm       tm;
    clock_gettime (CLOCK_REALTIME, &ts);
    localtime_r (&ts.tv_sec, &tm);
    snprintf (
        buffer,
        bufferSize,
        "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        ts.tv_nsec / 1000000);
#endif
}

static uint64_t
r_cstl_log_monotonic_us (void)
{
    static LARGE_INTEGER frequency = {0};
    LARGE_INTEGER        counter;
    if (frequency.QuadPart == 0) QueryPerformanceFrequency (&frequency);
    QueryPerformanceCounter (&counter);
    return (uint64_t)((counter.QuadPart * 1000000) / frequency.QuadPart);
}

static uint32_t
r_cstl_log_current_thread_id (void)
{
    return GetCurrentThreadId ();
}

#else
typedef pthread_mutex_t r_cstl_log_mutex;
typedef pthread_cond_t  r_cstl_log_cond;

#include <stdatomic.h>

typedef struct r_cstl_log_atomics
{
        atomic_int            running;
        atomic_uint_least32_t minLevel;
        atomic_uint_least64_t dropped;
} r_cstl_log_atomics;

static void
r_cstl_log_new_mutex (r_cstl_log_mutex* m)
{
    if (m) pthread_mutex_init (m, NULL);
}

static void
r_cstl_log_mutex_lock (r_cstl_log_mutex* m)
{
    if (m) pthread_mutex_lock (m);
}

static void
r_cstl_log_mutex_unlock (r_cstl_log_mutex* m)
{
    if (m) pthread_mutex_unlock (m);
}

static void
r_cstl_log_mutex_destroy (r_cstl_log_mutex* m)
{
    if (m) pthread_mutex_destroy (m);
}

static void
r_cstl_log_cond_init (r_cstl_log_cond* c)
{
    if (c) pthread_cond_init (c, NULL);
}

static void
r_cstl_log_cond_wait (r_cstl_log_cond* c, r_cstl_log_mutex* m)
{
    if (c && m) pthread_cond_wait (c, m);
}

static void
r_cstl_log_cond_signal (r_cstl_log_cond* c)
{
    if (c) pthread_cond_signal (c);
}

static void
r_cstl_log_cond_broadcast (r_cstl_log_cond* c)
{
    if (c) pthread_cond_broadcast (c);
}
static void
r_cstl_log_format_timestamp (char* buffer, size_t bufferSize)
{
    struct timespec ts;
    struct tm       tm;
    clock_gettime (CLOCK_REALTIME, &ts);
    localtime_r (&ts.tv_sec, &tm);
    snprintf (
        buffer,
        bufferSize,
        "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        ts.tv_nsec / 1000000);
}

static uint64_t
r_cstl_log_monotonic_us (void)
{
    struct timespec ts;
#if defined(CLOCK_MONOTONIC)
    clock_gettime (CLOCK_MONOTONIC, &ts);
#else
    clock_gettime (CLOCK_REALTIME, &ts);
#endif
    return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull;
}

static uint32_t
r_cstl_log_current_thread_id (void)
{
#if defined(__linux__)
    return (uint32_t)syscall (SYS_gettid);
#elif defined(__APPLE__)
    uint64_t tid = 0;
    pthread_threadid_np (NULL, &tid);
    return (uint32_t)tid;
#else
    return (uint32_t)(uintptr_t)pthread_self ();
#endif
}

#endif

static void
r_cstl_log_atomic_store_running (r_cstl_log_atomics* atomics, int value)
{
#if defined(_WIN32)
    InterlockedExchange (&atomics->running, (LONG)value);
#else
    atomic_store_explicit (&atomics->running, value, memory_order_release);
#endif
}

static int
r_cstl_log_atomic_load_running (const r_cstl_log_atomics* atomics)
{
#if defined(_WIN32)
    return (int)InterlockedCompareExchange ((volatile LONG*)&atomics->running, 0, 0);
#else
    return atomic_load_explicit (&atomics->running, memory_order_acquire);
#endif
}

static void
r_cstl_log_atomic_store_min_level (r_cstl_log_atomics* atomics, enum r_cstl_log_level level)
{
#if defined(_WIN32)
    InterlockedExchange (&atomics->minLevel, (LONG)level);
#else
    atomic_store_explicit (&atomics->minLevel, (uint_least32_t)level, memory_order_release);
#endif
}

static enum r_cstl_log_level
r_cstl_log_atomic_load_min_level (const r_cstl_log_atomics* atomics)
{
#if defined(_WIN32)
    return (enum r_cstl_log_level)InterlockedCompareExchange ((volatile LONG*)&atomics->minLevel, 0, 0);
#else
    return (enum r_cstl_log_level)atomic_load_explicit (&atomics->minLevel, memory_order_acquire);
#endif
}

static void
r_cstl_log_atomic_store_dropped (r_cstl_log_atomics* atomics, uint64_t value)
{
#if defined(_WIN32)
    InterlockedExchange64 (&atomics->dropped, (LONG64)value);
#else
    atomic_store_explicit (&atomics->dropped, value, memory_order_relaxed);
#endif
}

static uint64_t
r_cstl_log_atomic_load_dropped (const r_cstl_log_atomics* atomics)
{
#if defined(_WIN32)
    return (uint64_t)InterlockedCompareExchange64 ((volatile LONG64*)&atomics->dropped, 0, 0);
#else
    return atomic_load_explicit (&atomics->dropped, memory_order_relaxed);
#endif
}

static void
r_cstl_log_atomic_fetch_add_dropped (r_cstl_log_atomics* atomics, uint64_t value)
{
#if defined(_WIN32)
    InterlockedExchangeAdd64 (&atomics->dropped, (LONG64)value);
#else
    atomic_fetch_add_explicit (&atomics->dropped, value, memory_order_relaxed);
#endif
}

typedef struct r_cstl_log_state
{
        r_cstl_log_entry** ring;
        size_t            capacity;
        size_t            head;
        size_t            tail;
        size_t            count;
        r_cstl_log_mutex   mutex;
        r_cstl_log_cond    cond;
        r_cstl_log_cond    flushCond;
        r_cstl_log_atomics atomics;
        uint32_t          flags;
#if defined(_WIN32)
        HANDLE consumerThread;
        bool   symInitialized;
#else
        pthread_t consumerThread;
#endif
        bool initialized;
} r_cstl_log_state;

static r_cstl_log_state g_log = {0};

static void
r_cstl_log_heap_release (char* buf)
{
#if defined(R_LOG)
    if (!buf) return;
#endif
    r_cstl_heap_unregister_allocation (&g_log, buf);
    r_cstl_heap_free (buf);
}

static int
r_cstl_log_format_message (char* buffer, size_t bufferSize, const char* fmt, va_list args)
{
#if defined(R_LOG)
    if (!buffer || bufferSize == 0 || !fmt) return -1;
#endif
    int result = vsnprintf (buffer, bufferSize, fmt, args);
    if (result < 0) return -1;
    if ((size_t)result >= bufferSize) buffer[bufferSize - 1] = '\0';
    return result;
}

#if defined(_WIN32)
static void
r_cstl_log_capture_backtrace (char* buffer, size_t bufferSize)
{
    if (!buffer || bufferSize == 0) return;
    buffer[0] = '\0';

    void*  frames[R_CSTL_LOG_BACKTRACE_MAX_FRAMES];
    USHORT count = CaptureStackBackTrace (2u, R_CSTL_LOG_BACKTRACE_MAX_FRAMES, frames, NULL);
    if (count == 0) return;

    HANDLE process = GetCurrentProcess ();
    if (!g_log.symInitialized)
    {
        g_log.symInitialized = SymInitialize (process, NULL, TRUE) ? true : false;
    }

    char   line[512];
    size_t used = 0;

    for (USHORT i = 0; i < count; ++i)
    {
        DWORD64 address = (DWORD64)(uintptr_t)frames[i];
        line[0] = 0x00;

        char         symbolBuffer[sizeof (SYMBOL_INFO) + MAX_SYM_NAME];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof (SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;

        const char* name = "(unknown)";
        if (g_log.symInitialized && SymFromAddr (process, address, &displacement, symbol))
        {
            name = symbol->Name;
        }

        IMAGEHLP_LINE64 lineInfo;
        memset (&lineInfo, 0, sizeof (lineInfo));
        lineInfo.SizeOfStruct = sizeof (lineInfo);
        DWORD       lineDisplacement = 0;
        const char* file = "?";
        DWORD       lineNo = 0;
        if (g_log.symInitialized && SymGetLineFromAddr64 (process, address, &lineDisplacement, &lineInfo))
        {
            file = lineInfo.FileName ? lineInfo.FileName : "?";
            lineNo = lineInfo.LineNumber;
        }

        snprintf (
            line,
            sizeof (line),
            "#%u %p %s (%s:%lu)\n",
            (unsigned)i,
            frames[i],
            name,
            file,
            (unsigned long)lineNo);
        size_t lineLen = strlen (line);
        if (used + lineLen + 1u >= bufferSize) break;
        memcpy (buffer + used, line, lineLen + 1u);
        used += lineLen;
    }
}

#elif defined(__APPLE__) || defined(__linux__)
static void
r_cstl_log_capture_backtrace (char* buffer, size_t bufferSize)
{
    if (!buffer || bufferSize == 0) return;
    buffer[0] = '\0';

    void* frames[R_CSTL_LOG_BACKTRACE_MAX_FRAMES];
    int   count = backtrace (frames, (int)R_CSTL_LOG_BACKTRACE_MAX_FRAMES);
    if (count <= 2) return;

    char** symbols = backtrace_symbols (frames, count);
    if (!symbols) return;

    size_t used = 0;

    for (int i = 2; i < count; ++i)
    {
        char line[512];
        snprintf (line, sizeof (line), "#%d %p %s\n", i - 2, frames[i], symbols[i]);
        size_t lineLen = strlen (line);
        if (used + lineLen + 1u >= bufferSize) break;
        memcpy (buffer + used, line, lineLen + 1u);
        used += lineLen;
    }

    free (symbols);
}

#else
static void
r_cstl_log_capture_backtrace (char* buffer, size_t bufferSize)
{
    (void)buffer;
    (void)bufferSize;
}
#endif

static void
r_cstl_log_destroy_entry (r_cstl_log_entry* entry)
{
#if defined(R_LOG)
    if (!entry) return;
#endif
    r_cstl_heap_unregister_allocation (&g_log, entry);
    r_cstl_heap_free (entry);
}

static void
r_cstl_log_cleanup_partial_init (void)
{
    if (g_log.ring)
    {
        r_cstl_heap_unregister_allocation (&g_log, g_log.ring);
        r_cstl_heap_free (g_log.ring);
        g_log.ring = NULL;
    }
    r_cstl_log_mutex_destroy (&g_log.mutex);
    memset (&g_log, 0, sizeof (g_log));
}

#if defined(_WIN32)
static wchar_t*
r_cstl_log_utf8_to_wide (const char* utf8Str)
{
    if (!utf8Str) return NULL;

    int wideLen = MultiByteToWideChar (CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wideLen == 0) return NULL;

    wchar_t* wideStr = (wchar_t*)r_cstl_heap_alloc (wideLen * sizeof (wchar_t));
    if (!wideStr) return NULL;

    MultiByteToWideChar (CP_UTF8, 0, utf8Str, -1, wideStr, wideLen);
    return wideStr;
}

static void
r_cstl_log_write_to_debug_buffer (const r_cstl_log_entry* entry)
{
    char debugBuffer[8192];

    const char* level = r_cstl_log_level_name (entry->level);
    size_t      used = 0;
    snprintf (
        debugBuffer,
        sizeof (debugBuffer),
        "[%s][tid=%" PRIu32 "][%-5s] %s",
        entry->timestamp,
        entry->threadId,
        level,
        entry->message);
    used = strlen (debugBuffer);
    if (entry->message[0] != 0x00)
    {
        size_t len = strlen (entry->message);
        if (entry->message[len - 1] != '\n' && used + 1 < sizeof (debugBuffer))
        {
            debugBuffer[used] = '\n';
            debugBuffer[used + 1] = 0x00;
            used++;
        }
    }
    if (entry->backtrace && used + strlen (entry->backtrace) < sizeof (debugBuffer))
    {
        strncat (debugBuffer, entry->backtrace, sizeof (debugBuffer) - used - 1);
    }

    wchar_t* wideBuffer = r_cstl_log_utf8_to_wide (debugBuffer);
    if (wideBuffer)
    {
        OutputDebugStringW (wideBuffer);
        r_cstl_heap_free (wideBuffer);
    }
    else
    {
        OutputDebugStringA (debugBuffer);
    }
}
#endif

static int
r_cstl_log_colors_supported (void)
{
#if defined(_WIN32)
    HANDLE hConsole = GetStdHandle (STD_OUTPUT_HANDLE);
    DWORD  mode = 0;
    if (GetConsoleMode (hConsole, &mode))
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode (hConsole, mode))
        {
            return 1;
        }
    }
    return 0;
#else
    return isatty (STDERR_FILENO) ? 1 : 0;
#endif
}

static const char*
r_cstl_log_get_color_code (enum r_cstl_log_level level)
{
    switch (level)
    {
    case R_CSTL_LOG_LEVEL_TRACE:
        return "\033[36m";
    case R_CSTL_LOG_LEVEL_DEBUG:
        return "\033[37m";
    case R_CSTL_LOG_LEVEL_INFO:
        return "\033[32m";
    case R_CSTL_LOG_LEVEL_WARN:
        return "\033[33m";
    case R_CSTL_LOG_LEVEL_ERROR:
    case R_CSTL_LOG_LEVEL_FATAL:
        return "\033[31";
    default:
        return "\033[0m";
    }
}

static void
r_cstl_log_write_entry_to_stderr (const r_cstl_log_entry* entry)
{
#if defined(R_LOG)
    if (!entry || !entry->message) return;
#endif
    const char* level = r_cstl_log_level_name (entry->level);

    uint32_t flags = r_cstl_log_get_flags ();
    int      colorsEnabled = (flags & R_CSTL_LOG_FLAG_ENABLE_COLORS) && r_cstl_log_colors_supported ();
    int      disableTags = flags & R_CSTL_LOG_FLAG_DISABLE_TAGS;

    if (colorsEnabled)
    {
        const char* colorCode = r_cstl_log_get_color_code (entry->level);
        fprintf (stderr, "%s", colorCode);
    }

    if (disableTags)
    {
        fprintf (stderr, "%s", entry->message);
    }
    else
    {
        fprintf (
            stderr,
            "%s [TID %" PRIu32 "] %-5s %s",
            entry->timestamp,
            entry->threadId,
            level,
            entry->message);
    }

    if (entry->message[0] != 0x00)
    {
        size_t len = strlen (entry->message);
        if (entry->message[len - 1] != '\n') fputc ('\n', stderr);
    }

    if (colorsEnabled)
    {
        fprintf (stderr, "\033[0m");
    }

    if (entry->backtrace) fputs (entry->backtrace, stderr);
    fflush (stderr);

#if defined(_WIN32)
    r_cstl_log_write_to_debug_buffer (entry);
#endif
}

static void
r_cstl_log_drop_oldest_locked (void)
{
    if (g_log.count == 0) return;
    r_cstl_log_entry* dropped = g_log.ring[g_log.head];
    g_log.head = (g_log.head + 1u) % g_log.capacity;
    --g_log.count;
    r_cstl_log_atomic_fetch_add_dropped (&g_log.atomics, 1);
    r_cstl_log_destroy_entry (dropped);
}

static void
r_cstl_log_notify_flush_waiters_locked (void)
{
    if (g_log.count == 0) r_cstl_log_cond_broadcast (&g_log.flushCond);
}

static int
r_cstl_log_enqueue_entry (r_cstl_log_entry* entry)
{
#if defined(R_LOG)
    if (!entry) return -1;
#endif
    r_cstl_log_mutex_lock (&g_log.mutex);
    if (g_log.count >= g_log.capacity) r_cstl_log_drop_oldest_locked ();

    g_log.ring[g_log.tail] = entry;
    g_log.tail = (g_log.tail + 1u) % g_log.capacity;
    ++g_log.count;
    r_cstl_log_cond_signal (&g_log.cond);
    r_cstl_log_mutex_unlock (&g_log.mutex);
    return 0;
}

static r_cstl_log_entry*
r_cstl_log_dequeue_entry_locked (void)
{
    if (g_log.count == 0) return NULL;

    r_cstl_log_entry* entry = g_log.ring[g_log.head];
    g_log.head = (g_log.head + 1u) % g_log.capacity;
    --g_log.count;
    r_cstl_log_notify_flush_waiters_locked ();
    return entry;
}

#if defined(_WIN32)
static DWORD WINAPI
r_cstl_log_consumer_thread (LPVOID param)
{
    (void)param;
#else
static void*
r_cstl_log_consumer_thread (void* param)
{
    (void)param;
#endif
    for (;;)
    {
        r_cstl_log_mutex_lock (&g_log.mutex);
        while (g_log.count == 0)
        {
            if (!r_cstl_log_atomic_load_running (&g_log.atomics))
            {
                r_cstl_log_mutex_unlock (&g_log.mutex);
#if defined(_WIN32)
                return 0;
#else
                return NULL;
#endif
            }
            r_cstl_log_cond_wait (&g_log.cond, &g_log.mutex);
        }

        r_cstl_log_entry* entry = r_cstl_log_dequeue_entry_locked ();
        r_cstl_log_mutex_unlock (&g_log.mutex);

        if (entry)
        {
            r_cstl_log_write_entry_to_stderr (entry);
            r_cstl_log_destroy_entry (entry);
        }
    }
}

const char*
r_cstl_log_level_name (enum r_cstl_log_level level)
{
    switch (level)
    {
    case R_CSTL_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case R_CSTL_LOG_LEVEL_INFO:
        return "INFO";
    case R_CSTL_LOG_LEVEL_TRACE:
        return "TRACE";
    case R_CSTL_LOG_LEVEL_WARN:
        return "WARN";
    case R_CSTL_LOG_LEVEL_ERROR:
        return "ERROR";
    case R_CSTL_LOG_LEVEL_FATAL:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

#define R_CSTL_LOG_MAX_STACK_ENTRIES 128

int
r_cstl_log_init (void)
{
    if (g_log.initialized) return R_CSTL_OK;
    g_log.capacity = R_CSTL_LOG_RING_CAPACITY;
    g_log.ring = (r_cstl_log_entry**)r_cstl_heap_alloc (g_log.capacity * sizeof (r_cstl_log_entry*));
    if (!g_log.ring)
    {
        static struct r_cstl_log_entry* entries[R_CSTL_LOG_MAX_STACK_ENTRIES] = {0};
        g_log.ring = entries;
        g_log.capacity = R_CSTL_LOG_MAX_STACK_ENTRIES;
    }
    memset (g_log.ring, 0, g_log.capacity * sizeof (r_cstl_log_entry*));
    r_cstl_heap_register_allocation (
        &g_log,
        g_log.ring,
        g_log.capacity * sizeof (r_cstl_log_entry*),
        R_CSTL_HEAP_NAME (r_cstl_log_init));

    r_cstl_log_new_mutex (&g_log.mutex);
    r_cstl_log_cond_init (&g_log.cond);
    r_cstl_log_cond_init (&g_log.flushCond);
    r_cstl_log_atomic_store_running (&g_log.atomics, 1);
    r_cstl_log_atomic_store_min_level (&g_log.atomics, R_CSTL_LOG_LEVEL_TRACE);
    r_cstl_log_atomic_store_dropped (&g_log.atomics, 0);
    g_log.head = 0;
    g_log.tail = 0;
    g_log.count = 0;

#if defined(_WIN32)
    SetConsoleOutputCP (CP_UTF8);
    SetConsoleCP (CP_UTF8);
#endif

#if defined(_WIN32)
    g_log.symInitialized = false;
    g_log.consumerThread = CreateThread (NULL, 0, r_cstl_log_consumer_thread, NULL, 0, NULL);
    if (!g_log.consumerThread)
    {
        r_cstl_log_cleanup_partial_init ();
        return -1;
    }
#else
    if (pthread_create (&g_log.consumerThread, NULL, r_cstl_log_consumer_thread, NULL) != 0)
    {
        r_cstl_log_cleanup_partial_init ();
        return -1;
    }
#endif
    g_log.initialized = true;
    return R_CSTL_OK;
}

void
r_cstl_log_shutdown (void)
{
    if (!g_log.initialized) return;
    g_log.initialized = false;
    r_cstl_log_atomic_store_running (&g_log.atomics, 0);
    r_cstl_log_mutex_lock (&g_log.mutex);
    r_cstl_log_cond_broadcast (&g_log.cond);
    r_cstl_log_mutex_unlock (&g_log.mutex);

#if defined(_WIN32)
    if (g_log.consumerThread)
    {
        WaitForSingleObject (g_log.consumerThread, INFINITE);
        CloseHandle (g_log.consumerThread);
        g_log.consumerThread = NULL;
    }
    if (g_log.symInitialized)
    {
        SymCleanup (GetCurrentProcess ());
        g_log.symInitialized = false;
    }
#else
    pthread_join (g_log.consumerThread, NULL);
#endif

    r_cstl_log_mutex_lock (&g_log.mutex);
    while (g_log.count > 0)
    {
        r_cstl_log_entry* entry = r_cstl_log_dequeue_entry_locked ();
        r_cstl_log_mutex_unlock (&g_log.mutex);
        if (entry)
        {
            r_cstl_log_write_entry_to_stderr (entry);
            r_cstl_log_destroy_entry (entry);
        }
        r_cstl_log_mutex_lock (&g_log.mutex);
    }
    r_cstl_log_mutex_unlock (&g_log.mutex);

    if (g_log.ring)
    {
        r_cstl_heap_unregister_allocation (&g_log, g_log.ring);
        r_cstl_heap_free (g_log.ring);
        g_log.ring = NULL;
    }

    r_cstl_log_mutex_destroy (&g_log.mutex);
    memset (&g_log, 0, sizeof (g_log));
}

void
r_cstl_log_flush (void)
{
    if (!g_log.initialized) return;

    r_cstl_log_mutex_lock (&g_log.mutex);
    while (g_log.count > 0)
    {
        r_cstl_log_cond_signal (&g_log.cond);
        r_cstl_log_cond_wait (&g_log.flushCond, &g_log.mutex);
    }
    r_cstl_log_mutex_unlock (&g_log.mutex);
}

void
r_cstl_log_set_min_level (enum r_cstl_log_level level)
{
#if defined(R_LOG)
    if (level < R_CSTL_LOG_LEVEL_TRACE) goto cstl_fail;
    if (level >= _COUNT) goto cstl_fail;
#endif
    r_cstl_log_atomic_store_min_level (&g_log.atomics, level);
cstl_fail:
    return;
}

enum r_cstl_log_level
r_cstl_log_get_min_level (void)
{
    return r_cstl_log_atomic_load_min_level (&g_log.atomics);
}

uint64_t
r_cstl_log_get_dropped_count (void)
{
    return r_cstl_log_atomic_load_dropped (&g_log.atomics);
}

void
r_cstl_log_set_flags (uint32_t flags)
{
    r_cstl_log_mutex_lock (&g_log.mutex);
    g_log.flags = flags;
    r_cstl_log_mutex_unlock (&g_log.mutex);
}

uint32_t
r_cstl_log_get_flags (void)
{
    r_cstl_log_mutex_lock (&g_log.mutex);
    uint32_t flags = g_log.flags;
    r_cstl_log_mutex_unlock (&g_log.mutex);
    return flags;
}

void
r_cstl_log_writeV (enum r_cstl_log_level level, const char* fmt, va_list args)
{
    if (!g_log.initialized) return;
#if defined(R_LOG)
    if (!fmt) goto cstl_fail;
    if (level < R_CSTL_LOG_LEVEL_TRACE || level >= _COUNT) goto cstl_fail;
    if ((int)level < (int)r_cstl_log_get_min_level ()) goto cstl_fail;
#endif

    r_cstl_log_entry* entry = (r_cstl_log_entry*)r_cstl_heap_alloc (sizeof (r_cstl_log_entry));
    if (!entry)
    {
        r_cstl_log_atomic_fetch_add_dropped (&g_log.atomics, 1);
        goto cstl_fail;
    }

    entry->message[0] = '\0';
    if (r_cstl_log_format_message (entry->message, sizeof (entry->message), fmt, args) < 0)
    {
        r_cstl_heap_free (entry);
        r_cstl_log_atomic_fetch_add_dropped (&g_log.atomics, 1);
        goto cstl_fail;
    }

    entry->timestamp[0] = '\0';
    r_cstl_log_format_timestamp (entry->timestamp, sizeof (entry->timestamp));

    entry->backtrace[0] = '\0';
    if (level == R_CSTL_LOG_LEVEL_FATAL)
        r_cstl_log_capture_backtrace (entry->backtrace, sizeof (entry->backtrace));

    r_cstl_heap_register_allocation (
        &g_log,
        entry,
        sizeof (r_cstl_log_entry),
        R_CSTL_HEAP_NAME (r_cstl_log_writeV));
    entry->level = level;
    entry->threadId = r_cstl_log_current_thread_id ();

    if (r_cstl_log_enqueue_entry (entry) != 0)
    {
        r_cstl_log_destroy_entry (entry);
        r_cstl_log_atomic_fetch_add_dropped (&g_log.atomics, 1);
    }
cstl_fail:
    return;
}

void
r_cstl_log_write (enum r_cstl_log_level level, const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    r_cstl_log_writeV (level, fmt, args);
    va_end (args);
}
