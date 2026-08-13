#ifndef RL_CRASHDUMP_CRASH_DUMP_GPU_INFO_H
#define RL_CRASHDUMP_CRASH_DUMP_GPU_INFO_H

#include <string>
#include <vulkan/vulkan.hpp>

namespace rl
{

/** GPU crash dump collector for crash reports
 *
 * This module collects GPU-specific crash information using VK_EXT_device_fault
 * extension, including GPU fault codes, addresses, and detailed diagnostic information.
 */
class CrashDumpGPUInfo
{
        public:
                /** Collects GPU crash dump information for the crash report
                 * @param device Vulkan device handle
                 * @param physicalDevice Vulkan physical device handle
                 * @param instance Vulkan instance handle
                 * @return String containing formatted GPU crash dump information
                 */
                static std::string collectGPUCrashDump(VkDevice         device,
                                                       VkPhysicalDevice physicalDevice,
                                                       VkInstance       instance);

        private:
                /** Checks if VK_EXT_device_fault extension is available
                 * @param physicalDevice Vulkan physical device handle
                 * @return true if extension is available
                 */
                static bool isDeviceFaultExtensionAvailable(VkPhysicalDevice physicalDevice);

                /** Collects device fault counts if available
                 * @param device Vulkan device handle
                 * @return String containing fault count information
                 */
                static std::string collectDeviceFaultCounts(VkDevice device);

                /** Collects binary device fault data if available
                 * @param device Vulkan device handle
                 * @return String containing fault data information
                 */
                static std::string collectDeviceFaultData(VkDevice device);

                /** Collects device address fault information if available
                 * @param device Vulkan device handle
                 * @return String containing address fault information
                 */
                static std::string collectDeviceAddressFaults(VkDevice device);

                /** Collects GPU memory fault information
                 * @param device Vulkan device handle
                 * @return String containing memory fault information
                 */
                static std::string collectGPUMemoryFaults(VkDevice device);

                /** Collects GPU execution fault information
                 * @param device Vulkan device handle
                 * @return String containing execution fault information
                 */
                static std::string collectGPUExecutionFaults(VkDevice device);

                /** Collects GPU pipeline state information
                 * @param device Vulkan device handle
                 * @return String containing pipeline state information
                 */
                static std::string collectPipelineState(VkDevice device);

                /** Collects GPU descriptor set information
                 * @param device Vulkan device handle
                 * @return String containing descriptor set information
                 */
                static std::string collectDescriptorSetInfo(VkDevice device);

                /** Collects GPU buffer/image state information
                 * @param device Vulkan device handle
                 * @return String containing buffer/image state information
                 */
                static std::string collectBufferImageState(VkDevice device);

                /** Formats fault code to human-readable string
                 * @param faultCode Vulkan fault code
                 * @return Human-readable fault description
                 */
                static std::string formatFaultCode(uint64_t faultCode);

                /** Formats fault type to human-readable string
                 * @param faultType Vulkan fault type
                 * @return Human-readable fault type description
                 */
                static std::string formatFaultType(uint32_t faultType);
};

} // namespace rl

#endif // RL_CRASHDUMP_CRASH_DUMP_GPU_INFO_H
