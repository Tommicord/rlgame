#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque handle to a dynamic array
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct r_cstl_array;

/**
 * @brief Create an empty array
 *
 * Creates a new array with no allocated buffer. The buffer will be allocated
 * on first use (push, unshift, etc.).
 *
 * @return Pointer to new array, or NULL on allocation failure.
 *
 * @note The array must be freed with r_cstl_delete_array when no longer needed.
 * @note Thread-safe: each array instance is independent.
 */
R_CSTL_API struct r_cstl_array* r_cstl_new_array (void);

/**
 * @brief Create an array with pre-reserved capacity
 *
 * Creates a new array with the specified capacity but zero length.
 * This is useful when you know the approximate size needed to avoid
 * reallocations during growth.
 *
 * @param capacityBytes Minimum capacity in bytes to reserve.
 * @return Pointer to new array, or NULL on allocation failure.
 *
 * @note The array length is 0; use push/unshift to add data.
 * @note The actual capacity may be larger than requested due to alignment.
 */
R_CSTL_API struct r_cstl_array* r_cstl_new_array_with_capacity (size_t capacityBytes);

/**
 * @brief Create an array with specified length, zero-initialized
 *
 * Creates a new array with the given length and initializes all bytes to zero.
 *
 * @param lengthBytes Length of the array in bytes.
 * @return Pointer to new array, or NULL on allocation failure.
 *
 * @note All bytes are initialized to 0.
 * @note The capacity will be at least lengthBytes.
 */
R_CSTL_API struct r_cstl_array* r_cstl_new_array_with_length_zeroed (size_t lengthBytes);

/**
 * @brief Create an array with specified length, uninitialized
 *
 * Creates a new array with the given length but does not initialize the data.
 * This is faster than zero-initialization when you plan to overwrite all data.
 *
 * @param lengthBytes Length of the array in bytes.
 * @return Pointer to new array, or NULL on allocation failure.
 *
 * @warning Data is uninitialized; reading before writing is undefined behavior.
 * @note In debug mode, memory may be poisoned with 0xCD.
 */
R_CSTL_API struct r_cstl_array* r_cstl_new_array_with_length (size_t lengthBytes);

/**
 * @brief Create an array from existing data
 *
 * Creates a new array containing a copy of the provided data.
 *
 * @param pData Pointer to data to copy. If NULL and lengthBytes is 0, creates empty array.
 * @param lengthBytes Number of bytes to copy.
 * @return Pointer to new array, or NULL on allocation failure.
 *
 * @note The data is copied; the original buffer is not modified.
 * @note If pData is NULL and lengthBytes > 0, returns NULL.
 */
R_CSTL_API struct r_cstl_array* r_cstl_new_array_with_data (const uint8_t* pData, size_t lengthBytes);

/**
 * @brief Delete an array and free its resources
 *
 * Destroys the array and frees all associated memory.
 *
 * @param pArray Pointer to array to delete. If NULL, function does nothing.
 *
 * @note After this call, the pointer becomes invalid and must not be used.
 * @note Thread-safe: each array instance is independent.
 */
R_CSTL_API void r_cstl_delete_array (struct r_cstl_array* pArray);

/**
 * @brief Reserve capacity without changing length
 *
 * Ensures the array has at least the specified capacity. If the current
 * capacity is already sufficient, no reallocation occurs.
 *
 * @param pArray Pointer to array.
 * @param capacityBytes Minimum capacity to reserve.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note The array length is unchanged.
 * @note May trigger reallocation and data copying.
 */
R_CSTL_API int r_cstl_array_rev_bytes (struct r_cstl_array* pArray, size_t capacityBytes);

/**
 * @brief Push a byte to the end of the array
 *
 * Appends a single byte to the end of the array, growing capacity if needed.
 *
 * @param pArray Pointer to array.
 * @param value Byte value to append.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note May trigger reallocation and data copying.
 * @note Uses SIMD-optimized copy when reallocating.
 */
R_CSTL_API int r_cstl_array_push (struct r_cstl_array* pArray, uint8_t value);

