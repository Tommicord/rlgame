#include "rlgame.base/cvulkan/cvulkan_device.h"
#include "rlgame.base/cvulkan/cvulkan_instance.h"
#include "rlgame.base/cvulkan/cvulkan_queue.h"
#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_surface.h"
#include "rlgame.base/cstl/cstl_array.h"
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
#include "rlgame.base/cstl/cstl_log.h"
#include "rlgame.base/cstl/cstl_trace.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

#define R_CVULKAN_VALIDATION_LAYER_SIZE(VarName) sizeof (VarName) / sizeof (VarName[0])

static const char*    g_deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static const uint32_t g_deviceExtensionCount = R_CVULKAN_VALIDATION_LAYER_SIZE (g_deviceExtensions);

static const char* g_optionalDeviceExtensions[]
    = {VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
       VK_EXT_DEVICE_FAULT_EXTENSION_NAME,
       VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME,
       VK_EXT_DEBUG_MARKER_EXTENSION_NAME};
static const uint32_t g_optionalDeviceExtensionCount
    = R_CVULKAN_VALIDATION_LAYER_SIZE (g_optionalDeviceExtensions);

static enum R_CVulkanError
R_CVulkan_BuildDeviceExtensions (struct R_CSTL_Array** ppExtensions, VkPhysicalDevice physicalDevice);

static enum R_CVulkanError
R_CVulkan_CheckExtensionAvailability (
    const char*      pExtensionName,
    VkPhysicalDevice physicalDevice,
    bool*            pIsAvailable)
{
    R_CVULKAN_ASSERT (pExtensionName);
    R_CVULKAN_ASSERT (pIsAvailable);

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties (physicalDevice, NULL, &extensionCount, NULL);

    if (extensionCount == 0)
    {
        R_CSTL_LOG_DEBUG ("R_CVulkan_CheckExtensionAvailability: No extensions available on device");
        *pIsAvailable = false;
        return R_CVULKAN_OK;
    }

    VkExtensionProperties* extensions
        = (VkExtensionProperties*)R_CSTL_HeapAlloc (extensionCount * sizeof (VkExtensionProperties));
    if (extensions == NULL)
    {
        R_CSTL_LOG_ERROR (
            "R_CVulkan_CheckExtensionAvailability: Failed to allocate memory for extension "
            "properties");
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    vkEnumerateDeviceExtensionProperties (physicalDevice, NULL, &extensionCount, extensions);

    *pIsAvailable = false;
    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        if (strcmp (extensions[i].extensionName, pExtensionName) == 0)
        {
            *pIsAvailable = true;
            R_CSTL_LOG_DEBUG ("R_CVulkan_CheckExtensionAvailability: Found extension '%s'", pExtensionName);
            break;
        }
    }
    bool available = *pIsAvailable;
    if (!available)
    {
        R_CSTL_LOG_WARN (
            "R_CVulkan_CheckExtensionAvailability: Extension '%s' not available",
            pExtensionName);
    }

    R_CSTL_HeapFree (extensions);
    return R_CVULKAN_OK;
}

static void R_CVulkan_LogExtensionList (const struct R_CSTL_Array* pExtensions);

static const char**
R_CVulkan_StringArrayExtensionsData (const struct R_CSTL_Array* pStringArray, size_t* pOutCount)
{
    size_t length = R_CSTL_ArrayLength (pStringArray);
    size_t elementCount = length / sizeof (const char*);

    const char** ppStrings = (const char**)R_CSTL_HeapAlloc (elementCount * sizeof (const char*));
    if (ppStrings == NULL)
    {
        if (pOutCount)
        {
            *pOutCount = 0;
        }
        return NULL;
    }

    for (size_t i = 0; i < elementCount; ++i)
    {
        const char* pString = NULL;
        R_CSTL_ArrayTypedAt (pStringArray, const char*, i, &pString);
        ppStrings[i] = pString;
    }

    if (pOutCount)
    {
        *pOutCount = elementCount;
    }
    return ppStrings;
}

