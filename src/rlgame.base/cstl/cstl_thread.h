#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque handle to a thread
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct R_CSTL_Thread;

/**
 * @brief Thread entry point function signature
 *
 * @param pData User-provided data pointer passed to the thread on creation.
 * @return Thread exit code (implementation-defined).
 */
typedef void (*R_CSTL_ThreadFunc) (void* pData);

/**
 * @brief Opaque handle to a mutex (mutual exclusion lock)
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct R_CSTL_Mutex;

/**
 * @brief Opaque handle to a condition variable
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct R_CSTL_Condition;

/**
 * @brief Create a new thread
 *
 * Creates a new thread that begins executing the provided function.
 *
 * @param pFunc Thread entry point function.
 * @param pData User data pointer to pass to the thread function.
 * @return Pointer to new thread handle, or NULL on failure.
 *
 * @note The thread begins execution immediately.
 * @note The thread must be joined with R_CSTL_ThreadJoin to release resources.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API struct R_CSTL_Thread* R_CSTL_NewThread (R_CSTL_ThreadFunc pFunc, void* pData);

/**
 * @brief Wait for a thread to finish
 *
 * Blocks the calling thread until the specified thread terminates.
 *
 * @param pThread Pointer to thread handle.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note After joining, the thread handle becomes invalid and must not be used.
 * @note The thread handle is automatically freed after successful join.
 * @note If NULL is passed, returns R_CSTL_ERROR_INVALID_ARGUMENT.
 */
R_CSTL_API int R_CSTL_ThreadJoin (struct R_CSTL_Thread* pThread);

/**
 * @brief Get the current thread ID
 *
 * Returns a platform-specific identifier for the calling thread.
 *
 * @return Thread ID (platform-specific opaque value).
 *
 * @note The ID is only valid for comparison; do not rely on specific values.
 * @note Thread IDs may be reused after a thread terminates.
 */
R_CSTL_API uint64_t R_CSTL_ThreadGetCurrentId (void);

/**
 * @brief Yield the current thread
 *
 * Hints to the scheduler that the current thread is willing to yield
 * its current use of a processor.
 *
 * @note This is a hint; the scheduler may ignore it.
 * @note Useful for spin-wait loops to reduce CPU contention.
 */
R_CSTL_API void R_CSTL_ThreadYield (void);

/**
 * @brief Sleep the current thread for milliseconds
 *
 * Suspends the calling thread for at least the specified duration.
 *
 * @param milliseconds Duration to sleep in milliseconds.
 *
 * @note The actual sleep time may be longer due to scheduler granularity.
 * @note Thread-safe: can be called from any thread.
 */
R_CSTL_API void R_CSTL_ThreadSleep (uint32_t milliseconds);

/**
 * @brief Create a mutex
 *
 * Creates a new mutex for mutual exclusion synchronization.
 *
 * @return Pointer to new mutex handle, or NULL on failure.
 *
 * @note The mutex is initially unlocked.
 * @note The mutex must be destroyed with R_CSTL_MutexDestroy when no longer needed.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API struct R_CSTL_Mutex* R_CSTL_NewMutex (void);

/**
 * @brief Destroy a mutex
 *
 * Releases resources associated with a mutex.
 *
 * @param pMutex Pointer to mutex handle. If NULL, function does nothing.
 *
 * @note The mutex must not be locked when destroyed.
 * @note After this call, the pointer becomes invalid and must not be used.
 * @note Undefined behavior if a thread is waiting on the mutex.
 */
R_CSTL_API void R_CSTL_MutexDestroy (struct R_CSTL_Mutex* pMutex);

/**
 * @brief Lock a mutex
 *
 * Attempts to acquire exclusive ownership of a mutex.
 * Blocks until the mutex becomes available.
 *
 * @param pMutex Pointer to mutex handle.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note If the mutex is already locked by another thread, blocks until unlocked.
 * @note If the mutex is already locked by the calling thread, behavior is undefined
 *       (non-recursive mutex).
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_MutexLock (struct R_CSTL_Mutex* pMutex);

/**
 * @brief Try to lock a mutex without blocking
 *
 * Attempts to acquire exclusive ownership of a mutex without blocking.
 *
 * @param pMutex Pointer to mutex handle.
 * @param pOutLocked Pointer to receive lock result (1 if locked, 0 if not).
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Returns immediately with pOutLocked set to 0 if mutex is locked by another thread.
 * @note If successful, the caller must unlock the mutex with R_CSTL_MutexUnlock.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_MutexTryLock (struct R_CSTL_Mutex* pMutex, int* pOutLocked);

/**
 * @brief Unlock a mutex
 *
 * Releases exclusive ownership of a mutex.
 *
 * @param pMutex Pointer to mutex handle.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note The mutex must be locked by the calling thread.
 * @note Undefined behavior if unlocked by a thread that does not own it.
 * @note May wake a waiting thread.
 */