/**
 * @brief Push data with size to the end of the array
 *
 * Appends data to the end of the array, growing capacity if needed.
 *
 * @param pArray Pointer to array.
 * @param pData Pointer to data to append.
 * @param size Size of data to append.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note May trigger reallocation and data copying.
 * @note Uses SIMD-optimized copy when reallocating.
 */
R_CSTL_API int r_cstl_array_push_data (struct r_cstl_array* pArray, const uint8_t* pData, size_t size);

/**
 * @brief Pop a byte from the end of the array
 *
 * Removes and returns the last byte from the array.
 *
 * @param pArray Pointer to array.
 * @param pOutValue Pointer to receive the popped value. If NULL, value is discarded.
 * @return R_CSTL_OK on success, error code if array is empty.
 *
 * @note Decrements array length by 1.
 * @note Does not reduce capacity.
 */
R_CSTL_API int r_cstl_array_pop (struct r_cstl_array* pArray, uint8_t* pOutValue);

/**
 * @brief Shift a byte from the beginning of the array
 *
 * Removes and returns the first byte from the array, shifting all
 * remaining elements down by one position.
 *
 * @param pArray Pointer to array.
 * @param pOutValue Pointer to receive the shifted value. If NULL, value is discarded.
 * @return R_CSTL_OK on success, error code if array is empty.
 *
 * @note This is O(n) operation as it requires shifting all elements.
 * @note Decrements array length by 1.
 */
R_CSTL_API int r_cstl_array_shift (struct r_cstl_array* pArray, uint8_t* pOutValue);

/**
 * @brief Unshift a byte to the beginning of the array
 *
 * Inserts a byte at the beginning of the array, shifting all existing
 * elements up by one position.
 *
 * @param pArray Pointer to array.
 * @param value Byte value to insert.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note This is O(n) operation as it requires shifting all elements.
 * @note May trigger reallocation and data copying.
 */
R_CSTL_API int r_cstl_array_unshift (struct r_cstl_array* pArray, uint8_t value);

/**
 * @brief Create a slice of the array
 *
 * Returns a new array containing a subrange of the original array.
 * The range is [start, end), inclusive of start, exclusive of end.
 *
 * @param pArray Pointer to source array.
 * @param start Starting index (inclusive).
 * @param end Ending index (exclusive).
 * @return Pointer to new array, or NULL on invalid range or allocation failure.
 *
 * @note The data is copied; modifications to the slice don't affect the original.
 * @note If start >= end, returns an empty array.
 * @note If start or end are out of bounds, returns NULL.
 */
R_CSTL_API struct r_cstl_array*
r_cstl_array_slice (const struct r_cstl_array* pArray, size_t start, size_t end);

/**
 * @brief Get pointer to array data
 *
 * Returns a pointer to the underlying byte buffer.
 *
 * @param pArray Pointer to array.
 * @return Pointer to data buffer, or NULL if array is empty or invalid.
 *
 * @warning The pointer becomes invalid if the array is modified or deleted.
 * @note For empty arrays, returns NULL.
 */
R_CSTL_API const uint8_t* r_cstl_array_data (const struct r_cstl_array* pArray);

/**
 * @brief Get the current length of the array
 *
 * Returns the number of bytes currently stored in the array.
 *
 * @param pArray Pointer to array.
 * @return Length in bytes, or 0 if array is NULL or invalid.
 */
R_CSTL_API size_t r_cstl_array_length (const struct r_cstl_array* pArray);

/**
 * @brief Get the current capacity of the array
 *
 * Returns the total capacity in bytes (amount of allocated storage).
 *
 * @param pArray Pointer to array.
 * @return Capacity in bytes, or 0 if array is NULL or invalid.
 *
 * @note Capacity is always >= length.
 * @note Capacity may be larger than length due to growth strategy.
 */
R_CSTL_API size_t r_cstl_array_get_capacity (const struct r_cstl_array* pArray);

/**
 * @brief Get a byte at a specific index with bounds checking
 *
 * Reads the byte at the specified index, validating that the index is within bounds.
 *
 * @param pArray Pointer to array.
 * @param index Index to read.
 * @param pOutValue Pointer to receive the value.
 * @return R_CSTL_OK on success, R_CSTL_ERROR_INDEX_OUT_OF_BOUNDS if invalid.
 *
 * @note This function performs bounds checking.
 * @note For performance-critical code, use r_cstl_array_unchecked_at.
 */
