#include "rlgame.base/main.h"
#include "rlgame.base/main_platform_handle.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_trace.h"
#include "rlgame.base/game/game_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#if defined(__linux__)
#include <wayland-client.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#endif
void
r_game_loop_set_state (struct r_main_provider* pProvider, uint8_t flags)
{
    if (!pProvider) return;

#if defined(_MSC_VER)
    _InterlockedOr8 ((volatile char*)&pProvider->stateFlags, (char)flags);
#elif defined(__GNUC__) || defined(__clang__)
    __atomic_fetch_or (&pProvider->stateFlags, flags, __ATOMIC_SEQ_CST);
#else
    pProvider->stateFlags |= flags;
#endif
}

void
r_game_loop_clear_state (struct r_main_provider* pProvider, uint8_t flags)
{
    if (!pProvider) return;
#if defined(_MSC_VER)
    _InterlockedAnd8 ((volatile char*)&pProvider->stateFlags, (char)(~flags));
#elif defined(__GNUC__) || defined(__clang__)
    __atomic_fetch_and (&pProvider->stateFlags, (uint8_t)(~flags), __ATOMIC_SEQ_CST);
#else
    pProvider->stateFlags &= ~flags;
#endif
}

uint8_t
r_game_loop_get_state (const struct r_main_provider* pProvider)
{
    if (!pProvider) return R_GAMELOOP_STATE_NONE;
#if defined(_MSC_VER)
    return (uint8_t)_InterlockedOr8 ((volatile char*)&pProvider->stateFlags, 0);
#elif defined(__GNUC__) || defined(__clang__)
    return __atomic_load_n (&pProvider->stateFlags, __ATOMIC_SEQ_CST);
#else
    return pProvider->stateFlags;
#endif
}

bool
r_game_loop_has_state (const struct r_main_provider* pProvider, uint8_t flags)
{
    const uint8_t current = r_game_loop_get_state (pProvider);
    return (current & flags) == flags;
}

bool
r_game_loop_is_running (const struct r_main_provider* pProvider)
{
    return r_game_loop_has_state (pProvider, R_GAMELOOP_STATE_RUNNING);
}

bool
r_game_loop_is_paused (const struct r_main_provider* pProvider)
{
    return r_game_loop_has_state (pProvider, R_GAMELOOP_STATE_PAUSED);
}

bool
r_game_loop_is_destroyed (const struct r_main_provider* pProvider)
{
    return r_game_loop_has_state (pProvider, R_GAMELOOP_STATE_DESTROYED);
}

#define R_APP_INITIAL_HEAP_SIZE 536870912ul // 512 MB
#define R_APP_INIT()                                                                                         \
    do                                                                                                       \
    {                                                                                                        \
        R_CSTL_HeapInit (R_APP_INITIAL_HEAP_SIZE);                                                           \
        R_CSTL_LogInit ();                                                                                   \
        R_CSTL_TraceLogEnvironmentInfo ();                                                                   \
    } while (0)

#define R_APP_CLEANUP_INFO(Info)                                                                             \
    do                                                                                                       \
    {                                                                                                        \
        if ((Info).pExistingProcesses)                                                                       \
        {                                                                                                    \
            if ((Info).existingProcessCount > 0 && (Info).pExistingProcesses[0].pName)                       \
                R_CSTL_HeapFree ((void*)(Info).pExistingProcesses[0].pName);                                 \
            R_CSTL_HeapFree ((void*)(Info).pExistingProcesses);                                              \
        }                                                                                                    \
        if ((Info).args.pCmdLine) R_CSTL_HeapFree ((void*)(Info).args.pCmdLine);                             \
    } while (0)

#define R_APP_SHUTDOWN()                                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        R_CSTL_LogShutdown ();                                                                               \
        R_CSTL_HeapShutdown ();                                                                              \
    } while (0)

#define R_APP_GB_BINARY (1024 * 1024 * 1024)
#define R_APP_MB_BINARY (1024 * 1024)
#define R_APP_LOG_HEAP_STATS()                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        size_t totalSize = R_CSTL_Heap_GetTotalSize ();                                                      \
        size_t usedSize = R_CSTL_Heap_GetUsedSize ();                                                        \
        R_CSTL_LOG_INFO (                                                                                    \
            "Heap Stats: TotalSize=%.2f GB UsedSize=%.2f GB",                                                \
            (double)totalSize / R_APP_GB_BINARY,                                                             \
            (double)usedSize / R_APP_GB_BINARY);                                                             \
    } while (0)

