export module Rl.RayLog.Message;

import Rl.RayLog.LevelPrinter;

import <string>;
import <chrono>;
import <thread>;

namespace Rl::RayLog
{

export struct RayLogMessage
{
  RayLogLevel                           level;
  std::string                           formattedMessage;
  std::thread::id                       threadId;
  std::chrono::system_clock::time_point timestamp;
  std::string                           tag;

  /* MUST INITIALIZE THE FIELDS MANUALLY */
  RayLogMessage() = default;

  /* Creates a message for the RayLog logging */
  RayLogMessage(const RayLogLevel level, std::string msg, std::string tag) :
      level(level), formattedMessage(std::move(msg)),
      threadId(std::this_thread::get_id()), timestamp(std::chrono::system_clock::now()),
      tag(std::move(tag))
  {
  }
};

} // namespace Rl::RayLog
