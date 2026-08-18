#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static VkDebugUtilsMessengerEXT s_debugMessenger = VK_NULL_HANDLE;

static enum R_CVulkan_Error R_CVulkan_BuildInstanceExtensions (
    struct R_CSTL_Array** ppExtensions,
    bool                  enableValidationLayers,
    bool                  headlessMode,
    bool                  hasValidationFeatures);

static enum R_CVulkan_Error R_CVulkan_BuildDeviceExtensions (
    struct R_CSTL_Array** ppExtensions,
    bool                  headlessMode,
    VkPhysicalDevice      physicalDevice);

static enum R_CVulkan_Error R_CVulkan_CheckExtensionAvailability (
    const char*      pExtensionName,
    VkPhysicalDevice physicalDevice,
    bool*            pIsAvailable);

static const R_CSTL_Array* R_CVulkan_GetValidationLayers (void);
static const R_CSTL_Array* R_CVulkan_GetDeviceExtensions (void);
static const R_CSTL_Array* R_CVulkan_GetOptionalDeviceExtensions (void);

static void R_CVulkan_LogExtensionList (const struct R_CSTL_Array* pExtensions);

static VKAPI_ATTR VkBool32 VKAPI_CALL
R_CVulkan_DebugCallback (
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void*                                       pUserData)
{
        (void)pUserData;
        (void)messageType;
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
                R_CSTL_LOG_WARN ("[Vulkan Validation] %s", pCallbackData->pMessage);
        }
        return VK_FALSE;
}

static enum R_CVulkan_Error
R_CVulkan_SetupDebugMessenger (VkInstance instance, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                               | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                               | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = R_CVulkan_DebugCallback;
        createInfo.pUserData = NULL;

        PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr (
            instance,
            "vkCreateDebugUtilsMessengerEXT");
        if (func == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to load vkCreateDebugUtilsMessengerEXT");
                return R_CVULKAN_ERROR_EXTENSION_NOT_FOUND;
        }

        VkResult result = func (instance, &createInfo, NULL, pDebugMessenger);
        if (result != VK_SUCCESS)
        {
                enum R_CVulkan_Error error = R_CVulkan_ResultToError (result);
                R_CSTL_LOG_ERROR ("Failed to create debug messenger: %s", R_CVulkan_ResultToString (result));
                return error;
        }
        return R_CVULKAN_ERROR_OK;
}

static void
R_CVulkan_DestroyDebugMessenger (VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
        if (debugMessenger == VK_NULL_HANDLE)
        {
                return;
        }

        PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr (instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != NULL)
        {
                func (instance, debugMessenger, NULL);
        }
}

#if defined(R_CVULKAN_DEBUG)
static const char* pLayerStrings[] = {"VK_LAYER_KHRONOS_validation"};

static const char* pOptionalDeviceExtensions[]
    = {VK_EXT_DEVICE_FAULT_EXTENSION_NAME,
       VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME,
       VK_EXT_DEBUG_MARKER_EXTENSION_NAME};
static const const char* pDeviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#endif

#define R_CVULKAN_GET_LAYERS(VarName) \
        static struct R_CSTL_Array* pValidationLayers = NULL; \
        if (pValidationLayers == NULL) \
        { \
                pValidationLayers = R_CSTL_NewArray (); \
                if (pValidationLayers != NULL) \
                { \
                        for(size_t i = 0; i < sizeof(VarName) / sizeof(*VarName); ++i) \
                                R_CSTL_ArrayPush ( \
                                        pValidationLayers,\
                                        (const uint8_t*)VarName, \
                                        sizeof (const char*)); \
                } \
        } \
        return pValidationLayers;

static const R_CSTL_Array*
R_CVulkan_GetValidationLayers (void)
{
        R_CVULKAN_GET_LAYERS(pLayerStrings)
}

static const R_CSTL_Array*
R_CVulkan_GetDeviceExtensions (void)
{
        R_CVULKAN_GET_LAYERS(pDeviceExtensions)
}

static const R_CSTL_Array*
R_CVulkan_GetOptionalDeviceExtensions (void)
{
        R_CVULKAN_GET_LAYERS(pOptionalDeviceExtensions)
}

