#include "rlgame.base/main.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#if defined(_MSC_VER)
        #include <intrin.h>
#endif
void
R_GameLoop_SetState (R_MainProvider* pProvider, uint8_t flags)
{
        if (!pProvider)
                return;

#if defined(_MSC_VER)
        _InterlockedOr8 ((volatile char*)&pProvider->stateFlags, (char)flags);
#elif defined(__GNUC__) || defined(__clang__)
        __atomic_fetch_or (&pProvider->stateFlags, flags, __ATOMIC_SEQ_CST);
#else
        pProvider->stateFlags |= flags;
#endif
}

void
R_GameLoop_ClearState (R_MainProvider* pProvider, uint8_t flags)
{
        if (!pProvider)
                return;
#if defined(_MSC_VER)
        _InterlockedAnd8 ((volatile char*)&pProvider->stateFlags, (char)(~flags));
#elif defined(__GNUC__) || defined(__clang__)
        __atomic_fetch_and (&pProvider->stateFlags, (uint8_t)(~flags), __ATOMIC_SEQ_CST);
#else
        pProvider->stateFlags &= ~flags;
#endif
}

uint8_t
R_GameLoop_GetState (const R_MainProvider* pProvider)
{
        if (!pProvider)
                return R_GAMELOOP_STATE_NONE;
#if defined(_MSC_VER)
        return (uint8_t)_InterlockedOr8 ((volatile char*)&pProvider->stateFlags, 0);
#elif defined(__GNUC__) || defined(__clang__)
        return __atomic_load_n (&pProvider->stateFlags, __ATOMIC_SEQ_CST);
#else
        return pProvider->stateFlags;
#endif
}

bool
R_GameLoop_HasState (const R_MainProvider* pProvider, uint8_t flags)
{
        uint8_t current = R_GameLoop_GetState (pProvider);
        return (current & flags) == flags;
}

bool
R_GameLoop_IsRunning (const R_MainProvider* pProvider)
{ return R_GameLoop_HasState (pProvider, R_GAMELOOP_STATE_RUNNING); }

bool
R_GameLoop_IsPaused (const R_MainProvider* pProvider)
{ return R_GameLoop_HasState (pProvider, R_GAMELOOP_STATE_PAUSED); }

bool
R_GameLoop_IsDestroyed (const R_MainProvider* pProvider)
{ return R_GameLoop_HasState (pProvider, R_GAMELOOP_STATE_DESTROYED); }

#define R_APP_INITIAL_HEAP_SIZE (1024 * 1024 * 512) // 512 MB
#define R_APP_INIT()                                                                                         \
        do                                                                                                   \
        {                                                                                                    \
                R_CSTL_HeapInit (R_APP_INITIAL_HEAP_SIZE);                                                   \
                R_CSTL_LogInit ();                                                                           \
        } while (0)

#define R_APP_CLEANUP_INFO(info)                                                                             \
        do                                                                                                   \
        {                                                                                                    \
                if ((info).pExistingProcesses)                                                               \
                {                                                                                            \
                        if ((info).existingProcessCount > 0 && (info).pExistingProcesses[0].pName)           \
                                R_CSTL_HeapFree ((void*)(info).pExistingProcesses[0].pName);                 \
                        R_CSTL_HeapFree ((void*)(info).pExistingProcesses);                                  \
                }                                                                                            \
                if ((info).args.pCmdLine)                                                                    \
                        R_CSTL_HeapFree ((void*)(info).args.pCmdLine);                                       \
        } while (0)

#define R_APP_SHUTDOWN()                                                                                     \
        do                                                                                                   \
        {                                                                                                    \
                R_CSTL_LogShutdown ();                                                                       \
                R_CSTL_HeapShutdown ();                                                                      \
        } while (0)

#define R_APP_LOG_HEAP_STATS()                                                                               \
        do                                                                                                   \
        {                                                                                                    \
                size_t totalSize       = R_CSTL_Heap_GetTotalSize ();                                        \
                size_t usedSize        = R_CSTL_Heap_GetUsedSize ();                                         \
                size_t registeredCount = R_CSTL_HeapGetRegisteredCount ();                                   \
                R_CSTL_LOG_INFO ("Heap Stats: TotalSize=%zu UsedSize=%zu RegisteredCount=%zu", totalSize,    \
                                 usedSize, registeredCount);                                                 \
        } while (0)

