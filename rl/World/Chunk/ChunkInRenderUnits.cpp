import Rl.World.Chunk.ChunkInRenderUnits;

import <algorithm>;
import <stdexcept>;
import <chrono>;
import <atomic>;
import <mutex>;

import Rl.World.Chunk.UnitChunkBuffer;
import Rl.World.Chunk.ChunkTransaction;

namespace Rl::World::Chunk
{

ChunkInRenderUnits::ChunkInRenderUnits(int32_t renderDistanceX,
                                       int32_t renderDistanceY,
                                       int32_t renderDistanceZ) :
    renderDistanceX(renderDistanceX), renderDistanceY(renderDistanceY),
    renderDistanceZ(renderDistanceZ), totalChunks(0), transactionBuffer(4096),
    frameCounter(0), initialized(false), shuttingDown(false),
    systemHealth(SystemHealth::HEALTHY), totalTransactionsProcessed(0),
    totalDeltasGenerated(0), failedTransactions(0), transactionSequenceCounter(0)
{
    if (renderDistanceX <= 0 || renderDistanceY <= 0 || renderDistanceZ <= 0)
    {
        throw std::invalid_argument("Render distance must be positive");
    }

    totalChunks =
        static_cast<uint32_t>(renderDistanceX * renderDistanceY * renderDistanceZ);

    if (totalChunks == 0)
    {
        throw std::invalid_argument("Total chunks cannot be zero");
    }
}

ChunkInRenderUnits::~ChunkInRenderUnits()
{
    Shutdown();
}

ChunkInRenderUnits::ChunkInRenderUnits(ChunkInRenderUnits&& other) noexcept :
    renderDistanceX(other.renderDistanceX), renderDistanceY(other.renderDistanceY),
    renderDistanceZ(other.renderDistanceZ), totalChunks(other.totalChunks),
    chunkBuffers(std::move(other.chunkBuffers)),
    chunkCoords(std::move(other.chunkCoords)), chunkActive(std::move(other.chunkActive)),
    transactionBuffer(std::move(other.transactionBuffer)),
    pendingDeltas(std::move(other.pendingDeltas)),
    frameCounter(other.frameCounter.load()), initialized(other.initialized.load()),
    shuttingDown(other.shuttingDown.load()), systemHealth(other.systemHealth.load()),
    totalTransactionsProcessed(other.totalTransactionsProcessed.load()),
    totalDeltasGenerated(other.totalDeltasGenerated.load()),
    failedTransactions(other.failedTransactions.load()),
    transactionSequenceCounter(other.transactionSequenceCounter.load())
{
    other.totalChunks = 0;
    other.frameCounter.store(0);
    other.initialized.store(false);
    other.shuttingDown.store(false);
    other.systemHealth.store(SystemHealth::HEALTHY);
    other.totalTransactionsProcessed.store(0);
    other.totalDeltasGenerated.store(0);
    other.failedTransactions.store(0);
    other.transactionSequenceCounter.store(0);
}

ChunkInRenderUnits& ChunkInRenderUnits::operator=(ChunkInRenderUnits&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();

        renderDistanceX   = other.renderDistanceX;
        renderDistanceY   = other.renderDistanceY;
        renderDistanceZ   = other.renderDistanceZ;
        totalChunks       = other.totalChunks;
        chunkBuffers      = std::move(other.chunkBuffers);
        chunkCoords       = std::move(other.chunkCoords);
        chunkActive       = std::move(other.chunkActive);
        transactionBuffer = std::move(other.transactionBuffer);
        pendingDeltas     = std::move(other.pendingDeltas);
        frameCounter.store(other.frameCounter.load());
        initialized.store(other.initialized.load());
        shuttingDown.store(other.shuttingDown.load());
        systemHealth.store(other.systemHealth.load());
        totalTransactionsProcessed.store(other.totalTransactionsProcessed.load());
        totalDeltasGenerated.store(other.totalDeltasGenerated.load());
        failedTransactions.store(other.failedTransactions.load());
        transactionSequenceCounter.store(other.transactionSequenceCounter.load());

        other.totalChunks = 0;
        other.frameCounter.store(0);
        other.initialized.store(false);
        other.shuttingDown.store(false);
        other.systemHealth.store(SystemHealth::HEALTHY);
        other.totalTransactionsProcessed.store(0);
        other.totalDeltasGenerated.store(0);
        other.failedTransactions.store(0);
        other.transactionSequenceCounter.store(0);
    }
    return *this;
}