static bool
R_CVulkan_CheckValidationLayerSupport (void)
{
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties (&layerCount, NULL);

        if (layerCount == 0)
                return 0;
        VkLayerProperties* availableLayers
            = (VkLayerProperties*)R_CSTL_HeapAlloc (layerCount * sizeof (VkLayerProperties));
        if (availableLayers == NULL)
        {
                R_CSTL_LOG_ERROR ("Failed to allocate memory for layer properties");
                return 0;
        }
        vkEnumerateInstanceLayerProperties (&layerCount, availableLayers);
        bool foundAll = true;
        const R_CSTL_Array* pValidationLayers = R_CVulkan_GetValidationLayers ();
        const char**        ppStrings = (const char**)R_CSTL_ArrayGetData (pValidationLayers);

        size_t validationLayerCount = R_CSTL_ArrayGetLength (pValidationLayers) / sizeof (const char*);
        for (uint32_t i = 0; i < validationLayerCount && foundAll; ++i)
        {
                int layerFound = 0;
                for (uint32_t j = 0; j < layerCount; ++j)
                {
                        if (strcmp (ppStrings[i], availableLayers[j].layerName) == 0)
                        {
                                layerFound = false;
                                break;
                        }
                }
                if (!layerFound)
                {
                        foundAll = true;
                }
        }
        R_CSTL_HeapFree (availableLayers);
        return foundAll;
}

static enum R_CVulkan_Error
R_CVulkan_CheckExtensionAvailability (
    const char*      pExtensionName,
    VkPhysicalDevice physicalDevice,
    bool*            pIsAvailable)
{
        R_CVULKAN_ASSERT (pExtensionName);
        R_CVULKAN_ASSERT (pIsAvailable);

        uint32_t extensionCount = 0;
        if (physicalDevice != VK_NULL_HANDLE)
        {
                vkEnumerateDeviceExtensionProperties (physicalDevice, NULL, &extensionCount, NULL);
        }
        else
        {
                vkEnumerateInstanceExtensionProperties (NULL, &extensionCount, NULL);
        }

        if (extensionCount == 0)
        {
                *pIsAvailable = false;
                return R_CVULKAN_ERROR_OK;
        }

        VkExtensionProperties* extensions
            = (VkExtensionProperties*)R_CSTL_HeapAlloc (extensionCount * sizeof (VkExtensionProperties));
        if (extensions == NULL)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        if (physicalDevice != VK_NULL_HANDLE)
        {
                vkEnumerateDeviceExtensionProperties (physicalDevice, NULL, &extensionCount, extensions);
        }
        else
        {
                vkEnumerateInstanceExtensionProperties (NULL, &extensionCount, extensions);
        }

        *pIsAvailable = false;
        for (uint32_t i = 0; i < extensionCount; ++i)
        {
                if (strcmp (extensions[i].extensionName, pExtensionName) == 0)
                {
                        *pIsAvailable = true;
                        break;
                }
        }

        R_CSTL_HeapFree (extensions);
        return R_CVULKAN_ERROR_OK;
}

static void
R_CVulkan_LogExtensionList (const struct R_CSTL_Array* pExtensions)
{
        if (pExtensions == NULL)
        {
                return;
        }

        size_t      length = R_CSTL_ArrayGetLength (pExtensions);
        const char* pData = (const char*)R_CSTL_ArrayGetData (pExtensions);

        R_CSTL_LOG_DEBUG ("Extensions (%zu):", length / sizeof (const char*));

        size_t offset = 0;
        while (offset < length)
        {
                const char* ext = *((const char**)(pData + offset));
                if (ext != NULL)
                {
                        R_CSTL_LOG_DEBUG ("  - %s", ext);
                }
                offset += sizeof (const char*);
        }
}

