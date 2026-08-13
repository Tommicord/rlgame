#include "Rl.CrashDump/CrashDumpSystemInfo.h"
#include "Rl.Log/Log.h"

#include <sstream>
#include <thread>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/sysinfo.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/sysctl.h>
#endif

namespace rl
{

std::string CrashDumpSystemInfo::collectSystemInfo()
{
#if defined(_WIN32)
        return collectWindowsSystemInfo();
#elif defined(__linux__)
        return collectLinuxSystemInfo();
#elif defined(__APPLE__)
        return collectMacOSSystemInfo();
#else
        std::ostringstream oss;
        oss << "OS: Unknown\n";
        oss << "Process ID: " << getpid() << "\n";
        oss << "Thread ID: " << std::this_thread::get_id() << "\n";
        return oss.str();
#endif
}

std::string CrashDumpSystemInfo::collectWindowsSystemInfo()
{
        std::ostringstream oss;

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);

        oss << "OS: Windows\n";
        oss << "CPU Cores: " << sysInfo.dwNumberOfProcessors << "\n";
        oss << "Total Memory: " << (memInfo.ullTotalPhys / (1024 * 1024)) << " MB\n";
        oss << "Available Memory: " << (memInfo.ullAvailPhys / (1024 * 1024)) << " MB\n";
        oss << "Process ID: " << GetCurrentProcessId() << "\n";
        oss << "Thread ID: " << GetCurrentThreadId() << "\n";

        return oss.str();
}

std::string CrashDumpSystemInfo::collectLinuxSystemInfo()
{
#if defined(__linux__) || defined(__linux)
        std::ostringstream oss;

        oss << "OS: Linux\n";
        oss << "Process ID: " << getpid() << "\n";
        oss << "Thread ID: " << std::this_thread::get_id() << "\n";

        struct sysinfo info;
        if (sysinfo(&info) == 0)
        {
                oss << "Total Memory: " << (info.totalram * info.mem_unit / (1024 * 1024))
                    << " MB\n";
                oss << "Available Memory: " << (info.freeram * info.mem_unit / (1024 * 1024))
                    << " MB\n";
        }

        long numCores = sysconf(_SC_NPROCESSORS_ONLN);
        if (numCores > 0)
        {
                oss << "CPU Cores: " << numCores << "\n";
        }

        return oss.str();
#else
        // Not a Linux build: provide a portable stub so this translation unit
        // compiles cleanly on other platforms.
        std::ostringstream oss;
        oss << "OS: Linux (not available in this build)\n";
        oss << "Process ID: " << getpid() << "\n";
        oss << "Thread ID: " << std::this_thread::get_id() << "\n";
        return oss.str();
#endif
}

std::string CrashDumpSystemInfo::collectMacOSSystemInfo()
{
#if defined(__APPLE__)
        std::ostringstream oss;

        oss << "OS: macOS\n";
        oss << "Process ID: " << getpid() << "\n";
        oss << "Thread ID: " << std::this_thread::get_id() << "\n";

        int     mib[2];
        int64_t physicalMemory;
        size_t  length;

        mib[0] = CTL_HW;
        mib[1] = HW_MEMSIZE;
        length = sizeof(int64_t);
        sysctl(mib, 2, &physicalMemory, &length, nullptr, 0);
        oss << "Total Memory: " << (physicalMemory / (1024 * 1024)) << " MB\n";

        int numCores;
        mib[1] = HW_NCPU;
        length = sizeof(int);
        sysctl(mib, 2, &numCores, &length, nullptr, 0);
        oss << "CPU Cores: " << numCores << "\n";

        return oss.str();
#else
        // Not an Apple build: provide a portable stub to avoid unresolved symbols
        std::ostringstream oss;
        oss << "OS: macOS (not available in this build)\n";
        oss << "Process ID: " << getpid() << "\n";
        oss << "Thread ID: " << std::this_thread::get_id() << "\n";
        return oss.str();
#endif
}

} // namespace rl
