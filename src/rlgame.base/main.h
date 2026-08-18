#pragma once

#include "rlgame.base/cstl/cstl_string.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum R_GameLoopStateFlags
{
        R_GAMELOOP_STATE_NONE      = 0x00,
        R_GAMELOOP_STATE_RUNNING   = 0x01, // Loop is actively running
        R_GAMELOOP_STATE_PAUSED    = 0x02, // Loop is paused (update suspended)
        R_GAMELOOP_STATE_DESTROYED = 0x04, // Loop is being destroyed
        R_GAMELOOP_STATE_RESUMED   = 0x08, // Loop was just resumed
        R_GAMELOOP_STATE_SUSPENDED = 0x10, // Loop is suspended (background)
        R_GAMELOOP_STATE_ERROR     = 0x20, // Error state
        R_GAMELOOP_STATE_SHUTDOWN  = 0x40, // Shutdown requested
        R_GAMELOOP_STATE_ALL       = 0x7F
} R_GameLoopStateFlags;

typedef struct R_ProcessInfo
{
                uint32_t             pid;
                const struct R_CSTL_String* pName; // process/executable name
                const struct R_CSTL_String* pUser; // owning user or service name (if available)
                uint64_t             startTimeMs; // start time in epoch milliseconds, 0 if unknown
                uint64_t             memoryBytes; // working set / RSS in bytes, 0 if unknown
} R_ProcessInfo;

typedef struct R_MemoryInfo
{
                uint64_t totalPhysicalBytes; // total physical RAM
                uint64_t availablePhysicalBytes; // available RAM
                uint64_t totalVirtualBytes; // total virtual address space
                uint64_t usedBytes; // total used memory (approx)
                size_t   heapAllocatedBytes; // heap allocation tracked by app (optional)
                size_t   heapReservedBytes; // reserved heap size (optional)
} R_MemoryInfo;

typedef struct R_ApplicationArgs
{
                int                argc;
                const char* const* argv; // pointer to argv array (not owned)
                char*              pCmdLine; // optional full command-line string (owned)
} R_ApplicationArgs;

typedef struct R_ApplicationInfo
{
                const struct R_CSTL_String* pApplicationName; // short application name
                uint32_t             applicationVersionMajor;
                uint32_t             applicationVersionMinor;
                uint32_t             applicationVersionPatch;
                uint32_t             pid; // current process id
                const R_ProcessInfo* pExistingProcesses; // optional array (not owned)
                size_t               existingProcessCount; // number of entries above
                R_ApplicationArgs    args; // startup arguments
                R_MemoryInfo         memory; // snapshot of memory usage
} R_ApplicationInfo;

typedef bool (*const R_GameLoopCallback) (const R_ApplicationInfo* pAppInfo);

typedef struct R_MainProvider
{
                R_GameLoopCallback       pExecCallback; // pointer to game loop function
                const R_ApplicationInfo* pAppInfo; // pointer to application info (not owned)
                volatile uint8_t         stateFlags; // atomic state flags (R_GameLoopStateFlags)
} R_MainProvider;

// Atomic state operations for thread-safe game loop management
void    R_GameLoop_SetState (R_MainProvider* pProvider, uint8_t flags);
void    R_GameLoop_ClearState (R_MainProvider* pProvider, uint8_t flags);
uint8_t R_GameLoop_GetState (const R_MainProvider* pProvider);
bool    R_GameLoop_HasState (const R_MainProvider* pProvider, uint8_t flags);
bool    R_GameLoop_IsRunning (const R_MainProvider* pProvider);
bool    R_GameLoop_IsPaused (const R_MainProvider* pProvider);
bool    R_GameLoop_IsDestroyed (const R_MainProvider* pProvider);

void R_MainProvider_Run (R_MainProvider* pProvider);
void R_MainProvider_Stop (R_MainProvider* pProvider);

void R_LaunchMainProvider (R_GameLoopCallback pExecCallback, const void* pUserData);
void R_PopulateApplicationInfo (R_ApplicationInfo* info, int argc, char** argv);
void R_InitializeApplicationInfo (R_ApplicationInfo* info, int argc, char** argv);
void R_BuildCommandLine (R_ApplicationInfo* info, int argc, char** argv);
void R_FillMemoryInfo (R_MemoryInfo* out);
void R_AssignProcessName (R_ProcessInfo* proc, char* exePath, int argc, char** argv);
