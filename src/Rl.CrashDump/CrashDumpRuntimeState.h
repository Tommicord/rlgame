#ifndef RL_CRASHDUMP_CRASH_DUMP_RUNTIME_STATE_H
#define RL_CRASHDUMP_CRASH_DUMP_RUNTIME_STATE_H

#include <string>

namespace rl
{

/** Runtime state collector for crash reports
 *
 * This module collects runtime state information including
 * loaded modules, memory usage, and thread information.
 */
class CrashDumpRuntimeState
{
        public:
                /** Collects runtime state information for the crash report
                 * @return String containing formatted runtime state information
                 */
                static std::string collectRuntimeState();

        private:
                /** Collects loaded modules for Windows
                 * @return String containing loaded module list
                 */
                static std::string collectWindowsModules();

                /** Collects loaded modules for Linux
                 * @return String containing loaded module list
                 */
                static std::string collectLinuxModules();

                /** Collects loaded modules for macOS
                 * @return String containing loaded module list
                 */
                static std::string collectMacOSModules();

                /** Collects memory usage information
                 * @return String containing memory usage details
                 */
                static std::string collectMemoryUsage();

                /** Collects thread information
                 * @return String containing thread details
                 */
                static std::string collectThreadInfo();
};

} // namespace rl

#endif // RL_CRASHDUMP_CRASH_DUMP_RUNTIME_STATE_H
