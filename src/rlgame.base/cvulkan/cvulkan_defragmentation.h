#pragma once

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "rlgame.base/cvulkan/cvulkan_platform.h"
#include "rlgame.base/cvulkan/cvulkan_memory_allocator.h"

/**
 * @brief Backend type for GPU defragmentation
 */
enum R_CVulkan_DefragBackend
{
    R_CVULKAN_DEFRAG_BACKEND_NONE = 0,
    R_CVULKAN_DEFRAG_BACKEND_CUDA,
    R_CVULKAN_DEFRAG_BACKEND_OPENCL,
    R_CVULKAN_DEFRAG_BACKEND_CPU
};

/**
 * @brief Block metadata for defragmentation
 */
struct R_CVulkan_DefragBlockMetadata
{
        uint32_t blockIndex;
        uint64_t totalSize;
        uint64_t usedSize;
        uint64_t freeSize;
        float    fillLevel;
        uint32_t allocationCount;
        uint32_t isCandidate;
};

/**
 * @brief Move operation for defragmentation
 */
struct R_CVulkan_DefragMove
{
        uint32_t srcBlockIndex;
        uint32_t dstBlockIndex;
        uint64_t srcOffset;
        uint64_t dstOffset;
        uint64_t size;
        uint32_t operation;
};

/**
 * @brief Move operation types
 */
enum R_CVulkan_DefragMoveOperation
{
    R_CVULKAN_DEFRAG_MOVE_OPERATION_MOVE = 0,
    R_CVULKAN_DEFRAG_MOVE_OPERATION_IGNORE,
    R_CVULKAN_DEFRAG_MOVE_OPERATION_DESTROY
};

/**
 * @brief Settingsuration for defragmentation
 */
struct R_CVulkan_DefragSettings
{
        uint32_t                     mergeFactor;
        uint64_t                     maxBytesPerPass;
        uint32_t                     maxPasses;
        enum R_CVulkan_DefragBackend preferredBackend;
};

/**
 * @brief Defragmentation context
 */
struct R_CVulkan_DefragContext
{
        struct R_CVulkan_MemoryAllocator* pAllocator;
        struct R_CVulkan_DefragSettings   config;
        enum R_CVulkan_DefragBackend      backend;
        uint32_t                          currentPass;
        uint32_t                          totalMoves;
        uint64_t                          totalBytesMoved;

        void*    pBlockMetadata;
        uint32_t blockMetadataCount;
        uint32_t blockMetadataCapacity;

        void*    pMoves;
        uint32_t moveCount;
        uint32_t moveCapacity;

        void* pBackendContext;
        bool  booted;
};

/**
 * @brief Statistics from defragmentation
 */
struct R_CVulkan_DefragStats
{
        uint32_t passesCompleted;
        uint32_t totalMoves;
        uint64_t totalBytesMoved;
        uint32_t blocksFreed;
        float    fragmentationBefore;
        float    fragmentationAfter;
};

/**
 * @brief Initialize defragmentation context
 * @param pContext Pointer to context to initialize
 * @param pAllocator Pointer to memory allocator
 * @param pSettings Settingsuration parameters (can be NULL for defaults)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_DefragInitialize (
    struct R_CVulkan_DefragContext*        pContext,
    struct R_CVulkan_MemoryAllocator*      pAllocator,
    const struct R_CVulkan_DefragSettings* pSettings);

/**
 * @brief Cleanup defragmentation context
 * @param pContext Pointer to context to cleanup
 */
R_CVULKAN_API void R_CVulkan_DefragCleanup (struct R_CVulkan_DefragContext* pContext);

/**
 * @brief Begin defragmentation process
 * @param pContext Pointer to defragmentation context
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error R_CVulkan_DefragBegin (struct R_CVulkan_DefragContext* pContext);

/**
 * @brief Execute a single defragmentation pass
 * @param pContext Pointer to defragmentation context
 * @param commandBuffer Vulkan command buffer for GPU operations (can be VK_NULL_HANDLE for CPU)
 * @return R_CVULKAN_OK on success, R_CVULKAN_ERROR_INCOMPLETE if more passes needed, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_DefragExecutePass (struct R_CVulkan_DefragContext* pContext, VkCommandBuffer commandBuffer);

/**
 * @brief End defragmentation process and get statistics
 * @param pContext Pointer to defragmentation context
 * @param pStats Pointer to receive statistics (can be NULL)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_DefragEnd (struct R_CVulkan_DefragContext* pContext, struct R_CVulkan_DefragStats* pStats);

/**
 * @brief Get current fragmentation level
 * @param pContext Pointer to defragmentation context
 * @param pFragmentation Pointer to receive fragmentation level (0.0 = no fragmentation, 1.0 = fully
 * fragmented)
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_DefragGetFragmentationLevel (const struct R_CVulkan_DefragContext* pContext, float* pFragmentation);

/**
 * @brief Check if defragmentation is needed
 * @param pContext Pointer to defragmentation context
 * @param pNeeded Pointer to receive whether defragmentation is needed
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_DefragIsNeeded (const struct R_CVulkan_DefragContext* pContext, int* pNeeded);

/**
 * @brief Get available backend
 * @param pBackend Pointer to receive available backend
 * @return R_CVULKAN_OK on success, error code otherwise
 */
R_CVULKAN_API enum R_CVulkan_Error
R_CVulkan_DefragGetAvailableBackend (enum R_CVulkan_DefragBackend* pBackend);

/**
 * @brief Set default configuration
 * @param pSettings Pointer to config to set defaults
 */
R_CVULKAN_API void R_CVulkan_DefragSetDefaultSettings (struct R_CVulkan_DefragSettings* pSettings);
