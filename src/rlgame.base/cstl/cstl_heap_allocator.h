#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

/**
 * @file cstl_heap_allocator.h
 * @brief Custom buddy memory allocator with debug features
 *
 * This allocator provides a thread-safe, buddy-system memory allocator with
 * optional debug features including memory poisoning, leak detection, and
 * allocation tracking. It is designed for embedded systems and applications
 * requiring deterministic memory allocation behavior.
 */

/**
 * @brief Initialize the global heap with a specified size
 *
 * Initializes the buddy allocator with a heap of the given size. The size
 * will be rounded up to a power-of-two multiple of the minimum block size (32 bytes).
 *
 * @param heapSizeBytes Desired heap size in bytes. Must be > 0.
 * @return R_CSTL_OK on success, R_CSTL_ERROR_INVALID_ARGUMENT if size is 0,
 *         R_CSTL_ERROR_HEAP_ALREADY_INITIALIZED if already initialized,
 *         R_CSTL_ERROR_OUT_OF_MEMORY if mapping fails.
 *
 * @note This function is not thread-safe. Call once during initialization.
 * @note The actual heap size may be larger than requested due to alignment.
 * @note In debug mode, a guard page may be added for overflow detection.
 */
R_CSTL_API int R_CSTL_HeapInit (size_t heapSizeBytes);

/**
 * @brief Destroy the global heap and log any memory leaks
 *
 * Shuts down the allocator, frees all memory, and logs any remaining registered
 * allocations as leaks. After this call, all allocator functions become invalid.
 *
 * @note This function is not thread-safe. Call once during shutdown.
 * @note All pointers returned by the allocator become invalid after this call.
 * @note In debug mode, this will also validate CRT debug heap (MSVC).
 */
R_CSTL_API void R_CSTL_HeapShutdown (void);

/**
 * @brief Allocate memory from the global heap
 *
 * Allocates a block of memory of the specified size. The allocation is
 * thread-safe and may block if another thread is allocating.
 *
 * @param sizeBytes Size of the allocation in bytes. Must be > 0.
 * @return Pointer to allocated memory, or NULL on failure (out of memory or
 *         heap not initialized).
 *
 * @note The returned memory is not initialized (except in debug mode where
 *       it may be poisoned with 0xCD).
 * @note The actual allocated size may be larger than requested due to
 *       block size alignment.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API void* R_CSTL_HeapAlloc (size_t sizeBytes);

/**
 * @brief Allocate aligned memory from the global heap
 *
 * Allocates a block of memory with the specified alignment. The alignment
 * must be a power of two and at least sizeof(void*).
 *
 * @param sizeBytes Size of the allocation in bytes. Must be > 0.
 * @param alignment Required alignment in bytes. Must be power of two.
 * @return Pointer to aligned allocated memory, or NULL on failure.
 *
 * @note The returned pointer is aligned to the specified boundary.
 * @note May allocate more memory than requested to satisfy alignment.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API void* R_CSTL_HeapAllocAligned (size_t sizeBytes, size_t alignment);

/**
 * @brief Free memory allocated from the global heap
 *
 * Frees a previously allocated block of memory. The pointer must have been
 * returned by R_CSTL_HeapAlloc, R_CSTL_HeapAllocAligned, or R_CSTL_HeapRealloc.
 *
 * @param pData Pointer to memory to free. If NULL, the function does nothing.
 *
 * @note Passing an invalid pointer (not allocated by this heap) is undefined
 *       behavior in release mode, and will be detected in debug mode.
 * @note Thread-safe: uses internal mutex for synchronization.
 * @note In debug mode, freed memory is poisoned with 0xDD.
 */
R_CSTL_API void R_CSTL_HeapFree (void* pData);

/**
 * @brief Reallocate memory to a new size
 *
 * Grows or shrinks an existing allocation. If the block can be resized in place
 * (same buddy order), the same pointer is returned. Otherwise, a new block is
 * allocated, data is copied, and the old block is freed.
 *
 * @param pData Pointer to existing allocation, or NULL for new allocation.
 * @param newSizeBytes New desired size in bytes. If 0, equivalent to free.
 * @return Pointer to reallocated memory, or NULL on failure.
 *
 * @note If pData is NULL, behaves like R_CSTL_HeapAlloc.
 * @note If newSizeBytes is 0, behaves like R_CSTL_HeapFree and returns NULL.
 * @note If reallocation fails, the original block remains valid.
 * @note The C standard does not guarantee in-place reallocation.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API void* R_CSTL_HeapRealloc (void* pData, size_t newSizeBytes);

/**
 * @brief Register an allocation with an owner for leak tracking
 *
 * Associates an allocation with an owner object. When the owner is destroyed,
 * R_CSTL_HeapCheckObjectLeaks can be called to detect any remaining allocations.
 *
 * @param pOwner Pointer to the owner object (e.g., array, string).
 * @param pAllocation Pointer to the allocation being tracked.
 * @param sizeBytes Size of the allocation in bytes.
 * @param pName Name of the allocation type (use R_CSTL_HEAP_NAME macro).
 * @return 64-bit hash for the registration, or 0 on failure.
 *
 * @note This is for debug leak detection only. Does not affect allocation.
 * @note Thread-safe: uses internal mutex for synchronization.
 * @note Use R_CSTL_HEAP_NAME(MyType) macro for the pName parameter.
 */