bool ChunkInRenderUnits::Initialize()
{
    std::scoped_lock lock(chunkMutex);

    if (initialized.load(std::memory_order_acquire))
        return true;

    if (shuttingDown.load(std::memory_order_acquire))
        return false;

    try
    {
        chunkBuffers = std::make_unique<UnitChunkBuffer[]>(totalChunks);
        chunkCoords  = std::make_unique<WorldChunkCoord[]>(totalChunks);
        chunkActive  = std::make_unique<std::atomic<bool>[]>(totalChunks);

        for (uint32_t i = 0; i < totalChunks; ++i)
        {
            chunkActive[i].store(false, std::memory_order_release);
        }
        initialized.store(true, std::memory_order_release);
        systemHealth.store(SystemHealth::HEALTHY, std::memory_order_release);
        return true;
    }
    catch (const std::exception& e)
    {
        systemHealth.store(SystemHealth::UNHEALTHY, std::memory_order_release);
        return false;
    }
}

bool ChunkInRenderUnits::Shutdown()
{
    std::scoped_lock lock(chunkMutex);
    if (!initialized.load(std::memory_order_acquire))
        return true;
    shuttingDown.store(true, std::memory_order_release);
    initialized.store(false, std::memory_order_release);
    try
    {
        // Process remaining transactions before shutdown
        ProcessTransactions();

        chunkBuffers.reset();
        chunkCoords.reset();
        chunkActive.reset();
        pendingDeltas.clear();
        systemHealth.store(SystemHealth::HEALTHY, std::memory_order_release);
        return true;
    }
    catch (const std::exception& e)
    {
        systemHealth.store(SystemHealth::UNHEALTHY, std::memory_order_release);
        return false;
    }
}

bool ChunkInRenderUnits::IsInitialized() const
{
    return initialized.load(std::memory_order_acquire);
}

bool ChunkInRenderUnits::AddChunk(const WorldChunkCoord& coord,
                                  UnitChunkBuffer&       chunkBuffer)
{
    if (!initialized.load())
        return false;

    if (shuttingDown.load())
        return false;

    if (!ValidateChunkBuffer(chunkBuffer))
        return false;

    if (!IsInRenderDistance(coord))
        return false;

    uint32_t index = WorldCoordToIndex(coord);

    if (index >= totalChunks)
        return false;

    std::scoped_lock lock(chunkMutex);

    if (chunkActive[index].load(std::memory_order_acquire))
        return false; // Chunk already exists at this location

    chunkBuffers[index] = std::move(chunkBuffer);
    chunkCoords[index]  = coord;
    chunkActive[index].store(true, std::memory_order_release);

    UpdateSystemHealth();

    return true;
}

bool ChunkInRenderUnits::RemoveChunk(const WorldChunkCoord& coord)
{
    if (!initialized.load())
        return false;

    if (shuttingDown.load())
        return false;

    if (!IsInRenderDistance(coord))
        return false;

    uint32_t index = WorldCoordToIndex(coord);

    if (index >= totalChunks)
        return false;

    std::scoped_lock lock(chunkMutex);

    if (!chunkActive[index].load(std::memory_order_acquire))
        return false; // Chunk doesn't exist at this location

    chunkActive[index].store(false, std::memory_order_release);

    UpdateSystemHealth();

    return true;
}

UnitChunkBuffer* ChunkInRenderUnits::GetChunkBuffer(const WorldChunkCoord& coord)
{
    if (!initialized.load())
        return nullptr;
    if (shuttingDown.load())
        return nullptr;
    if (!IsInRenderDistance(coord))
        return nullptr;

    uint32_t index = WorldCoordToIndex(coord);
    if (index >= totalChunks)
        return nullptr;
    std::scoped_lock lock(chunkMutex);
    if (!chunkActive[index].load(std::memory_order_acquire))
        return nullptr;
    return &chunkBuffers[index];
}

