#include "rlgame.base/cvulkan/cvulkan_instance.h"
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

#if defined(R_CVULKAN_DEBUG)
static const char*    g_validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
static const uint32_t g_validationLayerCount = R_CVULKAN_VALIDATION_LAYER_SIZE (g_validationLayers);
#else
static const char*    g_validationLayers[] = {NULL};
static const uint32_t g_validationLayerCount = 0;
#endif

static const char*    g_deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static const uint32_t g_deviceExtensionCount = R_CVULKAN_VALIDATION_LAYER_SIZE (g_deviceExtensions);

static enum R_CVulkan_Error
r_cvulkan_build_instance_extensions (struct r_cstl_array** ppExtensions, bool hasValidationFeatures);

static enum R_CVulkan_Error r_cvulkan_check_extension_availability (
    const char*      pExtensionName,
    VkPhysicalDevice physicalDevice,
    bool*            pIsAvailable);

static void r_cvulkan_log_extension_list (const struct r_cstl_array* pExtensions);

static const char**
r_cvulkan_string_array_extensions_data (const struct r_cstl_array* pStringArray, size_t* pOutCount);

static void r_cvulkan_free_string_array_contents (const struct r_cstl_array* pStringArray);

#if defined(R_CVULKAN_DEBUG)
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
#endif

#if defined(R_CVULKAN_DEBUG)
static enum R_CVulkan_Error
r_cvulkan_setup_debug_messenger (VkInstance instance, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity
        = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
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
        enum R_CVulkan_Error error = r_cvulkan_result_to_error (result);
        R_CSTL_LOG_ERROR ("Failed to create debug messenger: %s", r_cvulkan_error_to_string (error));
        return error;
    }
    return R_CVULKAN_OK;
}
#endif

#if defined(R_CVULKAN_DEBUG)
static void
r_cvulkan_destroy_debug_messenger (VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
    if (debugMessenger == VK_NULL_HANDLE)
    {
        return;
    }

    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr (
        instance,
        "vkDestroyDebugUtilsMessengerEXT");
    if (func)
    {
        func (instance, debugMessenger, NULL);
    }
}
#endif

#if defined(R_CVULKAN_DEBUG)
static bool
r_cvulkan_check_validation_layer_support (void)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties (&layerCount, NULL);

    if (layerCount == 0) return false;
    VkLayerProperties* availableLayers
        = (VkLayerProperties*)r_cstl_heap_alloc (layerCount * sizeof (VkLayerProperties));
    if (availableLayers == NULL)
    {
        R_CSTL_LOG_ERROR ("Failed to allocate memory for layer properties");
        return false;
    }

    vkEnumerateInstanceLayerProperties (&layerCount, availableLayers);

    bool foundAll = true;
    for (uint32_t i = 0; i < g_validationLayerCount && foundAll; ++i)
    {
        bool layerFound = false;
        for (uint32_t j = 0; j < layerCount; ++j)
        {
            if (strcmp (g_validationLayers[i], availableLayers[j].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
        {
            foundAll = false;
        }
    }

    r_cstl_heap_free (availableLayers);
    return foundAll;
}
#endif

static enum R_CVulkan_Error
r_cvulkan_check_extension_availability (
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
        return R_CVULKAN_OK;
    }

    VkExtensionProperties* extensions
        = (VkExtensionProperties*)r_cstl_heap_alloc (extensionCount * sizeof (VkExtensionProperties));
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

    r_cstl_heap_free (extensions);
    return R_CVULKAN_OK;
}

static void
r_cvulkan_log_extension_list (const struct r_cstl_array* pExtensions)
{
    if (pExtensions == NULL)
    {
        return;
    }
    size_t length = r_cstl_array_length (pExtensions);
    size_t elementCount = length / sizeof (const char*);

    R_CSTL_LOG_DEBUG ("Instance Extensions (%zu):", elementCount);
    for (size_t i = 0; i < elementCount; ++i)
    {
        const char* ext = NULL;
        r_cstl_array_typed_at (pExtensions, const char*, i, &ext);
        if (ext)
        {
            R_CSTL_LOG_DEBUG ("  - %s", ext);
        }
    }
}

static const char**
r_cvulkan_string_array_extensions_data (const struct r_cstl_array* pStringArray, size_t* pOutCount)
{
    size_t length = r_cstl_array_length (pStringArray);
    size_t elementCount = length / sizeof (const char*);

    const char** ppStrings = (const char**)r_cstl_heap_alloc (elementCount * sizeof (const char*));
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
        r_cstl_array_typed_at (pStringArray, const char*, i, &pString);
        ppStrings[i] = pString;
    }

    if (pOutCount)
    {
        *pOutCount = elementCount;
    }
    return ppStrings;
}

