#include "Rl.CrashDump/CrashDumpGPUInfo.h"
#include "Rl.Log/Log.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <cstdint>

// Provide fallback definitions for VK_EXT_device_fault extension types and symbols
#ifndef VK_EXT_device_fault
#define VK_EXT_device_fault 1

#define _RL_CRASHDUMP_DEVICE_FAULT_FALLBACK 1

#ifndef VK_EXT_DEVICE_FAULT_EXTENSION_NAME
#define VK_EXT_DEVICE_FAULT_EXTENSION_NAME "VK_EXT_device_fault"
#endif

VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeviceFaultBinaryEXT)

typedef enum VkDeviceFaultVendorBinaryHeaderVersionEXT
{
        VK_DEVICE_FAULT_VENDOR_BINARY_HEADER_VERSION_ONE_EXT = 1,
} VkDeviceFaultVendorBinaryHeaderVersionEXT;

typedef struct VkDeviceFaultAddressesEXT
{
                VkDeviceAddress infoAddress;
                VkDeviceAddress blockStartAddress;
                VkDeviceAddress blockEndAddress;
} VkDeviceFaultAddressesEXT;

typedef struct VkDeviceFaultVendorInfoEXT
{
                VkDeviceFaultVendorBinaryHeaderVersionEXT vendorBinaryVersion;
                size_t                                    vendorBinarySize;
                const void*                               pVendorBinaryData;
} VkDeviceFaultVendorInfoEXT;

typedef struct VkDeviceFaultInfoEXT
{
                VkStructureType sType;
                const void*     pNext;
                const char*     description;
                // Provide vendor info inline so code can access pVendorInfos.vendorBinaryVersion
                // as used in this module. Also provide top-level aliases for convenience.
                VkDeviceFaultVendorInfoEXT pVendorInfos;
                size_t                     vendorBinarySize;
                const void*                pVendorBinaryData;
} VkDeviceFaultInfoEXT;

typedef struct VkDeviceFaultCountsEXT
{
                VkStructureType sType;
                const void*     pNext;
                uint32_t        addressInfoCount;
                uint32_t        vendorInfoCount;
} VkDeviceFaultCountsEXT;

// Define missing structure type enums if not present
#ifndef VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT
#define VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT ((VkStructureType)1000339000)
#endif
#ifndef VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT
#define VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT ((VkStructureType)1000339001)
#endif

// Correct function pointer signature: the function accepts counts and info
// pointers.
typedef VkResult(VKAPI_PTR* PFN_vkGetDeviceFaultInfoEXT)(VkDevice,
                                                         VkDeviceFaultCountsEXT*,
                                                         VkDeviceFaultInfoEXT*);
#endif

namespace rl
{

std::string CrashDumpGPUInfo::collectGPUCrashDump(VkDevice         device,
                                                  VkPhysicalDevice physicalDevice,
                                                  VkInstance       instance)
{
        std::ostringstream oss;

        oss << "GPU CRASH DUMP INFORMATION\n";
        oss << "==========================\n\n";

        if (!device || !physicalDevice)
        {
                oss << "Error: Invalid device or physical device handle\n";
                return oss.str();
        }

        oss << "Device Fault Extension Status: ";
        if (isDeviceFaultExtensionAvailable(physicalDevice))
        {
                oss << "Available\n\n";
                oss << collectDeviceFaultCounts(device);
                oss << collectDeviceFaultData(device);
                oss << collectDeviceAddressFaults(device);
                oss << collectGPUMemoryFaults(device);
                oss << collectGPUExecutionFaults(device);
                oss << collectPipelineState(device);
                oss << collectDescriptorSetInfo(device);
                oss << collectBufferImageState(device);
        }
        else
        {
                oss << "Not Available\n";
                oss << "The VK_EXT_device_fault extension is not supported by this device.\n";
                oss << "GPU crash dumps require driver support for this extension.\n";
        }

        return oss.str();
}

bool CrashDumpGPUInfo::isDeviceFaultExtensionAvailable(VkPhysicalDevice physicalDevice)
{
        if (!physicalDevice)
        {
                return false;
        }

        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

        if (extensionCount == 0)
        {
                return false;
        }

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount,
                                             extensions.data());

        for (const auto& ext : extensions)
        {
                if (strcmp(ext.extensionName, VK_EXT_DEVICE_FAULT_EXTENSION_NAME) == 0)
                {
                        return true;
                }
        }

        return false;
}

