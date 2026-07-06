export module Rl.World.Chunk.ChunkErrorRecovery;

import Rl.World.Chunk.ChunkTransaction;
import <cstdint>;
import <atomic>;
import <chrono>;

namespace Rl::World::Chunk
{

/* Circuit breaker states */
export enum class CircuitBreakerState : uint32_t
{
  CLOSED = 0,      // Normal operation
  OPEN = 1,        // Circuit is open, blocking operations
  HALF_OPEN = 2    // Testing if system has recovered
};

/* Error recovery configuration */
export struct ErrorRecoveryConfig
{
  uint32_t maxRetryAttempts;           // Maximum retry attempts for failed operations
  uint32_t retryDelayMs;               // Delay between retries
  uint32_t circuitBreakerThreshold;    // Failure threshold to trigger circuit breaker
  uint32_t circuitBreakerTimeoutMs;   // Time to keep circuit breaker open
  double successRateThreshold;         // Success rate threshold for recovery
};

/* Circuit breaker for preventing cascading failures */
export class ChunkCircuitBreaker
{
  public:
  explicit ChunkCircuitBreaker(const ErrorRecoveryConfig& config);
  ~ChunkCircuitBreaker() = default;
  
  /* Disable copy operations */
  ChunkCircuitBreaker(const ChunkCircuitBreaker&) = delete;
  ChunkCircuitBreaker& operator=(const ChunkCircuitBreaker&) = delete;
  
  /* Check if operation should be allowed */
  [[nodiscard]]
  bool AllowOperation();
  
  /* Record successful operation */
  void RecordSuccess();
  
  /* Record failed operation */
  void RecordFailure();
  
  /* Get current circuit breaker state */
  [[nodiscard]]
  CircuitBreakerState GetState() const;
  
  /* Reset circuit breaker to closed state */
  void Reset();
  
  /* Get failure count */
  [[nodiscard]]
  uint32_t GetFailureCount() const;
  
  /* Get success count */
  [[nodiscard]]
  uint32_t GetSuccessCount() const;

  private:
  ErrorRecoveryConfig config;
  std::atomic<CircuitBreakerState> state;
  std::atomic<uint32_t> failureCount;
  std::atomic<uint32_t> successCount;
  std::atomic<uint64_t> lastFailureTime;
  std::atomic<uint64_t> lastSuccessTime;
  
  /* Check if circuit breaker should transition to open */
  [[nodiscard]]
  bool ShouldOpenCircuit() const;
  
  /* Check if circuit breaker should transition to closed */
  [[nodiscard]]
  bool ShouldCloseCircuit() const;
  
  /* Get current timestamp in milliseconds */
  [[nodiscard]]
  static uint64_t GetCurrentTimeMs();
};

/* Transaction retry handler with exponential backoff */
export class TransactionRetryHandler
{
  public:
  explicit TransactionRetryHandler(const ErrorRecoveryConfig& config);
  ~TransactionRetryHandler() = default;
  
  /* Disable copy operations */
  TransactionRetryHandler(const TransactionRetryHandler&) = delete;
  TransactionRetryHandler& operator=(const TransactionRetryHandler&) = delete;
  
  /* Execute operation with retry logic */
  template<typename Func>
  auto ExecuteWithRetry(Func&& operation) -> decltype(operation())
  {
    uint32_t attempt = 0;
    uint32_t delay = config.retryDelayMs;
    
    while (attempt < config.maxRetryAttempts)
    {
      try
      {
        auto result = operation();
        // On success, reset delay
        delay = config.retryDelayMs;
        return result;
      }
      catch (...)
      {
        attempt++;
        if (attempt >= config.maxRetryAttempts)
        {
          throw; // Re-throw on final attempt
        }
        
        // Exponential backoff
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        delay *= 2; // Double the delay for next attempt
      }
    }
    
    // This should never be reached, but compiler needs it
    throw std::runtime_error("Retry handler exhausted attempts");
  }
  
  /* Get current retry statistics */
  [[nodiscard]]
  uint32_t GetTotalRetries() const;
  
  /* Reset retry statistics */
  void ResetStats();

  private:
  ErrorRecoveryConfig config;
  std::atomic<uint32_t> totalRetries;
};

/* Error recovery manager combining circuit breaker and retry logic */
export class ChunkErrorRecoveryManager
{
  public:
  explicit ChunkErrorRecoveryManager(const ErrorRecoveryConfig& config);
  ~ChunkErrorRecoveryManager() = default;
  
  /* Disable copy operations */
  ChunkErrorRecoveryManager(const ChunkErrorRecoveryManager&) = delete;
  ChunkErrorRecoveryManager& operator=(const ChunkErrorRecoveryManager&) = delete;
  
  /* Execute operation with full error recovery */
  template<typename Func>
  auto ExecuteWithRecovery(Func&& operation) -> decltype(operation())
  {
    if (!circuitBreaker.AllowOperation())
    {
      throw std::runtime_error("Circuit breaker is open, operation blocked");
    }
    
    try
    {
      auto result = retryHandler.ExecuteWithRetry(std::forward<Func>(operation));
      circuitBreaker.RecordSuccess();
      return result;
    }
    catch (...)
    {
      circuitBreaker.RecordFailure();
      throw;
    }
  }
  
  /* Get circuit breaker state */
  [[nodiscard]]
  CircuitBreakerState GetCircuitBreakerState() const;
  
  /* Get total retry count */
  [[nodiscard]]
  uint32_t GetTotalRetries() const;
  
  /* Reset all recovery mechanisms */
  void Reset();
  
  /* Update configuration */
  void UpdateConfig(const ErrorRecoveryConfig& newConfig);

  private:
  ErrorRecoveryConfig config;
  ChunkCircuitBreaker circuitBreaker;
  TransactionRetryHandler retryHandler;
};

/* Default configuration for production use */
export ErrorRecoveryConfig GetDefaultErrorRecoveryConfig()
{
  ErrorRecoveryConfig config;
  config.maxRetryAttempts = 3;
  config.retryDelayMs = 100;
  config.circuitBreakerThreshold = 5;
  config.circuitBreakerTimeoutMs = 30000; // 30 seconds
  config.successRateThreshold = 0.8; // 80% success rate
  return config;
}

} // namespace Rl::World::Chunk
