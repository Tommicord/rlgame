#ifndef RL_BASE_FATAL_ERROR_HANDLER_H
#define RL_BASE_FATAL_ERROR_HANDLER_H

#include <string>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vulkan/vulkan.hpp>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFShow.h>
#elif defined(__ANDROID__)
#include <android/log.h>
#endif

#include "Rl.CrashDump/CrashDump.h"
#include "Rl.Log/Log.h"

namespace rl
{

/** Fatal error logging and backtrace printing for failures like
 *  Vulkan creation errors or internal Game errors
 */
class GameError
{
  public:
    /** Handles a fatal error with a title and message
     * @param title Error title for display
     * @param message Detailed error message
     *
     * This function logs the error with a backtrace and displays a platform-specific
     * error dialog before terminating the application.
     */
    template <typename... Args>
    static void exitWithError(const std::string& title, const std::string& message, Args... args);

    /** Handles a fatal error with a message only
     * @param message Detailed error message
     *
     * This function logs the error with a backtrace and displays a platform-specific
     * error dialog with a default "Fatal Error" title before terminating the
     * application.
     */
    template <typename... Args> static void exitWithError(const std::string& message, Args... args);

    /** Handles a fatal error with a title and message (non-template overload)
     * @param title Error title for display
     * @param message Detailed error message
     */
    static void exitWithError(const std::string& title, const std::string& message);

    /** Handles a fatal error with a title, message, and Vulkan handles for GPU crash
     * dump
     * @param title Error title for display
     * @param message Detailed error message
     * @param device Vulkan device handle
     * @param physicalDevice Vulkan physical device handle
     * @param instance Vulkan instance handle
     */
    static void exitWithError(const std::string& title,
                              const std::string& message,
                              VkDevice           device,
                              VkPhysicalDevice   physicalDevice,
                              VkInstance         instance);

    /** Handles a fatal error with a message only (non-template overload)
     * @param message Detailed error message
     */
    static void exitWithError(const std::string& message);

    /** Converts a Vulkan result code to a human-readable string
     * @param result Vulkan result code
     * @return String representation of the Vulkan result
     */
    static std::string vulkanResultToString(int64_t result);

  private:
    /** Captures the current call stack as a string
     * @return String representation of the backtrace
     *
     * This function captures the current call stack and formats it as a readable
     * string for logging purposes. The format is platform-dependent.
     */
    static std::string captureBacktrace();
};

template <typename... Args>
void GameError::exitWithError(const std::string& title, const std::string& message, Args... args)
{
  std::string backtrace = captureBacktrace();
  char        formattedMessage[4096];
  snprintf(formattedMessage, sizeof(formattedMessage), message.c_str(), args...);

  std::string crashDumpPath = CrashDump::saveCrashDump(title, formattedMessage, backtrace);
  char        buffer[4096];
  snprintf(buffer, sizeof(buffer), "%s\n\n%s", formattedMessage, backtrace.c_str());
  Log::error(buffer);
  if (!crashDumpPath.empty())
  {
    Log::error("Crash dump saved to: %s", crashDumpPath.c_str());
  }
#if defined(_WIN32)
  MessageBox(nullptr, buffer, title.c_str(), MB_OK | MB_ICONERROR);
  exit(EXIT_FAILURE);
#elif defined(__ANDROID__)
  __android_log_print(ANDROID_LOG_FATAL, "rlgame", "%s: %s", title.c_str(), buffer);
  exit(EXIT_FAILURE);
#elif defined(__APPLE__)
  CFStringRef titleRef =
      CFStringCreateWithCString(kCFAllocatorDefault, title.c_str(), kCFStringEncodingUTF8);
  CFStringRef messageRef =
      CFStringCreateWithCString(kCFAllocatorDefault, buffer, kCFStringEncodingUTF8);

  if (titleRef && messageRef)
  {
    CFStringRef logString = CFStringCreateWithFormat(kCFAllocatorDefault, nullptr, CFSTR("%@: %@"),
                                                     titleRef, messageRef);
    if (logString)
    {
      CFShow(logString);
      CFRelease(logString);
    }
  }

  if (titleRef)
    CFRelease(titleRef);
  if (messageRef)
    CFRelease(messageRef);
  exit(EXIT_FAILURE);
#else
  std::cerr << "FATAL ERROR: " << title << ": " << buffer << std::endl;
  exit(EXIT_FAILURE);
#endif
}

template <typename... Args> void GameError::exitWithError(const std::string& message, Args... args)
{
  char formattedMessage[4096];
  snprintf(formattedMessage, sizeof(formattedMessage), message.c_str(), args...);
  GameError::exitWithError("Fatal Error", std::string(formattedMessage));
}

} // namespace rl

#endif // RL_BASE_FATAL_ERROR_HANDLER_H
