export module Rl.RayLog.FileOutput;

import Rl.RayLog.Uuid;
import <fstream>;
import <filesystem>;
import <string>;
import <mutex>;

namespace Rl::RayLog
{

export class RayLogFileOutput
{
  std::ofstream           logFile;
  std::mutex              fileMutex;
  std::string             logFilePath;
  size_t                  currentSize = 0;
  RayLogUuid              logId;
  static constexpr size_t MaxFileSize = 256 * 1024;

  public:
  RayLogFileOutput()
  {
    std::filesystem::create_directory(".log");
    logFilePath = ".log/" + logId.ToString() + ".log";
    logFile.open(logFilePath, std::ios::out | std::ios::app);
  }

  ~RayLogFileOutput()
  {
    if (logFile.is_open())
      logFile.close();
  }

  void Write(const std::string& message)
  {
    std::scoped_lock lock(fileMutex);
    if (!logFile.is_open())
      return;
    logFile << message << std::endl;
    currentSize += message.size() + 1;
    if (currentSize >= MaxFileSize)
      RotateFile();
  }

  private:
  void RotateFile()
  {
    logFile.close();
    currentSize = 0;
    logId = RayLogUuid();
    logFilePath = ".log/" + logId.ToString() + ".log";
    logFile.open(logFilePath, std::ios::out | std::ios::app);
  }
};

} // namespace Rl::RayLog
