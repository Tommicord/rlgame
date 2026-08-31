#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"

struct R_CVulkan_Device;

/**
 * @brief Settingsuration parameters for descriptor set layout creation
 */
struct r_cvulkan_descriptor_set_layout_create_info
{
        const struct R_CVulkan_Device*      device; /**< R_CVulkan device wrapper */
        const VkDescriptorSetLayoutBinding* bindings; /**< Array of descriptor bindings */
        uint32_t                            bindingCount; /**< Number of bindings */
};

/**
 * @brief Settingsuration parameters for descriptor pool creation
 */
struct r_cvulkan_descriptor_pool_create_info
{
        const struct R_CVulkan_Device* device; /**< R_CVulkan device wrapper */
        const VkDescriptorPoolSize*    poolSizes; /**< Array of pool sizes */
        uint32_t                       poolSizeCount; /**< Number of pool sizes */
        uint32_t                       maxSets; /**< Maximum number of descriptor sets */
};

/**
 * @brief Safe wrapper for VkDescriptorSetLayout
 */
struct r_cvulkan_descriptor_set_layout
{
        VkDescriptorSetLayout handle; /**< Raw Vulkan descriptor set layout handle */
        VkDevice              device; /**< Associated device */
};

/**
 * @brief Safe wrapper for VkDescriptorPool
 */
struct R_CVulkan_DescriptorPool
{
        VkDescriptorPool handle; /**< Raw Vulkan descriptor pool handle */
        VkDevice         device; /**< Associated device */
        uint32_t         maxSets; /**< Maximum number of descriptor sets */
};

/**
 * @brief Safe wrapper for VkDescriptorSet
 */
struct R_CVulkan_DescriptorSet
{
        VkDescriptorSet handle; /**< Raw Vulkan descriptor set handle */
        VkDevice        device; /**< Associated device */
};

/**
 * @brief Initialize a descriptor set layout
 * @param layout Pointer to layout to initialize
 * @param pCreateInfo Descriptor set layout creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_new_descriptor_set_layout (
    struct r_cvulkan_descriptor_set_layout*                 layout,
    const struct r_cvulkan_descriptor_set_layout_create_info* pCreateInfo);

/**
 * @brief Deletes a descriptor set layout and destroy the Vulkan object
 * @param layout Pointer to layout to delete
 */
R_CVULKAN_API void r_cvulkan_delete_descriptor_set_layout (struct r_cvulkan_descriptor_set_layout* layout);

/**
 * @brief Initialize a descriptor pool
 * @param pool Pointer to pool to initialize
 * @param pCreateInfo Descriptor pool creation parameters
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_new_descriptor_pool (
    struct R_CVulkan_DescriptorPool*                 pool,
    const struct r_cvulkan_descriptor_pool_create_info* pCreateInfo);

/**
 * @brief Deletes a descriptor pool and destroy the Vulkan object
 * @param pool Pointer to pool to delete
 */
R_CVULKAN_API void r_cvulkan_delete_descriptor_pool (struct R_CVulkan_DescriptorPool* pool);

/**
 * @brief Allocate descriptor sets from a pool
 * @param pool Descriptor pool
 * @param layouts Array of layouts to allocate from
 * @param layoutCount Number of layouts
 * @param outSets Pointer to receive the allocated descriptor sets
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error r_cvulkan_descriptor_set_allocate (
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
R_CVULKAN_API void r_cvulkan_descriptor_set_free (
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
R_CVULKAN_API void r_cvulkan_descriptor_set_update (
    const struct R_CVulkan_Device* device,
    const VkWriteDescriptorSet*    descriptorWrites,
    uint32_t                       descriptorWriteCount,
    const VkCopyDescriptorSet*     descriptorCopies,
    uint32_t                       descriptorCopyCount);

R_CVULKAN_API VkDescriptorSetLayout
r_cvulkan_descriptor_set_layout_get_handle (const struct r_cvulkan_descriptor_set_layout* layout);

R_CVULKAN_API VkDevice
r_cvulkan_descriptor_set_layout_get_device (const struct r_cvulkan_descriptor_set_layout* layout);

R_CVULKAN_API int
r_cvulkan_descriptor_set_layout_is_initialized (const struct r_cvulkan_descriptor_set_layout* layout);

R_CVULKAN_API VkDescriptorPool
r_cvulkan_descriptor_pool_get_handle (const struct R_CVulkan_DescriptorPool* pool);

R_CVULKAN_API VkDevice r_cvulkan_descriptor_pool_get_device (const struct R_CVulkan_DescriptorPool* pool);

R_CVULKAN_API uint32_t r_cvulkan_descriptor_pool_get_max_sets (const struct R_CVulkan_DescriptorPool* pool);
