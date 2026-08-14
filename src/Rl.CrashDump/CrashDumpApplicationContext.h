#ifndef RL_CRASHDUMP_CRASH_DUMP_APPLICATION_CONTEXT_H
#define RL_CRASHDUMP_CRASH_DUMP_APPLICATION_CONTEXT_H

#include <string>

namespace rl
{

/** Application context collector for crash reports
 *
 * This module collects application-specific information including
 * build configuration, executable path, and command line arguments.
 */
class CrashDumpApplicationContext
{
  public:
    /** Collects application context information for the crash report
     * @return String containing formatted application context information
     */
    static std::string collectApplicationContext();

  private:
    /** Gets the build configuration (Debug/Release)
     * @return String containing build configuration
     */
    static std::string getBuildConfiguration();

    /** Gets the executable path for the current platform
     * @return String containing executable path
     */
    static std::string getExecutablePath();

    /** Gets command line arguments
     * @return String containing command line arguments
     */
    static std::string getCommandLineArgs();
};

} // namespace rl

#endif // RL_CRASHDUMP_CRASH_DUMP_APPLICATION_CONTEXT_H