#define R_APP_LOG_INFO(Info)                                                                                 \
    do                                                                                                       \
    {                                                                                                        \
        const char* pAppName = R_CSTL_StringData ((Info).pApplicationName);                                  \
        if (!pAppName) goto r_next;                                                                          \
        R_CSTL_LOG_INFO ("App: %s pid=%u args=%d", pAppName, (Info).pid, (Info).args.argc);                  \
        if ((Info).args.pCmdLine) R_CSTL_LOG_INFO ("Cmd: %s", (Info).args.pCmdLine);                         \
        R_CSTL_LOG_INFO (                                                                                    \
            "Memory: total=%.2f GB avail=%.2f GB used=%.2f GB appHeap=%.2f MB",                              \
            (double)(Info).memory.totalPhysicalBytes / R_APP_GB_BINARY,                                      \
            (double)(Info).memory.availablePhysicalBytes / R_APP_GB_BINARY,                                  \
            (double)(Info).memory.usedBytes / R_APP_GB_BINARY,                                               \
            (double)(Info).memory.heapAllocatedBytes / R_APP_MB_BINARY);                                     \
    r_next:                                                                                                  \
        if ((Info).pExistingProcesses && (Info).existingProcessCount > 0)                                    \
        {                                                                                                    \
            size_t existingProcessCount = (Info).existingProcessCount;                                       \
            R_CSTL_LOG_INFO ("Existing subprocesses: %d", existingProcessCount);                             \
            for (int i = 0; i < existingProcessCount; i++)                                                   \
            {                                                                                                \
                struct r_process_info pProcess = (Info).pExistingProcesses[i];                                \
                const struct R_CSTL_String* pNameString = pProcess.pName;                                    \
                const char* pProcessName = pNameString ? R_CSTL_StringData (pNameString) : "(unknown)";      \
                R_CSTL_LOG_INFO (                                                                            \
                    "Process[%d]: pid=%u name=%s mem=%.2f GB",                                               \
                    i,                                                                                       \
                    pProcess.pid,                                                                            \
                    pProcessName,                                                                            \
                    (double)pProcess.memoryBytes / R_APP_GB_BINARY                                           \
                );                                                                                           \
            }                                                                                                \
        }                                                                                                    \
    } while (0)
#define R_APP_LAUNCH(RunCallback, Info)                                                                      \
    const void* pUserData = &Info;                                                                           \
    r_launch_main_provider (RunCallback, pUserData);

static bool g_runFlag = false;

static struct r_process_info* r_collect_processes (size_t* outCount, int argc, char** argv);
static uint32_t r_get_current_pid ();

void
r_initialize_application_info (struct r_application_info* info, int argc, char** argv)
{
    if (!info) return;
    memset (info, 0, sizeof (*info));
    info->pid = r_get_current_pid ();
    info->args.argc = argc;
    info->args.argv = (const char* const*)argv;
    info->applicationVersionMajor = 1;
    info->applicationVersionMinor = 0;
    info->applicationVersionPatch = 0;

    static struct R_CSTL_String* pAppName;
    if (pAppName == NULL)
    {
        struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();

        if (pBuilder)
        {
            R_CSTL_StringBuilderAppendf (
                pBuilder,
                "Real Game (rlgame) - v%d.%d.%d",
                info->applicationVersionMajor,
                info->applicationVersionMinor,
                info->applicationVersionPatch);
            pAppName = R_CSTL_StringBuilderToString (pBuilder);
            R_CSTL_DeleteStringBuilder (pBuilder);
        }
    }
    info->pApplicationName = pAppName;
}

