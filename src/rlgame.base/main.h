#pragma once

#include "rlgame.base/cstl/cstl_string.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Game loop state flags used to control and monitor the game loop lifecycle
 *
 * These flags are used in combination to represent the current state of the game loop.
 * Multiple flags can be set simultaneously using bitwise operations.
 */
enum r_game_loop_state_flags
{
    R_GAMELOOP_STATE_NONE = 0x00, ///< No state flags set
    R_GAMELOOP_STATE_RUNNING = 0x01, ///< Loop is actively running and processing frames
    R_GAMELOOP_STATE_PAUSED = 0x02, ///< Loop is paused (update suspended but loop continues)
    R_GAMELOOP_STATE_DESTROYED = 0x04, ///< Loop is being destroyed/cleanup in progress
    R_GAMELOOP_STATE_RESUMED = 0x08, ///< Loop was just resumed from pause
    R_GAMELOOP_STATE_SUSPENDED = 0x10, ///< Loop is suspended (background, low resource usage)
    R_GAMELOOP_STATE_ERROR = 0x20, ///< Error state, loop may need recovery
    R_GAMELOOP_STATE_SHUTDOWN = 0x40, ///< Shutdown requested, loop should exit
    R_GAMELOOP_STATE_ALL = 0x7F ///< All state flags combined
};

/**
 * @brief Information about a running process
 *
 * Contains process identification and resource usage information.
 * Used for monitoring and debugging purposes.
 */
struct r_process_info
{
        uint32_t                    pid; ///< Process ID
        const struct r_cstl_string* pName; ///< Process/executable name (UTF-8 string)
        const struct r_cstl_string* pUser; ///< Owning user or service name (if available)
        uint64_t                    startTimeMs; ///< Start time in epoch milliseconds, 0 if unknown
        uint64_t                    memoryBytes; ///< Working set / RSS in bytes, 0 if unknown
};

/**
 * @brief System memory information snapshot
 *
 * Contains information about system-wide and application-specific memory usage.
 * Values are snapshots at the time of collection and may change over time.
 */
struct r_memory_info
{
        uint64_t totalPhysicalBytes; ///< Total physical RAM in bytes
        uint64_t availablePhysicalBytes; ///< Available RAM in bytes
        uint64_t totalVirtualBytes; ///< Total virtual address space in bytes
        uint64_t usedBytes; ///< Total used memory (approximate) in bytes
        size_t   heapAllocatedBytes; ///< Heap allocation tracked by app in bytes (optional)
        size_t   heapReservedBytes; ///< Reserved heap size in bytes (optional)
};

/**
 * @brief Application command-line arguments
 *
 * Contains the parsed command-line arguments passed to the application.
 * The argv pointer is not owned by this structure, but pCmdLine is.
 */
struct r_application_args
{
        int                argc; ///< Argument count
        const char* const* argv; ///< Pointer to argv array (not owned by this struct)
        char*              pCmdLine; ///< Optional full command-line string (owned, heap-allocated)
};

/**
 * @brief Comprehensive application information
 *
 * Contains all relevant information about the application instance including
 * version, process ID, command-line arguments, memory usage, and related processes.
 * This structure is populated during application initialization.
 */
struct r_application_info
{
        const struct r_cstl_string* pApplicationName; /**< Short application name (UTF-8 string) */
        uint32_t                    applicationVersionMajor; /**< Major version number */
        uint32_t                    applicationVersionMinor; /**< Minor version number */
        uint32_t                    applicationVersionPatch; /**< Patch version number */
        uint32_t                    pid; /**< Current process ID */
        const struct r_process_info*
                                  pExistingProcesses; /**< Optional array of related processes (not owned) */
        size_t                    existingProcessCount; /**< Number of entries in pExistingProcesses */
        struct r_application_args args; /**< Startup arguments */
        struct r_memory_info      memory; /**< Snapshot of memory usage at init */
};

/**
 * @brief Game loop callback function type
 *
 * This callback is invoked each frame of the game loop.
 *
 * @param pAppInfo Pointer to application information structure
 * @param pUserData Optional user data pointer (can be NULL)
 * @return true to continue the game loop, false to request shutdown
 */
typedef bool (*const r_game_callback) (const struct r_application_info* pAppInfo, void* pUserData);

/**
 * @brief Main game loop provider
 *
 * Manages the game loop lifecycle, state, and execution callback.
 * The stateFlags field is atomic and should be accessed using the provided
 * state management functions for thread safety.
 */
struct r_main_provider
{
        r_game_callback                  pExecCallback; /**< Pointer to game loop function */
        const struct r_application_info* pAppInfo; /**< Pointer to application info (not owned) */
        void*                            pUserData; /**< Optional user data pointer (not owned) */
        volatile uint8_t                 stateFlags; /**< Atomic state flags (r_game_loop_state_flags) */
};

/**
 * @brief Sets game loop state flags using atomic operations
 *
 * Thread-safe operation to set specific state flags on the provider.
 *
 * @param pProvider Pointer to the main provider
 * @param flags State flags to set (bitwise OR of r_game_loop_state_flags)
 */
void r_game_loop_set_state (struct r_main_provider* pProvider, uint8_t flags);