static void
R_CVulkan_LogExtensionList (const struct R_CSTL_Array* pExtensions)
{
    if (pExtensions == NULL)
    {
        return;
    }
    size_t length = R_CSTL_ArrayLength (pExtensions);
    size_t elementCount = length / sizeof (const char*);

    R_CSTL_LOG_DEBUG ("Device Extensions (%zu):", elementCount);

    for (size_t i = 0; i < elementCount; ++i)
    {
        const char* ext = NULL;
        R_CSTL_ArrayTypedAt (pExtensions, const char*, i, &ext);
        if (ext)
        {
            R_CSTL_LOG_DEBUG ("  - %s", ext);
        }
    }
}

static enum R_CVulkanError
R_CVulkan_BuildDeviceExtensions (struct R_CSTL_Array** ppExtensions, VkPhysicalDevice physicalDevice)
{
    R_CVULKAN_ASSERT (ppExtensions);
    R_CVULKAN_ASSERT (physicalDevice != VK_NULL_HANDLE);

    struct R_CSTL_Array* pExtensions = R_CSTL_NewArray ();
    if (pExtensions == NULL)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

#if !defined(R_CVULKAN_HEADLESS)
    for (uint32_t i = 0; i < g_deviceExtensionCount; ++i)
    {
        R_CSTL_ArrayPushData (pExtensions, (const uint8_t*)&g_deviceExtensions[i], sizeof (const char*));
    }
#endif

    for (uint32_t i = 0; i < g_optionalDeviceExtensionCount; ++i)
    {
        bool                isAvailable = false;
        enum R_CVulkanError err = R_CVulkan_CheckExtensionAvailability (
            g_optionalDeviceExtensions[i],
            physicalDevice,
            &isAvailable);
        if (err != R_CVULKAN_OK)
        {
            R_CSTL_DeleteArray (pExtensions);
            return err;
        }

        if (isAvailable)
        {
            R_CSTL_LOG_DEBUG ("Optional device extension available: %s", g_optionalDeviceExtensions[i]);
            R_CSTL_ArrayPushData (
                pExtensions,
                (const uint8_t*)&g_optionalDeviceExtensions[i],
                sizeof (const char*));
        }
    }
    *ppExtensions = pExtensions;
    R_CVulkan_LogExtensionList (pExtensions);
    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_SelectPhysicalDevice (
    const struct R_CVulkan_Instance* pInstance,
    struct R_CVulkan_Device*         pDevice,
    VkSurfaceKHR                     surface)
{
    VkInstance vkInstance = R_CVulkan_InstanceGetHandle (pInstance);
    uint32_t   deviceCount = 0;
    vkEnumeratePhysicalDevices (vkInstance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
        R_CSTL_LOG_ERROR ("No physical devices found. Check Vulkan driver installation.");
        return R_CVULKAN_ERROR_PHYSICAL_DEVICE_NOT_FOUND;
    }

    VkPhysicalDevice* devices = (VkPhysicalDevice*)R_CSTL_HeapAlloc (deviceCount * sizeof (VkPhysicalDevice));
    if (devices == NULL)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    vkEnumeratePhysicalDevices (vkInstance, &deviceCount, devices);
    R_CSTL_LOG_INFO ("Found %u physical device(s)", deviceCount);

    struct R_CVulkan_QueueFamilyIndices indices;
    bool                                deviceFound = false;

    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        enum R_CVulkanError err = R_CVulkan_DeviceFindQueueFamilies (devices[i], surface, &indices);
        if (err != R_CVULKAN_OK)
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

    return R_CVULKAN_OK;
}

static enum R_CVulkanError
R_CVulkan_CreateLogicalDevice (struct R_CVulkan_Device* pDevice, VkSurfaceKHR surface)
{
    enum R_CVulkanError result = R_CVULKAN_OK;

    struct R_CVulkan_QueueFamilyIndices indices;
    result = R_CVulkan_DeviceFindQueueFamilies (pDevice->physicalDevice, surface, &indices);
    if (result != R_CVULKAN_OK)
    {
        return result;
    }
    struct R_CSTL_Array* queueCreateInfos = R_CSTL_NewArray ();
    struct R_CSTL_Array* uniqueQueueFamilies = R_CSTL_NewArray ();

    if (indices.graphicsFamily == indices.presentFamily)
    {
        R_CSTL_ArrayPushData (
            uniqueQueueFamilies,
            (const uint8_t*)&indices.graphicsFamily,
            sizeof (uint32_t));
    }
    else
    {
        R_CSTL_ArrayPushData (
            uniqueQueueFamilies,
            (const uint8_t*)&indices.graphicsFamily,
            sizeof (uint32_t));
        R_CSTL_ArrayPushData (uniqueQueueFamilies, (const uint8_t*)&indices.presentFamily, sizeof (uint32_t));
    }

    float queuePriority = 1.0f;
    for (uint32_t i = 0; i < R_CSTL_ArrayLength (uniqueQueueFamilies) / sizeof (uint32_t); ++i)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {0};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        uint32_t familyIndex = 0;
        R_CSTL_ArrayTypedAt (uniqueQueueFamilies, uint32_t, i, &familyIndex);
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        R_CSTL_ArrayPushData (
            queueCreateInfos,
            (const uint8_t*)&queueCreateInfo,
            sizeof (VkDeviceQueueCreateInfo));
    }
    VkPhysicalDeviceFeatures deviceFeatures = {0};

    struct R_CSTL_Array* pExtensions = NULL;
    result = R_CVulkan_BuildDeviceExtensions (&pExtensions, pDevice->physicalDevice);
    if (result != R_CVULKAN_OK)
    {
        return result;
    }

    VkDeviceCreateInfo deviceCreateInfo = {0};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = (VkDeviceQueueCreateInfo*)R_CSTL_ArrayData (queueCreateInfos);
    deviceCreateInfo.queueCreateInfoCount
        = R_CSTL_ArrayLength (queueCreateInfos) / sizeof (VkDeviceQueueCreateInfo);
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    size_t       extensionCount = 0;
    const char** ppExtensionNames = R_CVulkan_StringArrayExtensionsData (pExtensions, &extensionCount);

    deviceCreateInfo.enabledExtensionCount = (uint32_t)extensionCount;
    deviceCreateInfo.ppEnabledExtensionNames = ppExtensionNames;

    deviceCreateInfo.enabledLayerCount = 0;

    VkResult result1
        = vkCreateDevice (pDevice->physicalDevice, &deviceCreateInfo, NULL, &pDevice->logicalDevice);
    R_CSTL_HeapFree (ppExtensionNames);
    R_CSTL_DeleteArray (pExtensions);

    R_CSTL_DeleteArray (queueCreateInfos);
    R_CSTL_DeleteArray (uniqueQueueFamilies);

    if (result1 != VK_SUCCESS)
    {
        R_CSTL_LOG_ERROR ("Failed to create logical device: %d", result1);
        return R_CVulkan_ResultToError (result1);
    }

    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_NewDevice (struct R_CVulkan_Device* pDevice, const struct R_CVulkan_DeviceCreateInfo* pCreateInfo)
{
    R_CVULKAN_ASSERT (pCreateInfo);

    R_CSTL_TRACE_SCOPE_CTX ("instance=%p surface=%p", pCreateInfo->pInstance, pCreateInfo->pSurface);

    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pCreateInfo);

#if defined(R_CVULKAN_DEBUG)
    if (!pDevice || !pCreateInfo)
    {
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    pDevice->physicalDevice = VK_NULL_HANDLE;
    pDevice->logicalDevice = VK_NULL_HANDLE;
    pDevice->booted = false;
#endif

    if (pCreateInfo->pInstance == NULL)
    {
        R_CSTL_LOG_ERROR (
            "Instance is NULL. Create instance using R_CVulkan_NewInstance before creating device.");
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    if (!R_CVulkan_InstanceIsInitialized (pCreateInfo->pInstance))
    {
        R_CSTL_LOG_ERROR ("Instance not initialized. Call R_CVulkan_NewInstance first.");
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    pDevice->pInstance = pCreateInfo->pInstance;
    enum R_CVulkanError result = R_CVULKAN_OK;

#if !defined(R_CVULKAN_HEADLESS)
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (pCreateInfo->pSurface)
    {
        surface = R_CVulkan_SurfaceGetHandle (pCreateInfo->pSurface);
        if (surface == VK_NULL_HANDLE)
        {
            R_CSTL_LOG_ERROR ("Surface provided but handle is NULL. Check surface initialization.");
            result = R_CVULKAN_ERROR_SURFACE_NOT_PRESENT;
            goto cvulkan_cleanup;
        }
    }
    else
    {
        R_CSTL_LOG_ERROR (
            "Surface required for presentation but not provided. Create surface using "
            "R_CVulkan_NewSurface.");
        result = R_CVULKAN_ERROR_SURFACE_NOT_PRESENT;
        goto cvulkan_cleanup;
    }
#else
    VkSurfaceKHR surface = VK_NULL_HANDLE;
#endif

    result = R_CVulkan_SelectPhysicalDevice (pCreateInfo->pInstance, pDevice, surface);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to select physical device: %s", R_CVulkanErrorToString (result));
        goto cvulkan_cleanup;
    }

    result = R_CVulkan_CreateLogicalDevice (pDevice, surface);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create logical device: %s", R_CVulkanErrorToString (result));
        goto cvulkan_cleanup;
    }

#if defined(R_CVULKAN_DEBUG)
    pDevice->booted = true;
#endif

    struct R_CVulkan_QueueFamilyIndices indices;
    result = R_CVulkan_DeviceFindQueueFamilies (pDevice->physicalDevice, surface, &indices);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to find queue families: %s", R_CVulkanErrorToString (result));
        goto cvulkan_cleanup;
    }

    result = R_CVulkan_NewQueue (&pDevice->graphicsQueue, pDevice, indices.graphicsFamily, 0);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create graphics queue: %s", R_CVulkanErrorToString (result));
        goto cvulkan_cleanup;
    }