R_CSTL_API int r_cstl_array_at (const struct r_cstl_array* pArray, size_t index, uint8_t* pOutValue);

/**
 * @brief Get a byte at a specific index without bounds checking
 *
 * Reads the byte at the specified index without validating bounds.
 *
 * @param pArray Pointer to array.
 * @param index Index to read.
 * @param pOutValue Output pointer to buffer
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @warning No bounds checking; undefined behavior if index >= length.
 * @note Use only when you can guarantee the index is valid.
 * @note This is faster than r_cstl_array_at for tight loops.
 */
R_CSTL_API int r_cstl_array_unchecked_at (const struct r_cstl_array* pArray, size_t index, uint8_t* pOutValue);

/**
 * @brief Clear the array contents
 *
 * Removes all elements from the array, optionally zeroing the memory.
 *
 * @param pArray Pointer to array.
 * @param zeroMemory If non-zero, sets all bytes to 0; otherwise leaves data as-is.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Length is set to 0; capacity is unchanged.
 * @note Does not free the buffer.
 */
R_CSTL_API int r_cstl_array_clear (struct r_cstl_array* pArray, int zeroMemory);

/**
 * @brief Fill the array's current length with a byte value
 *
 * Sets all bytes in the array (up to current length) to the specified value.
 *
 * @param pArray Pointer to array.
 * @param value Byte value to fill with.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Only fills up to the current length, not the entire capacity.
 * @note Does not change the array length.
 */
R_CSTL_API int r_cstl_array_fill (struct r_cstl_array* pArray, uint8_t value);

/**
 * @brief Built-in comparator for 8-bit unsigned integers
 *
 * Standard comparison function for sorting uint8_t arrays.
 *
 * @param pLeft Pointer to first value.
 * @param pRight Pointer to second value.
 * @param pData User context (unused, can be NULL).
 * @return -1 if left < right, 0 if equal, 1 if left > right.
 */
R_CSTL_API int r_cstl_array_compare_u8 (const void* pLeft, const void* pRight, void* pData);

/**
 * @brief Built-in comparator for 16-bit unsigned integers
 *
 * Standard comparison function for sorting uint16_t arrays.
 *
 * @param pLeft Pointer to first value.
 * @param pRight Pointer to second value.
 * @param pData User context (unused, can be NULL).
 * @return -1 if left < right, 0 if equal, 1 if left > right.
 */
R_CSTL_API int r_cstl_array_compare_u16 (const void* pLeft, const void* pRight, void* pData);

/**
 * @brief Built-in comparator for 32-bit unsigned integers
 *
 * Standard comparison function for sorting uint32_t arrays.
 *
 * @param pLeft Pointer to first value.
 * @param pRight Pointer to second value.
 * @param pData User context (unused, can be NULL).
 * @return -1 if left < right, 0 if equal, 1 if left > right.
 */
R_CSTL_API int r_cstl_array_compare_u32 (const void* pLeft, const void* pRight, void* pData);

/**
 * @brief Built-in comparator for 64-bit unsigned integers
 *
 * Standard comparison function for sorting uint64_t arrays.
 *
 * @param pLeft Pointer to first value.
 * @param pRight Pointer to second value.
 * @param pData User context (unused, can be NULL).
 * @return -1 if left < right, 0 if equal, 1 if left > right.
 */
R_CSTL_API int r_cstl_array_compare_u64 (const void* pLeft, const void* pRight, void* pData);

/**
 * @brief Sort the array as a sequence of elements
 *
 * Sorts the array using a hybrid introsort algorithm: insertion sort for small
 * ranges, quicksort with median-of-three pivot for larger ranges, and heapsort
 * when recursion depth is exhausted.
 *
 * @param pArray Pointer to array to sort.
 * @param elemSize Size of each element in bytes. Array length must be a multiple.
 * @param pComparator Comparison function with qsort-like semantics plus context.
 * @param pData User context passed to comparator (can be NULL).
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note For primitive types (1/2/4/8 bytes), uses typed fast paths with built-in comparators.
 * @note For uint8_t arrays with r_cstl_array_compare_u8, uses counting sort (O(n)).
 * @note Larger elements use SIMD-optimized moves.
 * @note Comparator returns: negative if left < right, 0 if equal, positive if left > right.
 * @note Array length must be a multiple of elemSize.
 */
