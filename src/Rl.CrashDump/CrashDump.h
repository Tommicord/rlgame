#ifndef RL_BASE_FATAL_CRASH_DUMP_H
#define RL_BASE_FATAL_CRASH_DUMP_H

#include <string>
#include <mutex>
#include <vulkan/vulkan.hpp>
#include <mutex>

namespace rl
{

/** Crash dump manager for saving fatal error reports to disk
 *
 * This class handles saving crash reports including error messages,
 * backtraces, and metadata to a .crashdump folder with UUID-based filenames.
 * This allows post-mortem analysis of crashes, especially for intermittent
 * issues like VK_ERROR_DEVICE_LOST.
 *
 * Thread-safe: All operations are protected by a static mutex.
 * File size limit: Crash dumps are limited to 128 KB to prevent disk filling.
 */
class CrashDump
{
  public:
    /** Maximum crash dump file size in bytes (128 KB) */
    static constexpr size_t MAX_CRASH_DUMP_SIZE = 128 * 1024;

    /** Saves a crash report to disk
     * @param title Error title
     * @param message Detailed error message
     * @param backtrace Call stack backtrace
     * @param device Optional Vulkan device handle for GPU crash dump
     * @param physicalDevice Optional Vulkan physical device handle for GPU crash dump
     * @param instance Optional Vulkan instance handle for GPU crash dump
     * @return Path to the saved crash dump file, or empty string on failure
     *
     * This function creates a .crashdump folder if it doesn't exist,
     * generates a UUID for the filename, and writes the crash report
     * with timestamp, error details, and backtrace. If Vulkan handles
     * are provided, GPU crash dump information will be included.
     *
     * Thread-safe: Uses atomic temp file + rename pattern.
     * Size limit: Truncates output at MAX_CRASH_DUMP_SIZE bytes.
     */
    static std::string saveCrashDump(const std::string& title,
                                     const std::string& message,
                                     const std::string& backtrace,
                                     VkDevice           device         = VK_NULL_HANDLE,
                                     VkPhysicalDevice   physicalDevice = VK_NULL_HANDLE,
                                     VkInstance         instance       = VK_NULL_HANDLE);

    /** Gets the crash dump folder path
     * @return Path to the .crashdump folder
     */
    static std::string getCrashDumpFolder();
    static bool        ensureCrashDumpFolderExists();
    static std::string generateUUID();

  private:
    static std::recursive_mutex dumpMutex;
};

} // namespace rl

#endif // RL_BASE_FATAL_CRASH_DUMP_H