void
r_build_command_line (struct r_application_info* info, int argc, char** argv)
{
    struct R_CSTL_StringBuilder* pBuilder = NULL;
    struct R_CSTL_String* pCmdString = NULL;
    char* cmd = NULL;

    if (!info || argc <= 0 || !argv) return;

    pBuilder = R_CSTL_NewStringBuilder ();
    if (!pBuilder) goto r_cleanup;

    for (int i = 0; i < argc; ++i)
    {
        R_CSTL_StringBuilderEmplace (pBuilder, argv[i]);
        if (i + 1 < argc) R_CSTL_StringBuilderAppendChar (pBuilder, ' ');
    }

    pCmdString = R_CSTL_StringBuilderToString (pBuilder);
    if (!pCmdString) goto r_cleanup;

    size_t len = R_CSTL_StringLength (pCmdString);
    cmd = (char*)R_CSTL_HeapAlloc (len + 1);
    if (!cmd) goto r_cleanup;

    memcpy (cmd, R_CSTL_StringData (pCmdString), len);
    cmd[len] = '\0';
    info->args.pCmdLine = cmd;
    cmd = NULL;

r_cleanup:
    if (pCmdString) R_CSTL_StringDelete (pCmdString);
    if (pBuilder) R_CSTL_DeleteStringBuilder (pBuilder);
    if (cmd) R_CSTL_HeapFree (cmd);
}

void
r_populate_application_info (struct r_application_info* info, int argc, char** argv)
{
    if (!info) return;

    r_initialize_application_info (info, argc, argv);
    r_build_command_line (info, argc, argv);
    r_fill_memory_info (&info->memory);

    size_t count = 0;
    struct r_process_info* pProcs = r_collect_processes (&count, argc, argv);
    info->pExistingProcesses = pProcs;
    info->existingProcessCount = count;
}

void
r_assign_process_name (struct r_process_info* pProc, const struct R_CSTL_String* pExePath, int argc, char** argv)
{
    if (!pProc) return;
    if (pExePath)
    {
        static const struct R_CSTL_String* pExeName = NULL;
        if (pExeName == NULL)
        {
            pExeName = pExePath;
        }
        pProc->pName = pExeName;
    }
    else
    {
        static const char* pApplicationName = "rlgame";
        pProc->pName = R_CSTL_NewStringWithData (pApplicationName);
    }
}

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

static struct R_CSTL_String*
r_copyCStringToHeap (const char* src)
{
    if (!src) return NULL;
    return R_CSTL_NewStringWithData (src);
}

static uint32_t
r_get_current_pid ()
{
    return (uint32_t)GetCurrentProcessId ();
}

void
r_fill_memory_info (struct r_memory_info* out)
{
    if (!out) return;
    memset (out, 0, sizeof (*out));
    MEMORYSTATUSEX st;
    st.dwLength = sizeof (st);
    if (GlobalMemoryStatusEx (&st))
    {
        out->totalPhysicalBytes = st.ullTotalPhys;
        out->availablePhysicalBytes = st.ullAvailPhys;
        out->totalVirtualBytes = st.ullTotalVirtual;
        out->usedBytes = st.ullTotalPhys - st.ullAvailPhys;
    }
    PROCESS_MEMORY_COUNTERS pmc = {0};
    if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc)))
    {
        out->heapAllocatedBytes = (size_t)pmc.WorkingSetSize;
    }
}

static char*
r_get_executable_path ()
{
    char* buf = (char*)R_CSTL_HeapAlloc (MAX_PATH);
    if (!buf) return NULL;
    DWORD len = GetModuleFileNameA (NULL, buf, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        R_CSTL_HeapFree (buf);
        return NULL;
    }
    return buf;
}

struct r_process_info*
r_collect_processes (size_t* outCount, int argc, char** argv)
{
    struct r_process_info* arr = NULL;
    char* exe = NULL;

    if (!outCount) return NULL;

    *outCount = 1;
    arr = (struct r_process_info*)R_CSTL_HeapAlloc (sizeof (struct r_process_info));
    if (!arr)
    {
        *outCount = 0;
        goto r_cleanup;
    }
    memset (arr, 0, sizeof (struct r_process_info));

    arr[0].pid = r_get_current_pid ();
    arr[0].pUser = NULL;
    arr[0].startTimeMs = 0;
    arr[0].memoryBytes = 0;

    exe = r_get_executable_path ();
    r_assign_process_name (&arr[0], exe, argc, argv);

    PROCESS_MEMORY_COUNTERS pmc = {0};
    if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc)))
    {
        arr[0].memoryBytes = (uint64_t)pmc.WorkingSetSize;
    }

    return arr;

