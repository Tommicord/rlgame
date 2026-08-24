#include <cuda_runtime.h>
#include <device_launch_parameters.h>

/**
 * @brief Block metadata structure for GPU processing
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
 * @brief Move operation structure for GPU processing
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
 * @brief CUDA kernel for block analysis, calculates fill levels and identifies candidates
 * @param blockMetadata Array of block metadata
 * @param blockCount Number of blocks
 * @param mergeFactor Target merge factor n
 */
__global__ void
R_CVulkan_DefragAnalyzeBlocksKernel (
    struct R_CVulkan_DefragBlockMetadata* blockMetadata,
    uint32_t                              blockCount,
    uint32_t                              mergeFactor)
{
    uint32_t blockIndex = blockIdx.x * blockDim.x + threadIdx.x;

    if (blockIndex >= blockCount)
    {
        return;
    }

    struct R_CVulkan_DefragBlockMetadata* metadata = &blockMetadata[blockIndex];
    metadata->fillLevel
        = (metadata->totalSize > 0) ? ((float)metadata->usedSize / (float)metadata->totalSize) : 0.0f;
    metadata->freeSize = metadata->totalSize - metadata->usedSize;

    // Determine if this block is a candidate for defragmentation
    // A block is a candidate if its fill level <= n/(n+1)
    float threshold = (float)mergeFactor / (float)(mergeFactor + 1);
    metadata->isCandidate = (metadata->fillLevel <= threshold && metadata->usedSize > 0) ? 1 : 0;
}

/**
 * @brief CUDA kernel for creating move plan, merges source blocks into target blocks
 * @param blockMetadata Array of block metadata
 * @param moves Array of move operations
 * @param moveCount Number of moves (output)
 * @param blockCount Number of blocks
 * @param mergeFactor Target merge factor n
 * @param maxBytesPerPass Maximum bytes to move per pass
 */
__global__ void
R_CVulkan_DefragCreateMovePlanKernel (
    struct R_CVulkan_DefragBlockMetadata* blockMetadata,
    struct R_CVulkan_DefragMove*          moves,
    uint32_t*                             moveCount,
    uint32_t                              blockCount,
    uint32_t                              mergeFactor,
    uint64_t                              maxBytesPerPass)
{
    uint32_t blockIndex = blockIdx.x * blockDim.x + threadIdx.x;

    if (blockIndex >= blockCount)
    {
        return;
    }

    struct R_CVulkan_DefragBlockMetadata* metadata = &blockMetadata[blockIndex];

    if (!metadata->isCandidate)
    {
        return;
    }

    // Find best target block
    uint32_t targetBlockIndex = 0;
    float    bestFillLevel = 0.0f;

    for (uint32_t j = 0; j < blockCount; ++j)
    {
        if (blockIndex == j)
        {
            continue;
        }

        if (!blockMetadata[j].isCandidate)
        {
            continue;
        }

        // Prefer blocks with higher fill level as targets
        if (blockMetadata[j].fillLevel > bestFillLevel)
        {
            bestFillLevel = blockMetadata[j].fillLevel;
            targetBlockIndex = j;
        }
    }

    if (bestFillLevel > 0.0f)
    {
        uint32_t moveIndex = atomicAdd (moveCount, 1);
        uint64_t bytesSoFar = 0;
        for (uint32_t i = 0; i < moveIndex; ++i)
        {
            bytesSoFar += moves[i].size;
        }

        if (bytesSoFar + metadata->usedSize <= maxBytesPerPass)
        {
            moves[moveIndex].srcBlockIndex = blockIndex;
            moves[moveIndex].dstBlockIndex = targetBlockIndex;
            moves[moveIndex].srcOffset = 0;
            moves[moveIndex].dstOffset = blockMetadata[targetBlockIndex].usedSize;
            moves[moveIndex].size = metadata->usedSize;
            moves[moveIndex].operation = 0; // MOVE operation
        }
        else
        {
            // Revert the increment since we can't add this move
            atomicSub (moveCount, 1);
        }
    }
}

