import <gtest/gtest.h>;

import Rl.World.Chunk.ChunkErrorRecovery;

import <stdexcept>;

using namespace Rl::World::Chunk;

TEST(ChunkErrorRecoveryTest, DefaultConfigHasExpectedRecoveryDefaults)
{
  const ErrorRecoveryConfig config = GetDefaultErrorRecoveryConfig();

  EXPECT_EQ(config.maxRetryAttempts, 3u);
  EXPECT_EQ(config.retryDelayMs, 100u);
  EXPECT_EQ(config.circuitBreakerThreshold, 5u);
  EXPECT_EQ(config.circuitBreakerTimeoutMs, 30000u);
  EXPECT_DOUBLE_EQ(config.successRateThreshold, 0.8);
}

TEST(ChunkErrorRecoveryTest, CircuitBreakerOpensAfterRepeatedFailures)
{
  ErrorRecoveryConfig config{2, 0, 2, 1, 0.5};
  ChunkCircuitBreaker breaker(config);

  EXPECT_TRUE(breaker.AllowOperation());
  breaker.RecordFailure();
  EXPECT_TRUE(breaker.AllowOperation());
  breaker.RecordFailure();

  EXPECT_FALSE(breaker.AllowOperation());
  EXPECT_EQ(breaker.GetState(), CircuitBreakerState::OPEN);
  EXPECT_EQ(breaker.GetFailureCount(), 2u);
}

TEST(ChunkErrorRecoveryTest, RecoveryManagerRetriesUntilSuccessAndTracksStats)
{
  ErrorRecoveryConfig config{3, 1, 2, 100, 0.5};
  ChunkErrorRecoveryManager manager(config);

  int attempts = 0;
  const int result = manager.ExecuteWithRecovery([&]() -> int {
    ++attempts;
    if (attempts < 3)
    {
      throw std::runtime_error("temporary failure");
    }
    return 42;
  });

  EXPECT_EQ(result, 42);
  EXPECT_EQ(attempts, 3);
  EXPECT_EQ(manager.GetTotalRetries(), 2u);
  EXPECT_EQ(manager.GetCircuitBreakerState(), CircuitBreakerState::CLOSED);
}

TEST(ChunkErrorRecoveryTest, CircuitBreakerTransitionsToHalfOpenAfterTimeout)
{
  ErrorRecoveryConfig config{1, 0, 1, 0, 0.5};
  ChunkCircuitBreaker breaker(config);

  breaker.RecordFailure();
  EXPECT_EQ(breaker.GetState(), CircuitBreakerState::OPEN);
  EXPECT_TRUE(breaker.AllowOperation());
  EXPECT_EQ(breaker.GetState(), CircuitBreakerState::HALF_OPEN);
}

TEST(ChunkErrorRecoveryTest, CircuitBreakerResetsOnSuccess)
{
  ErrorRecoveryConfig config{1, 0, 1, 0, 0.5};
  ChunkCircuitBreaker breaker(config);

  breaker.RecordFailure();
  EXPECT_EQ(breaker.GetState(), CircuitBreakerState::OPEN);

  breaker.RecordSuccess();
  EXPECT_EQ(breaker.GetState(), CircuitBreakerState::CLOSED);
  EXPECT_EQ(breaker.GetFailureCount(), 0u);
  EXPECT_EQ(breaker.GetSuccessCount(), 1u);
}

TEST(ChunkErrorRecoveryTest, ResetClearsCircuitBreakerStats)
{
  ErrorRecoveryConfig config{2, 1, 1, 0, 0.5};
  ChunkCircuitBreaker breaker(config);

  breaker.RecordFailure();
  breaker.RecordSuccess();
  breaker.Reset();

  EXPECT_EQ(breaker.GetState(), CircuitBreakerState::CLOSED);
  EXPECT_EQ(breaker.GetFailureCount(), 0u);
  EXPECT_EQ(breaker.GetSuccessCount(), 0u);
}
