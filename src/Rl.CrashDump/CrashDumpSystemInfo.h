#ifndef RL_CRASHDUMP_CRASH_DUMP_SYSTEM_INFO_H
#define RL_CRASHDUMP_CRASH_DUMP_SYSTEM_INFO_H

#include <string>

namespace rl
{

/** System information collector for crash reports
 *
 * This module collects platform-specific system information including
 * OS details, CPU cores, memory information, and process/thread IDs.
 */
class CrashDumpSystemInfo
{
        public:
                /** Collects system information for the crash report
                 * @return String containing formatted system information
                 */
                static std::string collectSystemInfo();

        private:
                /** Collects Windows-specific system information
                 * @return String containing Windows system information
                 */
                static std::string collectWindowsSystemInfo();

                /** Collects Linux-specific system information
                 * @return String containing Linux system information
                 */
                static std::string collectLinuxSystemInfo();

                /** Collects macOS-specific system information
                 * @return String containing macOS system information
                 */
                static std::string collectMacOSSystemInfo();
};

} // namespace rl

#endif // RL_CRASHDUMP_CRASH_DUMP_SYSTEM_INFO_H