#define R_APP_LOG_INFO(Info)                                                                                 \
        do                                                                                                   \
        {                                                                                                    \
                R_CSTL_LOG_INFO ("App: %s pid=%u args=%d", (Info).pApplicationName, (Info).pid,              \
                                 (Info).args.argc);                                                          \
                if ((info).args.pCmdLine)                                                                    \
                        R_CSTL_LOG_INFO ("Cmd: %s", (Info).args.pCmdLine);                                   \
                R_CSTL_LOG_INFO ("Memory: total=%llu avail=%llu used=%llu heap=%zu",                         \
                                 (unsigned long long)(Info).memory.totalPhysicalBytes,                       \
                                 (unsigned long long)(Info).memory.availablePhysicalBytes,                   \
                                 (unsigned long long)(Info).memory.usedBytes,                                \
                                 (size_t)(Info).memory.heapAllocatedBytes);                                  \
                if ((Info).pExistingProcesses && (Info).existingProcessCount > 0)                            \
                {                                                                                            \
                        R_CSTL_LOG_INFO (                                                                    \
                            "Process[0]: pid=%u name=%s mem=%llu", (Info).pExistingProcesses[0].pid,         \
                            (Info).pExistingProcesses[0].pName ? (Info).pExistingProcesses[0].pName          \
                                                               : "(null)",                                   \
                            (unsigned long long)(Info).pExistingProcesses[0].memoryBytes);                   \
                }                                                                                            \
        } while (0)
#define R_APP_LAUNCH(RunCallback, Info)                                                                      \
        const void* pUserData = &Info;                                                                       \
        R_LaunchMainProvider (RunCallback, pUserData);

#if defined(_WIN32)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #include <psapi.h>
        #pragma comment(lib, "psapi.lib")

static struct R_CSTL_String*
R_CopyCStringToHeap (const char* src)
{
        if (!src)
                return NULL;
        return R_CSTL_NewStringWithData (src);
}

void
R_AssignProcessName (R_ProcessInfo* proc, char* exePath, int argc, char** argv)
{
        if (!proc)
                return;
        if (exePath)
        {
                static struct R_CSTL_String* pExeName = NULL;
                if (pExeName == NULL)
                {
                        pExeName = R_CSTL_NewStringWithData (exePath);
                }
                proc->pName = pExeName;
        }
        else if (argc && argv && argv[0])
        {
                proc->pName = R_CopyCStringToHeap (argv[0]);
        }
        else
        {
                proc->pName = R_CopyCStringToHeap ("rlgame");
        }
}

static uint32_t
R_GetCurrentPid ()
{ return (uint32_t)GetCurrentProcessId (); }

void
R_FillMemoryInfo (R_MemoryInfo* out)
{
        if (!out)
                return;
        memset (out, 0, sizeof (*out));
        MEMORYSTATUSEX st;
        st.dwLength = sizeof (st);
        if (GlobalMemoryStatusEx (&st))
        {
                out->totalPhysicalBytes     = st.ullTotalPhys;
                out->availablePhysicalBytes = st.ullAvailPhys;
                out->totalVirtualBytes      = st.ullTotalVirtual;
                out->usedBytes              = st.ullTotalPhys - st.ullAvailPhys;
        }
        PROCESS_MEMORY_COUNTERS pmc = {0};
        if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc)))
        {
                out->heapAllocatedBytes = (size_t)pmc.WorkingSetSize;
        }
}

static char*
R_GetExecutablePath ()
{
        char* buf = (char*)R_CSTL_HeapAlloc (MAX_PATH);
        if (!buf)
                return NULL;
        DWORD len = GetModuleFileNameA (NULL, buf, MAX_PATH);
        if (len == 0 || len == MAX_PATH)
        {
                R_CSTL_HeapFree (buf);
                return NULL;
        }
        return buf;
}

R_ProcessInfo*
R_CollectProcesses (size_t* outCount, int argc, char** argv)
{
        R_ProcessInfo* arr = NULL;
        char*          exe = NULL;

        if (!outCount)
                return NULL;

        *outCount = 1;
        arr       = (R_ProcessInfo*)R_CSTL_HeapAlloc (sizeof (R_ProcessInfo));
        if (!arr)
        {
                *outCount = 0;
                goto r_cleanup;
        }
        memset (arr, 0, sizeof (R_ProcessInfo));

        arr[0].pid         = R_GetCurrentPid ();
        arr[0].pUser       = NULL;
        arr[0].startTimeMs = 0;
        arr[0].memoryBytes = 0;

        exe = R_GetExecutablePath ();
        R_AssignProcessName (&arr[0], exe, argc, argv);

        PROCESS_MEMORY_COUNTERS pmc = {0};
        if (GetProcessMemoryInfo (GetCurrentProcess (), &pmc, sizeof (pmc)))
        {
                arr[0].memoryBytes = (uint64_t)pmc.WorkingSetSize;
        }

        return arr;

r_cleanup:
        if (arr)
                R_CSTL_HeapFree (arr);
        if (exe)
                R_CSTL_HeapFree (exe);
        return NULL;
}

