#ifndef RL_CRASHDUMP_CRASH_DUMP_VULKAN_INFO_H
#define RL_CRASHDUMP_CRASH_DUMP_VULKAN_INFO_H

#include <string>

namespace rl
{

/** Vulkan context collector for crash reports
 *
 * This module collects Vulkan-specific information including header version,
 * loader version, available extensions, layers, and physical device information.
 */
class CrashDumpVulkanInfo
{
  public:
    /** Collects Vulkan context information for the crash report
     * @return String containing formatted Vulkan context information
     */
    static std::string collectVulkanContext();

  private:
    /** Collects Vulkan header version information
     * @return String containing header version details
     */
    static std::string collectHeaderVersion();

    /** Collects Vulkan loader/runtime version information
     * @return String containing loader version details
     */
    static std::string collectLoaderVersion();

    /** Collects Vulkan instance extensions if available
     * @return String containing available instance extensions
     */
    static std::string collectInstanceExtensions();

    /** Collects Vulkan instance layers if available
     * @return String containing available instance layers
     */
    static std::string collectInstanceLayers();

    /** Collects physical device information if available
     * @return String containing physical device details
     */
    static std::string collectPhysicalDevices();

    /** Dynamically loads Vulkan library and queries version
     * @param libName Library name to load (platform-specific)
     * @return String containing loader version or error message
     */
    static std::string queryVulkanLoader(const char* libName);
};

} // namespace rl

#endif // RL_CRASHDUMP_CRASH_DUMP_VULKAN_INFO_H
