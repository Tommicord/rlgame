#include "Rl.CrashDump/CrashDumpFileSystem.h"
#include "Rl.Log/Log.h"

#include <fstream>
#include <random>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#endif

namespace rl
{

std::string CrashDumpFileSystem::getCrashDumpFolder()
{
#if defined(_WIN32)
  char path[MAX_PATH];
  if (SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path) == S_OK)
  {
    std::string appDataPath = path;
    appDataPath += "\\rlgame\\.crashdump";
    return appDataPath;
  }
  return ".crashdump";
#elif defined(__APPLE__)
  const char* homeDir = getenv("HOME");
  if (homeDir)
  {
    std::string path = homeDir;
    path += "/Library/Application Support/rlgame/.crashdump";
    return path;
  }
  return ".crashdump";
#elif defined(__linux__)
  const char* homeDir = getenv("HOME");
  if (homeDir)
  {
    std::string path = homeDir;
    path += "/.local/share/rlgame/.crashdump";
    return path;
  }
  return ".crashdump";
#else
  return ".crashdump";
#endif
}

bool CrashDumpFileSystem::ensureCrashDumpFolderExists()
{
  std::string folderPath = getCrashDumpFolder();
  Log::error("Crash dump folder path: %s", folderPath.c_str());

#if defined(_WIN32)
  int result = SHCreateDirectoryEx(nullptr, folderPath.c_str(), nullptr);
  if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS)
  {
    Log::error("Failed to create crash dump folder, error code: %d", result);
    return false;
  }
  return true;
#else
  size_t pos     = 0;
  bool   success = true;
  while ((pos = folderPath.find('/', pos)) != std::string::npos)
  {
    std::string subPath = folderPath.substr(0, pos);
    if (mkdir(subPath.c_str(), 0755) != 0 && errno != EEXIST)
    {
      Log::error("Failed to create intermediate directory: %s, error code: %d", subPath.c_str(),
                 errno);
      success = false;
    }
    pos++;
  }
  if (mkdir(folderPath.c_str(), 0755) != 0 && errno != EEXIST)
  {
    Log::error("Failed to create final crash dump folder: %s, error code: %d", folderPath.c_str(),
               errno);
    success = false;
  }
  return success;
#endif
}

std::string CrashDumpFileSystem::generateUUID()
{
  std::random_device              rd;
  std::mt19937                    gen(rd());
  std::uniform_int_distribution<> dis(0, 15);
  std::uniform_int_distribution<> dis2(8, 11);

  std::stringstream ss;
  ss << std::hex;

  for (int i = 0; i < 8; i++)
    ss << dis(gen);
  ss << "-";

  for (int i = 0; i < 4; i++)
    ss << dis(gen);
  ss << "-";

  ss << "4"; // version 4
  for (int i = 0; i < 3; i++)
    ss << dis(gen);
  ss << "-";

  ss << dis2(gen); // variant
  for (int i = 0; i < 3; i++)
    ss << dis(gen);
  ss << "-";

  for (int i = 0; i < 12; i++)
    ss << dis(gen);

  return ss.str();
}

std::string CrashDumpFileSystem::getPathSeparator()
{
#if defined(_WIN32)
  return "\\";
#else
  return "/";
#endif
}

} // namespace rl