UnitChunkBuffer* ChunkInRenderUnits::GetChunkBuffer(uint32_t index)
{
    if (!initialized.load())
        return nullptr;

    if (shuttingDown.load())
        return nullptr;

    if (index >= totalChunks)
        return nullptr;

    std::scoped_lock lock(chunkMutex);

    if (!chunkActive[index].load(std::memory_order_acquire))
        return nullptr;

    return &chunkBuffers[index];
}

TransactionResult ChunkInRenderUnits::ReadUnitId(const WorldChunkCoord& coord,
                                                 int32_t                localX,
                                                 int32_t                localY,
                                                 int32_t                localZ)
{
    if (!initialized.load())
        return TransactionResult::Error("System not initialized",
                                        ValidationResult::VALID);

    if (shuttingDown.load())
        return TransactionResult::Error("System is shutting down",
                                        ValidationResult::VALID);

    if (!IsInChunkBounds(localX, localY, localZ))
        return TransactionResult::Error("Local coordinates out of bounds",
                                        ValidationResult::INVALID_COORDINATES);

    UnitChunkBuffer* chunk = GetChunkBuffer(coord);
    if (!chunk)
        return TransactionResult::Error("Chunk not found",
                                        ValidationResult::INVALID_CHUNK_INDEX);

    auto unitId = chunk->GetUnitIdXYZ(localX, localY, localZ);
    if (!unitId)
        return TransactionResult::Error("Failed to read unit ID",
                                        ValidationResult::VALID);

    return TransactionResult::Ok(static_cast<uint32_t>(unitId.value()), 0);
}

TransactionResult ChunkInRenderUnits::WriteUnitId(const WorldChunkCoord& coord,
                                                  int32_t                localX,
                                                  int32_t                localY,
                                                  int32_t                localZ,
                                                  uint32_t               newUnitId)
{
    if (!initialized.load())
        return TransactionResult::Error("System not initialized",
                                        ValidationResult::VALID);

    if (shuttingDown.load())
        return TransactionResult::Error("System is shutting down",
                                        ValidationResult::VALID);

    if (!IsInChunkBounds(localX, localY, localZ))
        return TransactionResult::Error("Local coordinates out of bounds",
                                        ValidationResult::INVALID_COORDINATES);

    UnitChunkBuffer* chunk = GetChunkBuffer(coord);
    if (!chunk)
        return TransactionResult::Error("Chunk not found",
                                        ValidationResult::INVALID_CHUNK_INDEX);

    // Read current value for delta
    auto oldUnitId = chunk->GetUnitIdXYZ(localX, localY, localZ);
    if (!oldUnitId)
        return TransactionResult::Error("Failed to read current unit ID",
                                        ValidationResult::VALID);

    uint32_t chunkIndex = WorldCoordToIndex(coord);
    uint64_t sequence =
        transactionSequenceCounter.fetch_add(1, std::memory_order_release);

    // Create transaction
    ChunkTransaction transaction;
    transaction.chunkIndex     = chunkIndex;
    transaction.localX         = localX;
    transaction.localY         = localY;
    transaction.localZ         = localZ;
    transaction.oldUnitId      = static_cast<uint32_t>(oldUnitId.value());
    transaction.newUnitId      = newUnitId;
    transaction.type           = TransactionType::WRITE;
    transaction.state          = TransactionState::PENDING;
    transaction.timestamp      = frameCounter.load();
    transaction.sequenceNumber = sequence;

    // Validate transaction
    ValidationResult validation =
        transaction.Validate(UnitChunkBuffer::W, UnitChunkBuffer::H, UnitChunkBuffer::D);
    if (validation != ValidationResult::VALID)
    {
        return TransactionResult::Error("Transaction validation failed", validation);
    }

    // Enqueue transaction
    if (!EnqueueTransaction(std::move(transaction)))
    {
        failedTransactions.fetch_add(1, std::memory_order_release);
        return TransactionResult::Error("Failed to enqueue transaction",
                                        ValidationResult::VALID);
    }
    return TransactionResult::Ok(newUnitId, sequence);
}

bool ChunkInRenderUnits::EnqueueTransaction(ChunkTransaction transaction)
{
    if (shuttingDown.load())
        return false;

    return transactionBuffer.Push(std::move(transaction));
}

