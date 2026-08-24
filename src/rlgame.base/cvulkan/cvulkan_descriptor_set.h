#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Configuration parameters for descriptor set layout creation
 */
struct R_CVulkan_DescriptorSetLayoutCreateInfo
{
        const struct R_CVulkan_Device*      device; /**< R_CVulkan device wrapper */
        const VkDescriptorSetLayoutBinding* bindings; /**< Array of descriptor bindings */
        uint32_t                            bindingCount; /**< Number of bindings */
};

/**
 * @brief Configuration parameters for descriptor pool creation
 */
struct R_CVulkan_DescriptorPoolCreateInfo
{
        const struct R_CVulkan_Device* device; /**< R_CVulkan device wrapper */
        const VkDescriptorPoolSize*    poolSizes; /**< Array of pool sizes */
        uint32_t                       poolSizeCount; /**< Number of pool sizes */
        uint32_t                       maxSets; /**< Maximum number of descriptor sets */
};

/**
 * @brief Safe wrapper for VkDescriptorSetLayout
 */
struct R_CVulkan_DescriptorSetLayout
{
        VkDescriptorSetLayout handle; /**< Raw Vulkan descriptor set layout handle */
        VkDevice              device; /**< Associated device */
        R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Safe wrapper for VkDescriptorPool
 */
struct R_CVulkan_DescriptorPool
{
        VkDescriptorPool handle; /**< Raw Vulkan descriptor pool handle */
        VkDevice         device; /**< Associated device */
        uint32_t         maxSets; /**< Maximum number of descriptor sets */
        R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Safe wrapper for VkDescriptorSet
 */
struct R_CVulkan_DescriptorSet
{
        VkDescriptorSet handle; /**< Raw Vulkan descriptor set handle */
        VkDevice        device; /**< Associated device */
        R_CVULKAN_DEBUG_FIELD
};

/**
 * @brief Initialize a descriptor set layout
 * @param layout Pointer to layout to initialize
 * @param pCreateInfo Descriptor set layout creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_NewDescriptorSetLayout (
    struct R_CVulkan_DescriptorSetLayout*                 layout,
    const struct R_CVulkan_DescriptorSetLayoutCreateInfo* pCreateInfo);

/**
 * @brief Deletes a descriptor set layout and destroy the Vulkan object
 * @param layout Pointer to layout to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteDescriptorSetLayout (struct R_CVulkan_DescriptorSetLayout* layout);

/**
 * @brief Initialize a descriptor pool
 * @param pool Pointer to pool to initialize
 * @param pCreateInfo Descriptor pool creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_NewDescriptorPool (
    struct R_CVulkan_DescriptorPool*                 pool,
    const struct R_CVulkan_DescriptorPoolCreateInfo* pCreateInfo);

/**
 * @brief Deletes a descriptor pool and destroy the Vulkan object
 * @param pool Pointer to pool to delete
 */
R_CVULKAN_API void R_CVulkan_DeleteDescriptorPool (struct R_CVulkan_DescriptorPool* pool);

/**
 * @brief Allocate descriptor sets from a pool
 * @param pool Descriptor pool
 * @param layouts Array of layouts to allocate from
 * @param layoutCount Number of layouts
 * @param outSets Pointer to receive the allocated descriptor sets
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkanError R_CVulkan_DescriptorSetAllocate (
    const struct R_CVulkan_DescriptorPool* pool,
    const VkDescriptorSetLayout*           layouts,
    uint32_t                               layoutCount,
    VkDescriptorSet*                       outSets);

/**
 * @brief Free descriptor sets
 * @param pool Descriptor pool
 * @param sets Array of descriptor sets to free
 * @param setCount Number of descriptor sets
 */
R_CVULKAN_API void R_CVulkan_DescriptorSetFree (
    const struct R_CVulkan_DescriptorPool* pool,
    const VkDescriptorSet*                 sets,
    uint32_t                               setCount);

/**
 * @brief Update descriptor sets
 * @param device R_CVulkan device wrapper
 * @param descriptorWrites Array of descriptor write operations
 * @param descriptorWriteCount Number of descriptor write operations
 * @param descriptorCopies Array of descriptor copy operations (can be NULL)
 * @param descriptorCopyCount Number of descriptor copy operations (can be 0)
 */
R_CVULKAN_API void R_CVulkan_DescriptorSetUpdate (
    const struct R_CVulkan_Device* device,
    const VkWriteDescriptorSet*    descriptorWrites,
    uint32_t                       descriptorWriteCount,
    const VkCopyDescriptorSet*     descriptorCopies,
    uint32_t                       descriptorCopyCount);

R_CVULKAN_API VkDescriptorSetLayout
R_CVulkan_DescriptorSetLayoutGetHandle (const struct R_CVulkan_DescriptorSetLayout* layout);

R_CVULKAN_API VkDevice
R_CVulkan_DescriptorSetLayoutGetDevice (const struct R_CVulkan_DescriptorSetLayout* layout);

R_CVULKAN_API int
R_CVulkan_DescriptorSetLayoutIsInitialized (const struct R_CVulkan_DescriptorSetLayout* layout);

R_CVULKAN_API VkDescriptorPool
R_CVulkan_DescriptorPoolGetHandle (const struct R_CVulkan_DescriptorPool* pool);

R_CVULKAN_API VkDevice R_CVulkan_DescriptorPoolGetDevice (const struct R_CVulkan_DescriptorPool* pool);

R_CVULKAN_API uint32_t R_CVulkan_DescriptorPoolGetMaxSets (const struct R_CVulkan_DescriptorPool* pool);

R_CVULKAN_API int R_CVulkan_DescriptorPoolIsInitialized (const struct R_CVulkan_DescriptorPool* pool);