R_CSTL_API int r_cstl_array_sort (
    struct r_cstl_array* pArray,
    uint8_t              elemSize,
    int (*pComparator) (const void* pLeft, const void* pRight, void* pData),
    void* pData);

/**
 * @brief Get a typed element at a specific index with bounds checking
 *
 * Reads the element of type Type at the specified index, validating that the index is within bounds.
 *
 * @param pArray Pointer to array.
 * @param Type The type of element to retrieve.
 * @param index Index of the element to read.
 * @param pOutValue Pointer to receive the value.
 *
 * @note This macro performs bounds checking via r_cstl_array_at.
 */
#define r_cstl_array_typed_at(pArray, Type, index, pOutValue)                                                  \
    do                                                                                                       \
    {                                                                                                        \
        Type           _temp;                                                                                \
        size_t         _offset = (index) * sizeof (Type);                                                    \
        const uint8_t* _pData = r_cstl_array_data (pArray);                                                   \
        if (_offset + sizeof (Type) <= r_cstl_array_length (pArray))                                          \
        {                                                                                                    \
            memcpy (&_temp, _pData + _offset, sizeof (Type));                                                \
        }                                                                                                    \
        else                                                                                                 \
        {                                                                                                    \
            memset (&_temp, 0, sizeof (Type));                                                               \
        }                                                                                                    \
        *(pOutValue) = _temp;                                                                                \
    } while (0)

/**
 * @brief Get a typed element at a specific index without bounds checking
 *
 * Reads the element of type Type at the specified index without validating bounds.
 *
 * @param pArray Pointer to array.
 * @param Type The type of element to retrieve.
 * @param index Index of the element to read.
 * @param pOutValue Pointer to receive the value.
 *
 * @warning No bounds checking; undefined behavior if index is invalid.
 */
#define r_cstl_array_typed_unchecked_at(pArray, Type, index, pOutValue)                                         \
    do                                                                                                       \
    {                                                                                                        \
        Type           _temp;                                                                                \
        size_t         _offset = (index) * sizeof (Type);                                                    \
        const uint8_t* _pData = r_cstl_array_data (pArray);                                                   \
        memcpy (&_temp, _pData + _offset, sizeof (Type));                                                    \
        memcpy (pOutValue, &_temp, sizeof (Type));                                                           \
    } while (0)

/**
 * @brief Set a typed element at a specific index with bounds checking
 *
 * Writes the element of type Type at the specified index, validating that the index is within bounds.
 *
 * @param pArray Pointer to array.
 * @param Type The type of element to write.
 * @param index Index of the element to write.
 * @param pValue Pointer to the value to write.
 *
 * @note This macro performs bounds checking before writing.
 * @note Returns 0 on success, -1 if index is out of bounds.
 */
#define r_cstl_array_typed_set_at(pArray, Type, index, pValue)                                                  \
    do                                                                                                       \
    {                                                                                                        \
        size_t _offset = (index) * sizeof (Type);                                                            \
        if (_offset + sizeof (Type) <= r_cstl_array_length (pArray))                                          \
        {                                                                                                    \
            uint8_t* _pData = r_cstl_array_data (pArray);                                                     \
            memcpy (_pData + _offset, pValue, sizeof (Type));                                                \
        }                                                                                                    \
    } while (0)

/**
 * @brief Set a typed element at a specific index without bounds checking
 *
 * Writes the element of type Type at the specified index without validating bounds.
 *
 * @param pArray Pointer to array.
 * @param Type The type of element to write.
 * @param index Index of the element to write.
 * @param pValue Pointer to the value to write.
 *
 * @warning No bounds checking; undefined behavior if index is invalid.
 */
#define r_cstl_array_typed_set_at_unchecked(pArray, Type, index, pValue)                                         \
    do                                                                                                       \
    {                                                                                                        \
        size_t   _offset = (index) * sizeof (Type);                                                          \
        uint8_t* _pData = r_cstl_array_data (pArray);                                                         \
        memcpy (_pData + _offset, pValue, sizeof (Type));                                                    \
    } while (0)
