export module Rl.RayLog.FileOutput;

import Rl.RayLog.Uuid;
import Rl.RayLog.Config;
import <fstream>;
import <filesystem>;
import <string>;
import <mutex>;

namespace Rl::RayLog
{

export class RayLogFileOutput
{
  /* The log file */
  std::ofstream           logf;

  /* The file mutex for synchronization */
  std::mutex              fileMutex;

  /* The file path of the log file */
  std::string             logfp;

  /* The current size of the log file */
  size_t                  currentSize = 0;

  /* The UUID of the log */
  RayLogUuid              logUuid;

  public:
  RayLogFileOutput()
  {
    // This should not fail
    std::filesystem::create_directory(".log");
    logfp = ".log/" + logUuid.ToString() + ".log";
    logf.open(logfp, std::ios::out | std::ios::app);
  }

  ~RayLogFileOutput()
  {
    if (logf.is_open())
      logf.close();
  }

  void Write(const std::string& message)
  {
    std::scoped_lock lock(fileMutex);
    if (!logf.is_open())
      return;
    logf << message << std::endl;
    currentSize += message.size() + 1;
    if (currentSize >= MaxFileSize)
      RotateFile();
  }

  private:
  void RotateFile()
  {
    logf.close();
    currentSize = 0;
    logUuid = RayLogUuid();
    std::sstr
    logfp = ".log/" + logUuid.ToString() + ".log";
    logf.open(logfp, std::ios::out | std::ios::app);
  }
};

} // namespace Rl::RayLog
