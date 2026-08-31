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
struct r_cstl_thread;

/**
 * @brief Thread entry point function signature
 *
 * @param pData User-provided data pointer passed to the thread on creation.
 * @return Thread exit code (implementation-defined).
 */
typedef void (*r_cstl_thread_func) (void* pData);

/**
 * @brief Opaque handle to a mutex (mutual exclusion lock)
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct r_cstl_mutex;

/**
 * @brief Opaque handle to a condition variable
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct r_cstl_condition;

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
 * @note The thread must be joined with r_cstl_thread_join to release resources.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API struct r_cstl_thread* r_cstl_new_thread (r_cstl_thread_func pFunc, void* pData);

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
R_CSTL_API int r_cstl_thread_join (struct r_cstl_thread* pThread);

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
R_CSTL_API uint64_t r_cstl_thread_get_current_id (void);

/**
 * @brief Yield the current thread
 *
 * Hints to the scheduler that the current thread is willing to yield
 * its current use of a processor.
 *
 * @note This is a hint; the scheduler may ignore it.
 * @note Useful for spin-wait loops to reduce CPU contention.
 */
R_CSTL_API void r_cstl_thread_yield (void);

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
R_CSTL_API void r_cstl_thread_sleep (uint32_t milliseconds);

/**
 * @brief Create a mutex
 *
 * Creates a new mutex for mutual exclusion synchronization.
 *
 * @return Pointer to new mutex handle, or NULL on failure.
 *
 * @note The mutex is initially unlocked.
 * @note The mutex must be destroyed with r_cstl_mutex_destroy when no longer needed.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API struct r_cstl_mutex* r_cstl_new_mutex (void);

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
R_CSTL_API void r_cstl_mutex_destroy (struct r_cstl_mutex* pMutex);

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
R_CSTL_API int r_cstl_mutex_lock (struct r_cstl_mutex* pMutex);

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
 * @note If successful, the caller must unlock the mutex with r_cstl_mutex_unlock.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int r_cstl_mutex_try_lock (struct r_cstl_mutex* pMutex, int* pOutLocked);

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
R_CSTL_API int r_cstl_mutex_unlock (struct r_cstl_mutex* pMutex);

/**
 * @brief Create a condition variable
 *
 * Creates a new condition variable for thread synchronization.
 *
 * @return Pointer to new condition variable handle, or NULL on failure.
 *
 * @note The condition variable must be used with a mutex.
 * @note The condition variable must be destroyed with r_cstl_condition_destroy.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API struct r_cstl_condition* r_cstl_condition_create (void);

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
R_CSTL_API void r_cstl_condition_destroy (struct r_cstl_condition* pCondition);

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
R_CSTL_API int r_cstl_condition_wait (struct r_cstl_condition* pCondition, struct r_cstl_mutex* pMutex);

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
R_CSTL_API int r_cstl_condition_signal (struct r_cstl_condition* pCondition);

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
R_CSTL_API int r_cstl_condition_broadcast (struct r_cstl_condition* pCondition);

/**
 * @brief RAII-style mutex lock guard
 *
 * Helper structure for automatic mutex unlocking when leaving a scope.
 * Use with r_cstl_mutex_lock_guard and r_cstl_mutex_unlock_guard macros.
 */
struct r_cstl_mutex_lock_guard
{
        struct r_cstl_mutex* pMutex;
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
 * @note Use r_cstl_mutex_unlock_guard when leaving the scope.
 * @note If lock fails, guard.locked is set to 0.
 */
#define r_cstl_mutex_lock_guard(pGuard, pMutex)                                                                \
    do                                                                                                       \
    {                                                                                                        \
        (pGuard)->pMutex = (pMutex);                                                                         \
        (pGuard)->locked = (r_cstl_mutex_lock (pMutex) == R_CSTL_OK) ? 1 : 0;                                 \
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
#define r_cstl_mutex_unlock_guard(pGuard)                                                                      \
    do                                                                                                       \
    {                                                                                                        \
        if ((pGuard)->locked)                                                                                \
        {                                                                                                    \
            r_cstl_mutex_unlock ((pGuard)->pMutex);                                                           \
            (pGuard)->locked = 0;                                                                            \
        }                                                                                                    \
    } while (0)
