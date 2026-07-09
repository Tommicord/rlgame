export module Rl.World.Chunk.ChunkTransaction;

import <cstdint>;
import <atomic>;
import <cstring>;

namespace Rl::World::Chunk
{

/* Transaction type for chunk modifications */
export enum class TransactionType : uint32_t {
  READ = 0,
  WRITE = 1,
  READ_WRITE = 2,
  INVALID = 3
};

/* Transaction state for lifecycle management */
export enum class TransactionState : uint32_t {
  PENDING = 0,
  IN_PROGRESS = 1,
  COMPLETED = 2,
  FAILED = 3,
  ROLLED_BACK = 4
};

/* Transaction validation result */
export enum class ValidationResult : uint32_t {
  VALID = 0,
  INVALID_CHUNK_INDEX = 1,
  INVALID_COORDINATES = 2,
  INVALID_UNIT_ID = 3,
  INVALID_TYPE = 4,
  ALREADY_COMPLETED = 5
};

/* Single chunk modification transaction with robustness features */
export struct ChunkTransaction
{
  uint32_t          chunkIndex; // Index of the chunk in the render distance
  int32_t           localX; // Local X coordinate within chunk
  int32_t           localY; // Local Y coordinate within chunk
  int32_t           localZ; // Local Z coordinate within chunk
  uint32_t          oldUnitId; // Previous unit ID (for rollback)
  uint32_t          newUnitId; // New unit ID to write
  TransactionType   type; // Transaction type
  TransactionState  state; // Transaction state
  uint64_t          timestamp; // Transaction timestamp
  uint64_t          sequenceNumber; // Unique sequence number for ordering
  std::atomic<bool> completed; // Transaction completion flag

  ChunkTransaction() :
      chunkIndex(0), localX(0), localY(0), localZ(0), oldUnitId(0), newUnitId(0),
      type(TransactionType::READ), state(TransactionState::PENDING), timestamp(0),
      sequenceNumber(0), completed(false)
  {
  }

  /* Delete copy constructor: atomic<bool> is not copyable */
  ChunkTransaction(const ChunkTransaction&) = delete;
  ChunkTransaction& operator=(const ChunkTransaction&) = delete;

  /* Move constructor */
  ChunkTransaction(ChunkTransaction&& other) noexcept :
      chunkIndex(other.chunkIndex), localX(other.localX), localY(other.localY),
      localZ(other.localZ), oldUnitId(other.oldUnitId), newUnitId(other.newUnitId),
      type(other.type), state(other.state), timestamp(other.timestamp),
      sequenceNumber(other.sequenceNumber),
      completed(other.completed.load(std::memory_order_acquire))
  {
  }

  /* Move assignment operator */
  ChunkTransaction& operator=(ChunkTransaction&& other) noexcept
  {
    if (this != &other)
    {
      chunkIndex = other.chunkIndex;
      localX = other.localX;
      localY = other.localY;
      localZ = other.localZ;
      oldUnitId = other.oldUnitId;
      newUnitId = other.newUnitId;
      type = other.type;
      state = other.state;
      timestamp = other.timestamp;
      sequenceNumber = other.sequenceNumber;
      completed.store(
          other.completed.load(std::memory_order_acquire), std::memory_order_release);
    }
    return *this;
  }

  /* Validate transaction data */
  [[nodiscard]]
  ValidationResult Validate(
      int32_t chunkWidth, int32_t chunkHeight, int32_t chunkDepth) const
  {
    if (completed.load(std::memory_order_acquire))
    {
      return ValidationResult::ALREADY_COMPLETED;
    }

    if (type == TransactionType::INVALID)
    {
      return ValidationResult::INVALID_TYPE;
    }

    if (localX < 0 || localX >= chunkWidth || localY < 0 || localY >= chunkHeight ||
        localZ < 0 || localZ >= chunkDepth)
    {
      return ValidationResult::INVALID_COORDINATES;
    }

    if (type == TransactionType::WRITE || type == TransactionType::READ_WRITE)
    {
      if (oldUnitId == newUnitId)
      {
        // No-op transaction is valid but wasteful
      }
    }

    return ValidationResult::VALID;
  }

