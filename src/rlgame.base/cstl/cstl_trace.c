#include "rlgame.base/cstl/cstl_trace.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <string.h>

#if defined(R_CSTL_PLATFORM_WINDOWS)
#include <windows.h>
#include <psapi.h>
#define R_CSTL_THREAD_LOCAL __declspec (thread)
#elif defined(R_CSTL_PLATFORM_LINUX)
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#define R_CSTL_THREAD_LOCAL __thread
#else
#define R_CSTL_THREAD_LOCAL
#endif

static struct R_CSTL_TraceSettings g_traceSettings
    = {.enableFunctionEntryExit = true,
       .enablePerformanceTiming = true,
       .minDurationMicroseconds = true,
       .enableCallDepthIndentation = true};
static R_CSTL_THREAD_LOCAL int g_traceCallDepth = 0;

R_CSTL_API const struct R_CSTL_TraceSettings*
R_CSTL_TraceGetSettings (void)
{
    return &g_traceSettings;
}

R_CSTL_API void
R_CSTL_TraceSetMinDuration (uint64_t microseconds)
{
    g_traceSettings.minDurationMicroseconds = microseconds;
}

#define R_CSTL_TRACE_MICROTIME (1000)
#define R_CSTL_TRACE_MILLITIME (1000000)
#define R_CSTL_TRACE_SECTIME   (1000000000)

static const char*
R_CSTL_TraceExtractFileName (const char* filePath)
{
    if (!filePath)
    {
        return "<unknown>";
    }
    const char* lastSlash = strrchr (filePath, '/');
    const char* lastBackslash = strrchr (filePath, '\\');
    const char* lastSeparator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;
    return lastSeparator ? lastSeparator + 1 : filePath;
}

R_CSTL_API uint64_t
R_CSTL_TraceGetTimestamp (void)
{
#if defined(R_CSTL_PLATFORM_WINDOWS)
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency (&frequency);
    QueryPerformanceCounter (&counter);
    return (uint64_t)((counter.QuadPart * 1000000ULL) / frequency.QuadPart);
#elif defined(R_CSTL_PLATFORM_LINUX)
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * R_CSTL_TRACE_SECTIME + ts.tv_nsec / 1000);
#else
    return 0;
#endif
}

R_CSTL_API void
R_CSTL_TraceFunctionEntry (const char* functionName, const char* fileName, uint32_t lineNumber)
{
    if (!g_traceSettings.enableFunctionEntryExit)
    {
        return;
    }
    const char* shortFileName = R_CSTL_TraceExtractFileName (fileName);
    R_CSTL_LOG_TRACE ("  Enter: %s (%s:%u)", functionName, shortFileName, lineNumber);

    if (g_traceSettings.enableCallDepthIndentation)
    {
        g_traceCallDepth++;
    }
}

R_CSTL_API void
R_CSTL_TraceFunctionExit (
    const char* functionName,
    const char* fileName,
    uint32_t    lineNumber,
    uint64_t    durationMicroseconds)
{
    if (!g_traceSettings.enableFunctionEntryExit)
    {
        return;
    }

    if (g_traceSettings.enablePerformanceTiming
        && durationMicroseconds < g_traceSettings.minDurationMicroseconds)
    {
        return;
    }

    if (g_traceSettings.enableCallDepthIndentation && g_traceCallDepth > 0)
    {
        g_traceCallDepth--;
    }

    const char* shortFileName = R_CSTL_TraceExtractFileName (fileName);
    if (g_traceSettings.enablePerformanceTiming)
    {
        if (durationMicroseconds < R_CSTL_TRACE_MICROTIME)
        {
            R_CSTL_LOG_TRACE (
                "  Exit: %s (%s:%u) - %llu µs",
                functionName,
                shortFileName,
                lineNumber,
                (unsigned long long)durationMicroseconds);
        }
        else if (durationMicroseconds < R_CSTL_TRACE_MILLITIME)
        {
            R_CSTL_LOG_TRACE (
                "  Exit: %s (%s:%u) - %.2f ms",
                functionName,
                shortFileName,
                lineNumber,
                durationMicroseconds / 1000.0);
        }
        else
        {
            R_CSTL_LOG_TRACE (
                "  Exit: %s (%s:%u) - %.2f s",
                functionName,
                shortFileName,
                lineNumber,
                durationMicroseconds / 1000000.0);
        }
    }
    else
    {
        R_CSTL_LOG_TRACE ("  Exit: %s (%s:%u)", functionName, shortFileName, lineNumber);
    }
}