r_cleanup:
    if (arr) R_CSTL_HeapFree (arr);
    if (exe) R_CSTL_HeapFree (exe);
    return NULL;
}

void
r_launch_main_provider (r_game_callback pExecCallback, const void* pUserData)
{
    const struct r_application_info* pAppInfo = (const struct r_application_info*)pUserData;
    struct r_main_provider provider = {
        .pExecCallback = pExecCallback,
        .pAppInfo = pAppInfo,
        .pUserData = (void*)pUserData,
        .stateFlags = R_GAMELOOP_STATE_NONE,
    };
    r_main_provider_Run (&provider);
}

static bool
r_game_loop_callback (const struct r_application_info* pAppInfo, void* pUserData)
{
    struct r_game_state* pGameState = (struct r_game_state*)pUserData;
    if (!g_runFlag)
    {
        struct r_game_state_create_info createInfo = {0};
        createInfo.pApplicationName = R_CSTL_StringData (pAppInfo->pApplicationName);
        HINSTANCE hInstance = GetModuleHandle (NULL);
        if (hInstance == NULL)
        {
            R_CSTL_LOG_WARN ("HINSTANCE handle is NULL, skipping callback initialization.");
            return false;
        }
        createInfo.hInstance = hInstance;
        createInfo.hWnd = r_get_window_handle ();

        enum R_CVulkan_Error result = r_game_state_Initialize (pGameState, &createInfo);
        if (result != R_CVULKAN_OK)
        {
            return false;
        }
        R_CSTL_LOG_INFO ("r_game_loop_callback: Game state initialized");
        g_runFlag = true;
        return true;
    }
    return true;
}

static void
r_game_loop_cleanup (struct r_game_state* pGameState)
{
    if (g_runFlag) r_game_state_Cleanup (pGameState);
}

void
r_main_provider_Run (struct r_main_provider* pProvider)
{
    if (!pProvider)
    {
        return;
    }
    r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_RUNNING);
    LARGE_INTEGER frequency;
    LARGE_INTEGER lastTime;
    if (!QueryPerformanceFrequency (&frequency))
    {
        R_CSTL_LOG_ERROR ("GameLoop: QueryPerformanceFrequency failed");
        r_game_loop_clear_state (pProvider, R_GAMELOOP_STATE_RUNNING);
        r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_DESTROYED);
        return;
    }
    if (!QueryPerformanceCounter (&lastTime))
    {
        R_CSTL_LOG_ERROR ("GameLoop: QueryPerformanceCounter failed for initial time");
        r_game_loop_clear_state (pProvider, R_GAMELOOP_STATE_RUNNING);
        r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_DESTROYED);
        return;
    }
    R_CSTL_LOG_INFO (
        "GameLoop: Timer initialized, frequency=%llu Hz",
        (unsigned long long)frequency.QuadPart);
    MSG msg;
    uint64_t frameCount = 0;
    while (r_game_loop_is_running (pProvider) && !r_game_loop_is_destroyed (pProvider))
    {
        while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                R_CSTL_LOG_INFO ("GameLoop: WM_QUIT received, initiating shutdown");
                r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
                goto r_endloop;
            }
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
        if (!r_game_loop_is_running (pProvider) || r_game_loop_is_destroyed (pProvider))
        {
            R_CSTL_LOG_INFO (
                "GameLoop: State changed, exiting loop (running=%d, destroyed=%d)",
                r_game_loop_is_running (pProvider),
                r_game_loop_is_destroyed (pProvider));
            break;
        }

        if (r_game_loop_is_paused (pProvider))
        {
            Sleep (1);
            if (!QueryPerformanceCounter (&lastTime))
            {
                R_CSTL_LOG_ERROR ("GameLoop: QueryPerformanceCounter failed during pause");
            }
            continue;
        }

        LARGE_INTEGER currentTime;
        if (!QueryPerformanceCounter (&currentTime))
        {
            R_CSTL_LOG_ERROR ("GameLoop: QueryPerformanceCounter failed for frame time");
            Sleep (16); // Fallback to 60fps timing
            continue;
        }
        const float delta
            = (float)((currentTime.QuadPart - lastTime.QuadPart) * 1000.0 / frequency.QuadPart) / 1000.0f;
        lastTime = currentTime;

        frameCount++;
        bool shouldContinue = pProvider->pExecCallback (pProvider->pAppInfo, pProvider->pUserData);
        if (!shouldContinue)
        {
            R_CSTL_LOG_INFO ("GameLoop: Callback returned false, initiating shutdown");
            r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
            goto r_endloop;
        }
    }

