#include "Rl.Base/GameError.h"
#include "Rl.CrashDump/CrashDump.h"
#include "Rl.Log/Log.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vulkan/vulkan.hpp>

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#pragma comment(lib, "dbghelp.lib")
#elif defined(__linux__)
#include <cerrno>
#include <cstring>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#elif defined(__ANDROID__)
#include <android/log.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#endif

namespace rl
{

namespace
{
#if defined(_WIN32)
std::string captureBacktraceWindows()
{
        HANDLE process = GetCurrentProcess();

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        if (!SymInitialize(process, nullptr, TRUE))
        {
                return "[Failed to initialize symbol handler]";
        }

        void*  stack[64];
        USHORT start  = 3;
        USHORT frames = CaptureStackBackTrace(0, 64, stack, nullptr);

        std::ostringstream oss;
        oss << "Backtrace (" << frames - start << " frames):\n";

        for (USHORT i = start; i < frames; ++i)
        {
                DWORD64 address = (DWORD64)stack[i];

                SYMBOL_INFO* symbol =
                    (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
                symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                symbol->MaxNameLen   = 255;

                IMAGEHLP_LINE64 line = {};
                line.SizeOfStruct    = sizeof(IMAGEHLP_LINE64);
                DWORD displacement   = 0;

                if (SymFromAddr(process, address, nullptr, symbol))
                {
                        oss << "  #" << std::setw(2) << i - start << " " << symbol->Name << " at 0x"
                            << std::hex << symbol->Address << std::dec;

                        if (SymGetLineFromAddr64(process, address, &displacement, &line))
                        {
                                oss << " (" << line.FileName << ":" << line.LineNumber << ")";
                        }
                        oss << "\n";
                }
                else
                {
                        oss << "  #" << std::setw(2) << i - start << " ??? at 0x" << std::hex
                            << address << std::dec << "\n";
                }

                free(symbol);
        }

        SymCleanup(process);
        return oss.str();
}
#elif defined(__linux__) || defined(__APPLE__)
std::string captureBacktraceUnix()
{
        void*       stack[64];
        size_t start  = 3;
        int         frames = backtrace(stack, 64);

        std::ostringstream oss;
        oss << "Backtrace (" << frames - start << " frames):\n";

        char** symbols = backtrace_symbols(stack, frames);
        if (symbols)
        {
                for (int i = start; i < frames; ++i)
                {
                        oss << "  #" << std::setw(2) << i - start << " ";

                        std::string symbolStr(symbols[i]);

                        size_t mangleStart = symbolStr.find('(');
                        size_t mangleEnd   = symbolStr.find('+', mangleStart);

                        if (mangleStart != std::string::npos && mangleEnd != std::string::npos)
                        {
                                std::string mangled =
                                    symbolStr.substr(mangleStart + 1, mangleEnd - mangleStart - 1);
                                int   status = 0;
                                char* demangled =
                                    abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
                                if (demangled && status == 0)
                                {
                                        oss << demangled;
                                        free(demangled);
                                        oss << symbolStr.substr(mangleEnd);
                                }
                                else
                                {
                                        oss << symbolStr;
                                }
                        }
                        else
                        {
                                oss << symbolStr;
                        }
                        oss << "\n";
                }
                free(symbols);
        }

        return oss.str();
}
#endif
} // namespace

std::string GameError::captureBacktrace()
{
#if defined(_WIN32)
        return captureBacktraceWindows();
#elif defined(__linux__) || defined(__APPLE__)
        return captureBacktraceUnix();
#else
        return "[Backtrace not supported on this platform]";
#endif
}

std::string GameError::vulkanResultToString(int64_t result)
{
        switch (result)
        {
        case 0:
                return "VK_SUCCESS";
        case 1:
                return "VK_NOT_READY";
        case 2:
                return "VK_TIMEOUT";
        case 3:
                return "VK_EVENT_SET";
        case 4:
                return "VK_EVENT_RESET";
        case 5:
                return "VK_INCOMPLETE";
        case -1:
                return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case -2:
                return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case -3:
                return "VK_ERROR_INITIALIZATION_FAILED";
        case -4:
                return "VK_ERROR_DEVICE_LOST";
        case -5:
                return "VK_ERROR_MEMORY_MAP_FAILED";
        case -6:
                return "VK_ERROR_LAYER_NOT_PRESENT";
        case -7:
                return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case -8:
                return "VK_ERROR_FEATURE_NOT_PRESENT";
        case -9:
                return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case -10:
                return "VK_ERROR_TOO_MANY_OBJECTS";
        case -11:
                return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case -12:
                return "VK_ERROR_FRAGMENTED_POOL";
        case -13:
                return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case -14:
                return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case -1000000001:
                return "VK_ERROR_SURFACE_LOST_KHR";
        case -1000000002:
                return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case 1000000003:
                return "VK_SUBOPTIMAL_KHR";
        case -1000000004:
                return "VK_ERROR_OUT_OF_DATE_KHR";
        case -1000000005:
                return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case -1000011001:
                return "VK_ERROR_VALIDATION_FAILED_EXT";
        case -1000012000:
                return "VK_ERROR_INVALID_SHADER_NV";
        case -1000157000:
                return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case -1000174001:
                return "VK_ERROR_NOT_PERMITTED_EXT";
        case -1000255000:
                return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        default:
                return "Unknown Vulkan error (" + std::to_string(result) + ")";
        }
}

void GameError::exitWithError(const std::string& title, const std::string& message)
{
        exitWithError(title, message, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
}

void GameError::exitWithError(const std::string& title,
                                             const std::string& message,
                                             VkDevice           device,
                                             VkPhysicalDevice   physicalDevice,
                                             VkInstance         instance)
{
        std::string backtrace = captureBacktrace();
        std::string crashDumpPath =
            CrashDump::saveCrashDump(title, message, backtrace, device, physicalDevice, instance);

        char buffer[4096];
        snprintf(buffer, sizeof(buffer), "%s\n\n%s", message.c_str(), backtrace.c_str());

        if (!crashDumpPath.empty())
        {
                Log::error("Crash dump saved to: %s", crashDumpPath.c_str());
        }

        Log::error("%s: %s", title.c_str(), buffer);
#if defined(_WIN32)
        MessageBoxA(nullptr, buffer, title.c_str(), MB_OK | MB_ICONERROR);
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
                CFStringRef logString = CFStringCreateWithFormat(
                    kCFAllocatorDefault, nullptr, CFSTR("%@: %@"), titleRef, messageRef);
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

void GameError::exitWithError(const std::string& message)
{
        exitWithError("Fatal Error", message);
}

} // namespace rl
