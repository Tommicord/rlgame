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

typedef struct R_CSTL_LogEntry
{
        enum R_CSTL_LogLevel level;
        char                 timestamp[32];
        uint32_t             threadId;
        char                 message[R_CSTL_LOG_MESSAGE_SIZE];
        char                 backtrace[R_CSTL_LOG_BACKTRACE_SIZE];
} R_CSTL_LogEntry;

#if defined(_WIN32)
typedef CRITICAL_SECTION   R_CSTL_LogMutex;
typedef CONDITION_VARIABLE R_CSTL_LogCond;

typedef struct R_CSTL_LogAtomic
{
        volatile LONG   running;
        volatile LONG   minLevel;
        volatile LONG64 dropped;
} R_CSTL_LogAtomics;

static void
R_CSTL_LogNewMutex (R_CSTL_LogMutex* m)
{
    if (m) InitializeCriticalSection (m);
}

static void
R_CSTL_LogMutexLock (R_CSTL_LogMutex* m)
{
    if (m) EnterCriticalSection (m);
}

static void
R_CSTL_LogMutexUnlock (R_CSTL_LogMutex* m)
{
    if (m) LeaveCriticalSection (m);
}

static void
R_CSTL_LogMutexDestroy (R_CSTL_LogMutex* m)
{
    if (m) DeleteCriticalSection (m);
}

static void
R_CSTL_LogCondInit (R_CSTL_LogCond* c)
{
    if (c) InitializeConditionVariable (c);
}

static void
R_CSTL_LogCondWait (R_CSTL_LogCond* c, R_CSTL_LogMutex* m)
{
    if (c && m) SleepConditionVariableCS (c, m, INFINITE);
}

static void
R_CSTL_LogCondSignal (R_CSTL_LogCond* c)
{
    if (c) WakeConditionVariable (c);
}

static void
R_CSTL_LogCondBroadcast (R_CSTL_LogCond* c)
{
    if (c) WakeAllConditionVariable (c);
}

static void
R_CSTL_LogFormatTimestamp (char* buffer, size_t bufferSize)
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
R_CSTL_LogMonotonicUs (void)
{
    static LARGE_INTEGER frequency = {0};
    LARGE_INTEGER        counter;
    if (frequency.QuadPart == 0) QueryPerformanceFrequency (&frequency);
    QueryPerformanceCounter (&counter);
    return (uint64_t)((counter.QuadPart * 1000000) / frequency.QuadPart);
}

static uint32_t
R_CSTL_LogCurrentThreadId (void)
{
    return GetCurrentThreadId ();
}

#else
typedef pthread_mutex_t R_CSTL_LogMutex;
typedef pthread_cond_t  R_CSTL_LogCond;

#include <stdatomic.h>

typedef struct R_CSTL_LogAtomics
{
        atomic_int            running;
        atomic_uint_least32_t minLevel;
        atomic_uint_least64_t dropped;
} R_CSTL_LogAtomics;

static void
R_CSTL_LogNewMutex (R_CSTL_LogMutex* m)
{
    if (m) pthread_mutex_init (m, NULL);
}

static void
R_CSTL_LogMutexLock (R_CSTL_LogMutex* m)
{
    if (m) pthread_mutex_lock (m);
}

static void
R_CSTL_LogMutexUnlock (R_CSTL_LogMutex* m)
{
    if (m) pthread_mutex_unlock (m);
}

static void
R_CSTL_LogMutexDestroy (R_CSTL_LogMutex* m)
{
    if (m) pthread_mutex_destroy (m);
}

static void
R_CSTL_LogCondInit (R_CSTL_LogCond* c)
{
    if (c) pthread_cond_init (c, NULL);
}

static void
R_CSTL_LogCondWait (R_CSTL_LogCond* c, R_CSTL_LogMutex* m)
{
    if (c && m) pthread_cond_wait (c, m);
}

static void
R_CSTL_LogCondSignal (R_CSTL_LogCond* c)
{
    if (c) pthread_cond_signal (c);
}

static void
R_CSTL_LogCondBroadcast (R_CSTL_LogCond* c)
{
    if (c) pthread_cond_broadcast (c);
}

