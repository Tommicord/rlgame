import Rl.World.Chunk.ChunkErrorRecovery;

import <thread>;
import <stdexcept>;
import <chrono>;

namespace Rl::World::Chunk
{

ChunkCircuitBreaker::ChunkCircuitBreaker(const ErrorRecoveryConfig& config) :
    config(config), state(CircuitBreakerState::CLOSED), failureCount(0), successCount(0),
    lastFailureTime(0), lastSuccessTime(0)
{
}

bool ChunkCircuitBreaker::AllowOperation()
{
    CircuitBreakerState currentState = state.load(std::memory_order_acquire);

    if (currentState == CircuitBreakerState::OPEN)
    {
        // Check if timeout has elapsed
        uint64_t currentTime = GetCurrentTimeMs();
        uint64_t lastFailure = lastFailureTime.load(std::memory_order_acquire);

        if (currentTime - lastFailure >= config.circuitBreakerTimeoutMs)
        {
            // Transition to half-open to test recovery
            state.store(CircuitBreakerState::HALF_OPEN, std::memory_order_release);
            return true;
        }

        return false; // Circuit is still open
    }

    return true; // Allow operation in closed or half-open state
}

void ChunkCircuitBreaker::RecordSuccess()
{
    successCount.fetch_add(1, std::memory_order_release);
    lastSuccessTime.store(GetCurrentTimeMs(), std::memory_order_release);

    CircuitBreakerState currentState = state.load(std::memory_order_acquire);

    if (currentState == CircuitBreakerState::HALF_OPEN)
    {
        // If we're in half-open and get success, transition to closed
        state.store(CircuitBreakerState::CLOSED, std::memory_order_release);
        failureCount.store(0, std::memory_order_release);
    }
    else if (currentState == CircuitBreakerState::OPEN)
    {
        // Unexpected success in open state, transition to closed
        state.store(CircuitBreakerState::CLOSED, std::memory_order_release);
        failureCount.store(0, std::memory_order_release);
    }
}

void ChunkCircuitBreaker::RecordFailure()
{
    failureCount.fetch_add(1, std::memory_order_release);
    lastFailureTime.store(GetCurrentTimeMs(), std::memory_order_release);

    CircuitBreakerState currentState = state.load(std::memory_order_acquire);

    if (currentState == CircuitBreakerState::HALF_OPEN)
    {
        // Failure in half-open means system hasn't recovered, go back to open
        state.store(CircuitBreakerState::OPEN, std::memory_order_release);
    }
    else if (ShouldOpenCircuit())
    {
        // Threshold reached, open the circuit
        state.store(CircuitBreakerState::OPEN, std::memory_order_release);
    }
}

CircuitBreakerState ChunkCircuitBreaker::GetState() const
{
    return state.load(std::memory_order_acquire);
}

void ChunkCircuitBreaker::Reset()
{
    state.store(CircuitBreakerState::CLOSED, std::memory_order_release);
    failureCount.store(0, std::memory_order_release);
    successCount.store(0, std::memory_order_release);
    lastFailureTime.store(0, std::memory_order_release);
    lastSuccessTime.store(0, std::memory_order_release);
}

uint32_t ChunkCircuitBreaker::GetFailureCount() const
{
    return failureCount.load(std::memory_order_acquire);
}

uint32_t ChunkCircuitBreaker::GetSuccessCount() const
{
    return successCount.load(std::memory_order_acquire);
}

bool ChunkCircuitBreaker::ShouldOpenCircuit() const
{
    uint32_t failures = failureCount.load(std::memory_order_acquire);
    return failures >= config.circuitBreakerThreshold;
}

bool ChunkCircuitBreaker::ShouldCloseCircuit() const
{
    uint64_t totalOps = successCount.load(std::memory_order_acquire) +
                        failureCount.load(std::memory_order_acquire);

    if (totalOps == 0)
        return false;

    double successRate =
        static_cast<double>(successCount.load(std::memory_order_acquire)) /
        static_cast<double>(totalOps);

    return successRate >= config.successRateThreshold;
}

uint64_t ChunkCircuitBreaker::GetCurrentTimeMs()
{
    auto now      = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

TransactionRetryHandler::TransactionRetryHandler(const ErrorRecoveryConfig& config) :
    config(config), totalRetries(0)
{
}

uint32_t TransactionRetryHandler::GetTotalRetries() const
{
    return totalRetries.load(std::memory_order_acquire);
}

void TransactionRetryHandler::ResetStats()
{
    totalRetries.store(0, std::memory_order_release);
}

ChunkErrorRecoveryManager::ChunkErrorRecoveryManager(const ErrorRecoveryConfig& config) :
    config(config), circuitBreaker(config), retryHandler(config)
{
}

CircuitBreakerState ChunkErrorRecoveryManager::GetCircuitBreakerState() const
{
    return circuitBreaker.GetState();
}

uint32_t ChunkErrorRecoveryManager::GetTotalRetries() const
{
    return retryHandler.GetTotalRetries();
}

void ChunkErrorRecoveryManager::Reset()
{
    circuitBreaker.Reset();
    retryHandler.ResetStats();
}

void ChunkErrorRecoveryManager::UpdateConfig(const ErrorRecoveryConfig& newConfig)
{
    config = newConfig;
    // Note: Circuit breaker and retry handler would need to be recreated or updated
    // For simplicity, this is a placeholder for config updates
}

} // namespace Rl::World::Chunk