/**
 * @brief Clears game loop state flags using atomic operations
 *
 * Thread-safe operation to clear specific state flags from the provider.
 *
 * @param pProvider Pointer to the main provider
 * @param flags State flags to clear (bitwise OR of r_game_loop_state_flags)
 */
void r_game_loop_clear_state (struct r_main_provider* pProvider, uint8_t flags);

/**
 * @brief Gets the current game loop state flags
 *
 * Thread-safe operation to retrieve all current state flags.
 *
 * @param pProvider Pointer to the main provider
 * @return Current state flags (bitwise combination of r_game_loop_state_flags)
 */
uint8_t r_game_loop_get_state (const struct r_main_provider* pProvider);

/**
 * @brief Checks if specific state flags are set
 *
 * Thread-safe operation to check if all specified flags are currently set.
 *
 * @param pProvider Pointer to the main provider
 * @param flags State flags to check (bitwise OR of r_game_loop_state_flags)
 * @return true if all specified flags are set, false otherwise
 */
bool r_game_loop_has_state (const struct r_main_provider* pProvider, uint8_t flags);

/**
 * @brief Checks if the game loop is currently running
 *
 * Convenience function to check R_GAMELOOP_STATE_RUNNING flag.
 *
 * @param pProvider Pointer to the main provider
 * @return true if the loop is running, false otherwise
 */
bool r_game_loop_is_running (const struct r_main_provider* pProvider);

/**
 * @brief Checks if the game loop is currently paused
 *
 * Convenience function to check R_GAMELOOP_STATE_PAUSED flag.
 *
 * @param pProvider Pointer to the main provider
 * @return true if the loop is paused, false otherwise
 */
bool r_game_loop_is_paused (const struct r_main_provider* pProvider);

/**
 * @brief Checks if the game loop is being destroyed
 *
 * Convenience function to check R_GAMELOOP_STATE_DESTROYED flag.
 *
 * @param pProvider Pointer to the main provider
 * @return true if the loop is being destroyed, false otherwise
 */
bool r_game_loop_is_destroyed (const struct r_main_provider* pProvider);

/**
 * @brief Runs the main game loop
 *
 * Enters the main game loop, processing messages and invoking the execution callback
 * each frame. The loop continues until the callback returns false or shutdown is requested.
 * Platform-specific message handling is integrated (Windows messages, etc.).
 *
 * @param pProvider Pointer to the main provider with configured callback and app info
 */
void r_main_provider_Run (struct r_main_provider* pProvider);

/**
 * @brief Stops the main game loop
 *
 * Requests the game loop to stop by clearing the running state and setting shutdown flag.
 * The loop will exit on its next iteration check.
 *
 * @param pProvider Pointer to the main provider
 */
void r_main_provider_Stop (struct r_main_provider* pProvider);

/**
 * @brief Launches the main provider with a game callback
 *
 * Creates and runs a main provider with the specified callback and user data.
 * This is a convenience function that wraps provider creation and execution.
 *
 * @param pExecCallback Game loop callback function to invoke each frame
 * @param pUserData User data pointer (typically r_application_info*)
 */
void r_launch_main_provider (r_game_callback pExecCallback, const void* pUserData);

/**
 * @brief Populates application information from command-line arguments
 *
 * Comprehensive function that initializes all fields of ApplicationInfo including
 * process ID, memory info, command-line arguments, and related processes.
 *
 * @param info Pointer to ApplicationInfo structure to populate
 * @param argc Argument count from main()
 * @param argv Argument array from main()
 */
void r_populate_application_info (struct r_application_info* info, int argc, char** argv);

/**
 * @brief Initializes basic application information fields
 *
 * Sets up the application name, version, and process ID.
 * Does not populate memory or process information.
 *
 * @param info Pointer to ApplicationInfo structure to initialize
 * @param argc Argument count from main()
 * @param argv Argument array from main()
 */
void r_initialize_application_info (struct r_application_info* info, int argc, char** argv);

/**
 * @brief Builds a command-line string from argc/argv
 *
 * Creates a heap-allocated string containing the full command line.
 * The result is stored in info->args.pCmdLine and owned by the structure.
 *
 * @param info Pointer to ApplicationInfo structure with args field
 * @param argc Argument count from main()
 * @param argv Argument array from main()
 */
void r_build_command_line (struct r_application_info* info, int argc, char** argv);

/**
 * @brief Fills memory information structure with current system stats
 *
 * Populates the MemoryInfo structure with current system-wide and
 * application-specific memory usage statistics.
 *
 * @param out Pointer to MemoryInfo structure to populate
 */
void r_fill_memory_info (struct r_memory_info* out);

/**
 * @brief Assigns a process name to a ProcessInfo structure
 *
 * Sets the process name field from the executable path or argv[0].
 * The name string is heap-allocated and should be freed by the caller.
 *
 * @param pProc Pointer to ProcessInfo structure
 * @param pExePath Executable path (can be NULL)
 * @param argc Argument count from main()
 * @param argv Argument array from main()
 */
void r_assign_process_name (
    struct r_process_info*      pProc,
    const struct r_cstl_string* pExePath,
    int                         argc,
    char**                      argv);