R_CSTL_API uint64_t
R_CSTL_HeapRegisterAllocation (void* pOwner, void* pAllocation, size_t sizeBytes, const char* pName);

/**
 * @brief Unregister a previously tracked allocation
 *
 * Removes a registration created by R_CSTL_HeapRegisterAllocation.
 *
 * @param pOwner Pointer to the owner object.
 * @param pAllocation Pointer to the allocation being unregistered.
 *
 * @note If the registration is not found, this is a no-op.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API void R_CSTL_HeapUnregisterAllocation (void* pOwner, void* pAllocation);

/**
 * @brief Check for memory leaks belonging to a specific owner
 *
 * Logs all registered allocations still associated with the given owner.
 *
 * @param pOwner Pointer to the owner object being checked.
 * @return Number of leaked allocations found.
 *
 * @note Call this before destroying an owner object to detect leaks.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API size_t R_CSTL_HeapCheckObjectLeaks (void* pOwner);

/**
 * @brief Log all remaining memory leaks in the global registry
 *
 * Scans the entire allocation registry and logs any remaining allocations
 * as potential leaks.
 *
 * @return Total number of leaked allocations found.
 *
 * @note Call during shutdown to detect global leaks.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API size_t R_CSTL_HeapLogLeaks (void);

/**
 * @brief Check if a pointer is a valid live allocation
 *
 * Validates that the pointer points to a currently allocated block within
 * the managed heap.
 *
 * @param ptr Pointer to validate.
 * @return 1 if the pointer is valid and allocated, 0 otherwise.
 *
 * @note This is a debug helper. Does not guarantee the pointer is safe to use.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API int R_CSTL_HeapIsValidPointer (const void* ptr);

/**
 * @brief Get the total number of registered allocations
 *
 * Returns the count of allocations currently tracked in the registry.
 *
 * @return Number of registered allocations (potential leaks if not freed).
 *
 * @note This includes all registrations, not just leaks.
 * @note Thread-safe: uses internal mutex for synchronization.
 */
R_CSTL_API size_t R_CSTL_HeapGetRegisteredCount (void);

/**
 * @brief Get the total heap size in bytes
 *
 * Returns the total size of the heap as configured during initialization.
 *
 * @return Total heap size in bytes.
 *
 * @note This is the configured size, not necessarily the mapped size.
 * @note Thread-safe: reads atomic variable.
 */
R_CSTL_API size_t R_CSTL_Heap_GetTotalSize (void);

/**
 * @brief Get the number of bytes currently in use
 *
 * Returns an approximation of the heap memory currently allocated.
 *
 * @return Number of bytes in use.
 *
 * @note This is an approximation due to block size alignment.
 * @note Thread-safe: reads atomic variable.
 */
R_CSTL_API size_t R_CSTL_Heap_GetUsedSize (void);

/**
 * @brief Macro to convert a type name to a string for allocation tracking
 *
 * Use this macro when registering allocations to provide a human-readable
 * type name for leak reports.
 *
 * @param type The C type name (e.g., R_CSTL_Array)
 * @return String literal of the type name
 *
 * @example R_CSTL_HeapRegisterAllocation(pArray, pData, size, R_CSTL_HEAP_NAME(R_CSTL_Array));
 */
#define R_CSTL_HEAP_NAME(type) #type

#ifndef R_CSTL_HEAP_DEBUG_ENABLED
#ifdef R_CSTL_HEAP_DEBUG
#define R_CSTL_HEAP_DEBUG_ENABLED 1
#else
#define R_CSTL_HEAP_DEBUG_ENABLED 0
#endif
#endif

#ifdef R_CSTL_HEAP_DEBUG
/**
 * @brief Validate heap bookkeeping and CRT debug heap
 *
 * Performs consistency checks on the heap data structures and, on MSVC,
 * validates the CRT debug heap.
 *
 * @return R_CSTL_OK if heap is healthy, error code otherwise.
 *
 * @note This is a debug-only function for development.
 * @note May be slow; use sparingly in production debug builds.
 */
R_CSTL_API int R_CSTL_HeapDebugVerify (void);
#endif