  /* Check if this transaction is valid */
  [[nodiscard]]
  bool IsValid() const
  {
    return !completed.load(std::memory_order_acquire) &&
           type != TransactionType::INVALID && state != TransactionState::FAILED &&
           state != TransactionState::ROLLED_BACK;
  }

  /* Mark transaction as completed */
  void Complete()
  {
    state = TransactionState::COMPLETED;
    completed.store(true, std::memory_order_release);
  }

  /* Mark transaction as failed */
  void Fail()
  {
    state = TransactionState::FAILED;
    completed.store(true, std::memory_order_release);
  }

  /* Rollback transaction */
  void Rollback()
  {
    state = TransactionState::ROLLED_BACK;
    completed.store(true, std::memory_order_release);
  }

  /* Get transaction state */
  [[nodiscard]]
  TransactionState GetState() const
  { return state; }

  /* Set transaction state */
  void SetState(const TransactionState newState)
  { state = newState; }

  /* Reset transaction for reuse */
  void Reset()
  {
    chunkIndex = 0;
    localX = 0;
    localY = 0;
    localZ = 0;
    oldUnitId = 0;
    newUnitId = 0;
    type = TransactionType::READ;
    state = TransactionState::PENDING;
    timestamp = 0;
    sequenceNumber = 0;
    completed.store(false, std::memory_order_release);
  }

  /* Create a copy for rollback purposes */
  [[nodiscard]]
  ChunkTransaction CreateRollbackCopy() const
  {
    ChunkTransaction rollback;
    rollback.chunkIndex = chunkIndex;
    rollback.localX = localX;
    rollback.localY = localY;
    rollback.localZ = localZ;
    rollback.oldUnitId = newUnitId; // Swap for rollback
    rollback.newUnitId = oldUnitId; // Swap for rollback
    rollback.type = TransactionType::WRITE;
    rollback.state = TransactionState::PENDING;
    rollback.timestamp = timestamp;
    rollback.sequenceNumber = sequenceNumber + 1000000; // Ensure different sequence
    return rollback;
  }
};

/* Transaction result for operations with enhanced error reporting */
export struct TransactionResult
{
  bool             success;
  uint32_t         readUnitId;
  const char*      errorMessage;
  ValidationResult validationCode;
  uint64_t         transactionSequence;

  TransactionResult() :
      success(false), readUnitId(0), errorMessage(nullptr),
      validationCode(ValidationResult::VALID), transactionSequence(0)
  {
  }

  static TransactionResult Ok(uint32_t unitId, uint64_t sequence)
  {
    TransactionResult result;
    result.success = true;
    result.readUnitId = unitId;
    result.validationCode = ValidationResult::VALID;
    result.transactionSequence = sequence;
    return result;
  }

  static TransactionResult Error(
      const char* error, ValidationResult code = ValidationResult::VALID)
  {
    TransactionResult result;
    result.success = false;
    result.errorMessage = error;
    result.validationCode = code;
    return result;
  }

  /* Get human-readable error message */
  [[nodiscard]]
  const char* GetErrorMessage() const
  {
    if (errorMessage)
      return errorMessage;

    switch (validationCode)
    {
    case ValidationResult::INVALID_CHUNK_INDEX:
      return "Invalid chunk index";
    case ValidationResult::INVALID_COORDINATES:
      return "Invalid local coordinates";
    case ValidationResult::INVALID_UNIT_ID:
      return "Invalid unit ID";
    case ValidationResult::INVALID_TYPE:
      return "Invalid transaction type";
    case ValidationResult::ALREADY_COMPLETED:
      return "Transaction already completed";
    default:
      return "Unknown error";
    }
  }
};

/* Transaction statistics for monitoring */
export struct TransactionStats
{
  uint64_t totalTransactions;
  uint64_t successfulTransactions;
  uint64_t failedTransactions;
  uint64_t rolledBackTransactions;
  uint64_t averageProcessingTimeNs;
};

} // namespace Rl::World::Chunk
