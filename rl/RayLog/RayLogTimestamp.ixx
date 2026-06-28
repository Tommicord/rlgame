export module Rl.RayLog.Timestamp;

import <chrono>;
import <sstream>;
import <iomanip>;
import <thread>;
import <string>;

namespace Rl::RayLog
{

export class RayLogTimestamp
{
  public:
  [[nodiscard]]
  static std::string Format()
  {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%m-%d-%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
  }

  [[nodiscard]]
  static std::string GetThreadId()
  {
    std::stringstream ss;
    ss << std::this_thread::get_id();
    return ss.str();
  }
};

}