void
R_InitializeApplicationInfo (R_ApplicationInfo* info, int argc, char** argv)
{
        if (!info)
                return;
        memset (info, 0, sizeof (*info));
        static struct R_CSTL_String* pAppName;
        if (pAppName == NULL)
        {
                pAppName = R_CSTL_NewStringWithData ("Real Game (rlgame)");
        }
        info->pApplicationName        = pAppName;
        info->applicationVersionMajor = 0;
        info->applicationVersionMinor = 1;
        info->applicationVersionPatch = 0;
        info->pid                     = R_GetCurrentPid ();
        info->args.argc               = argc;
        info->args.argv               = (const char* const*)argv;
}

void
R_BuildCommandLine (R_ApplicationInfo* info, int argc, char** argv)
{
        R_CSTL_StringBuilder* pBuilder   = NULL;
        struct R_CSTL_String*        pCmdString = NULL;
        char*                 cmd        = NULL;

        if (!info || argc <= 0 || !argv)
                return;

        pBuilder = R_CSTL_NewStringBuilder ();
        if (!pBuilder)
                goto r_cleanup;

        for (int i = 0; i < argc; ++i)
        {
                R_CSTL_StringBuilderEmplace (pBuilder, argv[i]);
                if (i + 1 < argc)
                        R_CSTL_StringBuilderAppendChar (pBuilder, ' ');
        }

        pCmdString = R_CSTL_StringBuilderToString (pBuilder);
        if (!pCmdString)
                goto r_cleanup;

        size_t len = R_CSTL_StringLength (pCmdString);
        cmd        = (char*)R_CSTL_HeapAlloc (len + 1);
        if (!cmd)
                goto r_cleanup;

        memcpy (cmd, R_CSTL_StringData (pCmdString), len);
        cmd[len]            = '\0';
        info->args.pCmdLine = cmd;
        cmd                 = NULL;

r_cleanup:
        if (pCmdString)
                R_CSTL_StringDelete (pCmdString);
        if (pBuilder)
                R_CSTL_DeleteStringBuilder (pBuilder);
        if (cmd)
                R_CSTL_HeapFree (cmd);
}

void
R_PopulateApplicationInfo (R_ApplicationInfo* info, int argc, char** argv)
{
        if (!info)
                return;

        R_InitializeApplicationInfo (info, argc, argv);
        R_BuildCommandLine (info, argc, argv);
        R_FillMemoryInfo (&info->memory);

        size_t         count       = 0;
        R_ProcessInfo* procs       = R_CollectProcesses (&count, argc, argv);
        info->pExistingProcesses   = procs;
        info->existingProcessCount = count;
}

void
R_LaunchMainProvider (R_GameLoopCallback pExecCallback, const void* pUserData)
{
        const R_ApplicationInfo* pAppInfo = (const R_ApplicationInfo*)pUserData;
        R_MainProvider           provider = {
                      .pExecCallback = pExecCallback,
                      .pAppInfo      = pAppInfo,
                      .stateFlags    = R_GAMELOOP_STATE_NONE,
        };
        R_MainProvider_Run (&provider);
}

static HWND g_hwnd = NULL;

