#include "Rl.CrashDump/CrashDumpRuntimeState.h"
#include "Rl.Log/Log.h"

#include <sstream>
#include <thread>
#include <set>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#elif defined(__linux__)
#include <unistd.h>
#include <fstream>
#include <sys/sysinfo.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task_info.h>
#include <pthread.h>
#endif

namespace rl
{

std::string CrashDumpRuntimeState::collectRuntimeState()
{
  std::ostringstream oss;

  oss << collectMemoryUsage();
  oss << collectThreadInfo();

#if defined(_WIN32)
  oss << collectWindowsModules();
#elif defined(__linux__)
  oss << collectLinuxModules();
#elif defined(__APPLE__)
  oss << collectMacOSModules();
#else
  oss << "Loaded Modules: (not implemented for this platform)\n";
#endif

  return oss.str();
}

std::string CrashDumpRuntimeState::collectWindowsModules()
{
  std::ostringstream oss;

  HMODULE hModules[1024];
  DWORD   cbNeeded;

  if (EnumProcessModules(GetCurrentProcess(), hModules, sizeof(hModules), &cbNeeded))
  {
    int count = static_cast<int>(cbNeeded / sizeof(HMODULE));
    oss << "Loaded Modules (" << count << "):\n";
    for (int i = 0; i < count; i++)
    {
      char moduleName[MAX_PATH];
      if (GetModuleFileNameExA(GetCurrentProcess(), hModules[i], moduleName, MAX_PATH))
      {
        MODULEINFO modInfo;
        if (GetModuleInformation(GetCurrentProcess(), hModules[i], &modInfo, sizeof(modInfo)))
        {
          oss << "  " << moduleName << " (base: 0x" << std::hex << modInfo.lpBaseOfDll
              << ", size: " << std::dec << modInfo.SizeOfImage << " bytes)\n";
        }
        else
        {
          oss << "  " << moduleName << "\n";
        }
      }
    }
  }
  else
  {
    oss << "Loaded Modules: Failed to enumerate (error code: " << GetLastError() << ")\n";
  }

  return oss.str();
}

std::string CrashDumpRuntimeState::collectLinuxModules()
{
  std::ostringstream oss;

  FILE* maps = fopen("/proc/self/maps", "r");
  if (maps)
  {
    std::set<std::string> uniqueModules;
    char                  line[1024];
    while (fgets(line, sizeof(line), maps))
    {
      char path[512] = "";
      sscanf(line, "%*x-%*x %*s %*s %*s %*s %511s", path);
      if (strlen(path) > 0 && path[0] == '/')
      {
        uniqueModules.insert(path);
      }
    }
    fclose(maps);

    oss << "Loaded Modules (" << uniqueModules.size() << "):\n";
    for (const auto& module : uniqueModules)
    {
      oss << "  " << module << "\n";
    }
  }
  else
  {
    oss << "Loaded Modules: Failed to read /proc/self/maps\n";
  }

  return oss.str();
}

std::string CrashDumpRuntimeState::collectMacOSModules()
{
#if defined(__APPLE__)
  std::ostringstream oss;

  uint32_t count = _dyld_image_count();
  oss << "Loaded Modules (" << count << "):\n";
  for (uint32_t i = 0; i < count; i++)
  {
    const char* imageName = _dyld_get_image_name(i);
    if (imageName)
    {
      const struct mach_header* header = _dyld_get_image_header(i);
      if (header)
      {
        oss << "  " << imageName << " (base: 0x" << std::hex << header << ")\n";
      }
      else
      {
        oss << "  " << imageName << "\n";
      }
    }
  }

  return oss.str();
#else
  // Not an Apple build: return a stub to avoid unresolved dyld symbols.
  std::ostringstream oss;
  oss << "Loaded Modules: macOS module listing not available in this build\n";
  return oss.str();
#endif
}

std::string CrashDumpRuntimeState::collectMemoryUsage()
{
  std::ostringstream oss;

#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS_EX memCounter;
  memCounter.cb = sizeof(memCounter);
  if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memCounter,
                           sizeof(memCounter)))
  {
    oss << "Memory Usage:\n";
    oss << "  Working Set: " << (memCounter.WorkingSetSize / (1024 * 1024)) << " MB\n";
    oss << "  Peak Working Set: " << (memCounter.PeakWorkingSetSize / (1024 * 1024)) << " MB\n";
    oss << "  Private Bytes: " << (memCounter.PrivateUsage / (1024 * 1024)) << " MB\n";
    oss << "  Page File Usage: " << (memCounter.PagefileUsage / (1024 * 1024)) << " MB\n";
  }
  else
  {
    oss << "Memory Usage: Failed to retrieve\n";
  }
#elif defined(__linux__)
  std::ifstream status("/proc/self/status");
  if (status.is_open())
  {
    oss << "Memory Usage:\n";
    std::string line;
    while (std::getline(status, line))
    {
      if (line.find("VmRSS:") == 0 || line.find("VmSize:") == 0 || line.find("VmPeak:") == 0 ||
          line.find("VmHWM:") == 0)
      {
        oss << "  " << line << "\n";
      }
    }
    status.close();
  }
  else
  {
    oss << "Memory Usage: Failed to read /proc/self/status\n";
  }
#elif defined(__APPLE__)
  struct task_basic_info info;
  mach_msg_type_number_t size = sizeof(info);
  if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS)
  {
    oss << "Memory Usage:\n";
    oss << "  Resident Size: " << (info.resident_size / (1024 * 1024)) << " MB\n";
    oss << "  Virtual Size: " << (info.virtual_size / (1024 * 1024)) << " MB\n";
  }
  else
  {
    oss << "Memory Usage: Failed to retrieve\n";
  }
#else
  oss << "Memory Usage: Not implemented for this platform\n";
#endif

  return oss.str();
}

std::string CrashDumpRuntimeState::collectThreadInfo()
{
  std::ostringstream oss;

  oss << "Thread Information:\n";
  oss << "  Current Thread ID: " << std::this_thread::get_id() << "\n";

#if defined(_WIN32)
  DWORD  threadCount = GetProcessId(GetCurrentProcess());
  HANDLE snapshot    = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot != INVALID_HANDLE_VALUE)
  {
    THREADENTRY32 te;
    te.dwSize                  = sizeof(THREADENTRY32);
    DWORD processID            = GetCurrentProcessId();
    int   threadCountInProcess = 0;

    if (Thread32First(snapshot, &te))
    {
      do
      {
        if (te.th32OwnerProcessID == processID)
        {
          threadCountInProcess++;
        }
      } while (Thread32Next(snapshot, &te));
    }
    CloseHandle(snapshot);

    oss << "  Total Threads in Process: " << threadCountInProcess << "\n";
  }
  else
  {
    oss << "  Total Threads in Process: Failed to count\n";
  }
#elif defined(__linux__)
  std::ifstream status("/proc/self/status");
  if (status.is_open())
  {
    std::string line;
    while (std::getline(status, line))
    {
      if (line.find("Threads:") == 0)
      {
        oss << "  " << line << "\n";
        break;
      }
    }
    status.close();
  }
  else
  {
    oss << "  Total Threads in Process: Failed to read /proc/self/status\n";
  }
#elif defined(__APPLE__)
  mach_msg_type_number_t count;
  thread_act_array_t     list;
  task_threads(mach_task_self(), &list, &count);
  oss << "  Total Threads in Process: " << count << "\n";
  vm_deallocate(mach_task_self(), (vm_address_t)list, count * sizeof(thread_act_t));
#else
  oss << "  Total Threads in Process: Not implemented for this platform\n";
#endif

  return oss.str();
}

} // namespace rl
