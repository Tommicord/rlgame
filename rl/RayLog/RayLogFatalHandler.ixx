export module Rl.RayLog.FatalHandler;

import Rl.RayLog.StackTrace;
import Rl.RayLog.SymbolDemangler;
import Rl.RayLog.PlatformOutput;
import <exception>;
import <string>;

namespace Rl::RayLog
{

export class RayLogFatalHandler
{
  public:
  [[noreturn]]
  static void Handle(const std::string& message)
  {
    const auto frames = RayLogStackTrace::Capture(1);
    const auto demangled = RayLogSymbolDemangler::DemangleStackTrace(frames);
    std::string fatalMessage = "FATAL ERROR: " + message + "\n";
    fatalMessage += demangled;
    RayLogPlatformOutput::WriteError(fatalMessage);
    std::terminate();
  }

  [[noreturn]]
  static void Handle()
  {
    Handle("Unknown fatal error");
  }
};

}