r_endloop:
    r_game_loop_clear_state (pProvider, R_GAMELOOP_STATE_RUNNING);
    r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_DESTROYED);
    return;
}

void
r_main_provider_Stop (struct r_main_provider* pProvider)
{
    if (pProvider)
    {
        r_game_loop_clear_state (pProvider, R_GAMELOOP_STATE_RUNNING);
        r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
    }
}

int WINAPI
wWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    R_CSTL_TRACE_FUNCTION ();

    R_APP_INIT ();

    struct r_application_info info;
    LPSTR cmd = GetCommandLineA ();
    r_populate_application_info (&info, 0, NULL);

    if (!r_init_win_main (hInstance, &info, nCmdShow))
    {
        R_APP_SHUTDOWN ();
        R_CSTL_TRACE_RETURN ();
        return 1;
    }
    R_APP_LOG_HEAP_STATS ();
    R_APP_LOG_INFO (info);

    struct r_game_state gameState = {0};
    struct r_main_provider provider = {
        .pExecCallback = r_game_loop_callback,
        .pAppInfo = &info,
        .pUserData = &gameState,
        .stateFlags = R_GAMELOOP_STATE_NONE,
    };
    r_main_provider_Run (&provider);

    r_game_loop_cleanup (&gameState);
    R_APP_CLEANUP_INFO (info);
    R_APP_SHUTDOWN ();

    R_CSTL_TRACE_RETURN ();
    return 0;
}

#elif defined(__linux__)

#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <dirent.h>
#include <time.h>

static uint32_t
r_get_current_pid ()
{
    return (uint32_t)getpid ();
}
void
r_fill_memory_info (struct r_memory_info* out)
{
    if (!out) return;
    memset (out, 0, sizeof (*out));
    struct sysinfo si;
    if (sysinfo (&si) == 0)
    {
        out->totalPhysicalBytes = (uint64_t)si.totalram * (uint64_t)si.mem_unit;
        out->availablePhysicalBytes = (uint64_t)si.freeram * (uint64_t)si.mem_unit;
        out->usedBytes = out->totalPhysicalBytes - out->availablePhysicalBytes;
        out->totalVirtualBytes = (uint64_t)si.totalswap * (uint64_t)si.mem_unit + out->totalPhysicalBytes;
    }
    FILE* f = fopen ("/proc/self/statm", "r");
    if (f)
    {
        unsigned long size = 0, rss = 0;
        if (fscanf (f, "%lu %lu", &size, &rss) >= 2)
        {
            long page = sysconf (_SC_PAGESIZE);
            out->heapAllocatedBytes = (size_t)(rss * page);
        }
        fclose (f);
    }
}

static char*
r_get_executable_path ()
{
    char buf[NAME_MAX + 1];
    ssize_t len = readlink ("/proc/self/exe", buf, sizeof (buf) - 1);
    if (len <= 0) return NULL;
    buf[len] = 0x00;
    size_t allocLen = len + 1;
    char* copy = (char*)R_CSTL_HeapAlloc (allocLen);
    if (copy)
    {
        memcpy (copy, buf, allocLen);
        return copy;
    }
    return NULL;
}

