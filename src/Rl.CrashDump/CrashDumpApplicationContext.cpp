#include "Rl.CrashDump/CrashDumpApplicationContext.h"
#include "Rl.Log/Log.h"

#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include <limits.h>
#include <mach-o/dyld.h>
#endif

namespace rl
{

std::string CrashDumpApplicationContext::collectApplicationContext()
{
  std::ostringstream oss;

  oss << getBuildConfiguration();
  oss << getExecutablePath();
  oss << getCommandLineArgs();

  return oss.str();
}

std::string CrashDumpApplicationContext::getBuildConfiguration()
{
  std::ostringstream oss;

#ifdef _DEBUG
  oss << "Build Configuration: Debug\n";
#else
  oss << "Build Configuration: Release\n";
#endif

#ifdef NDEBUG
  oss << "Assertions: Disabled\n";
#else
  oss << "Assertions: Enabled\n";
#endif

  return oss.str();
}

std::string CrashDumpApplicationContext::getExecutablePath()
{
  std::ostringstream oss;

#if defined(_WIN32)
  char  exePath[MAX_PATH];
  DWORD result = GetModuleFileName(nullptr, exePath, MAX_PATH);
  if (result > 0 && result < MAX_PATH)
  {
    oss << "Executable: " << exePath << "\n";
  }
  else
  {
    oss << "Executable: Failed to retrieve path\n";
  }
#elif defined(__linux__)
  char    exePath[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", exePath, PATH_MAX - 1);
  if (count != -1)
  {
    exePath[count] = '\0';
    oss << "Executable: " << exePath << "\n";
  }
  else
  {
    oss << "Executable: Failed to retrieve path\n";
  }
#elif defined(__APPLE__)
  char     exePath[PATH_MAX];
  uint32_t bufsize = PATH_MAX;
  if (_NSGetExecutablePath(exePath, &bufsize) == 0)
  {
    oss << "Executable: " << exePath << "\n";
  }
  else
  {
    oss << "Executable: Failed to retrieve path (buffer too small)\n";
  }
#else
  oss << "Executable: Not implemented for this platform\n";
#endif

  return oss.str();
}

std::string CrashDumpApplicationContext::getCommandLineArgs()
{
  std::ostringstream oss;

#if defined(_WIN32)
  int     argc;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv)
  {
    oss << "Command Line Arguments (" << argc << "):\n";
    for (int i = 0; i < argc; i++)
    {
      int size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
      if (size > 0)
      {
        std::string arg(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &arg[0], size, nullptr, nullptr);
        oss << "  [" << i << "] " << arg << "\n";
      }
    }
    LocalFree(argv);
  }
  else
  {
    oss << "Command Line Arguments: Failed to retrieve\n";
  }
#elif defined(__linux__) || defined(__APPLE__)
  // On Unix-like systems, we can access /proc/self/cmdline on Linux
  // On macOS, we'd need to use _NSGetExecutablePath or similar
#if defined(__linux__)
  std::ifstream cmdline("/proc/self/cmdline");
  if (cmdline.is_open())
  {
    std::string arg;
    int         count = 0;
    oss << "Command Line Arguments:\n";
    while (std::getline(cmdline, arg, '\0'))
    {
      if (!arg.empty())
      {
        oss << "  [" << count++ << "] " << arg << "\n";
      }
    }
    cmdline.close();
  }
  else
  {
    oss << "Command Line Arguments: Failed to retrieve\n";
  }
#else
  oss << "Command Line Arguments: Not implemented for this platform\n";
#endif
#else
  oss << "Command Line Arguments: Not implemented for this platform\n";
#endif

  return oss.str();
}

} // namespace rl
