export module Rl.RayLog.Message;

import Rl.RayLog.LogLevel;
import <string>;
import <chrono>;
import <thread>;

namespace Rl::RayLog
{

export struct RayLogMessage
{
  RayLogLevel level;
  std::string formattedMessage;
  std::thread::id threadId;
  std::chrono::system_clock::time_point timestamp;
  std::string tag;

  RayLogMessage(RayLogLevel lvl, std::string msg, std::string t)
    : level(lvl), formattedMessage(std::move(msg)), threadId(std::this_thread::get_id()),
      timestamp(std::chrono::system_clock::now()), tag(std::move(t))
  {}
};

}