#if !defined(R_CVULKAN_HEADLESS)
    result = R_CVulkan_NewQueue (&pDevice->presentQueue, pDevice, indices.presentFamily, 0);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to create present queue: %s", R_CVulkanErrorToString (result));
        R_CVulkan_DeleteQueue (&pDevice->graphicsQueue);
        goto cvulkan_cleanup;
    }
#endif

    R_CSTL_TRACE_SCOPE_EXIT ();
    return R_CVULKAN_OK;

cvulkan_cleanup:
    if (pDevice->logicalDevice != VK_NULL_HANDLE)
    {
        vkDestroyDevice (pDevice->logicalDevice, NULL);
        pDevice->logicalDevice = VK_NULL_HANDLE;
    }
    pDevice->pInstance = NULL;
    R_CSTL_TRACE_SCOPE_EXIT ();
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

#if defined(R_CVULKAN_DEBUG)
    pDevice->physicalDevice = VK_NULL_HANDLE;
    pDevice->surface = VK_NULL_HANDLE;
    pDevice->pInstance = NULL;
    pDevice->booted = false;
#endif
}

R_CVULKAN_API const struct R_CVulkan_Instance*
R_CVulkan_DeviceGetInstance (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
#endif
    return pDevice->pInstance;
}

R_CVULKAN_API VkPhysicalDevice
R_CVulkan_DeviceGetPhysicalDevice (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
#endif
    return pDevice->physicalDevice;
}

