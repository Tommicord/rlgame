#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque handle to a stack
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct R_CSTL_Stack;

/**
 * @brief Create an empty stack
 *
 * Creates a new stack with no allocated buffer. The buffer will be allocated
 * on first push operation.
 *
 * @return Pointer to new stack, or NULL on allocation failure.
 *
 * @note The stack must be freed with R_CSTL_DeleteStack when no longer needed.
 * @note Thread-safe: each stack instance is independent.
 */
R_CSTL_API struct R_CSTL_Stack* R_CSTL_NewStack (void);

/**
 * @brief Create a stack with pre-reserved capacity
 *
 * Creates a new stack with the specified capacity but zero size.
 * This is useful when you know the approximate size needed to avoid
 * reallocations during growth.
 *
 * @param capacityBytes Minimum capacity in bytes to reserve.
 * @return Pointer to new stack, or NULL on allocation failure.
 *
 * @note The stack size is 0; use push to add data.
 * @note The actual capacity may be larger than requested due to alignment.
 */
R_CSTL_API struct R_CSTL_Stack* R_CSTL_NewStackWithCapacity (size_t capacityBytes);

/**
 * @brief Delete a stack and free its resources
 *
 * Destroys the stack and frees all associated memory.
 *
 * @param pStack Pointer to stack to delete. If NULL, function does nothing.
 *
 * @note After this call, the pointer becomes invalid and must not be used.
 * @note Thread-safe: each stack instance is independent.
 */
R_CSTL_API void R_CSTL_DeleteStack (struct R_CSTL_Stack* pStack);

/**
 * @brief Push a byte onto the top of the stack
 *
 * Appends a single byte to the top of the stack, growing capacity if needed.
 *
 * @param pStack Pointer to stack.
 * @param value Byte value to push.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note May trigger reallocation and data copying.
 * @note Uses SIMD-optimized copy when reallocating.
 */
R_CSTL_API int R_CSTL_StackPush (struct R_CSTL_Stack* pStack, uint8_t value);

/**
 * @brief Push data with size onto the top of the stack
 *
 * Appends data to the top of the stack, growing capacity if needed.
 *
 * @param pStack Pointer to stack.
 * @param pData Pointer to data to push.
 * @param size Size of data to push.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note May trigger reallocation and data copying.
 * @note Uses SIMD-optimized copy when reallocating.
 */
R_CSTL_API int R_CSTL_StackPushData (struct R_CSTL_Stack* pStack, const uint8_t* pData, size_t size);

/**
 * @brief Pop a byte from the top of the stack
 *
 * Removes and returns the byte at the top of the stack.
 *
 * @param pStack Pointer to stack.
 * @param pOutValue Pointer to receive the popped value. If NULL, value is discarded.
 * @return R_CSTL_OK on success, error code if stack is empty.
 *
 * @note Decrements stack size by 1.
 * @note Does not reduce capacity.
 */
R_CSTL_API int R_CSTL_StackPop (struct R_CSTL_Stack* pStack, uint8_t* pOutValue);

/**
 * @brief Pop data from the top of the stack
 *
 * Removes and returns multiple bytes from the top of the stack.
 *
 * @param pStack Pointer to stack.
 * @param pOutData Pointer to buffer to receive popped data.
 * @param size Number of bytes to pop.
 * @return R_CSTL_OK on success, error code if stack has insufficient data.
 *
 * @note Decrements stack size by size.
 * @note Does not reduce capacity.
 */
R_CSTL_API int R_CSTL_StackPopData (struct R_CSTL_Stack* pStack, uint8_t* pOutData, size_t size);

/**
 * @brief Peek at the byte at the top of the stack without removing it
 *
 * Returns the byte at the top of the stack without modifying the stack.
 *
 * @param pStack Pointer to stack.
 * @param pOutValue Pointer to receive the peeked value.
 * @return R_CSTL_OK on success, error code if stack is empty.
 *
 * @note Does not modify the stack.
 */
R_CSTL_API int R_CSTL_StackPeek (struct R_CSTL_Stack* pStack, uint8_t* pOutValue);

/**
 * @brief Peek at data from the top of the stack without removing it
 *
 * Returns multiple bytes from the top of the stack without modifying the stack.
 *
 * @param pStack Pointer to stack.
 * @param pOutData Pointer to buffer to receive peeked data.
 * @param size Number of bytes to peek.
 * @return R_CSTL_OK on success, error code if stack has insufficient data.
 *
 * @note Does not modify the stack.
 */
R_CSTL_API int R_CSTL_StackPeekData (struct R_CSTL_Stack* pStack, uint8_t* pOutData, size_t size);

/**
 * @brief Check if the stack is empty
 *
 * Tests whether the stack contains any elements.
 *
 * @param pStack Pointer to stack.
 * @return Non-zero if stack is empty, zero if stack has elements.
 *
 * @note Returns 1 (true) if size is 0, 0 (false) otherwise.
 */
R_CSTL_API int R_CSTL_StackEmpty (const struct R_CSTL_Stack* pStack);

/**
 * @brief Get the number of elements in the stack
 *
 * Returns the current size of the stack in bytes.
 *
 * @param pStack Pointer to stack.
 * @return Size in bytes, or 0 if stack is NULL or invalid.
 */
R_CSTL_API size_t R_CSTL_StackSize (const struct R_CSTL_Stack* pStack);