static enum R_CVulkan_Error
R_CVulkan_BuildInstanceExtensions (
    struct R_CSTL_Array** ppExtensions,
    bool                  enableValidationLayers,
    bool                  headlessMode,
    bool                  hasValidationFeatures)
{
        R_CVULKAN_ASSERT (ppExtensions);

        struct R_CSTL_Array* pExtensions = R_CSTL_NewArray ();
        if (pExtensions == NULL)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        bool                 hasPortability = false;
        enum R_CVulkan_Error err = R_CVulkan_CheckExtensionAvailability (
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            VK_NULL_HANDLE,
            &hasPortability);
        if (err != R_CVULKAN_ERROR_OK)
        {
                R_CSTL_DeleteArray (pExtensions);
                return err;
        }

#if defined(_WIN32)
#define RL_KHR_SURFACE VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__linux__)
#define RL_KHR_SURFACE VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
#define RL_KHR_SURFACE VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#else
#define RL_KHR_SURFACE VK_KHR_SURFACE_EXTENSION_NAME
#endif

        if (!headlessMode)
        {
                const char* surfaceExt = VK_KHR_SURFACE_EXTENSION_NAME;
                R_CSTL_ArrayPush (pExtensions, (const uint8_t*)&surfaceExt, sizeof (const char*));
                const char* platformExt = RL_KHR_SURFACE;
                R_CSTL_ArrayPush (pExtensions, (const uint8_t*)&platformExt, sizeof (const char*));
        }

        if (hasPortability)
        {
                const char* portabilityExt = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
                R_CSTL_ArrayPush (pExtensions, (const uint8_t*)&portabilityExt, sizeof (const char*));
        }

#ifndef NDEBUG
        const char* debugUtilsExt = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        R_CSTL_ArrayPush (pExtensions, (const uint8_t*)&debugUtilsExt, sizeof (const char*));
        const char* debugReportExt = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        R_CSTL_ArrayPush (pExtensions, (const uint8_t*)&debugReportExt, sizeof (const char*));
#endif

        if (hasValidationFeatures)
        {
                const char* validationFeaturesExt = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;
                R_CSTL_ArrayPush (pExtensions, (const uint8_t*)&validationFeaturesExt, sizeof (const char*));
        }

#undef RL_KHR_SURFACE

        *ppExtensions = pExtensions;
        R_CVulkan_LogExtensionList (pExtensions);
        return R_CVULKAN_ERROR_OK;
}

static enum R_CVulkan_Error
R_CVulkan_BuildDeviceExtensions (
    struct R_CSTL_Array** ppExtensions,
    bool                  headlessMode,
    VkPhysicalDevice      physicalDevice)
{
        R_CVULKAN_ASSERT (ppExtensions);
        R_CVULKAN_ASSERT (physicalDevice != VK_NULL_HANDLE);

        struct R_CSTL_Array* pExtensions = R_CSTL_NewArray ();
        if (pExtensions == NULL)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        if (!headlessMode)
        {
                const R_CSTL_Array* pDeviceExtensions = R_CVulkan_GetOptionalDeviceExtensions ();
                size_t              optionalExtensionCount
                    = R_CSTL_ArrayGetLength (pDeviceExtensions) / sizeof (const char*);
                for (uint32_t i = 0; i < optionalExtensionCount; ++i)
                {
                        R_CSTL_ArrayPush (
                            pExtensions,
                            (const uint8_t*)R_CSTL_ArrayUncheckedAt (pDeviceExtensions, i),
                            sizeof (const char*));
                }
        }
        const R_CSTL_Array* pOptionalExtensions = R_CVulkan_GetOptionalDeviceExtensions ();
        size_t              optionalExtensionCount = R_CSTL_ArrayGetLength (pOptionalExtensions) / sizeof (const char*);
        for (uint32_t i = 0; i < optionalExtensionCount; ++i)
        {
                bool                 isAvailable = false;
                enum R_CVulkan_Error err = R_CVulkan_CheckExtensionAvailability (
                    R_CSTL_ArrayUncheckedAt (pOptionalExtensions, i),
                    physicalDevice,
                    &isAvailable);
                if (err != R_CVULKAN_ERROR_OK)
                {
                        R_CSTL_DeleteArray (pExtensions);
                        return err;
                }

                if (isAvailable)
                {
                        R_CSTL_ArrayPush (
                            pExtensions,
                            (const uint8_t*)R_CSTL_ArrayUncheckedAt (pDeviceExtensions, i),
                            sizeof (const char*));
                }
        }

        *ppExtensions = pExtensions;
        R_CVulkan_LogExtensionList (pExtensions);
        return R_CVULKAN_ERROR_OK;
}