static void
R_CSTL_LogFormatTimestamp (char* buffer, size_t bufferSize)
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
R_CSTL_LogMonotonicUs (void)
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
R_CSTL_LogCurrentThreadId (void)
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
R_CSTL_LogAtomicStoreRunning (R_CSTL_LogAtomics* atomics, int value)
{
#if defined(_WIN32)
    InterlockedExchange (&atomics->running, (LONG)value);
#else
    atomic_store_explicit (&atomics->running, value, memory_order_release);
#endif
}

static int
R_CSTL_LogAtomicLoadRunning (const R_CSTL_LogAtomics* atomics)
{
#if defined(_WIN32)
    return (int)InterlockedCompareExchange ((volatile LONG*)&atomics->running, 0, 0);
#else
    return atomic_load_explicit (&atomics->running, memory_order_acquire);
#endif
}

static void
R_CSTL_LogAtomicStoreMinLevel (R_CSTL_LogAtomics* atomics, enum R_CSTL_LogLevel level)
{
#if defined(_WIN32)
    InterlockedExchange (&atomics->minLevel, (LONG)level);
#else
    atomic_store_explicit (&atomics->minLevel, (uint_least32_t)level, memory_order_release);
#endif
}

static enum R_CSTL_LogLevel
R_CSTL_LogAtomicLoadMinLevel (const R_CSTL_LogAtomics* atomics)
{
#if defined(_WIN32)
    return (enum R_CSTL_LogLevel)InterlockedCompareExchange ((volatile LONG*)&atomics->minLevel, 0, 0);
#else
    return (enum R_CSTL_LogLevel)atomic_load_explicit (&atomics->minLevel, memory_order_acquire);
#endif
}

static void
R_CSTL_LogAtomicStoreDropped (R_CSTL_LogAtomics* atomics, uint64_t value)
{
#if defined(_WIN32)
    InterlockedExchange64 (&atomics->dropped, (LONG64)value);
#else
    atomic_store_explicit (&atomics->dropped, value, memory_order_relaxed);
#endif
}

static uint64_t
R_CSTL_LogAtomicLoadDropped (const R_CSTL_LogAtomics* atomics)
{
#if defined(_WIN32)
    return (uint64_t)InterlockedCompareExchange64 ((volatile LONG64*)&atomics->dropped, 0, 0);
#else
    return atomic_load_explicit (&atomics->dropped, memory_order_relaxed);
#endif
}

static void
R_CSTL_LogAtomicFetchAddDropped (R_CSTL_LogAtomics* atomics, uint64_t value)
{
#if defined(_WIN32)
    InterlockedExchangeAdd64 (&atomics->dropped, (LONG64)value);
#else
    atomic_fetch_add_explicit (&atomics->dropped, value, memory_order_relaxed);
#endif
}

typedef struct R_CSTL_LogState
{
        R_CSTL_LogEntry** ring;
        size_t            capacity;
        size_t            head;
        size_t            tail;
        size_t            count;
        R_CSTL_LogMutex   mutex;
        R_CSTL_LogCond    cond;
        R_CSTL_LogCond    flushCond;
        R_CSTL_LogAtomics atomics;
        uint32_t          flags;
#if defined(_WIN32)
        HANDLE consumerThread;
        bool   symInitialized;
#else
        pthread_t consumerThread;
#endif
        bool initialized;
} R_CSTL_LogState;

static R_CSTL_LogState g_log = {0};

static void
R_CSTL_LogHeapRelease (char* buf)
{
#if defined(R_LOG)
    if (!buf) return;
#endif
    R_CSTL_HeapUnregisterAllocation (&g_log, buf);
    R_CSTL_HeapFree (buf);
}

static int
R_CSTL_LogFormatMessage (char* buffer, size_t bufferSize, const char* fmt, va_list args)
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
R_CSTL_LogCaptureBacktrace (char* buffer, size_t bufferSize)
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
R_CSTL_LogCaptureBacktrace (char* buffer, size_t bufferSize)
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
R_CSTL_LogCaptureBacktrace (char* buffer, size_t bufferSize)
{
    (void)buffer;
    (void)bufferSize;
}
#endif

static void
R_CSTL_LogDestroyEntry (R_CSTL_LogEntry* entry)
{
#if defined(R_LOG)
    if (!entry) return;
#endif
    R_CSTL_HeapUnregisterAllocation (&g_log, entry);
    R_CSTL_HeapFree (entry);
}