std::string CrashDumpGPUInfo::collectDeviceFaultCounts(VkDevice device)
{
        std::ostringstream oss;

        oss << "Device Fault Counts:\n";

        auto vkGetDeviceFaultInfoEXT =
            (PFN_vkGetDeviceFaultInfoEXT)vkGetDeviceProcAddr(device, "vkGetDeviceFaultInfoEXT");

        if (!vkGetDeviceFaultInfoEXT)
        {
                oss << "  vkGetDeviceFaultInfoEXT function not available\n";
                return oss.str();
        }
        VkDeviceFaultCountsEXT faultCounts = {VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT};
        VkResult               result      = vkGetDeviceFaultInfoEXT(device, &faultCounts, nullptr);
        if (result == VK_SUCCESS || result == VK_INCOMPLETE)
        {
                oss << "  Address info count: " << faultCounts.addressInfoCount << "\n";
                oss << "  Vendor info count: " << faultCounts.vendorInfoCount << "\n";
        }
        else
        {
                oss << "  Error: Failed to query device fault counts (VkResult: " << result
                    << ")\n";
        }

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::collectDeviceFaultData(VkDevice device)
{
        std::ostringstream oss;

        oss << "Device Fault Data:\n";

        auto vkGetDeviceFaultInfoEXT =
            (PFN_vkGetDeviceFaultInfoEXT)vkGetDeviceProcAddr(device, "vkGetDeviceFaultInfoEXT");

        if (!vkGetDeviceFaultInfoEXT)
        {
                oss << "  vkGetDeviceFaultInfoEXT function not available\n";
                return oss.str();
        }

        VkDeviceFaultCountsEXT faultCounts = {VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT};

        // Query counts first (allow nullptr for infos per driver behavior)
        VkResult result = vkGetDeviceFaultInfoEXT(device, &faultCounts, nullptr);
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
                oss << "  Error: Failed to query device fault info counts (VkResult: " << result
                    << ")\n";
                oss << "\n";
                return oss.str();
        }

        uint32_t totalInfos = faultCounts.addressInfoCount + faultCounts.vendorInfoCount;
        if (totalInfos == 0)
        {
                oss << "  No device fault entries reported by driver\n\n";
                return oss.str();
        }
        std::vector<VkDeviceFaultInfoEXT> infos(totalInfos);
        for (auto& inf : infos)
        {
                std::memset(&inf, 0, sizeof(inf));
                inf.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;
        }
        result = vkGetDeviceFaultInfoEXT(device, &faultCounts, infos.data());
        if (result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
                oss << "  Error: Failed to retrieve detailed fault info (VkResult: " << result
                    << ")\n\n";
                return oss.str();
        }

        oss << "  Retrieved " << totalInfos << " fault info entries:\n";
        for (uint32_t i = 0; i < totalInfos; ++i)
        {
                VkDeviceFaultInfoEXT& fi = infos[i];
                if (fi.description)
                {
                        oss << "    Entry " << i << " Description: " << fi.description << "\n";
                }

                // Vendor-specific info: try to read vendor binary fields if present
                // Our fallback typedef places vendor binary data in pVendorBinaryData /
                // vendorBinarySize or inside pVendorInfos. Attempt both safely.
                const void* vendorData             = nullptr;
                size_t      vendorSize             = 0;
                uint32_t    vendorVersion          = 0;
                bool        vendorDataNotExtracted = false;

#if defined(_RL_CRASHDUMP_DEVICE_FAULT_FALLBACK)
                // Use the fallback layout we defined earlier
                vendorData    = fi.pVendorBinaryData;
                vendorSize    = fi.vendorBinarySize;
                vendorVersion = static_cast<uint32_t>(fi.pVendorInfos.vendorBinaryVersion);
#else
                // When building against the real Vulkan headers we avoid touching
                // vendor-specific binary fields here to remain compatible with
                // different driver/SDK layouts. Report presence without extracting.
                vendorDataNotExtracted = true;
#endif

                if (vendorDataNotExtracted)
                {
                        oss << "    Vendor-specific binary data: not extracted in this build\n";
                }
                else if (vendorData && vendorSize > 0)
                {
                        oss << "    Vendor binary version: " << vendorVersion << "\n";
                        oss << "    Vendor binary size: " << vendorSize << " bytes\n";

                        const uint8_t* data     = static_cast<const uint8_t*>(vendorData);
                        size_t         dumpSize = vendorSize < 256 ? vendorSize : 256;
                        oss << "    Vendor Binary (first " << dumpSize << " bytes):\n";
                        for (size_t off = 0; off < dumpSize; off += 16)
                        {
                                oss << "      " << std::hex << std::setw(8) << std::setfill('0')
                                    << off << ": ";
                                for (size_t j = 0; j < 16 && (off + j) < dumpSize; ++j)
                                {
                                        oss << std::setw(2) << std::setfill('0')
                                            << static_cast<uint32_t>(data[off + j]) << " ";
                                }
                                oss << std::dec << "\n";
                        }
                }
                else
                {
                        oss << "    No vendor binary data for entry " << i << "\n";
                }
        }

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::collectDeviceAddressFaults(VkDevice device)
{
        std::ostringstream oss;

        oss << "Device Address Faults:\n";

        oss << "  Detailed address fault information requires VK_EXT_device_fault_features\n";
        oss << "  This extension provides specific fault addresses and memory regions\n";
        oss << "  Status: Not implemented (requires extension support)\n";

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::collectGPUMemoryFaults(VkDevice device)
{
        std::ostringstream oss;

        oss << "GPU Memory Faults:\n";

        VkPhysicalDeviceMemoryProperties memProps;
        VkPhysicalDevice                 physicalDevice = VK_NULL_HANDLE;

        oss << "  Note: Detailed GPU memory fault information requires:\n";
        oss << "    - VK_EXT_device_fault extension\n";
        oss << "    - VK_EXT_device_fault_features extension\n";
        oss << "    - Driver support for memory fault reporting\n";
        oss << "  Status: Limited information available\n";

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::collectGPUExecutionFaults(VkDevice device)
{
        std::ostringstream oss;

        oss << "GPU Execution Faults:\n";

        auto vkGetDeviceFaultInfoEXT =
            (PFN_vkGetDeviceFaultInfoEXT)vkGetDeviceProcAddr(device, "vkGetDeviceFaultInfoEXT");

        if (!vkGetDeviceFaultInfoEXT)
        {
                oss << "  vkGetDeviceFaultInfoEXT function not available\n";
                return oss.str();
        }

        VkDeviceFaultInfoEXT faultInfo{};
        faultInfo.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT;

        VkDeviceFaultCountsEXT faultCounts = {VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT};
        VkResult               result = vkGetDeviceFaultInfoEXT(device, &faultCounts, &faultInfo);
        if (result == VK_SUCCESS)
        {
                if (faultInfo.description && strlen(faultInfo.description) > 0)
                {
                        oss << "  Execution Fault Description:\n";
                        oss << "    " << faultInfo.description << "\n";
                }
                else
                {
                        oss << "  No execution fault description available\n";
                }
        }
        else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
                oss << "  Error: Out of device memory when retrieving fault info\n";
        }
        else if (result == VK_ERROR_UNKNOWN)
        {
                oss << "  Error: Unknown error when retrieving fault info\n";
        }
        else
        {
                oss << "  Error: Failed to retrieve fault info (VkResult: " << result << ")\n";
        }

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::collectPipelineState(VkDevice device)
{
        std::ostringstream oss;

        oss << "Pipeline State:\n";
        oss << "  Note: Detailed pipeline state requires:\n";
        oss << "    - VK_EXT_device_fault extension\n";
        oss << "    - VK_EXT_pipeline_creation_feedback extension\n";
        oss << "    - Shader debug information\n";
        oss << "  Status: Not implemented (requires additional extensions)\n";

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::collectDescriptorSetInfo(VkDevice device)
{
        std::ostringstream oss;

        oss << "Descriptor Set Information:\n";
        oss << "  Note: Detailed descriptor set information requires:\n";
        oss << "    - VK_EXT_device_fault extension\n";
        oss << "    - VK_EXT_descriptor_indexing extension\n";
        oss << "    - Descriptor set tracking\n";
        oss << "  Status: Not implemented (requires additional extensions)\n";

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::collectBufferImageState(VkDevice device)
{
        std::ostringstream oss;

        oss << "Buffer/Image State:\n";
        oss << "  Note: Detailed buffer/image state requires:\n";
        oss << "    - VK_EXT_device_fault extension\n";
        oss << "    - Resource tracking and validation\n";
        oss << "  Status: Not implemented (requires resource tracking)\n";

        oss << "\n";
        return oss.str();
}

std::string CrashDumpGPUInfo::formatFaultCode(uint64_t faultCode)
{
        std::ostringstream oss;

        // Common fault codes (these are vendor-specific, but we can provide some general info)
        oss << "0x" << std::hex << std::setw(16) << std::setfill('0') << faultCode << std::dec;

        // Some common fault patterns
        if ((faultCode & 0xFFFFFFFF) == 0xDEADBEEF)
        {
                oss << " (Pattern: DEADBEEF - Possible memory corruption)";
        }
        else if ((faultCode & 0xFFFFFFFF) == 0xCAFEBABE)
        {
                oss << " (Pattern: CAFEBABE - Possible initialization issue)";
        }
        else if ((faultCode & 0xFFFFFFFF) == 0x00000000)
        {
                oss << " (Pattern: Zero - Null pointer or uninitialized data)";
        }
        else if ((faultCode & 0xFFFFFFFF) == 0xFFFFFFFF)
        {
                oss << " (Pattern: All ones - Possible invalid address)";
        }

        return oss.str();
}

std::string CrashDumpGPUInfo::formatFaultType(uint32_t faultType)
{
        switch (faultType)
        {
        case VK_DEVICE_FAULT_VENDOR_BINARY_HEADER_VERSION_ONE_EXT:
                return "Version 1.0 (Standard)";
        default:
                return "Unknown Version";
        }
}

} // namespace rl