R_CVULKAN_API VkDevice
R_CVulkan_DeviceGetLogicalDevice (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
#endif
    return pDevice->logicalDevice;
}

R_CVULKAN_API struct R_CVulkan_Queue*
R_CVulkan_DeviceGetGraphicsQueue (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
#endif
    return (struct R_CVulkan_Queue*)&pDevice->graphicsQueue;
}

R_CVULKAN_API struct R_CVulkan_Queue*
R_CVulkan_DeviceGetPresentQueue (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
#endif
    return (struct R_CVulkan_Queue*)&pDevice->presentQueue;
}

R_CVULKAN_API VkSurfaceKHR
R_CVulkan_DeviceGetSurface (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
#endif
    return pDevice->surface;
}

R_CVULKAN_API int
R_CVulkan_DeviceIsInitialized (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
    return pDevice->booted;
#else
    (void)pDevice;
    return 1;
#endif
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DeviceFindQueueFamilies (
    VkPhysicalDevice                     physicalDevice,
    VkSurfaceKHR                         surface,
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

    VkQueueFamilyProperties* queueFamilies
        = (VkQueueFamilyProperties*)R_CSTL_HeapAlloc (queueFamilyCount * sizeof (VkQueueFamilyProperties));
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

        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            pOutIndices->computeFamily = i;
            pOutIndices->hasComputeFamily = true;
        }

        if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            pOutIndices->transferFamily = i;
            pOutIndices->hasTransferFamily = true;
        }