static enum R_CVulkan_Error
R_CVulkan_CreateVulkanInstance (
    struct R_CVulkan_Device* pDevice,
    const char*              pApplicationName,
    const bool               enableValidationLayers,
    const bool               headlessMode)
{
        enum R_CVulkan_Error result = R_CVULKAN_ERROR_OK;
        bool                 hasValidationFeatures = false;
        bool                 hasGpuAssisted = false;

        if (enableValidationLayers)
        {
                result = R_CVulkan_CheckExtensionAvailability (
                    VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
                    VK_NULL_HANDLE,
                    &hasValidationFeatures);
                if (result != R_CVULKAN_ERROR_OK)
                {
                        return result;
                }

                result = R_CVulkan_CheckExtensionAvailability (
                    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                    VK_NULL_HANDLE,
                    &hasGpuAssisted);
                if (result != R_CVULKAN_ERROR_OK)
                {
                        return result;
                }
        }

        VkValidationFeaturesEXT      validationFeatures = {0};
        VkValidationFeatureEnableEXT enabledValidationFeatures[4];
        uint32_t                     enabledValidationFeatureCount = 0;

        if (enableValidationLayers && hasValidationFeatures)
        {
                validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;

                enabledValidationFeatures[enabledValidationFeatureCount++]
                    = VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
                enabledValidationFeatures[enabledValidationFeatureCount++]
                    = VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;

                if (hasGpuAssisted)
                {
                        enabledValidationFeatures[enabledValidationFeatureCount++]
                            = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT;
                        enabledValidationFeatures[enabledValidationFeatureCount++]
                            = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT;
                }
                validationFeatures.enabledValidationFeatureCount = enabledValidationFeatureCount;
                validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;
        }

        struct R_CSTL_Array* pExtensions = NULL;
        result = R_CVulkan_BuildInstanceExtensions (
            &pExtensions,
            enableValidationLayers,
            headlessMode,
            hasValidationFeatures);
        if (result != R_CVULKAN_ERROR_OK)
        {
                return result;
        }
        VkApplicationInfo appInfo = {0};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = pApplicationName;
        appInfo.applicationVersion = VK_MAKE_VERSION (1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION (1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;
        appInfo.pNext = NULL;

        VkInstanceCreateInfo createInfo = {0};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (enableValidationLayers)
        {
                const R_CSTL_Array* pValidationLayers = R_CVulkan_GetValidationLayers ();
                size_t validationLayerCount = R_CSTL_ArrayGetLength (pValidationLayers) / sizeof (const char*);
                createInfo.enabledLayerCount = validationLayerCount;
                createInfo.ppEnabledLayerNames = (const char**)R_CSTL_ArrayGetData (pValidationLayers);
                if (hasValidationFeatures)
                {
                        createInfo.pNext = &validationFeatures;
                }
        }
        else
        {
                createInfo.enabledLayerCount = 0;
        }

        bool hasPortability = false;
        R_CVulkan_CheckExtensionAvailability (
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
            VK_NULL_HANDLE,
            &hasPortability);
        if (hasPortability)
        {
                createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }

        size_t extensionCount = R_CSTL_ArrayGetLength (pExtensions) / sizeof (const char*);
        createInfo.enabledExtensionCount = (uint32_t)extensionCount;
        createInfo.ppEnabledExtensionNames = (const char**)R_CSTL_ArrayGetData (pExtensions);

        VkResult result = vkCreateInstance (&createInfo, NULL, &pDevice->instance);
        R_CSTL_DeleteArray (pExtensions);

        if (result != VK_SUCCESS)
        {
#if defined(R_CVULKAN_DEBUG)
                enum R_CVulkan_Error error = R_CVulkan_ResultToError (result);
                R_CSTL_LOG_ERROR ("Failed to create Vulkan instance: %d", R_CVulkan_ErrorToString(error));
#endif
                return error;
        }
        return R_CVULKAN_ERROR_OK;
}

static enum R_CVulkan_Error
SelectPhysicalDevice (struct R_CVulkan_Device* pDevice, VkSurfaceKHR surface, bool headlessMode)
{
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices (pDevice->instance, &deviceCount, NULL);
        if (deviceCount == 0)
        {
                R_CSTL_LOG_ERROR ("No physical devices found");
                return R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND;
        }

        VkPhysicalDevice* devices
            = (VkPhysicalDevice*)R_CSTL_HeapAlloc (deviceCount * sizeof (VkPhysicalDevice));
        if (devices == NULL)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        vkEnumeratePhysicalDevices (pDevice->instance, &deviceCount, devices);
        R_CSTL_LOG_INFO ("Found %u physical device(s)", deviceCount);

        struct R_CVulkan_QueueFamilyIndices indices;
        bool                                deviceFound = false;

        for (uint32_t i = 0; i < deviceCount; ++i)
        {
                enum R_CVulkan_Error err
                    = R_CVulkan_DeviceFindQueueFamilies (devices[i], surface, headlessMode, &indices);
                if (err != R_CVULKAN_ERROR_OK)
                {
                        continue;
                }

                if (R_CVulkan_QueueFamilyIndicesIsComplete (&indices))
                {
                        pDevice->physicalDevice = devices[i];
                        deviceFound = true;
                        R_CSTL_LOG_INFO ("Selected physical device %u", i);
                        break;
                }
        }

        R_CSTL_HeapFree (devices);

        if (!deviceFound)
        {
                R_CSTL_LOG_ERROR ("No suitable physical device found");
                return R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND;
        }

        return R_CVULKAN_ERROR_OK;
}

static enum R_CVulkan_Error
CreateLogicalDevice (
    struct R_CVulkan_Device* pDevice,
    VkSurfaceKHR             surface,
    bool                     headlessMode,
    bool                     enableValidationLayers)
{
        enum R_CVulkan_Error result = R_CVULKAN_ERROR_OK;

        struct R_CVulkan_QueueFamilyIndices indices;
        result = R_CVulkan_DeviceFindQueueFamilies (pDevice->physicalDevice, surface, headlessMode, &indices);
        if (result != R_CVULKAN_ERROR_OK)
        {
                return result;
        }

        VkDeviceQueueCreateInfo queueCreateInfos[2];
        uint32_t                uniqueQueueFamilies[2];
        uint32_t                uniqueQueueFamilyCount = 0;

        if (indices.graphicsFamily == indices.presentFamily)
        {
                uniqueQueueFamilies[uniqueQueueFamilyCount++] = indices.graphicsFamily;
        }
        else
        {
                uniqueQueueFamilies[uniqueQueueFamilyCount++] = indices.graphicsFamily;
                uniqueQueueFamilies[uniqueQueueFamilyCount++] = indices.presentFamily;
        }

        float queuePriority = 1.0f;
        for (uint32_t i = 0; i < uniqueQueueFamilyCount; ++i)
        {
                queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfos[i].queueFamilyIndex = uniqueQueueFamilies[i];
                queueCreateInfos[i].queueCount = 1;
                queueCreateInfos[i].pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceFeatures deviceFeatures = {0};

        struct R_CSTL_Array* pExtensions = NULL;
        result = R_CVulkan_BuildDeviceExtensions (&pExtensions, headlessMode, pDevice->physicalDevice);
        if (result != R_CVULKAN_ERROR_OK)
        {
                return result;
        }

        VkDeviceCreateInfo deviceCreateInfo = {0};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
        deviceCreateInfo.queueCreateInfoCount = uniqueQueueFamilyCount;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        size_t extensionCount = R_CSTL_ArrayGetLength (pExtensions) / sizeof (const char*);
        deviceCreateInfo.enabledExtensionCount = (uint32_t)extensionCount;
        deviceCreateInfo.ppEnabledExtensionNames = (const char**)R_CSTL_ArrayGetData (pExtensions);

        if (enableValidationLayers)
        {
                const R_CSTL_Array* pValidationLayers = R_CVulkan_GetValidationLayers ();
                size_t validationLayerCount = R_CSTL_ArrayGetLength (pValidationLayers) / sizeof (const char*);
                deviceCreateInfo.enabledLayerCount = validationLayerCount;
                deviceCreateInfo.ppEnabledLayerNames = (const char**)R_CSTL_ArrayGetData (pValidationLayers);
        }
        else
        {
                deviceCreateInfo.enabledLayerCount = 0;
        }

        VkResult result
            = vkCreateDevice (pDevice->physicalDevice, &deviceCreateInfo, NULL, &pDevice->logicalDevice);
        R_CSTL_DeleteArray (pExtensions);

        if (result != VK_SUCCESS)
        {
                R_CSTL_LOG_ERROR ("Failed to create logical device: %d", result);
                return R_CVulkan_ResultToError (result);
        }

        return R_CVULKAN_ERROR_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewDevice (
    struct R_CVulkan_Device* pDevice,
    const char*              pApplicationName,
    bool                     enableValidationLayers,
    bool                     headlessMode,
    VkSurfaceKHR             surface)
{
        R_CVULKAN_ASSERT (pDevice);

#if defined(R_CVULKAN_DEBUG)
        if (!pDevice)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
        pDevice->instance = VK_NULL_HANDLE;
        pDevice->physicalDevice = VK_NULL_HANDLE;
        pDevice->logicalDevice = VK_NULL_HANDLE;
#endif
        pDevice->debugMessenger = VK_NULL_HANDLE;
        pDevice->surface = surface;
        pDevice->enableValidationLayers = enableValidationLayers;
        pDevice->headlessMode = headlessMode;

#if defined(R_CVULKAN_DEBUG)
        pDevice->isInitialized = false;
#endif

        enum R_CVulkan_Error result = R_CVULKAN_ERROR_OK;

        if (enableValidationLayers && !R_CVulkan_CheckValidationLayerSupport ())
        {
                R_CSTL_LOG_ERROR ("Validation layers requested but not available");
                result = R_CVULKAN_ERROR_LAYER_NOT_FOUND;
                goto cvulkan_cleanup;
        }

        result = R_CVulkan_CreateVulkanInstance (
            pDevice,
            pApplicationName,
            enableValidationLayers,
            headlessMode);
        if (result != R_CVULKAN_ERROR_OK)
        {
                goto cvulkan_cleanup;
        }

        if (enableValidationLayers)
        {
                result = R_CVulkan_SetupDebugMessenger (pDevice->instance, &pDevice->debugMessenger);
                if (result != R_CVULKAN_ERROR_OK)
                {
                        goto cvulkan_cleanup;
                }
        }

        result = SelectPhysicalDevice (pDevice, surface, headlessMode);
        if (result != R_CVULKAN_ERROR_OK)
        {
                goto cvulkan_cleanup;
        }

        result = CreateLogicalDevice (pDevice, surface, headlessMode, enableValidationLayers);
        if (result != R_CVULKAN_ERROR_OK)
        {
                goto cvulkan_cleanup;
        }

        struct R_CVulkan_QueueFamilyIndices indices;
        result = R_CVulkan_DeviceFindQueueFamilies (pDevice->physicalDevice, surface, headlessMode, &indices);
        if (result != R_CVULKAN_ERROR_OK)
        {
                goto cvulkan_cleanup;
        }

        result = R_CVulkan_NewQueue (&pDevice->graphicsQueue, pDevice, indices.graphicsFamily, 0);
        if (result != R_CVULKAN_ERROR_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to create graphics queue");
                goto cvulkan_cleanup;
        }

        result = R_CVulkan_NewQueue (&pDevice->presentQueue, pDevice, indices.presentFamily, 0);
        if (result != R_CVULKAN_ERROR_OK)
        {
                R_CSTL_LOG_ERROR ("Failed to create present queue");
                R_CVulkan_DeleteQueue (&pDevice->graphicsQueue);
                goto cvulkan_cleanup;
        }

#if defined(R_CVULKAN_DEBUG)
        pDevice->isInitialized = true;
#endif
        return R_CVULKAN_ERROR_OK;

cvulkan_cleanup:
        if (pDevice->logicalDevice != VK_NULL_HANDLE)
        {
                vkDestroyDevice (pDevice->logicalDevice, NULL);
                pDevice->logicalDevice = VK_NULL_HANDLE;
        }
        R_CVulkan_DestroyDebugMessenger (pDevice->instance, pDevice->debugMessenger);
        pDevice->debugMessenger = VK_NULL_HANDLE;
        if (pDevice->instance != VK_NULL_HANDLE)
        {
                vkDestroyInstance (pDevice->instance, NULL);
                pDevice->instance = VK_NULL_HANDLE;
        }
        return result;
}

R_CVULKAN_API void
R_CVulkan_DeleteDevice (struct R_CVulkan_Device* pDevice)
{
        R_CVULKAN_ASSERT (pDevice);

#if defined(R_CVULKAN_DEBUG)
        if (!pDevice)
        {
                return;
        }
#endif
        R_CVulkan_DeleteQueue (&pDevice->graphicsQueue);
        R_CVulkan_DeleteQueue (&pDevice->presentQueue);

        if (pDevice->logicalDevice != VK_NULL_HANDLE)
        {
                vkDestroyDevice (pDevice->logicalDevice, NULL);
                pDevice->logicalDevice = VK_NULL_HANDLE;
        }

        R_CVulkan_DestroyDebugMessenger (pDevice->instance, pDevice->debugMessenger);
        pDevice->debugMessenger = VK_NULL_HANDLE;

        if (pDevice->instance != VK_NULL_HANDLE)
        {
                vkDestroyInstance (pDevice->instance, NULL);
                pDevice->instance = VK_NULL_HANDLE;
        }

#if defined(R_CVULKAN_DEBUG)
        pDevice->physicalDevice = VK_NULL_HANDLE;
        pDevice->surface = VK_NULL_HANDLE;
        pDevice->isInitialized = false;
#endif
}

R_CVULKAN_API VkInstance
R_CVulkan_DeviceGetInstance (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDevice != NULL);
#endif
        return pDevice->instance;
}

R_CVULKAN_API VkPhysicalDevice
R_CVulkan_DeviceGetPhysicalDevice (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDevice != NULL);
#endif
        return pDevice->physicalDevice;
}

R_CVULKAN_API VkDevice
R_CVulkan_DeviceGetLogicalDevice (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDevice != NULL);
#endif
        return pDevice->logicalDevice;
}

R_CVULKAN_API struct R_CVulkan_Queue*
R_CVulkan_DeviceGetGraphicsQueue (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDevice != NULL);
#endif
        return (struct R_CVulkan_Queue*)&pDevice->graphicsQueue;
}

