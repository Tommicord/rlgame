#ifndef RL_CRASHDUMP_CRASH_DUMP_RENDER_DOC_INFO_H
#define RL_CRASHDUMP_CRASH_DUMP_RENDER_DOC_INFO_H

#include <string>

namespace rl
{

/** RenderDoc context collector for crash reports
 *
 * This module collects RenderDoc-specific information including
 * availability status, API version, capture state, and active captures.
 */
class CrashDumpRenderDocInfo
{
  public:
    /** Collects RenderDoc context information for the crash report
     * @return String containing formatted RenderDoc context information
     */
    static std::string collectRenderDocContext();

  private:
    /** Collects RenderDoc availability and initialization status
     * @return String containing availability information
     */
    static std::string collectAvailabilityInfo();

    /** Collects RenderDoc API version information
     * @return String containing API version details
     */
    static std::string collectAPIVersion();

    /** Collects RenderDoc capture state information
     * @return String containing capture status and details
     */
    static std::string collectCaptureState();

    /** Collects information about active RenderDoc captures
     * @return String containing active capture details
     */
    static std::string collectActiveCaptures();

    /** Collects information about RenderDoc trigger capabilities
     * @return String containing trigger / capture trigger info
     */
    static std::string collectTriggerInfo();
};

} // namespace rl

#endif // RL_CRASHDUMP_CRASH_DUMP_RENDER_DOC_INFO_H