static struct r_process_info*
r_collect_processes (size_t* outCount, int argc, char** argv)
{
    struct r_process_info* arr = NULL;
    struct R_CSTL_String* exe = NULL;
    FILE* fs = NULL;

    if (!outCount) return NULL;

    *outCount = 1;
    arr = (struct r_process_info*)R_CSTL_HeapAlloc (sizeof (struct r_process_info));
    if (!arr)
    {
        *outCount = 0;
        goto r_cleanup;
    }
    memset (arr, 0, sizeof (struct r_process_info));

    arr[0].pid = r_get_current_pid ();
    arr[0].pUser = NULL;
    arr[0].startTimeMs = 0;
    arr[0].memoryBytes = 0;

    exe = R_CSTL_NewStringWithData (r_get_executable_path ());
    r_assign_process_name (&arr[0], exe, argc, argv);

    fs = fopen ("/proc/self/status", "r");
    if (fs)
    {
        char line[256];
        while (fgets (line, sizeof (line), fs))
        {
            if (strncmp (line, "VmRSS:", 6) == 0)
            {
                unsigned long kb = 0;
                if (sscanf (line + 6, "%lu", &kb) == 1)
                {
                    arr[0].memoryBytes = (uint64_t)kb * 1024ULL;
                }
                break;
            }
        }
        fclose (fs);
        fs = NULL;
    }
    return arr;

r_cleanup:
    if (arr) R_CSTL_HeapFree (arr);
    if (exe) R_CSTL_HeapFree (exe);
    if (fs) fclose (fs);
    return NULL;
}

static enum r_window_handle_backend g_currentBackend = R_WINDOW_BACKEND_NONE;
static union r_window_handle_handle g_windowHandle = {0};

static bool
r_game_loop_callback (const struct r_application_info* pAppInfo, void* pUserData)
{
    (void)pAppInfo;
    struct r_game_state* pGameState = (struct r_game_state*)pUserData;
    if (!g_runFlag)
    {
        struct r_game_state_create_info createInfo = {0};
        createInfo.pApplicationName = R_CSTL_StringData (pAppInfo->pApplicationName);
        if (g_currentBackend == R_WINDOW_BACKEND_WAYLAND)
        {
            createInfo.linuxBackend = R_GAME_LINUX_BACKEND_WAYLAND;
            struct r_window_handle_style waylandStyle = {R_WINDOW_STYLE_DEFAULT};
            g_windowHandle.waylandWindow
                = r_init_wayland_window ((struct r_application_info*)pAppInfo, &waylandStyle);
            if (!g_windowHandle.waylandWindow)
            {
                R_CSTL_LOG_ERROR ("Failed to initialize Wayland window handle");
                return false;
            }
            R_CSTL_LOG_INFO ("Wayland window initialized");
            int windowWidth = 0, windowHeight = 0;
            r_wayland_window_wait_for_settings (g_windowHandle.waylandWindow, &windowWidth, &windowHeight);
            R_CSTL_LOG_INFO ("Window dimensions received: %dx%d", windowWidth, windowHeight);
            struct wl_display* display = r_wayland_window_get_display (g_windowHandle.waylandWindow);
            struct wl_surface* surface = r_wayland_window_get_surface (g_windowHandle.waylandWindow);
            R_CSTL_LOG_INFO ("Wayland display pointer: %p", (void*)display);
            R_CSTL_LOG_INFO ("Wayland surface pointer: %p", (void*)surface);

            if (!display || !surface)
            {
                R_CSTL_LOG_ERROR ("Invalid Wayland display or surface pointer");
                r_destroy_wayland_window (g_windowHandle.waylandWindow);
                g_windowHandle.waylandWindow = NULL;
                return false;
            }

            createInfo.pDisplay = display;
            createInfo.pSurface = surface;
            createInfo.windowWidth = windowWidth;
            createInfo.windowHeight = windowHeight;
        }
        else if (g_currentBackend == R_WINDOW_BACKEND_X11)
        {
            createInfo.linuxBackend = R_GAME_LINUX_BACKEND_X11;
            struct r_window_handle_style x11Style = {R_WINDOW_STYLE_DEFAULT};
            g_windowHandle.x11Window = r_init_x11_window ((struct r_application_info*)pAppInfo, &x11Style);
            if (!g_windowHandle.x11Window)
            {
                R_CSTL_LOG_ERROR ("Failed to initialize X11 window handle");
                return false;
            }
            R_CSTL_LOG_INFO ("X11 window initialized");

            int windowWidth = 0, windowHeight = 0;
            r_x11_window_get_size (g_windowHandle.x11Window, &windowWidth, &windowHeight);
            R_CSTL_LOG_INFO ("Window dimensions received: %dx%d", windowWidth, windowHeight);

            Display* display = r_x11_window_get_display (g_windowHandle.x11Window);
            if (!display)
            {
                R_CSTL_LOG_ERROR ("Invalid X11 display pointer");
                r_destroy_x11_window (g_windowHandle.x11Window);
                g_windowHandle.x11Window = 0;
                return false;
            }
            createInfo.pX11Display = display;
            createInfo.x11Window = g_windowHandle.x11Window;
            createInfo.windowWidth = windowWidth;
            createInfo.windowHeight = windowHeight;
        }
        else if (g_currentBackend == R_WINDOW_BACKEND_XCB)
        {
            createInfo.linuxBackend = R_GAME_LINUX_BACKEND_XCB;
            struct r_window_handle_style xcbStyle = {R_WINDOW_STYLE_DEFAULT};
            g_windowHandle.xcbWindow = r_initXCBWindow ((struct r_application_info*)pAppInfo, &xcbStyle);
            if (!g_windowHandle.xcbWindow)
            {
                R_CSTL_LOG_ERROR ("Failed to initialize XCB window handle");
                return false;
            }
            R_CSTL_LOG_INFO ("XCB window initialized");
            int windowWidth, windowHeight;
            R_XCBWindowGetSize (g_windowHandle.xcbWindow, &windowWidth, &windowHeight);
            R_CSTL_LOG_INFO ("Window dimensions received: %dx%d", windowWidth, windowHeight);
            xcb_connection_t* connection = R_XCBWindowGetConnection (g_windowHandle.xcbWindow);
            if (!connection)
            {
                R_CSTL_LOG_ERROR ("Invalid XCB connection pointer");
                r_destroyXCBWindow (g_windowHandle.xcbWindow);
                g_windowHandle.xcbWindow = 0;
                return false;
            }
            createInfo.pXCBConnection = connection;
            createInfo.xcbWindow = g_windowHandle.xcbWindow;
            createInfo.windowWidth = windowWidth;
            createInfo.windowHeight = windowHeight;
        }
        else
        {
            R_CSTL_LOG_ERROR ("No valid window backend selected");
            return false;
        }
        R_CSTL_LOG_INFO ("Got valid display and surface");
        enum R_CVulkan_Error result = r_game_state_Initialize (pGameState, &createInfo);
        if (result != R_CVULKAN_OK)
        {
            R_CSTL_LOG_ERROR ("ailed to initialize game state (error: %d)", result);
            if (g_currentBackend == R_WINDOW_BACKEND_WAYLAND && g_windowHandle.waylandWindow)
            {
                r_destroy_wayland_window (g_windowHandle.waylandWindow);
                g_windowHandle.waylandWindow = NULL;
            }
            else if (g_currentBackend == R_WINDOW_BACKEND_X11 && g_windowHandle.x11Window)
            {
                r_destroy_x11_window (g_windowHandle.x11Window);
                g_windowHandle.x11Window = 0;
            }
            else if (g_currentBackend == R_WINDOW_BACKEND_XCB && g_windowHandle.xcbWindow)
            {
                r_destroyXCBWindow (g_windowHandle.xcbWindow);
                g_windowHandle.xcbWindow = 0;
            }
            return false;
        }
        g_runFlag = true;
        return true;
    }
    // TODO: Add game update and render calls here later
    // r_game_state_Update (pGameState, deltaTime);
    // r_game_state_Render (pGameState);
    return true;
}