static void
R_CSTL_LogCleanupPartialInit (void)
{
    if (g_log.ring)
    {
        R_CSTL_HeapUnregisterAllocation (&g_log, g_log.ring);
        R_CSTL_HeapFree (g_log.ring);
        g_log.ring = NULL;
    }
    R_CSTL_LogMutexDestroy (&g_log.mutex);
    memset (&g_log, 0, sizeof (g_log));
}

#if defined(_WIN32)
static wchar_t*
R_CSTL_LogUtf8ToWide (const char* utf8Str)
{
    if (!utf8Str) return NULL;

    int wideLen = MultiByteToWideChar (CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wideLen == 0) return NULL;

    wchar_t* wideStr = (wchar_t*)R_CSTL_HeapAlloc (wideLen * sizeof (wchar_t));
    if (!wideStr) return NULL;

    MultiByteToWideChar (CP_UTF8, 0, utf8Str, -1, wideStr, wideLen);
    return wideStr;
}

static void
R_CSTL_LogWriteToDebugBuffer (const R_CSTL_LogEntry* entry)
{
    char debugBuffer[8192];

    const char* level = R_CSTL_LogLevelName (entry->level);
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

    wchar_t* wideBuffer = R_CSTL_LogUtf8ToWide (debugBuffer);
    if (wideBuffer)
    {
        OutputDebugStringW (wideBuffer);
        R_CSTL_HeapFree (wideBuffer);
    }
    else
    {
        OutputDebugStringA (debugBuffer);
    }
}
#endif

