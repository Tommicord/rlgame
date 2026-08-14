#include "Rl.CrashDump/CrashDump.h"
#include "Rl.CrashDump/CrashDumpFileSystem.h"
#include "Rl.CrashDump/CrashDumpSystemInfo.h"
#include "Rl.CrashDump/CrashDumpVulkanInfo.h"
#include "Rl.CrashDump/CrashDumpRenderDocInfo.h"
#include "Rl.CrashDump/CrashDumpApplicationContext.h"
#include "Rl.CrashDump/CrashDumpRuntimeState.h"
#include "Rl.CrashDump/CrashDumpGPUInfo.h"
#include "Rl.Log/Log.h"

#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vulkan/vulkan.hpp>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#endif

namespace rl
{

std::recursive_mutex CrashDump::dumpMutex;

std::string CrashDump::getCrashDumpFolder()
{
  std::scoped_lock lock(dumpMutex);
  return CrashDumpFileSystem::getCrashDumpFolder();
}

bool CrashDump::ensureCrashDumpFolderExists()
{
  std::scoped_lock lock(dumpMutex);
  return CrashDumpFileSystem::ensureCrashDumpFolderExists();
}

std::string CrashDump::generateUUID()
{
  std::scoped_lock lock(dumpMutex);
  return CrashDumpFileSystem::generateUUID();
}

std::string CrashDump::saveCrashDump(const std::string& title,
                                     const std::string& message,
                                     const std::string& backtrace,
                                     VkDevice           device,
                                     VkPhysicalDevice   physicalDevice,
                                     VkInstance         instance)
{
  std::scoped_lock lock(dumpMutex);

  if (!ensureCrashDumpFolderExists())
  {
    Log::error("Failed to create crash dump folder");
    return "";
  }

  std::string uuid       = generateUUID();
  std::string folderPath = getCrashDumpFolder();
  std::string separator  = CrashDumpFileSystem::getPathSeparator();
  std::string tempPath   = folderPath + separator + uuid + ".tmp";
  std::string filePath   = folderPath + separator + uuid + ".log";

  std::ofstream file(tempPath);
  if (!file.is_open())
  {
    Log::error("Failed to open crash dump temp file: %s", tempPath.c_str());
    return "";
  }

  size_t currentSize = 0;
  auto   writeSection =
      [&file, &currentSize](const std::string& sectionName, const std::string& content)
  {
    if (currentSize >= MAX_CRASH_DUMP_SIZE)
      return;

    std::string section = sectionName + "\n" + std::string(sectionName.length(), '-') + "\n";
    if (content.empty())
    {
      section += "[Not available or collection failed]\n";
    }
    else
    {
      section += content;
    }
    section += "\n";

    if (currentSize + section.length() > MAX_CRASH_DUMP_SIZE)
    {
      size_t remaining = MAX_CRASH_DUMP_SIZE - currentSize;
      file.write(section.c_str(), remaining);
      file << "\n[TRUNCATED: Maximum size of " << MAX_CRASH_DUMP_SIZE << " bytes reached]\n";
      currentSize = MAX_CRASH_DUMP_SIZE;
    }
    else
    {
      file << section;
      currentSize += section.length();
    }
  };

  auto now    = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::ostringstream timeStream;
  timeStream << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "."
             << std::setfill('0') << std::setw(3) << ms.count();
  std::string header = "Crash Report - " + timeStream.str() + "\n";
  header += "UUID: " + uuid + "\n";
  header += "========================================\n\n";

  if (currentSize + header.length() > MAX_CRASH_DUMP_SIZE)
  {
    file.write(header.c_str(), MAX_CRASH_DUMP_SIZE);
    currentSize = MAX_CRASH_DUMP_SIZE;
  }
  else
  {
    file << header;
    currentSize += header.length();
  }

  writeSection("SYSTEM INFORMATION", CrashDumpSystemInfo::collectSystemInfo());
  writeSection("APPLICATION CONTEXT", CrashDumpApplicationContext::collectApplicationContext());
  writeSection("VULKAN CONTEXT", CrashDumpVulkanInfo::collectVulkanContext());

  if (device != VK_NULL_HANDLE && physicalDevice != VK_NULL_HANDLE && instance != VK_NULL_HANDLE)
  {
    writeSection("GPU CRASH DUMP",
                 CrashDumpGPUInfo::collectGPUCrashDump(device, physicalDevice, instance));
  }

  writeSection("RUNTIME STATE", CrashDumpRuntimeState::collectRuntimeState());

#ifdef _RL_RENDERDOC_ENABLE
  writeSection("RENDERDOC CONTEXT", CrashDumpRenderDocInfo::collectRenderDocContext());
#endif

  writeSection("ERROR DETAILS", title + ": " + message);
  writeSection("BACKTRACE", backtrace);

  file.flush();
  file.close();
#if defined(_WIN32)
  if (!MoveFileExA(tempPath.c_str(), filePath.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
  {
    DWORD error = GetLastError();
    Log::error("Failed to rename crash dump file from %s to %s, error: %lu", tempPath.c_str(),
               filePath.c_str(), error);
    DeleteFileA(tempPath.c_str());
    return "";
  }
#else
  if (rename(tempPath.c_str(), filePath.c_str()) != 0)
  {
    Log::error("Failed to rename crash dump file from %s to %s, error: %d", tempPath.c_str(),
               filePath.c_str(), errno);
    unlink(tempPath.c_str());
    return "";
  }
#endif

#if defined(_WIN32)
  HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile != INVALID_HANDLE_VALUE)
  {
    FlushFileBuffers(hFile);
    CloseHandle(hFile);
  }
#else
  int fd = open(filePath.c_str(), O_WRONLY);
  if (fd >= 0)
  {
    fsync(fd);
    close(fd);
  }
#endif

  Log::error("Crash dump saved to: %s (size: %zu bytes)", filePath.c_str(), currentSize);
  return filePath;
}

} // namespace rl