static enum R_CVulkan_Error
r_cvulkan_build_instance_extensions (struct r_cstl_array** ppExtensions, bool hasValidationFeatures)
{
    R_CVULKAN_ASSERT (ppExtensions);

    struct r_cstl_array* pExtensions = r_cstl_new_array ();
    if (pExtensions == NULL)
    {
        return R_CVULKAN_ERROR_OUT_OF_MEMORY;
    }
    bool                 hasPortability = false;
    enum R_CVulkan_Error err = r_cvulkan_check_extension_availability (
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_NULL_HANDLE,
        &hasPortability);
    if (err != R_CVULKAN_OK)
    {
        r_cstl_delete_array (pExtensions);
        return err;
    }
#if defined(_WIN32)
#define RL_KHR_SURFACE VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__linux__)
#define RL_KHR_SURFACE      VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#define RL_KHR_XLIB_SURFACE VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#define RL_KHR_XCB_SURFACE  VK_KHR_XCB_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
#define RL_KHR_SURFACE VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#else
#define RL_KHR_SURFACE VK_KHR_SURFACE_EXTENSION_NAME
#endif

#if !defined(R_CVULKAN_HEADLESS)
    static const char* pExt1 = VK_KHR_SURFACE_EXTENSION_NAME;
    r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExt1, sizeof (const char*));
    static const char* pExt2 = RL_KHR_SURFACE;
    r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExt2, sizeof (const char*));
#if defined(__linux__)
    static const char* pExtXlib = RL_KHR_XLIB_SURFACE;
    r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExtXlib, sizeof (const char*));
    static const char* pExtXcb = RL_KHR_XCB_SURFACE;
    r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExtXcb, sizeof (const char*));
#endif
#endif

    if (hasPortability)
    {
        static const char* pExt = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
        r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExt, sizeof (const char*));
    }

#if defined(R_CVULKAN_DEBUG)
    static const char* pExt3 = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExt3, sizeof (const char*));
    static const char* pExt4 = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExt4, sizeof (const char*));
#endif
    if (hasValidationFeatures)
    {
        static const char* pExt5 = VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME;
        r_cstl_array_push_data (pExtensions, (const uint8_t*)&pExt5, sizeof (const char*));
    }

#undef RL_KHR_SURFACE
    *ppExtensions = pExtensions;
    r_cvulkan_log_extension_list (pExtensions);
    return R_CVULKAN_OK;
}

R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_NewInstance (
    struct R_CVulkan_Instance*                 pInstance,
    const struct r_cvulkan_instance_create_info* pCreateInfo)
{
    R_CVULKAN_ASSERT (pCreateInfo);
    R_CSTL_TRACE_SCOPE_CTX ("app=%s", pCreateInfo->pApplicationName ? pCreateInfo->pApplicationName : "NULL");
    R_CVULKAN_ASSERT (pInstance);
    R_CVULKAN_ASSERT (pCreateInfo);

#if defined(R_CVULKAN_DEBUG)
    if (!pInstance || !pCreateInfo)
    {
        R_CSTL_TRACE_SCOPE_EXIT ();
        return R_CVULKAN_ERROR_NULL_POINTER;
    }
    pInstance->handle = VK_NULL_HANDLE;
    pInstance->debugMessenger = VK_NULL_HANDLE;

#endif

    enum R_CVulkan_Error result = R_CVULKAN_OK;
#if defined(R_CVULKAN_DEBUG)
    bool validationLayersSupported = r_cvulkan_check_validation_layer_support ();
    if (!validationLayersSupported)
    {
        R_CSTL_LOG_ERROR (
            "Validation layers not available. Install Vulkan SDK with "
            "validation layers.");
        result = R_CVULKAN_ERROR_LAYER_NOT_FOUND;
        goto cvulkan_cleanup;
    }
#endif
    bool hasValidationFeatures = false;
    bool hasGpuAssisted = false;
#if defined(R_CVULKAN_DEBUG)
    result = r_cvulkan_check_extension_availability (
        VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
        VK_NULL_HANDLE,
        &hasValidationFeatures);
    if (result != R_CVULKAN_OK)
    {
        goto cvulkan_cleanup;
    }

    result = r_cvulkan_check_extension_availability (
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_NULL_HANDLE,
        &hasGpuAssisted);
    if (result != R_CVULKAN_OK)
    {
        goto cvulkan_cleanup;
    }
#endif

#if defined(R_CVULKAN_DEBUG)
    VkValidationFeaturesEXT      validationFeatures = {0};
    VkValidationFeatureEnableEXT enabledValidationFeatures[4];
    uint32_t                     enabledValidationFeatureCount = 0;

    if (hasValidationFeatures)
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
#else
    (void)hasValidationFeatures;
    (void)hasGpuAssisted;
#endif

    struct r_cstl_array* pExtensions = NULL;
    result = r_cvulkan_build_instance_extensions (&pExtensions, hasValidationFeatures);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to build instance extensions: %s", r_cvulkan_error_to_string (result));
        goto cvulkan_cleanup;
    }
    R_CVULKAN_ASSERT (pCreateInfo->pApplicationName);
    const char* pApplicationName = pCreateInfo->pApplicationName;
    const char* pEngineName = pCreateInfo->pEngineName;
    if (!pEngineName)
    {
        pEngineName = "rlgame (UNKNOWN)";
    }
    uint32_t applicationVersion = pCreateInfo->applicationVersion;
    uint32_t engineVersion = pCreateInfo->engineVersion;
    uint32_t apiVersion = VK_API_VERSION_1_2;

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = pApplicationName;
    appInfo.applicationVersion = applicationVersion;
    appInfo.pEngineName = pEngineName;
    appInfo.engineVersion = engineVersion;
    appInfo.apiVersion = apiVersion;
    appInfo.pNext = NULL;

    VkInstanceCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

