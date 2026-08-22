struct R_CVulkan_DefragBlockMetadata
{
    uint    blockIndex;
    ulong   totalSize;
    ulong   usedSize;
    ulong   freeSize;
    float   fillLevel;
    uint    allocationCount;
    uint    isCandidate;
};

struct R_CVulkan_DefragMove
{
    uint    srcBlockIndex;
    uint    dstBlockIndex;
    ulong   srcOffset;
    ulong   dstOffset;
    ulong   size;
    uint    operation;
};

/**
 * @brief OpenCL kernel for block analysis, calculates fill levels and identifies candidates
 * @param blockMetadata Array of block metadata
 * @param blockCount Number of blocks
 * @param mergeFactor Target merge factor n
 */
__kernel void R_CVulkan_DefragAnalyzeBlocksKernel (
    __global struct R_CVulkan_DefragBlockMetadata* blockMetadata,
    const uint                                     blockCount,
    const uint                                     mergeFactor)
{
    uint blockIndex = get_global_id (0);

    if (blockIndex >= blockCount)
    {
        return;
    }

    __global R_CVulkan_DefragBlockMetadata* metadata = &blockMetadata[blockIndex];

    metadata->fillLevel = (metadata->totalSize > 0) 
        ? ((float)metadata->usedSize / (float)metadata->totalSize) 
        : 0.0f;
    metadata->freeSize = metadata->totalSize - metadata->usedSize;

    // Determine if this block is a candidate for defragmentation
    // A block is a candidate if its fill level <= n/(n+1)
    float threshold = (float)mergeFactor / (float)(mergeFactor + 1);
    metadata->isCandidate = (metadata->fillLevel <= threshold && metadata->usedSize > 0) ? 1 : 0;
}

/**
 * @brief OpenCL kernel for creating move plan, merges source blocks into target blocks
 * @param blockMetadata Array of block metadata
 * @param moves Array of move operations
 * @param moveCount Number of moves (output)
 * @param blockCount Number of blocks
 * @param mergeFactor Target merge factor n
 * @param maxBytesPerPass Maximum bytes to move per pass
 */
__kernel void R_CVulkan_DefragCreateMovePlanKernel (
    __global struct R_CVulkan_DefragBlockMetadata* blockMetadata,
    __global struct R_CVulkan_DefragMove*         moves,
    __global uint*                moveCount,
    const uint                    blockCount,
    const uint                    mergeFactor,
    const ulong                   maxBytesPerPass)
{
    uint blockIndex = get_global_id (0);

    if (blockIndex >= blockCount)
    {
        return;
    }

    __global struct R_CVulkan_DefragBlockMetadata* metadata = &blockMetadata[blockIndex];

    if (!metadata->isCandidate)
    {
        return;
    }

    // Find best target block
    uint targetBlockIndex = 0;
    float bestFillLevel = 0.0f;

    for (uint j = 0; j < blockCount; ++j)
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
        uint moveIndex = atomic_add (moveCount, 1);

        ulong bytesSoFar = 0;
        for (uint i = 0; i < moveIndex; ++i)
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
            atomic_sub (moveCount, 1);
        }
    }
}

/**
 * @brief OpenCL kernel for updating block metadata after moves
 * @param blockMetadata Array of block metadata
 * @param moves Array of move operations
 * @param moveCount Number of moves
 * @param blockCount Number of blocks
 */
__kernel void R_CVulkan_DefragUpdateMetadataKernel (
    __global struct R_CVulkan_DefragBlockMetadata* blockMetadata,
    __global struct R_CVulkan_DefragMove*         moves,
    const uint                    moveCount,
    const uint                    blockCount)
{
    uint moveIndex = get_global_id (0);

    if (moveIndex >= moveCount)
    {
        return;
    }

    __global struct R_CVulkan_DefragMove* move = &moves[moveIndex];

    if (move->operation != 0)
    {
        return;
    }
    if (move->srcBlockIndex < blockCount)
    {
        atomic_sub (&blockMetadata[move->srcBlockIndex].usedSize, move->size);
        atomic_add (&blockMetadata[move->srcBlockIndex].freeSize, move->size);
    }
    if (move->dstBlockIndex < blockCount)
    {
        atomic_add (&blockMetadata[move->dstBlockIndex].usedSize, move->size);
        atomic_sub (&blockMetadata[move->dstBlockIndex].freeSize, move->size);
    }
}