/**
 * @brief Get the current capacity of the stack
 *
 * Returns the total capacity in bytes (amount of allocated storage).
 *
 * @param pStack Pointer to stack.
 * @return Capacity in bytes, or 0 if stack is NULL or invalid.
 *
 * @note Capacity is always >= size.
 * @note Capacity may be larger than size due to growth strategy.
 */
R_CSTL_API size_t R_CSTL_StackGetCapacity (const struct R_CSTL_Stack* pStack);

/**
 * @brief Search for a byte in the stack
 *
 * Returns the 1-based position of the specified byte from the top of the stack.
 * The topmost element has position 1, the next has position 2, etc.
 *
 * @param pStack Pointer to stack.
 * @param value Byte value to search for.
 * @return 1-based position from top, or 0 if not found.
 *
 * @note Returns the position of the first occurrence from the top.
 * @note If the value is not found, returns 0.
 */
R_CSTL_API size_t R_CSTL_StackSearch (const struct R_CSTL_Stack* pStack, uint8_t value);

/**
 * @brief Search for data in the stack
 *
 * Returns the 1-based position of the specified data pattern from the top of the stack.
 * The topmost element has position 1, the next has position 2, etc.
 *
 * @param pStack Pointer to stack.
 * @param pData Pointer to data pattern to search for.
 * @param size Size of data pattern.
 * @return 1-based position from top, or 0 if not found.
 *
 * @note Returns the position of the first occurrence from the top.
 * @note If the pattern is not found, returns 0.
 */
R_CSTL_API size_t R_CSTL_StackSearchData (const struct R_CSTL_Stack* pStack, const uint8_t* pData, size_t size);

/**
 * @brief Clear the stack contents
 *
 * Removes all elements from the stack, optionally zeroing the memory.
 *
 * @param pStack Pointer to stack.
 * @param zeroMemory If non-zero, sets all bytes to 0; otherwise leaves data as-is.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Size is set to 0; capacity is unchanged.
 * @note Does not free the buffer.
 */
R_CSTL_API int R_CSTL_StackClear (struct R_CSTL_Stack* pStack, int zeroMemory);

/**
 * @brief Get pointer to stack data
 *
 * Returns a pointer to the underlying byte buffer.
 *
 * @param pStack Pointer to stack.
 * @return Pointer to data buffer, or NULL if stack is empty or invalid.
 *
 * @warning The pointer becomes invalid if the stack is modified or deleted.
 * @note For empty stacks, returns NULL.
 * @note The data is organized with the top of the stack at the end of the buffer.
 */
R_CSTL_API const uint8_t* R_CSTL_StackData (const struct R_CSTL_Stack* pStack);

/**
 * @brief Reserve capacity without changing size
 *
 * Ensures the stack has at least the specified capacity. If the current
 * capacity is already sufficient, no reallocation occurs.
 *
 * @param pStack Pointer to stack.
 * @param capacityBytes Minimum capacity to reserve.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note The stack size is unchanged.
 * @note May trigger reallocation and data copying.
 */
R_CSTL_API int R_CSTL_StackReserve (struct R_CSTL_Stack* pStack, size_t capacityBytes);

/**
 * @brief Get a typed element at a specific position from the top
 *
 * Reads the element of type Type at the specified position from the top (1-based).
 * Position 1 is the top element, position 2 is the second from top, etc.
 *
 * @param pStack Pointer to stack.
 * @param Type The type of element to retrieve.
 * @param position 1-based position from the top.
 * @param pOutValue Pointer to receive the value.
 *
 * @note This macro performs bounds checking.
 * @note The element is copied using memcpy for optimal performance (SIMD-friendly).
 */
#define R_CSTL_StackTypedAt(pStack, Type, position, pOutValue)                                             \
        do                                                                                                   \
        {                                                                                                    \
                Type _temp;                                                                                  \
                size_t _offset = (position - 1) * sizeof (Type);                                            \
                size_t _stackSize = R_CSTL_StackSize (pStack);                                              \
                const uint8_t* _pData = R_CSTL_StackData (pStack);                                         \
                if (_offset + sizeof (Type) <= _stackSize)                                                    \
                {                                                                                            \
                        memcpy (&_temp, _pData + (_stackSize - _offset - sizeof (Type)), sizeof (Type));  \
                }                                                                                            \
                else                                                                                         \
                {                                                                                            \
                        memset (&_temp, 0, sizeof (Type));                                                   \
                }                                                                                            \
                memcpy (pOutValue, &_temp, sizeof (Type));                                                   \
        } while (0)

/**
 * @brief Get a typed element at a specific position from the top without bounds checking
 *
 * Reads the element of type Type at the specified position from the top (1-based).
 * Position 1 is the top element, position 2 is the second from top, etc.
 *
 * @param pStack Pointer to stack.
 * @param Type The type of element to retrieve.
 * @param position 1-based position from the top.
 * @param pOutValue Pointer to receive the value.
 *
 * @warning No bounds checking; undefined behavior if position is invalid.
 * @note The element is copied using memcpy for optimal performance (SIMD-friendly).
 */
#define R_CSTL_StackTypedAtUnchecked(pStack, Type, position, pOutValue)                                    \
        do                                                                                                   \
        {                                                                                                    \
                Type _temp;                                                                                  \
                size_t _offset = (position - 1) * sizeof (Type);                                            \
                size_t _stackSize = R_CSTL_StackSize (pStack);                                              \
                const uint8_t* _pData = R_CSTL_StackData (pStack);                                         \
                memcpy (&_temp, _pData + (_stackSize - _offset - sizeof (Type)), sizeof (Type));            \
                memcpy (pOutValue, &_temp, sizeof (Type));                                                   \
        } while (0)