static void
r_game_loop_cleanup (struct r_game_state* pGameState)
{
    R_CSTL_TRACE_FUNCTION ();
    if (g_runFlag)
    {
        r_game_state_Cleanup (pGameState);
        if (g_currentBackend == R_WINDOW_BACKEND_WAYLAND && g_windowHandle.waylandWindow)
        {
            r_destroy_wayland_window (g_windowHandle.waylandWindow);
            g_windowHandle.waylandWindow = NULL;
        }
        else if (g_currentBackend == R_WINDOW_BACKEND_X11 && g_windowHandle.x11Window)
        {
            r_destroy_x11_window (g_windowHandle.x11Window);
            g_windowHandle.x11Window = 0;
        }
        else if (g_currentBackend == R_WINDOW_BACKEND_XCB && g_windowHandle.xcbWindow)
        {
            r_destroyXCBWindow (g_windowHandle.xcbWindow);
            g_windowHandle.xcbWindow = 0;
        }
    }
    R_CSTL_TRACE_RETURN ();
}

void
r_launch_main_provider (r_game_callback pExecCallback, const void* pUserData)
{
    const struct r_application_info* pAppInfo = (const struct r_application_info*)pUserData;
    struct r_main_provider provider = {
        .pExecCallback = pExecCallback,
        .pAppInfo = pAppInfo,
        .pUserData = (void*)pUserData,
        .stateFlags = R_GAMELOOP_STATE_NONE,
    };
    r_main_provider_Run (&provider);
}