void
R_MainProvider_Run (R_MainProvider* pProvider)
{
        if (!pProvider)
        {
                R_CSTL_LOG_ERROR ("GameLoop: Provider is NULL, cannot run");
                return;
        }

        R_GameLoop_SetState (pProvider, R_GAMELOOP_STATE_RUNNING);
        R_CSTL_LOG_INFO ("GameLoop: Started with state flags=0x%02X", pProvider->stateFlags);

        LARGE_INTEGER frequency;
        LARGE_INTEGER lastTime;
        if (!QueryPerformanceFrequency (&frequency))
        {
                R_CSTL_LOG_ERROR ("GameLoop: QueryPerformanceFrequency failed");
                R_GameLoop_ClearState (pProvider, R_GAMELOOP_STATE_RUNNING);
                R_GameLoop_SetState (pProvider, R_GAMELOOP_STATE_DESTROYED);
                return;
        }
        if (!QueryPerformanceCounter (&lastTime))
        {
                R_CSTL_LOG_ERROR ("GameLoop: QueryPerformanceCounter failed for initial time");
                R_GameLoop_ClearState (pProvider, R_GAMELOOP_STATE_RUNNING);
                R_GameLoop_SetState (pProvider, R_GAMELOOP_STATE_DESTROYED);
                return;
        }
        R_CSTL_LOG_INFO ("GameLoop: Timer initialized, frequency=%llu Hz",
                         (unsigned long long)frequency.QuadPart);

        MSG      msg;
        uint64_t frameCount = 0;
        while (R_GameLoop_IsRunning (pProvider) && !R_GameLoop_IsDestroyed (pProvider))
        {
                while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
                {
                        if (msg.message == WM_QUIT)
                        {
                                R_CSTL_LOG_INFO ("GameLoop: WM_QUIT received, initiating shutdown");
                                R_GameLoop_SetState (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
                                goto r_endloop;
                        }
                        TranslateMessage (&msg);
                        DispatchMessage (&msg);
                }
                if (!R_GameLoop_IsRunning (pProvider) || R_GameLoop_IsDestroyed (pProvider))
                {
                        R_CSTL_LOG_INFO ("GameLoop: State changed, exiting loop (running=%d, destroyed=%d)",
                                         R_GameLoop_IsRunning (pProvider),
                                         R_GameLoop_IsDestroyed (pProvider));
                        break;
                }

                if (R_GameLoop_IsPaused (pProvider))
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
                        Sleep (16); // Fallback to ~60fps timing
                        continue;
                }

                const float deltaTime
                    = (float)((currentTime.QuadPart - lastTime.QuadPart) * 1000.0 / frequency.QuadPart)
                      / 1000.0f;
                lastTime = currentTime;

                frameCount++;
#if defined(_R_DEVMODE)
                if (!pProvider->pExecCallback)
                {
                        continue;
                }
#endif
                bool shouldContinue = pProvider->pExecCallback (pProvider->pAppInfo);
                if (!shouldContinue)
                {
                        R_CSTL_LOG_INFO ("GameLoop: Callback returned false, initiating shutdown");
                        R_GameLoop_SetState (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
                        goto r_endloop;
                }
        }

r_endloop:
        R_GameLoop_ClearState (pProvider, R_GAMELOOP_STATE_RUNNING);
        R_GameLoop_SetState (pProvider, R_GAMELOOP_STATE_DESTROYED);
        return;
}

void
R_MainProvider_Stop (R_MainProvider* pProvider)
{
        if (pProvider)
        {
                R_GameLoop_ClearState (pProvider, R_GAMELOOP_STATE_RUNNING);
                R_GameLoop_SetState (pProvider, R_GAMELOOP_STATE_SHUTDOWN);
        }
}

LRESULT CALLBACK
WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        switch (uMsg)
        {
        case WM_CREATE:
                g_hwnd = hwnd;
                return 0;
        case WM_DESTROY:
                g_hwnd = NULL;
                PostQuitMessage (0);
                return 0;
        default:
                return DefWindowProc (hwnd, uMsg, wParam, lParam);
        }
}

#define R_WIN32_INSTANCE HINSTANCE

static int
R_InitWinMain (R_WIN32_INSTANCE hInstance, R_ApplicationInfo* pApplicationInfo, int nCmdShow)
{
        const wchar_t CLASS_NAME[] = L"GameWindowClass";
        WNDCLASSW     wc           = {0};
        wc.lpfnWndProc             = WindowProc;
        wc.hInstance               = hInstance;
        wc.lpszClassName           = CLASS_NAME;
        if (!RegisterClassW (&wc))
                return 0;
        HWND hwnd = CreateWindowExW (0, CLASS_NAME, 
                                     R_CSTL_StringData(pApplicationInfo->pApplicationName), 
                                     WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                     800, 600, NULL, NULL, hInstance, NULL);
        if (!hwnd)
                return 0;
        ShowWindow (hwnd, nCmdShow);
        return 1;
}
#undef R_WIN32_INSTANCE

int WINAPI
wWinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
        R_APP_INIT ();

        R_ApplicationInfo info;
        LPSTR             cmd = GetCommandLineA ();
        R_PopulateApplicationInfo (&info, 0, NULL);

        if (!R_InitWinMain (hInstance, &info, nCmdShow))
        {
                R_CSTL_LOG_ERROR ("WinMain: Failed to initialize window");
                R_APP_SHUTDOWN ();
                return 1;
        }
        R_APP_LOG_HEAP_STATS ();

        R_APP_LAUNCH (NULL, info);

        R_APP_CLEANUP_INFO (info);
        R_APP_SHUTDOWN ();

        return 0;
}