R_CVULKAN_API struct R_CVulkan_Queue*
R_CVulkan_DeviceGetPresentQueue (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDevice != NULL);
#endif
        return (struct R_CVulkan_Queue*)&pDevice->presentQueue;
}

R_CVULKAN_API VkSurfaceKHR
R_CVulkan_DeviceGetSurface (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDevice != NULL);
#endif
        return pDevice->surface;
}

R_CVULKAN_API int
R_CVulkan_DeviceIsInitialized (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pDevice != NULL);
        return pDevice->isInitialized;
#else
        (void)pDevice;
        return 1;
#endif
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_DeviceFindQueueFamilies (
    VkPhysicalDevice                     physicalDevice,
    VkSurfaceKHR                         surface,
    int                                  headlessMode,
    struct R_CVulkan_QueueFamilyIndices* pOutIndices)
{
        R_CVULKAN_ASSERT (pOutIndices);
        R_CVULKAN_ASSERT (physicalDevice != VK_NULL_HANDLE);

#if defined(R_CVULKAN_DEBUG)
        if (!pOutIndices || physicalDevice == VK_NULL_HANDLE)
        {
                return R_CVULKAN_ERROR_NULL_POINTER;
        }
#endif
        memset (pOutIndices, 0, sizeof (struct R_CVulkan_QueueFamilyIndices));

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, NULL);

        if (queueFamilyCount == 0)
        {
                return R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND;
        }

        VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)R_CSTL_HeapAlloc (
            queueFamilyCount * sizeof (VkQueueFamilyProperties));
        if (queueFamilies == NULL)
        {
                return R_CVULKAN_ERROR_OUT_OF_MEMORY;
        }

        vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &queueFamilyCount, queueFamilies);

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                        pOutIndices->graphicsFamily = i;
                        pOutIndices->hasGraphicsFamily = true;
                }

                if (!headlessMode && surface != VK_NULL_HANDLE)
                {
                        VkBool32 presentSupport = VK_FALSE;
                        vkGetPhysicalDeviceSurfaceSupportKHR (physicalDevice, i, surface, &presentSupport);

                        if (presentSupport)
                        {
                                pOutIndices->presentFamily = i;
                                pOutIndices->hasPresentFamily = true;
                        }
                }
                else if (headlessMode)
                {
                        pOutIndices->presentFamily = pOutIndices->graphicsFamily;
                        pOutIndices->hasPresentFamily = pOutIndices->hasGraphicsFamily;
                }

                if (R_CVulkan_QueueFamilyIndicesIsComplete (pOutIndices))
                {
                        break;
                }
        }

        R_CSTL_HeapFree (queueFamilies);

        if (!R_CVulkan_QueueFamilyIndicesIsComplete (pOutIndices))
        {
                return R_CVULKAN_ERROR_QUEUE_FAMILY_NOT_FOUND;
        }

        return R_CVULKAN_ERROR_OK;
}

R_CVULKAN_API int
R_CVulkan_QueueFamilyIndicesIsComplete (const struct R_CVulkan_QueueFamilyIndices* pIndices)
{
#if defined(R_CVULKAN_DEBUG)
        R_CVULKAN_ASSERT (pIndices != NULL);
#endif
        return pIndices->hasGraphicsFamily && pIndices->hasPresentFamily;
}