void
R_CSTL_TraceLogEnvironmentInfo (void)
{
    R_CSTL_LOG_TRACE ("=== Environment Information ===");

#if defined(R_CSTL_PLATFORM_WINDOWS)
    OSVERSIONINFOEXW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEXW);
#pragma warning(push)
#pragma warning(disable : 4996)
    GetVersionExW ((OSVERSIONINFOW*)&osvi);
#pragma warning(pop)

    R_CSTL_LOG_TRACE ("Platform: Windows");
    R_CSTL_LOG_TRACE (
        "  OS Version: %lu.%lu (Build %lu)",
        osvi.dwMajorVersion,
        osvi.dwMinorVersion,
        osvi.dwBuildNumber);
    R_CSTL_LOG_TRACE ("  Product Type: %lu", osvi.wProductType);

    MEMORYSTATUSEX memStatus = {0};
    memStatus.dwLength = sizeof (MEMORYSTATUSEX);
    GlobalMemoryStatusEx (&memStatus);
    R_CSTL_LOG_TRACE ("  Total Memory: %llu MB", memStatus.ullTotalPhys / (1024 * 1024));
    R_CSTL_LOG_TRACE ("  Available Memory: %llu MB", memStatus.ullAvailPhys / (1024 * 1024));

    SYSTEM_INFO sysInfo = {0};
    GetSystemInfo (&sysInfo);
    R_CSTL_LOG_TRACE ("  Processor Count: %lu", sysInfo.dwNumberOfProcessors);
    R_CSTL_LOG_TRACE ("  Page Size: %lu bytes", sysInfo.dwPageSize);

    PROCESS_MEMORY_COUNTERS_EX pmc = {0};
    pmc.cb = sizeof (PROCESS_MEMORY_COUNTERS_EX);
    if (GetProcessMemoryInfo (GetCurrentProcess (), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof (pmc)))
    {
        R_CSTL_LOG_TRACE ("  Process Working Set: %llu MB", pmc.WorkingSetSize / (1024 * 1024));
        R_CSTL_LOG_TRACE ("  Process Peak Working Set: %llu MB", pmc.PeakWorkingSetSize / (1024 * 1024));
    }

#elif defined(R_CSTL_PLATFORM_LINUX)
    struct sysinfo sysInfo = {0};
    sysinfo (&sysInfo);

    R_CSTL_LOG_TRACE ("Platform: Linux");
    R_CSTL_LOG_TRACE ("  Total Memory: %lu MB", sysInfo.totalram / (1024 * 1024));
    R_CSTL_LOG_TRACE ("  Available Memory: %lu MB", sysInfo.freeram / (1024 * 1024));
    R_CSTL_LOG_TRACE ("  Processor Count: %u", sysInfo.procs);
    R_CSTL_LOG_TRACE ("  Load Average (1min): %.2f", sysInfo.loads[0] / 65536.0);
    R_CSTL_LOG_TRACE ("  Load Average (5min): %.2f", sysInfo.loads[1] / 65536.0);
    R_CSTL_LOG_TRACE ("  Load Average (15min): %.2f", sysInfo.loads[2] / 65536.0);

    long pageSize = sysconf (_SC_PAGESIZE);
    R_CSTL_LOG_TRACE ("  Page Size: %ld bytes", pageSize);

    long numProcessors = sysconf (_SC_NPROCESSORS_ONLN);
    if (numProcessors > 0)
    {
        R_CSTL_LOG_TRACE ("  Online Processors: %ld", numProcessors);
    }
#else
    R_CSTL_LOG_TRACE ("Platform: Unknown");
#endif

    R_CSTL_LOG_TRACE ("=== End Environment Information ===");
}