#elif defined(__linux__)

        #include <unistd.h>
        #include <sys/types.h>
        #include <sys/sysinfo.h>
        #include <dirent.h>

static char*
R_CopyStringToHeap (const char* src)
{
        if (!src)
                return NULL;
        size_t len  = strlen (src);
        char*  copy = (char*)R_CSTL_HeapAlloc (len + 1);
        if (copy)
        {
                memcpy (copy, src, len);
                copy[len] = '\0';
        }
        return copy;
}

void
R_AssignProcessName (R_ProcessInfo* proc, char* exePath, int argc, char** argv)
{
        if (!proc)
                return;
        if (exePath)
        {
                proc->pName = exePath;
        }
        else if (argc && argv && argv[0])
        {
                proc->pName = R_CopyStringToHeap (argv[0]);
        }
        else
        {
                proc->pName = R_CopyStringToHeap ("UNKNOWN");
        }
}

static uint32_t
R_GetCurrentPid ()
{ return (uint32_t)getpid (); }
void
R_FillMemoryInfo (R_MemoryInfo* out)
{
        if (!out)
                return;
        memset (out, 0, sizeof (*out));
        struct sysinfo si;
        if (sysinfo (&si) == 0)
        {
                out->totalPhysicalBytes     = (uint64_t)si.totalram * (uint64_t)si.mem_unit;
                out->availablePhysicalBytes = (uint64_t)si.freeram * (uint64_t)si.mem_unit;
                out->usedBytes              = out->totalPhysicalBytes - out->availablePhysicalBytes;
                out->totalVirtualBytes
                    = (uint64_t)si.totalswap * (uint64_t)si.mem_unit + out->totalPhysicalBytes;
        }
        FILE* f = fopen ("/proc/self/statm", "r");
        if (f)
        {
                unsigned long size = 0, rss = 0;
                if (fscanf (f, "%lu %lu", &size, &rss) >= 2)
                {
                        long page               = sysconf (_SC_PAGESIZE);
                        out->heapAllocatedBytes = (size_t)(rss * page);
                }
                fclose (f);
        }
}

static char*
R_GetExecutablePath ()
{
        char    buf[4096];
        ssize_t len = readlink ("/proc/self/exe", buf, sizeof (buf) - 1);
        if (len <= 0)
                return NULL;
        buf[len]        = 0x00;
        size_t allocLen = len + 1;
        char*  copy     = (char*)R_CSTL_HeapAlloc (allocLen);
        if (copy)
        {
                memcpy (copy, buf, allocLen);
                return copy;
        }
        return NULL;
}

R_ProcessInfo*
R_CollectProcesses (size_t* outCount, int argc, char** argv)
{
        R_ProcessInfo* arr = NULL;
        char*          exe = NULL;
        FILE*          f   = NULL;

        if (!outCount)
                return NULL;

        *outCount = 1;
        arr       = (R_ProcessInfo*)R_CSTL_HeapAlloc (sizeof (R_ProcessInfo));
        if (!arr)
        {
                *outCount = 0;
                goto r_cleanup;
        }
        memset (arr, 0, sizeof (R_ProcessInfo));

        arr[0].pid         = R_GetCurrentPid ();
        arr[0].pUser       = NULL;
        arr[0].startTimeMs = 0;
        arr[0].memoryBytes = 0;

        exe = R_GetExecutablePath ();
        R_AssignProcessName (&arr[0], exe, argc, argv);

        f = fopen ("/proc/self/status", "r");
        if (f)
        {
                char line[256];
                while (fgets (line, sizeof (line), f))
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
                fclose (f);
                f = NULL;
        }
        return arr;

r_cleanup:
        if (arr)
                R_CSTL_HeapFree (arr);
        if (exe)
                R_CSTL_HeapFree (exe);
        if (f)
                fclose (f);
        return NULL;
}

int
main (int argc, char** argv)
{
        R_APP_INIT ();

        R_ApplicationInfo info;
        R_PopulateApplicationInfo (&info, argc, argv);

        R_APP_LOG_HEAP_STATS ();

        R_APP_LOG_INFO (info);

        R_APP_CLEANUP_INFO (info);
        R_APP_SHUTDOWN ();

        return 0;
}

#endif