/**
 * @brief CUDA kernel for updating block metadata after moves
 * @param blockMetadata Array of block metadata
 * @param moves Array of move operations
 * @param moveCount Number of moves
 * @param blockCount Number of blocks
 */
__global__ void
R_CVulkan_DefragUpdateMetadataKernel (
    struct R_CVulkan_DefragBlockMetadata* blockMetadata,
    struct R_CVulkan_DefragMove*          moves,
    uint32_t                              moveCount,
    uint32_t                              blockCount)
{
    uint32_t moveIndex = blockIdx.x * blockDim.x + threadIdx.x;

    if (moveIndex >= moveCount)
    {
        return;
    }

    struct R_CVulkan_DefragMove* move = &moves[moveIndex];

    if (move->operation != 0)
    {
        return; // Skip non-move operations
    }
    if (move->srcBlockIndex < blockCount)
    {
        atomicSub ((unsigned long long*)&blockMetadata[move->srcBlockIndex].usedSize, move->size);
        atomicAdd ((unsigned long long*)&blockMetadata[move->srcBlockIndex].freeSize, move->size);
    }
    if (move->dstBlockIndex < blockCount)
    {
        atomicAdd ((unsigned long long*)&blockMetadata[move->dstBlockIndex].usedSize, move->size);
        atomicSub ((unsigned long long*)&blockMetadata[move->dstBlockIndex].freeSize, move->size);
    }
}

/**
 * @brief Host function to launch block analysis kernel
 */
extern "C" cudaError_t
R_CVulkan_DefragLaunchAnalyzeBlocks (
    void*        blockMetadata,
    uint32_t     blockCount,
    uint32_t     mergeFactor,
    cudaStream_t stream)
{
    int blockSize = 256;
    int gridSize = (blockCount + blockSize - 1) / blockSize;

    R_CVulkan_DefragAnalyzeBlocksKernel<<<gridSize, blockSize, 0, stream>>> (
        (struct R_CVulkan_DefragBlockMetadata*)blockMetadata,
        blockCount,
        mergeFactor);

    return cudaGetLastError ();
}

/**
 * @brief Host function to launch move plan creation kernel
 */
extern "C" cudaError_t
R_CVulkan_DefragLaunchCreateMovePlan (
    void*        blockMetadata,
    void*        moves,
    uint32_t*    moveCount,
    uint32_t     blockCount,
    uint32_t     mergeFactor,
    uint64_t     maxBytesPerPass,
    cudaStream_t stream)
{
    int blockSize = 256;
    int gridSize = (blockCount + blockSize - 1) / blockSize;

    cudaError_t error = cudaMemsetAsync (moveCount, 0, sizeof (uint32_t), stream);
    if (error != cudaSuccess)
    {
        return error;
    }

    R_CVulkan_DefragCreateMovePlanKernel<<<gridSize, blockSize, 0, stream>>> (
        (struct R_CVulkan_DefragBlockMetadata*)blockMetadata,
        (struct R_CVulkan_DefragMove*)moves,
        moveCount,
        blockCount,
        mergeFactor,
        maxBytesPerPass);

    return cudaGetLastError ();
}

/**
 * @brief Host function to launch metadata update kernel
 */
extern "C" cudaError_t
R_CVulkan_DefragCudaLaunchUpdateMetadata (
    void*        blockMetadata,
    void*        moves,
    uint32_t     moveCount,
    uint32_t     blockCount,
    cudaStream_t stream)
{
    int blockSize = 256;
    int gridSize = (moveCount + blockSize - 1) / blockSize;

    R_CVulkan_DefragUpdateMetadataKernel<<<gridSize, blockSize, 0, stream>>> (
        (struct R_CVulkan_DefragBlockMetadata*)blockMetadata,
        (struct R_CVulkan_DefragMove*)moves,
        moveCount,
        blockCount);
    return cudaGetLastError ();
}