void
r_main_provider_Run (struct r_main_provider* pProvider)
{
    if (!pProvider)
    {
        return;
    }
    r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_RUNNING);
    struct timespec lastTime;
    if (clock_gettime (CLOCK_MONOTONIC, &lastTime) != 0)
    {
        R_CSTL_LOG_ERROR ("clock_gettime failed for initial time");
        r_game_loop_clear_state (pProvider, R_GAMELOOP_STATE_RUNNING);
        r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_DESTROYED);
        return;
    }
    uint64_t frameCount = 0;
    while (r_game_loop_is_running (pProvider) && !r_game_loop_is_destroyed (pProvider))
    {
        r_process_window_events (&g_windowHandle);
        if (!r_game_loop_is_running (pProvider) | r_game_loop_is_destroyed (pProvider))
        {
            R_CSTL_LOG_INFO (
                "State changed, exiting loop (running=%d, destroyed=%d)",
                r_game_loop_is_running (pProvider),
                r_game_loop_is_destroyed (pProvider));
            break;
        }

        if (r_game_loop_is_paused (pProvider))
        {
            usleep (1000);
            if (clock_gettime (CLOCK_MONOTONIC, &lastTime) != 0)
            {
                R_CSTL_LOG_ERROR ("clock_gettime failed during pause");
            }
            continue;
        }
        struct timespec currentTime;
        if (clock_gettime (CLOCK_MONOTONIC, &currentTime) != 0)
        {
            usleep (16000); // Fallback to ~60fps timing
            continue;
        }
        const double deltaSeconds = (double)(currentTime.tv_sec - lastTime.tv_sec)
                                    + (double)(currentTime.tv_nsec - lastTime.tv_nsec) / 1e9;
        lastTime = currentTime;
        frameCount++;
        bool shouldContinue = pProvider->pExecCallback (pProvider->pAppInfo, pProvider->pUserData);

        if (!shouldContinue)
        {
            r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
            goto r_endloop;
        }

        if (deltaSeconds < 0.016)
        {
            usleep ((int)((0.016 - deltaSeconds) * 1e6));
        }
    }
r_endloop:
    r_game_loop_clear_state (pProvider, R_GAMELOOP_STATE_RUNNING);
    r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_DESTROYED);
}

void
r_main_provider_Stop (struct r_main_provider* pProvider)
{
    if (pProvider)
    {
        r_game_loop_clear_state (pProvider, R_GAMELOOP_STATE_RUNNING);
        r_game_loop_set_state (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
    }
}

int
main (int argc, char** argv)
{
    R_CSTL_TRACE_FUNCTION ();
    R_APP_INIT ();

    struct r_application_info info;
    r_populate_application_info (&info, argc, argv);

    R_APP_LOG_HEAP_STATS ();
    R_APP_LOG_INFO (info);

    struct r_capabilities gpuCapabilities = {0};
    g_currentBackend = r_detect_capabilities (&gpuCapabilities);
    if (g_currentBackend == R_WINDOW_BACKEND_NONE)
    {
        R_CSTL_LOG_ERROR ("No suitable window backend found");
        R_APP_CLEANUP_INFO (info);
        R_APP_SHUTDOWN ();
        R_CSTL_TRACE_RETURN ();
        return EXIT_FAILURE;
    }
    struct r_game_state gameState = {0};
    struct r_main_provider provider = {
        .pExecCallback = r_game_loop_callback,
        .pAppInfo = &info,
        .pUserData = &gameState,
        .stateFlags = R_GAMELOOP_STATE_NONE,
    };
    r_main_provider_Run (&provider);

    r_game_loop_cleanup (&gameState);
    R_APP_CLEANUP_INFO (info);
    R_APP_SHUTDOWN ();

    R_CSTL_TRACE_RETURN ();
    return EXIT_SUCCESS;
}

#endif