#if defined(R_CVULKAN_DEBUG)
    createInfo.enabledLayerCount = g_validationLayerCount;
    createInfo.ppEnabledLayerNames = g_validationLayers;
    if (hasValidationFeatures)
    {
        createInfo.pNext = &validationFeatures;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }
#endif
    bool hasPortability = false;
    r_cvulkan_check_extension_availability (
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_NULL_HANDLE,
        &hasPortability);
    if (hasPortability)
    {
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    size_t       extensionCount = 0;
    const char** ppExtensionNames = r_cvulkan_string_array_extensions_data (pExtensions, &extensionCount);

    createInfo.enabledExtensionCount = (uint32_t)extensionCount;
    createInfo.ppEnabledExtensionNames = ppExtensionNames;

    VkResult result1 = vkCreateInstance (&createInfo, NULL, &pInstance->handle);
    r_cstl_heap_free (ppExtensionNames);
    r_cstl_delete_array (pExtensions);

    if (result1 != VK_SUCCESS)
    {
        enum R_CVulkan_Error err = r_cvulkan_result_to_error (result1);
        R_CSTL_LOG_ERROR (
            "Failed to create Vulkan instance: %s. Check Vulkan driver installation.",
            r_cvulkan_error_to_string (err));
        result = err;
        goto cvulkan_cleanup;
    }

#if defined(R_CVULKAN_DEBUG)
    result = r_cvulkan_setup_debug_messenger (pInstance->handle, &pInstance->debugMessenger);
    if (result != R_CVULKAN_OK)
    {
        R_CSTL_LOG_ERROR ("Failed to setup debug messenger: %s", r_cvulkan_error_to_string (result));
        goto cvulkan_cleanup;
    }
#endif

#if defined(R_CVULKAN_DEBUG)

#endif
    R_CSTL_TRACE_SCOPE_EXIT ();
    return R_CVULKAN_OK;

cvulkan_cleanup:
#if defined(R_CVULKAN_DEBUG)

    r_cvulkan_destroy_debug_messenger (pInstance->handle, pInstance->debugMessenger);
#endif
    vkDestroyInstance (pInstance->handle, NULL);
    R_CSTL_TRACE_SCOPE_EXIT ();
    return result;
}

R_CVULKAN_API void
R_CVulkan_DeleteInstance (struct R_CVulkan_Instance* pInstance)
{
    R_CVULKAN_ASSERT (pInstance);
#if defined(R_CVULKAN_DEBUG)
    r_cvulkan_destroy_debug_messenger (pInstance->handle, pInstance->debugMessenger);
#endif
    vkDestroyInstance (pInstance->handle, NULL);
#if defined(R_CVULKAN_DEBUG)

#endif
}

R_CVULKAN_API VkInstance
r_cvulkan_instance_get_handle (const struct R_CVulkan_Instance* pInstance)
{
#if defined(R_CVULKAN_DEBUG)
    R_CVULKAN_ASSERT (pInstance);
#endif
    return pInstance->handle;
}