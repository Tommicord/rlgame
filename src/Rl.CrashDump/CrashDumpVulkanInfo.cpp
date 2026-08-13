#include "Rl.CrashDump/CrashDumpVulkanInfo.h"
#include "Rl.Log/Log.h"

#include <sstream>

#if defined(__has_include)
#if __has_include(<vulkan/vulkan_core.h>)
#include <vulkan/vulkan_core.h>
#endif
#endif

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

namespace rl
{

std::string CrashDumpVulkanInfo::collectVulkanContext()
{
        std::ostringstream oss;

        oss << collectHeaderVersion();
        oss << collectLoaderVersion();
        oss << collectInstanceExtensions();
        oss << collectInstanceLayers();
        oss << collectPhysicalDevices();

        return oss.str();
}

std::string CrashDumpVulkanInfo::collectHeaderVersion()
{
        std::ostringstream oss;

#ifdef VK_HEADER_VERSION
        oss << "Vulkan header version (VK_HEADER_VERSION): " << VK_HEADER_VERSION << "\n";
#else
        oss << "Vulkan header version: Not available at compile time\n";
#endif

#ifdef VK_API_VERSION_1_0
        oss << "Vulkan API version 1.0: Supported\n";
#endif
#ifdef VK_API_VERSION_1_1
        oss << "Vulkan API version 1.1: Supported\n";
#endif
#ifdef VK_API_VERSION_1_2
        oss << "Vulkan API version 1.2: Supported\n";
#endif
#ifdef VK_API_VERSION_1_3
        oss << "Vulkan API version 1.3: Supported\n";
#endif

        return oss.str();
}

std::string CrashDumpVulkanInfo::collectLoaderVersion()
{
        std::ostringstream oss;

#if defined(_WIN32)
        oss << queryVulkanLoader("vulkan-1.dll");
#elif defined(__linux__)
        oss << queryVulkanLoader("libvulkan.so.1");
#elif defined(__APPLE__)
        oss << queryVulkanLoader("libvulkan.1.dylib");
#else
        oss << "Vulkan runtime: Platform not supported for runtime detection\n";
#endif

        return oss.str();
}

std::string CrashDumpVulkanInfo::collectInstanceExtensions()
{
        std::ostringstream oss;

#if defined(_WIN32)
        HMODULE vkModule = LoadLibrary(TEXT("vulkan-1.dll"));
        if (!vkModule)
        {
                oss << "Instance Extensions: Vulkan loader not available\n";
                return oss.str();
        }

        using PFN_vkEnumerateInstanceVersion = VkResult (*)(uint32_t*);
        using PFN_vkCreateInstance =
            VkResult (*)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
        using PFN_vkEnumerateInstanceExtensionProperties =
            VkResult (*)(const char*, uint32_t*, VkExtensionProperties*);
        using PFN_vkDestroyInstance = void (*)(VkInstance, const VkAllocationCallbacks*);

        auto fnVersion =
            (PFN_vkEnumerateInstanceVersion)GetProcAddress(vkModule, "vkEnumerateInstanceVersion");
        auto fnCreate  = (PFN_vkCreateInstance)GetProcAddress(vkModule, "vkCreateInstance");
        auto fnEnumExt = (PFN_vkEnumerateInstanceExtensionProperties)GetProcAddress(
            vkModule, "vkEnumerateInstanceExtensionProperties");
        auto fnDestroy = (PFN_vkDestroyInstance)GetProcAddress(vkModule, "vkDestroyInstance");

        if (!fnCreate || !fnEnumExt || !fnDestroy)
        {
                oss << "Instance Extensions: Required Vulkan functions not available\n";
                FreeLibrary(vkModule);
                return oss.str();
        }

        VkInstance           instance   = VK_NULL_HANDLE;
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        if (fnCreate(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
                oss << "Instance Extensions: Failed to create Vulkan instance\n";
                FreeLibrary(vkModule);
                return oss.str();
        }

        uint32_t extensionCount = 0;
        if (fnEnumExt(nullptr, &extensionCount, nullptr) == VK_SUCCESS && extensionCount > 0)
        {
                std::vector<VkExtensionProperties> extensions(extensionCount);
                if (fnEnumExt(nullptr, &extensionCount, extensions.data()) == VK_SUCCESS)
                {
                        oss << "Instance Extensions (" << extensionCount << "):\n";
                        for (const auto& ext : extensions)
                        {
                                oss << "  " << ext.extensionName
                                    << " (spec version: " << ext.specVersion << ")\n";
                        }
                }
                else
                {
                        oss << "Instance Extensions: Failed to enumerate extensions\n";
                }
        }
        else
        {
                oss << "Instance Extensions: None available or enumeration failed\n";
        }

        fnDestroy(instance, nullptr);
        FreeLibrary(vkModule);

#elif defined(__linux__) || defined(__APPLE__)
        const char* libName =
#if defined(__linux__)
            "libvulkan.so.1";
#else
            "libvulkan.1.dylib";
#endif

        void* lib = dlopen(libName, RTLD_LAZY);
        if (!lib)
        {
                oss << "Instance Extensions: Vulkan loader not available\n";
                return oss.str();
        }

        using PFN_vkEnumerateInstanceVersion = VkResult (*)(uint32_t*);
        using PFN_vkCreateInstance =
            VkResult (*)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
        using PFN_vkEnumerateInstanceExtensionProperties =
            VkResult (*)(const char*, uint32_t*, VkExtensionProperties*);
        using PFN_vkDestroyInstance = void (*)(VkInstance, const VkAllocationCallbacks*);

        auto fnVersion = (PFN_vkEnumerateInstanceVersion)dlsym(lib, "vkEnumerateInstanceVersion");
        auto fnCreate  = (PFN_vkCreateInstance)dlsym(lib, "vkCreateInstance");
        auto fnEnumExt = (PFN_vkEnumerateInstanceExtensionProperties)dlsym(
            lib, "vkEnumerateInstanceExtensionProperties");
        auto fnDestroy = (PFN_vkDestroyInstance)dlsym(lib, "vkDestroyInstance");

        if (!fnCreate || !fnEnumExt || !fnDestroy)
        {
                oss << "Instance Extensions: Required Vulkan functions not available\n";
                dlclose(lib);
                return oss.str();
        }

        VkInstance           instance   = VK_NULL_HANDLE;
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        if (fnCreate(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
                oss << "Instance Extensions: Failed to create Vulkan instance\n";
                dlclose(lib);
                return oss.str();
        }

        uint32_t extensionCount = 0;
        if (fnEnumExt(nullptr, &extensionCount, nullptr) == VK_SUCCESS && extensionCount > 0)
        {
                std::vector<VkExtensionProperties> extensions(extensionCount);
                if (fnEnumExt(nullptr, &extensionCount, extensions.data()) == VK_SUCCESS)
                {
                        oss << "Instance Extensions (" << extensionCount << "):\n";
                        for (const auto& ext : extensions)
                        {
                                oss << "  " << ext.extensionName
                                    << " (spec version: " << ext.specVersion << ")\n";
                        }
                }
                else
                {
                        oss << "Instance Extensions: Failed to enumerate extensions\n";
                }
        }
        else
        {
                oss << "Instance Extensions: None available or enumeration failed\n";
        }

        fnDestroy(instance, nullptr);
        dlclose(lib);

#else
        oss << "Instance Extensions: Platform not supported\n";
#endif

        return oss.str();
}

std::string CrashDumpVulkanInfo::collectInstanceLayers()
{
        std::ostringstream oss;

#if defined(_WIN32)
        HMODULE vkModule = LoadLibrary(TEXT("vulkan-1.dll"));
        if (!vkModule)
        {
                oss << "Instance Layers: Vulkan loader not available\n";
                return oss.str();
        }

        using PFN_vkEnumerateInstanceLayerProperties = VkResult (*)(uint32_t*, VkLayerProperties*);
        auto fnEnumLayers = (PFN_vkEnumerateInstanceLayerProperties)GetProcAddress(
            vkModule, "vkEnumerateInstanceLayerProperties");

        if (!fnEnumLayers)
        {
                oss << "Instance Layers: vkEnumerateInstanceLayerProperties not available\n";
                FreeLibrary(vkModule);
                return oss.str();
        }

        uint32_t layerCount = 0;
        if (fnEnumLayers(&layerCount, nullptr) == VK_SUCCESS && layerCount > 0)
        {
                std::vector<VkLayerProperties> layers(layerCount);
                if (fnEnumLayers(&layerCount, layers.data()) == VK_SUCCESS)
                {
                        oss << "Instance Layers (" << layerCount << "):\n";
                        for (const auto& layer : layers)
                        {
                                oss << "  " << layer.layerName
                                    << " (spec version: " << layer.specVersion
                                    << ", impl version: " << layer.implementationVersion << ")\n";
                                oss << "    Description: " << layer.description << "\n";
                        }
                }
                else
                {
                        oss << "Instance Layers: Failed to enumerate layers\n";
                }
        }
        else
        {
                oss << "Instance Layers: None available or enumeration failed\n";
        }

        FreeLibrary(vkModule);

#elif defined(__linux__) || defined(__APPLE__)
        const char* libName =
#if defined(__linux__)
            "libvulkan.so.1";
#else
            "libvulkan.1.dylib";
#endif

        void* lib = dlopen(libName, RTLD_LAZY);
        if (!lib)
        {
                oss << "Instance Layers: Vulkan loader not available\n";
                return oss.str();
        }

        using PFN_vkEnumerateInstanceLayerProperties = VkResult (*)(uint32_t*, VkLayerProperties*);
        auto fnEnumLayers = (PFN_vkEnumerateInstanceLayerProperties)dlsym(
            lib, "vkEnumerateInstanceLayerProperties");

        if (!fnEnumLayers)
        {
                oss << "Instance Layers: vkEnumerateInstanceLayerProperties not available\n";
                dlclose(lib);
                return oss.str();
        }

        uint32_t layerCount = 0;
        if (fnEnumLayers(&layerCount, nullptr) == VK_SUCCESS && layerCount > 0)
        {
                std::vector<VkLayerProperties> layers(layerCount);
                if (fnEnumLayers(&layerCount, layers.data()) == VK_SUCCESS)
                {
                        oss << "Instance Layers (" << layerCount << "):\n";
                        for (const auto& layer : layers)
                        {
                                oss << "  " << layer.layerName
                                    << " (spec version: " << layer.specVersion
                                    << ", impl version: " << layer.implementationVersion << ")\n";
                                oss << "    Description: " << layer.description << "\n";
                        }
                }
                else
                {
                        oss << "Instance Layers: Failed to enumerate layers\n";
                }
        }
        else
        {
                oss << "Instance Layers: None available or enumeration failed\n";
        }

        dlclose(lib);

#else
        oss << "Instance Layers: Platform not supported\n";
#endif

        return oss.str();
}

std::string CrashDumpVulkanInfo::collectPhysicalDevices()
{
        std::ostringstream oss;

#if defined(_WIN32)
        HMODULE vkModule = LoadLibrary(TEXT("vulkan-1.dll"));
        if (!vkModule)
        {
                oss << "Physical Devices: Vulkan loader not available\n";
                return oss.str();
        }

        using PFN_vkCreateInstance =
            VkResult (*)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
        using PFN_vkDestroyInstance = void (*)(VkInstance, const VkAllocationCallbacks*);
        using PFN_vkEnumeratePhysicalDevices =
            VkResult (*)(VkInstance, uint32_t*, VkPhysicalDevice*);
        using PFN_vkGetPhysicalDeviceProperties =
            void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
        using PFN_vkGetPhysicalDeviceMemoryProperties =
            void (*)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*);

        auto fnCreate  = (PFN_vkCreateInstance)GetProcAddress(vkModule, "vkCreateInstance");
        auto fnDestroy = (PFN_vkDestroyInstance)GetProcAddress(vkModule, "vkDestroyInstance");
        auto fnEnumPhys =
            (PFN_vkEnumeratePhysicalDevices)GetProcAddress(vkModule, "vkEnumeratePhysicalDevices");
        auto fnGetProps = (PFN_vkGetPhysicalDeviceProperties)GetProcAddress(
            vkModule, "vkGetPhysicalDeviceProperties");
        auto fnGetMemProps = (PFN_vkGetPhysicalDeviceMemoryProperties)GetProcAddress(
            vkModule, "vkGetPhysicalDeviceMemoryProperties");

        if (!fnCreate || !fnDestroy || !fnEnumPhys || !fnGetProps || !fnGetMemProps)
        {
                oss << "Physical Devices: Required Vulkan functions not available\n";
                FreeLibrary(vkModule);
                return oss.str();
        }

        VkInstance           instance   = VK_NULL_HANDLE;
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        if (fnCreate(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
                oss << "Physical Devices: Failed to create Vulkan instance\n";
                FreeLibrary(vkModule);
                return oss.str();
        }

        uint32_t deviceCount = 0;
        if (fnEnumPhys(instance, &deviceCount, nullptr) == VK_SUCCESS && deviceCount > 0)
        {
                std::vector<VkPhysicalDevice> devices(deviceCount);
                if (fnEnumPhys(instance, &deviceCount, devices.data()) == VK_SUCCESS)
                {
                        oss << "Physical Devices (" << deviceCount << "):\n";
                        for (uint32_t i = 0; i < deviceCount; i++)
                        {
                                VkPhysicalDeviceProperties props;
                                fnGetProps(devices[i], &props);

                                uint32_t major = VK_VERSION_MAJOR(props.apiVersion);
                                uint32_t minor = VK_VERSION_MINOR(props.apiVersion);
                                uint32_t patch = VK_VERSION_PATCH(props.apiVersion);

                                oss << "  Device " << i << ": " << props.deviceName << "\n";
                                oss << "    Vendor ID: 0x" << std::hex << props.vendorID << std::dec
                                    << "\n";
                                oss << "    Device ID: 0x" << std::hex << props.deviceID << std::dec
                                    << "\n";
                                oss << "    Driver Version: " << props.driverVersion << "\n";
                                oss << "    API Version: " << major << "." << minor << "." << patch
                                    << "\n";
                                oss << "    Device Type: ";
                                switch (props.deviceType)
                                {
                                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                                        oss << "integrated GPU\n";
                                        break;
                                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                                        oss << "Discrete GPU\n";
                                        break;
                                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                                        oss << "Virtual GPU\n";
                                        break;
                                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                                        oss << "CPU\n";
                                        break;
                                default:
                                        oss << "Other\n";
                                        break;
                                }

                                VkPhysicalDeviceMemoryProperties memProps;
                                fnGetMemProps(devices[i], &memProps);
                                oss << "    Memory Heaps: " << memProps.memoryHeapCount << "\n";
                                oss << "    Memory Types: " << memProps.memoryTypeCount << "\n";
                        }
                }
                else
                {
                        oss << "Physical Devices: Failed to enumerate devices\n";
                }
        }
        else
        {
                oss << "Physical Devices: None available or enumeration failed\n";
        }

        fnDestroy(instance, nullptr);
        FreeLibrary(vkModule);

#elif defined(__linux__) || defined(__APPLE__)
        const char* libName =
#if defined(__linux__)
            "libvulkan.so.1";
#else
            "libvulkan.1.dylib";
#endif

        void* lib = dlopen(libName, RTLD_LAZY);
        if (!lib)
        {
                oss << "Physical Devices: Vulkan loader not available\n";
                return oss.str();
        }

        using PFN_vkCreateInstance =
            VkResult (*)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
        using PFN_vkDestroyInstance = void (*)(VkInstance, const VkAllocationCallbacks*);
        using PFN_vkEnumeratePhysicalDevices =
            VkResult (*)(VkInstance, uint32_t*, VkPhysicalDevice*);
        using PFN_vkGetPhysicalDeviceProperties =
            void (*)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
        using PFN_vkGetPhysicalDeviceMemoryProperties =
            void (*)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*);

        auto fnCreate   = (PFN_vkCreateInstance)dlsym(lib, "vkCreateInstance");
        auto fnDestroy  = (PFN_vkDestroyInstance)dlsym(lib, "vkDestroyInstance");
        auto fnEnumPhys = (PFN_vkEnumeratePhysicalDevices)dlsym(lib, "vkEnumeratePhysicalDevices");
        auto fnGetProps =
            (PFN_vkGetPhysicalDeviceProperties)dlsym(lib, "vkGetPhysicalDeviceProperties");
        auto fnGetMemProps = (PFN_vkGetPhysicalDeviceMemoryProperties)dlsym(
            lib, "vkGetPhysicalDeviceMemoryProperties");

        if (!fnCreate || !fnDestroy || !fnEnumPhys || !fnGetProps || !fnGetMemProps)
        {
                oss << "Physical Devices: Required Vulkan functions not available\n";
                dlclose(lib);
                return oss.str();
        }

        VkInstance           instance   = VK_NULL_HANDLE;
        VkInstanceCreateInfo createInfo = {};
        createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        if (fnCreate(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
                oss << "Physical Devices: Failed to create Vulkan instance\n";
                dlclose(lib);
                return oss.str();
        }

        uint32_t deviceCount = 0;
        if (fnEnumPhys(instance, &deviceCount, nullptr) == VK_SUCCESS && deviceCount > 0)
        {
                std::vector<VkPhysicalDevice> devices(deviceCount);
                if (fnEnumPhys(instance, &deviceCount, devices.data()) == VK_SUCCESS)
                {
                        oss << "Physical Devices (" << deviceCount << "):\n";
                        for (uint32_t i = 0; i < deviceCount; i++)
                        {
                                VkPhysicalDeviceProperties props;
                                fnGetProps(devices[i], &props);

                                uint32_t major = VK_VERSION_MAJOR(props.apiVersion);
                                uint32_t minor = VK_VERSION_MINOR(props.apiVersion);
                                uint32_t patch = VK_VERSION_PATCH(props.apiVersion);

                                oss << "  Device " << i << ": " << props.deviceName << "\n";
                                oss << "    Vendor ID: 0x" << std::hex << props.vendorID << std::dec
                                    << "\n";
                                oss << "    Device ID: 0x" << std::hex << props.deviceID << std::dec
                                    << "\n";
                                oss << "    Driver Version: " << props.driverVersion << "\n";
                                oss << "    API Version: " << major << "." << minor << "." << patch
                                    << "\n";
                                oss << "    Device Type: ";
                                switch (props.deviceType)
                                {
                                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                                        oss << "integrated GPU\n";
                                        break;
                                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                                        oss << "Discrete GPU\n";
                                        break;
                                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                                        oss << "Virtual GPU\n";
                                        break;
                                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                                        oss << "CPU\n";
                                        break;
                                default:
                                        oss << "Other\n";
                                        break;
                                }

                                VkPhysicalDeviceMemoryProperties memProps;
                                fnGetMemProps(devices[i], &memProps);
                                oss << "    Memory Heaps: " << memProps.memoryHeapCount << "\n";
                                oss << "    Memory Types: " << memProps.memoryTypeCount << "\n";
                        }
                }
                else
                {
                        oss << "Physical Devices: Failed to enumerate devices\n";
                }
        }
        else
        {
                oss << "Physical Devices: None available or enumeration failed\n";
        }

        fnDestroy(instance, nullptr);
        dlclose(lib);

#else
        oss << "Physical Devices: Platform not supported\n";
#endif

        return oss.str();
}

std::string CrashDumpVulkanInfo::queryVulkanLoader(const char* libName)
{
        std::ostringstream oss;

#if defined(_WIN32)
        HMODULE vkModule = LoadLibrary(TEXT(libName));
        if (vkModule)
        {
                using PFN_vkEnumerateInstanceVersion = VkResult (*)(uint32_t*);
                auto fn = (PFN_vkEnumerateInstanceVersion)GetProcAddress(
                    vkModule, "vkEnumerateInstanceVersion");
                if (fn)
                {
                        uint32_t ver = 0;
                        if (fn(&ver) == VK_SUCCESS)
                        {
                                uint32_t major = ver >> 22;
                                uint32_t minor = (ver >> 12) & 0x3ff;
                                uint32_t patch = ver & 0xfff;
                                oss << "Vulkan loader instance version: " << major << "." << minor
                                    << "." << patch << "\n";
                        }
                        else
                        {
                                oss << "Vulkan loader: vkEnumerateInstanceVersion call failed\n";
                        }
                }
                else
                {
                        oss << "Vulkan loader: vkEnumerateInstanceVersion not available\n";
                }
                FreeLibrary(vkModule);
        }
        else
        {
                oss << "Vulkan loader not found (" << libName << ")\n";
        }
#elif defined(__linux__) || defined(__APPLE__)
        void* lib = dlopen(libName, RTLD_LAZY);
        if (lib)
        {
                using PFN_vkEnumerateInstanceVersion = VkResult (*)(uint32_t*);
                auto fn = (PFN_vkEnumerateInstanceVersion)dlsym(lib, "vkEnumerateInstanceVersion");
                if (fn)
                {
                        uint32_t ver = 0;
                        if (fn(&ver) == VK_SUCCESS)
                        {
                                uint32_t major = ver >> 22;
                                uint32_t minor = (ver >> 12) & 0x3ff;
                                uint32_t patch = ver & 0xfff;
                                oss << "Vulkan loader instance version: " << major << "." << minor
                                    << "." << patch << "\n";
                        }
                        else
                        {
                                oss << "Vulkan loader: vkEnumerateInstanceVersion call failed\n";
                        }
                }
                else
                {
                        oss << "Vulkan loader: vkEnumerateInstanceVersion not available\n";
                }
                dlclose(lib);
        }
        else
        {
                oss << "Vulkan loader not found (" << libName << ")\n";
        }
#else
        oss << "Vulkan runtime: Platform not supported for runtime detection\n";
#endif

        return oss.str();
}

} // namespace rl