bool ChunkInRenderUnits::ProcessTransactions()
{
    if (!initialized.load())
        return false;

    if (shuttingDown.load())
        return false;

    bool             processedAny = false;
    ChunkTransaction transaction;

    while (transactionBuffer.Pop(transaction))
    {
        processedAny = true;

        // Apply the transaction to the chunk
        UnitChunkBuffer* chunk = GetChunkBuffer(transaction.chunkIndex);
        if (chunk && chunkActive[transaction.chunkIndex].load())
        {
            transaction.SetState(TransactionState::IN_PROGRESS);

            // Get the raw buffer and write the new unit ID
            int* buffer = chunk->GetRaw();
            int  index  = IndexMap3d2<UnitChunkBuffer::W, UnitChunkBuffer::H>(
                transaction.localX, transaction.localY, transaction.localZ);

            if (index >= 0 && index < UnitChunkBuffer::GetTotalBlocks())
            {
                buffer[index] = static_cast<int>(transaction.newUnitId);

                // Add delta for GPU synchronization
                ChunkDelta delta{};
                delta.chunkIndex  = transaction.chunkIndex;
                delta.localX      = transaction.localX;
                delta.localY      = transaction.localY;
                delta.localZ      = transaction.localZ;
                delta.oldUnitId   = transaction.oldUnitId;
                delta.newUnitId   = transaction.newUnitId;
                delta.frameNumber = frameCounter.load();
                delta.timestamp =
                    std::chrono::steady_clock::now().time_since_epoch().count();

                AddDelta(delta);
                totalDeltasGenerated.fetch_add(1, std::memory_order_release);

                transaction.Complete();
                totalTransactionsProcessed.fetch_add(1, std::memory_order_release);
            }
            else
            {
                transaction.Fail();
                failedTransactions.fetch_add(1, std::memory_order_release);
            }
        }
        else
        {
            transaction.Fail();
            failedTransactions.fetch_add(1, std::memory_order_release);
        }
    }

    if (processedAny)
    {
        frameCounter.fetch_add(1, std::memory_order_release);
        UpdateSystemHealth();
    }

    return processedAny;
}

std::vector<ChunkDelta> ChunkInRenderUnits::GetPendingDeltas()
{
    std::scoped_lock lock(deltaMutex);
    return pendingDeltas;
}

void ChunkInRenderUnits::ClearProcessedDeltas()
{
    std::scoped_lock lock(deltaMutex);
    pendingDeltas.clear();
}

uint32_t ChunkInRenderUnits::GetChunkCount() const
{
    if (!initialized.load())
        return 0;

    uint32_t count = 0;
    for (uint32_t i = 0; i < totalChunks; ++i)
    {
        if (chunkActive[i].load(std::memory_order_acquire))
            count++;
    }
    return count;
}

WorldChunkCoord ChunkInRenderUnits::GetRenderDistance() const
{
    return {renderDistanceX, renderDistanceY, renderDistanceZ};
}

bool ChunkInRenderUnits::IsInRenderDistance(const WorldChunkCoord& coord) const
{
    return coord.chunkX >= 0 && coord.chunkX < renderDistanceX && coord.chunkY >= 0 &&
           coord.chunkY < renderDistanceY && coord.chunkZ >= 0 &&
           coord.chunkZ < renderDistanceZ;
}

uint32_t ChunkInRenderUnits::GetPendingTransactionCount() const
{
    return transactionBuffer.Size();
}

ChunkSystemStats ChunkInRenderUnits::GetSystemStats() const
{
    ChunkSystemStats stats{};
    stats.activeChunks        = GetChunkCount();
    stats.totalChunkSlots     = totalChunks;
    stats.pendingTransactions = transactionBuffer.Size();

    {
        std::scoped_lock lock(deltaMutex);
        stats.pendingDeltas = static_cast<uint32_t>(pendingDeltas.size());
    }

    stats.totalTransactionsProcessed =
        totalTransactionsProcessed.load(std::memory_order_acquire);
    stats.totalDeltasGenerated    = totalDeltasGenerated.load(std::memory_order_acquire);
    stats.failedTransactions      = failedTransactions.load(std::memory_order_acquire);
    stats.averageProcessingTimeMs = 0.0; // TODO: Implement timing
    stats.health                  = systemHealth.load(std::memory_order_acquire);

    return stats;
}