static int
R_CSTL_LogColorsSupported (void)
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
R_CSTL_LogGetColorCode (enum R_CSTL_LogLevel level)
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
R_CSTL_LogWriteEntryToStderr (const R_CSTL_LogEntry* entry)
{
#if defined(R_LOG)
    if (!entry || !entry->message) return;
#endif
    const char* level = R_CSTL_LogLevelName (entry->level);

    uint32_t flags = R_CSTL_LogGetFlags ();
    int      colorsEnabled = (flags & R_CSTL_LOG_FLAG_ENABLE_COLORS) && R_CSTL_LogColorsSupported ();
    int      disableTags = flags & R_CSTL_LOG_FLAG_DISABLE_TAGS;

    if (colorsEnabled)
    {
        const char* colorCode = R_CSTL_LogGetColorCode (entry->level);
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
            "[%s][tid=%" PRIu32 "][%-5s] %s",
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
    R_CSTL_LogWriteToDebugBuffer (entry);
#endif
}

static void
R_CSTL_LogDropOldestLocked (void)
{
    if (g_log.count == 0) return;
    R_CSTL_LogEntry* dropped = g_log.ring[g_log.head];
    g_log.head = (g_log.head + 1u) % g_log.capacity;
    --g_log.count;
    R_CSTL_LogAtomicFetchAddDropped (&g_log.atomics, 1);
    R_CSTL_LogDestroyEntry (dropped);
}

static void
R_CSTL_LogNotifyFlushWaitersLocked (void)
{
    if (g_log.count == 0) R_CSTL_LogCondBroadcast (&g_log.flushCond);
}

static int
R_CSTL_LogEnqueueEntry (R_CSTL_LogEntry* entry)
{
#if defined(R_LOG)
    if (!entry) return -1;
#endif
    R_CSTL_LogMutexLock (&g_log.mutex);
    if (g_log.count >= g_log.capacity) R_CSTL_LogDropOldestLocked ();

    g_log.ring[g_log.tail] = entry;
    g_log.tail = (g_log.tail + 1u) % g_log.capacity;
    ++g_log.count;
    R_CSTL_LogCondSignal (&g_log.cond);
    R_CSTL_LogMutexUnlock (&g_log.mutex);
    return 0;
}

static R_CSTL_LogEntry*
R_CSTL_LogDequeueEntryLocked (void)
{
    if (g_log.count == 0) return NULL;

    R_CSTL_LogEntry* entry = g_log.ring[g_log.head];
    g_log.head = (g_log.head + 1u) % g_log.capacity;
    --g_log.count;
    R_CSTL_LogNotifyFlushWaitersLocked ();
    return entry;
}

#if defined(_WIN32)
static DWORD WINAPI
R_CSTL_LogConsumerThread (LPVOID param)
{
    (void)param;
#else
static void*
R_CSTL_LogConsumerThread (void* param)
{
    (void)param;
#endif
    for (;;)
    {
        R_CSTL_LogMutexLock (&g_log.mutex);
        while (g_log.count == 0)
        {
            if (!R_CSTL_LogAtomicLoadRunning (&g_log.atomics))
            {
                R_CSTL_LogMutexUnlock (&g_log.mutex);
#if defined(_WIN32)
                return 0;
#else
                return NULL;
#endif
            }
            R_CSTL_LogCondWait (&g_log.cond, &g_log.mutex);
        }

        R_CSTL_LogEntry* entry = R_CSTL_LogDequeueEntryLocked ();
        R_CSTL_LogMutexUnlock (&g_log.mutex);

        if (entry)
        {
            R_CSTL_LogWriteEntryToStderr (entry);
            R_CSTL_LogDestroyEntry (entry);
        }
    }
}

const char*
R_CSTL_LogLevelName (enum R_CSTL_LogLevel level)
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

int
R_CSTL_LogInit (void)
{
    if (g_log.initialized) return 0;
    g_log.capacity = R_CSTL_LOG_RING_CAPACITY;
    g_log.ring = (R_CSTL_LogEntry**)R_CSTL_HeapAlloc (g_log.capacity * sizeof (R_CSTL_LogEntry*));
    if (!g_log.ring) return -1;

    memset (g_log.ring, 0, g_log.capacity * sizeof (R_CSTL_LogEntry*));
    R_CSTL_HeapRegisterAllocation (
        &g_log,
        g_log.ring,
        g_log.capacity * sizeof (R_CSTL_LogEntry*),
        R_CSTL_HEAP_NAME (R_CSTL_LogInit));

    R_CSTL_LogNewMutex (&g_log.mutex);
    R_CSTL_LogCondInit (&g_log.cond);
    R_CSTL_LogCondInit (&g_log.flushCond);
    R_CSTL_LogAtomicStoreRunning (&g_log.atomics, 1);
    R_CSTL_LogAtomicStoreMinLevel (&g_log.atomics, R_CSTL_LOG_LEVEL_TRACE);
    R_CSTL_LogAtomicStoreDropped (&g_log.atomics, 0);
    g_log.head = 0;
    g_log.tail = 0;
    g_log.count = 0;

#if defined(_WIN32)
    SetConsoleOutputCP (CP_UTF8);
    SetConsoleCP (CP_UTF8);
#endif

#if defined(_WIN32)
    g_log.symInitialized = false;
    g_log.consumerThread = CreateThread (NULL, 0, R_CSTL_LogConsumerThread, NULL, 0, NULL);
    if (!g_log.consumerThread)
    {
        R_CSTL_LogCleanupPartialInit ();
        return -1;
    }
#else
    if (pthread_create (&g_log.consumerThread, NULL, R_CSTL_LogConsumerThread, NULL) != 0)
    {
        R_CSTL_LogCleanupPartialInit ();
        return -1;
    }
#endif
    g_log.initialized = true;
    return 0;
}

void
R_CSTL_LogShutdown (void)
{
    if (!g_log.initialized) return;
    g_log.initialized = false;
    R_CSTL_LogAtomicStoreRunning (&g_log.atomics, 0);
    R_CSTL_LogMutexLock (&g_log.mutex);
    R_CSTL_LogCondBroadcast (&g_log.cond);
    R_CSTL_LogMutexUnlock (&g_log.mutex);

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

    R_CSTL_LogMutexLock (&g_log.mutex);
    while (g_log.count > 0)
    {
        R_CSTL_LogEntry* entry = R_CSTL_LogDequeueEntryLocked ();
        R_CSTL_LogMutexUnlock (&g_log.mutex);
        if (entry)
        {
            R_CSTL_LogWriteEntryToStderr (entry);
            R_CSTL_LogDestroyEntry (entry);
        }
        R_CSTL_LogMutexLock (&g_log.mutex);
    }
    R_CSTL_LogMutexUnlock (&g_log.mutex);

    if (g_log.ring)
    {
        R_CSTL_HeapUnregisterAllocation (&g_log, g_log.ring);
        R_CSTL_HeapFree (g_log.ring);
        g_log.ring = NULL;
    }

    R_CSTL_LogMutexDestroy (&g_log.mutex);
    memset (&g_log, 0, sizeof (g_log));
}

void
R_CSTL_LogFlush (void)
{
    if (!g_log.initialized) return;

    R_CSTL_LogMutexLock (&g_log.mutex);
    while (g_log.count > 0)
    {
        R_CSTL_LogCondSignal (&g_log.cond);
        R_CSTL_LogCondWait (&g_log.flushCond, &g_log.mutex);
    }
    R_CSTL_LogMutexUnlock (&g_log.mutex);
}

void
R_CSTL_LogSetMinLevel (enum R_CSTL_LogLevel level)
{
#if defined(R_LOG)
    if (level < R_CSTL_LOG_LEVEL_TRACE) goto cstl_fail;
    if (level >= _COUNT) goto cstl_fail;
#endif
    R_CSTL_LogAtomicStoreMinLevel (&g_log.atomics, level);
cstl_fail:
    return;
}

enum R_CSTL_LogLevel
R_CSTL_LogGetMinLevel (void)
{
    return R_CSTL_LogAtomicLoadMinLevel (&g_log.atomics);
}

uint64_t
R_CSTL_LogGetDroppedCount (void)
{
    return R_CSTL_LogAtomicLoadDropped (&g_log.atomics);
}

void
R_CSTL_LogSetFlags (uint32_t flags)
{
    R_CSTL_LogMutexLock (&g_log.mutex);
    g_log.flags = flags;
    R_CSTL_LogMutexUnlock (&g_log.mutex);
}

uint32_t
R_CSTL_LogGetFlags (void)
{
    R_CSTL_LogMutexLock (&g_log.mutex);
    uint32_t flags = g_log.flags;
    R_CSTL_LogMutexUnlock (&g_log.mutex);
    return flags;
}

void
R_CSTL_LogWriteV (enum R_CSTL_LogLevel level, const char* fmt, va_list args)
{
    if (!g_log.initialized) return;
#if defined(R_LOG)
    if (!fmt) goto cstl_fail;
    if (level < R_CSTL_LOG_LEVEL_TRACE || level >= _COUNT) goto cstl_fail;
    if ((int)level < (int)R_CSTL_LogGetMinLevel ()) goto cstl_fail;
#endif

    R_CSTL_LogEntry* entry = (R_CSTL_LogEntry*)R_CSTL_HeapAlloc (sizeof (R_CSTL_LogEntry));
    if (!entry)
    {
        R_CSTL_LogAtomicFetchAddDropped (&g_log.atomics, 1);
        goto cstl_fail;
    }

    entry->message[0] = '\0';
    if (R_CSTL_LogFormatMessage (entry->message, sizeof (entry->message), fmt, args) < 0)
    {
        R_CSTL_HeapFree (entry);
        R_CSTL_LogAtomicFetchAddDropped (&g_log.atomics, 1);
        goto cstl_fail;
    }

    entry->timestamp[0] = '\0';
    R_CSTL_LogFormatTimestamp (entry->timestamp, sizeof (entry->timestamp));

    entry->backtrace[0] = '\0';
    if (level == R_CSTL_LOG_LEVEL_FATAL)
        R_CSTL_LogCaptureBacktrace (entry->backtrace, sizeof (entry->backtrace));

    R_CSTL_HeapRegisterAllocation (
        &g_log,
        entry,
        sizeof (R_CSTL_LogEntry),
        R_CSTL_HEAP_NAME (R_CSTL_LogWriteV));
    entry->level = level;
    entry->threadId = R_CSTL_LogCurrentThreadId ();

    if (R_CSTL_LogEnqueueEntry (entry) != 0)
    {
        R_CSTL_LogDestroyEntry (entry);
        R_CSTL_LogAtomicFetchAddDropped (&g_log.atomics, 1);
    }
cstl_fail:
    return;
}

void
R_CSTL_LogWrite (enum R_CSTL_LogLevel level, const char* fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    R_CSTL_LogWriteV (level, fmt, args);
    va_end (args);
}