#if !defined(R_CVULKAN_HEADLESS)
        if (surface != VK_NULL_HANDLE)
        {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR (physicalDevice, i, surface, &presentSupport);

            if (presentSupport)
            {
                pOutIndices->presentFamily = i;
                pOutIndices->hasPresentFamily = true;
            }
        }
#else
        pOutIndices->presentFamily = pOutIndices->graphicsFamily;
        pOutIndices->hasPresentFamily = pOutIndices->hasGraphicsFamily;
#endif

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

    return R_CVULKAN_OK;
}

R_CVULKAN_API int
R_CVulkan_QueueFamilyIndicesIsComplete (const struct R_CVulkan_QueueFamilyIndices* pIndices)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pIndices);
#endif
    return pIndices->hasGraphicsFamily && pIndices->hasPresentFamily && pIndices->hasComputeFamily
           && pIndices->hasTransferFamily;
}

R_CVULKAN_API int
R_CVulkan_DeviceIsDynamicRenderingSupported (const struct R_CVulkan_Device* pDevice)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pDevice);
    if (!pDevice)
    {
        return 0;
    }
#endif
    if (pDevice->physicalDevice == VK_NULL_HANDLE)
    {
        return 0;
    }

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {0};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {0};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &dynamicRenderingFeatures;

    vkGetPhysicalDeviceFeatures2 (pDevice->physicalDevice, &deviceFeatures2);
    return dynamicRenderingFeatures.dynamicRendering;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DeviceQueryExtensionSupport (
    const struct R_CVulkan_Device* pDevice,
    const char*                    pExtensionName,
    bool*                          pIsSupported)
{
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pExtensionName);
    R_CVULKAN_ASSERT (pIsSupported);

#if defined(R_CVULKAN_DEBUG)
    if (!pDevice || !pExtensionName || !pIsSupported)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
#endif

    if (pDevice->physicalDevice == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    *pIsSupported = false;

    uint32_t extensionCount = 0;
    VkResult result
        = vkEnumerateDeviceExtensionProperties (pDevice->physicalDevice, NULL, &extensionCount, NULL);
    if (result != VK_SUCCESS)
    {
        return R_CVulkan_ResultToError (result);
    }

    if (extensionCount == 0)
    {
        return R_CVULKAN_OK; // No extensions available, not supported
    }

    VkExtensionProperties* extensions
        = (VkExtensionProperties*)R_CSTL_HeapAlloc (extensionCount * sizeof (VkExtensionProperties));
    if (extensions == NULL)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }

    result
        = vkEnumerateDeviceExtensionProperties (pDevice->physicalDevice, NULL, &extensionCount, extensions);
    if (result != VK_SUCCESS)
    {
        R_CSTL_HeapFree (extensions);
        return R_CVulkan_ResultToError (result);
    }

    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        if (strcmp (extensions[i].extensionName, pExtensionName) == 0)
        {
            *pIsSupported = true;
            break;
        }
    }

    R_CSTL_HeapFree (extensions);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkanError
R_CVulkan_DeviceQueryFeatureSupport (const struct R_CVulkan_Device* pDevice, void* pFeatureStructure)
{
    R_CVULKAN_ASSERT (pDevice);
    R_CVULKAN_ASSERT (pFeatureStructure);

#if defined(R_CVULKAN_DEBUG)
    if (!pDevice || !pFeatureStructure)
    {
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
#endif

    if (pDevice->physicalDevice == VK_NULL_HANDLE)
    {
        return R_CVULKAN_ERROR_NOT_INITIALIZED;
    }

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {0};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = pFeatureStructure;

    vkGetPhysicalDeviceFeatures2 (pDevice->physicalDevice, &deviceFeatures2);
    return R_CVULKAN_OK;
}