SystemHealth ChunkInRenderUnits::GetSystemHealth() const
{
    return systemHealth.load(std::memory_order_acquire);
}

bool ChunkInRenderUnits::ValidateSystemIntegrity() const
{
    if (!initialized.load())
        return false;

    if (shuttingDown.load())
        return false;

    // Check ring buffer health
    if (!transactionBuffer.IsHealthy())
        return false;

    // Check if chunk count is reasonable
    uint32_t chunkCount = GetChunkCount();
    if (chunkCount > totalChunks)
        return false;

    // Check system health
    SystemHealth health = systemHealth.load(std::memory_order_acquire);
    if (health == SystemHealth::UNHEALTHY)
        return false;

    return true;
}

void ChunkInRenderUnits::EmergencyShutdown()
{
    shuttingDown.store(true, std::memory_order_release);
    systemHealth.store(SystemHealth::UNHEALTHY, std::memory_order_release);

    try
    {
        std::scoped_lock lock(chunkMutex);

        chunkBuffers.reset();
        chunkCoords.reset();
        chunkActive.reset();
        pendingDeltas.clear();

        initialized.store(false, std::memory_order_release);
    }
    catch (...)
    {
        // Emergency shutdown should not throw
    }
}

uint32_t ChunkInRenderUnits::WorldCoordToIndex(const WorldChunkCoord& coord) const
{
    return static_cast<uint32_t>(coord.chunkX + coord.chunkY * renderDistanceX +
                                 coord.chunkZ * renderDistanceX * renderDistanceY);
}

WorldChunkCoord ChunkInRenderUnits::IndexToWorldCoord(uint32_t index) const
{
    WorldChunkCoord coord{};
    coord.chunkX = index % renderDistanceX;
    coord.chunkY = (index / renderDistanceX) % renderDistanceY;
    coord.chunkZ = (index / (renderDistanceX)*renderDistanceY);
    return coord;
}

bool ChunkInRenderUnits::IsInChunkBounds(int32_t localX,
                                         int32_t localY,
                                         int32_t localZ) const
{
    return localX >= 0 && localX < UnitChunkBuffer::W && localY >= 0 &&
           localY < UnitChunkBuffer::H && localZ >= 0 && localZ < UnitChunkBuffer::D;
}

void ChunkInRenderUnits::AddDelta(const ChunkDelta& delta)
{
    std::scoped_lock lock(deltaMutex);
    pendingDeltas.push_back(delta);
}

bool ChunkInRenderUnits::ValidateChunkBuffer(const UnitChunkBuffer& buffer) const
{
    return buffer.IsValid();
}

void ChunkInRenderUnits::UpdateSystemHealth()
{
    SystemHealth currentHealth = SystemHealth::HEALTHY;

    // Check transaction buffer health
    if (!transactionBuffer.IsHealthy())
    {
        currentHealth = SystemHealth::DEGRADED;
    }

    // Check failure rate
    uint64_t totalProcessed = totalTransactionsProcessed.load(std::memory_order_acquire);
    uint64_t totalFailed    = failedTransactions.load(std::memory_order_acquire);
    if (totalProcessed > 0)
    {
        const double failureRate =
            static_cast<double>(totalFailed) / static_cast<double>(totalProcessed);
        if (failureRate > 0.1) // More than 10% failure rate
        {
            currentHealth = SystemHealth::UNHEALTHY;
        }
        else if (failureRate > 0.05) // More than 5% failure rate
        {
            if (currentHealth == SystemHealth::HEALTHY)
                currentHealth = SystemHealth::DEGRADED;
        }
    }
    // Check pending transactions
    const uint32_t pendingTrans = transactionBuffer.Size();
    if (pendingTrans > transactionBuffer.Capacity() * 0.9) // More than 90% full
    {
        if (currentHealth == SystemHealth::HEALTHY)
            currentHealth = SystemHealth::DEGRADED;
    }

    systemHealth.store(currentHealth, std::memory_order_release);
}

} // namespace Rl::World::Chunk
