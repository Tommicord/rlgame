#ifndef RL_CRASHDUMP_CRASH_DUMP_FILE_SYSTEM_H
#define RL_CRASHDUMP_CRASH_DUMP_FILE_SYSTEM_H

#include <string>

namespace rl
{

/** File system operations for crash dump management
 *
 * This module handles all file system-related operations for crash dumps,
 * including folder path resolution, directory creation, and UUID generation.
 */
class CrashDumpFileSystem
{
        public:
                /** Gets the crash dump folder path for the current platform
                 * @return Path to the .crashdump folder
                 */
                static std::string getCrashDumpFolder();

                /** Creates the crash dump folder if it doesn't exist
                 * @return true if folder exists or was created successfully
                 */
                static bool ensureCrashDumpFolderExists();

                /** Generates a UUID string for crash dump filename
                 * @return UUID string in format xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
                 */
                static std::string generateUUID();

                /** Gets the path separator for the current platform
                 * @return "\\" on Windows, "/" on other platforms
                 */
                static std::string getPathSeparator();
};

} // namespace rl

#endif // RL_CRASHDUMP_CRASH_DUMP_FILE_SYSTEM_H