R_CSTL_API int R_CSTL_MutexUnlock (struct R_CSTL_Mutex* pMutex);

/**
 * @brief Create a condition variable
 *
 * Creates a new condition variable for thread synchronization.
 *
 * @return Pointer to new condition variable handle, or NULL on failure.
 *
 * @note The condition variable must be used with a mutex.
 * @note The condition variable must be destroyed with R_CSTL_ConditionDestroy.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API struct R_CSTL_Condition* R_CSTL_ConditionCreate (void);

/**
 * @brief Destroy a condition variable
 *
 * Releases resources associated with a condition variable.
 *
 * @param pCondition Pointer to condition variable handle. If NULL, function does nothing.
 *
 * @note The condition variable must not be in use when destroyed.
 * @note After this call, the pointer becomes invalid and must not be used.
 * @note Undefined behavior if a thread is waiting on the condition variable.
 */
R_CSTL_API void R_CSTL_ConditionDestroy (struct R_CSTL_Condition* pCondition);

/**
 * @brief Wait on a condition variable
 *
 * Atomically releases the mutex and blocks the calling thread until
 * the condition variable is signaled. Upon return, the mutex is re-acquired.
 *
 * @param pCondition Pointer to condition variable handle.
 * @param pMutex Pointer to mutex handle (must be locked by caller).
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note The mutex must be locked by the calling thread before calling this function.
 * @note Spurious wakeups may occur; always check the condition in a loop.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_ConditionWait (struct R_CSTL_Condition* pCondition, struct R_CSTL_Mutex* pMutex);

/**
 * @brief Signal a condition variable (wake one thread)
 *
 * Wakes one thread waiting on the condition variable (if any).
 *
 * @param pCondition Pointer to condition variable handle.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note If multiple threads are waiting, exactly one is woken (unspecified which).
 * @note If no threads are waiting, the signal is lost.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_ConditionSignal (struct R_CSTL_Condition* pCondition);

/**
 * @brief Broadcast a condition variable (wake all threads)
 *
 * Wakes all threads waiting on the condition variable.
 *
 * @param pCondition Pointer to condition variable handle.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note All waiting threads will compete to re-acquire the associated mutex.
 * @note If no threads are waiting, the broadcast has no effect.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_ConditionBroadcast (struct R_CSTL_Condition* pCondition);

/**
 * @brief RAII-style mutex lock guard
 *
 * Helper structure for automatic mutex unlocking when leaving a scope.
 * Use with R_CSTL_MutexLockGuard and R_CSTL_MutexUnlockGuard macros.
 */
struct R_CSTL_MutexLockGuard
{
                struct R_CSTL_Mutex* pMutex;
                int                  locked;
};

/**
 * @brief Lock a mutex and initialize a guard
 *
 * Locks the mutex and initializes the guard structure for automatic unlocking.
 *
 * @param pGuard Pointer to guard structure.
 * @param pMutex Pointer to mutex to lock.
 *
 * @note Use R_CSTL_MutexUnlockGuard when leaving the scope.
 * @note If lock fails, guard.locked is set to 0.
 */
#define R_CSTL_MutexLockGuard(pGuard, pMutex)                                                                \
        do                                                                                                   \
        {                                                                                                    \
                (pGuard)->pMutex = (pMutex);                                                                 \
                (pGuard)->locked = (R_CSTL_MutexLock (pMutex) == R_CSTL_OK) ? 1 : 0;                         \
        } while (0)

/**
 * @brief Unlock a mutex via guard
 *
 * Unlocks the mutex if it was successfully locked by the guard.
 *
 * @param pGuard Pointer to guard structure.
 *
 * @note Safe to call even if lock failed (checks guard.locked).
 * @note Sets guard.locked to 0 after unlocking.
 */
#define R_CSTL_MutexUnlockGuard(pGuard)                                                                      \
        do                                                                                                   \
        {                                                                                                    \
                if ((pGuard)->locked)                                                                        \
                {                                                                                            \
                        R_CSTL_MutexUnlock ((pGuard)->pMutex);                                               \
                        (pGuard)->locked = 0;                                                                \
                }                                                                                            \
        } while (0)
