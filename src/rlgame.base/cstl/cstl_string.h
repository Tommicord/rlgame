#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Opaque handle to a dynamic string
     *
     * The internal structure is opaque to maintain ABI stability and allow
     * implementation changes without breaking client code. Uses small buffer
     * optimization for strings up to a certain size.
     */
    struct r_cstl_string;

    /**
     * @brief Opaque handle to a string builder
     *
     * The string builder provides an efficient way to construct strings through
     * multiple append operations without repeated allocations.
     */
    struct r_cstl_string_builder;

    /**
     * @brief Create an empty string
     *
     * Creates a new empty string with no allocated buffer.
     *
     * @return Pointer to new string, or NULL on allocation failure.
     *
     * @note The string must be freed with r_cstl_string_delete when no longer needed.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_new_string (void);

    /**
     * @brief Create a string with pre-reserved capacity
     *
     * Creates a new empty string with the specified capacity.
     *
     * @param capacity Minimum capacity to reserve.
     * @return Pointer to new string, or NULL on allocation failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_new_string_with_capacity (const size_t capacity);

    /**
     * @brief Create a string from data with explicit length
     *
     * Creates a new string containing a copy of the provided data.
     *
     * @param pData Pointer to data to copy. If NULL and length is 0, creates empty string.
     * @param length Number of bytes to copy.
     * @return Pointer to new string, or NULL on allocation failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_new_string_with_data_sized (const char* pData, const size_t length);

    /**
     * @brief Create a string from null-terminated data
     *
     * Creates a new string containing a copy of the provided null-terminated string.
     *
     * @param pData Pointer to null-terminated string. If NULL, creates empty string.
     * @return Pointer to new string, or NULL on allocation failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_new_string_with_data (const char* pData);

    /**
     * @brief Create a formatted string
     *
     * Creates a new string using printf-style formatting.
     *
     * @param pFormat Printf-style format string.
     * @param ... Variable arguments for formatting.
     * @return Pointer to new string, or NULL on allocation failure.
     *
     * @note If pFormat is NULL, returns an empty string.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_new_string_with_format (const char* pFormat, ...);

    /**
     * @brief Delete a string and free its resources
     *
     * Destroys the string and frees all associated memory.
     *
     * @param pString Pointer to string to delete. If NULL, function does nothing.
     */
    R_CSTL_API void r_cstl_string_delete (struct r_cstl_string* pString);

    /**
     * @brief Get the length of the string
     *
     * Returns the number of characters in the string (excluding null terminator).
     *
     * @param pString Pointer to string.
     * @return Length in bytes, or 0 if string is NULL or invalid.
     */
    R_CSTL_API size_t r_cstl_string_length (const struct r_cstl_string* pString);

    /**
     * @brief Get pointer to string data
     *
     * Returns a pointer to the null-terminated character buffer.
     *
     * @param pString Pointer to string.
     * @return Pointer to null-terminated string data, or "" if invalid.
     *
     * @warning The pointer becomes invalid if the string is modified or deleted.
     */
    R_CSTL_API const char* r_cstl_string_data (const struct r_cstl_string* pString);

    /**
     * @brief Get character at index
     *
     * Returns the character at the specified index.
     *
     * @param pString Pointer to string.
     * @param index Index of character to retrieve.
     * @return Character at index, or '\0' if index is out of bounds.
     *
     * @note In debug mode, returns '\0' for invalid index.
     */
    R_CSTL_API char r_cstl_string_char_at (const struct r_cstl_string* pString, const size_t index);

    /**
     * @brief Check if two strings are equal
     *
     * Compares two strings for equality.
     *
     * @param pLeft Pointer to first string.
     * @param pRight Pointer to second string.
     * @return 1 if equal, 0 if not equal or either is NULL.
     */
    R_CSTL_API int
    r_cstl_string_equals (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight);

    /**
     * @brief Compare two strings
     *
     * Compares two strings lexicographically.
     *
     * @param pLeft Pointer to first string.
     * @param pRight Pointer to second string.
     * @return -1 if left < right, 0 if equal, 1 if left > right.
     */
    R_CSTL_API int
    r_cstl_string_compare (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight);

    /**
     * @brief Concatenate two strings
     *
     * Creates a new string that is the concatenation of the two input strings.
     *
     * @param pLeft Pointer to first string.
     * @param pRight Pointer to second string.
     * @return Pointer to new concatenated string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string*
    r_cstl_string_concat (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight);

    /**
     * @brief Create a substring
     *
     * Returns a new string containing a subrange of the original string.
     * The range is [begin, end) - inclusive of begin, exclusive of end.
     *
     * @param pString Pointer to source string.
     * @param begin Starting index (inclusive).
     * @param end Ending index (exclusive).
     * @return Pointer to new substring, or NULL on invalid range.
     */
    R_CSTL_API struct r_cstl_string*
    r_cstl_string_substring (const struct r_cstl_string* pString, size_t begin, size_t end);

    /**
     * @brief Check if string starts with prefix
     *
     * Tests if the string begins with the specified prefix.
     *
     * @param pString Pointer to string.
     * @param pPrefix Null-terminated prefix to check.
     * @return 1 if starts with prefix, 0 otherwise or if either is NULL.
     */
    R_CSTL_API int r_cstl_string_starts_with (const struct r_cstl_string* pString, const char* pPrefix);

    /**
     * @brief Check if string ends with suffix
     *
     * Tests if the string ends with the specified suffix.
     *
     * @param pString Pointer to string.
     * @param pSuffix Null-terminated suffix to check.
     * @return 1 if ends with suffix, 0 otherwise or if either is NULL.
     */
    R_CSTL_API int r_cstl_string_ends_with (const struct r_cstl_string* pString, const char* pSuffix);

    /**
     * @brief Find first occurrence of substring
     *
     * Searches for the first occurrence of a substring within the string.
     *
     * @param pString Pointer to string.
     * @param pNeedle Null-terminated substring to search for.
     * @return Index of first occurrence, or (size_t)-1 if not found.
     */
    R_CSTL_API size_t r_cstl_string_index_of (const struct r_cstl_string* pString, const char* pNeedle);

    /**
     * @brief Find last occurrence of substring
     *
     * Searches for the last occurrence of a substring within the string.
     *
     * @param pString Pointer to string.
     * @param pNeedle Null-terminated substring to search for.
     * @return Index of last occurrence, or (size_t)-1 if not found.
     */
    R_CSTL_API size_t r_cstl_string_last_index_of (const struct r_cstl_string* pString, const char* pNeedle);

    /**
     * @brief Find first occurrence of character
     *
     * Searches for the first occurrence of a character within the string.
     *
     * @param pString Pointer to string.
     * @param ch Character to search for.
     * @return Index of first occurrence, or (size_t)-1 if not found.
     */
    R_CSTL_API size_t r_cstl_string_index_of_char (const struct r_cstl_string* pString, char ch);

    /**
     * @brief Find last occurrence of character
     *
     * Searches for the last occurrence of a character within the string.
     *
     * @param pString Pointer to string.
     * @param ch Character to search for.
     * @return Index of last occurrence, or (size_t)-1 if not found.
     */
    R_CSTL_API size_t r_cstl_string_last_index_of_char (const struct r_cstl_string* pString, char ch);

    /**
     * @brief Check if string contains substring
     *
     * Tests if the string contains the specified substring.
     *
     * @param pString Pointer to string.
     * @param pNeedle Null-terminated substring to search for.
     * @return 1 if contains substring, 0 otherwise or if either is NULL.
     */
    R_CSTL_API int r_cstl_string_contains (const struct r_cstl_string* pString, const char* pNeedle);

    /**
     * @brief Check if string is empty
     *
     * Tests if the string has zero length.
     *
     * @param pString Pointer to string.
     * @return 1 if empty, 0 if not empty, -1 if NULL or invalid.
     */
    R_CSTL_API int r_cstl_string_is_empty (const struct r_cstl_string* pString);

    /**
     * @brief Convert string to lowercase
     *
     * Creates a new string with all characters converted to lowercase.
     *
     * @param pString Pointer to string.
     * @return Pointer to new lowercase string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_to_lower_case (const struct r_cstl_string* pString);

    /**
     * @brief Convert string to uppercase
     *
     * Creates a new string with all characters converted to uppercase.
     *
     * @param pString Pointer to string.
     * @return Pointer to new uppercase string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_to_upper_case (const struct r_cstl_string* pString);

    /**
     * @brief Trim whitespace from string
     *
     * Creates a new string with leading and trailing whitespace removed.
     *
     * @param pString Pointer to string.
     * @return Pointer to new trimmed string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_trim (const struct r_cstl_string* pString);

    /**
     * @brief Replace all occurrences of substring
     *
     * Creates a new string with all occurrences of target replaced with replacement.
     *
     * @param pString Pointer to string.
     * @param pTarget Null-terminated substring to replace.
     * @param pReplacement Null-terminated replacement string.
     * @return Pointer to new string with replacements, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string*
    r_cstl_string_replace (const struct r_cstl_string* pString, const char* pTarget, const char* pReplacement);

    /**
     * @brief Replace all occurrences of character
     *
     * Creates a new string with all occurrences of oldChar replaced with newChar.
     *
     * @param pString Pointer to string.
     * @param oldChar Character to replace.
     * @param newChar Replacement character.
     * @return Pointer to new string with replacements, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string*
    r_cstl_string_replace_char (const struct r_cstl_string* pString, char oldChar, char newChar);

    /**
     * @brief Repeat string
     *
     * Creates a new string by repeating the input string count times.
     *
     * @param pString Pointer to string to repeat.
     * @param count Number of times to repeat.
     * @return Pointer to new repeated string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_repeat (struct r_cstl_string* pString, size_t count);

    /**
     * @brief Case-insensitive string equality
     *
     * Compares two strings for equality, ignoring case.
     *
     * @param pLeft Pointer to first string.
     * @param pRight Pointer to second string.
     * @return 1 if equal (case-insensitive), 0 if not equal or either is NULL.
     */
    R_CSTL_API int
    r_cstl_string_equals_ignore_case (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight);

    /**
     * @brief Case-insensitive string comparison
     *
     * Compares two strings lexicographically, ignoring case.
     *
     * @param pLeft Pointer to first string.
     * @param pRight Pointer to second string.
     * @return -1 if left < right, 0 if equal, 1 if left > right (case-insensitive).
     */
    R_CSTL_API int
    r_cstl_string_compare_ignore_case (const struct r_cstl_string* pLeft, const struct r_cstl_string* pRight);

    /**
     * @brief Get hash code of string
     *
     * Returns a hash code for the string, useful for hash tables.
     *
     * @param pString Pointer to string.
     * @return Hash code value.
     */
    R_CSTL_API size_t r_cstl_string_hash_code (const struct r_cstl_string* pString);

    /**
     * @brief Remove characters from string
     *
     * Creates a new string with characters in the range [start, end) removed.
     *
     * @param pString Pointer to string.
     * @param start Starting index (inclusive).
     * @param end Ending index (exclusive).
     * @return Pointer to new string with characters removed, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string*
    r_cstl_string_remove (const struct r_cstl_string* pString, size_t start, size_t end);

    /**
     * @brief Convert integer to string
     *
     * Creates a new string representation of an integer.
     *
     * @param value Integer value to convert.
     * @return Pointer to new string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_value_of_int (int value);

    /**
     * @brief Convert long to string
     *
     * Creates a new string representation of a long integer.
     *
     * @param value Long integer value to convert.
     * @return Pointer to new string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_value_of_long (long long value);

    /**
     * @brief Convert double to string
     *
     * Creates a new string representation of a double-precision floating-point value.
     *
     * @param value Double value to convert.
     * @return Pointer to new string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_value_of_double (double value);

    /**
     * @brief Join strings with delimiter
     *
     * Creates a new string by joining an array of strings with a delimiter.
     *
     * @param pStringDelimiter Delimiter string.
     * @param pStringElements Array of strings to join.
     * @param count Number of strings in the array.
     * @return Pointer to new joined string, or NULL on failure.
     */
    R_CSTL_API struct r_cstl_string* r_cstl_string_join (
        const struct r_cstl_string* pStringDelimiter,
        const struct r_cstl_string* pStringElements[],
        size_t                      count);

    /**
     * @brief Copy string
     *
     * Copies the contents of src string to dst string.
     *
     * @param pDst Destination string (must be valid).
     * @param pSrc Source string.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_copy (
        struct r_cstl_string* R_CSTL_RESTRICT       pDst,
        const struct r_cstl_string* R_CSTL_RESTRICT pSrc);

    /**
     * @brief Create a new string builder
     *
     * Creates a new string builder for efficient string construction.
     *
     * @return Pointer to new string builder, or NULL on allocation failure.
     */
    R_CSTL_API struct r_cstl_string_builder* r_cstl_new_string_builder (void);

    /**
     * @brief Create a string builder from initial data
     *
     * Creates a new string builder initialized with the provided string.
     *
     * @param pString Initial string data (null-terminated).
     * @return Pointer to new string builder, or NULL on allocation failure.
     */
    R_CSTL_API struct r_cstl_string_builder* r_cstl_new_string_builder_with_data (const char* pString);

    /**
     * @brief Create a string builder with capacity
     *
     * Creates a new string builder with pre-reserved capacity.
     *
     * @param capacity Minimum capacity to reserve.
     * @return Pointer to new string builder, or NULL on allocation failure.
     */
    R_CSTL_API struct r_cstl_string_builder* r_cstl_new_string_builder_with_capacity (size_t capacity);

    /**
     * @brief Delete a string builder
     *
     * Destroys the string builder and frees its resources.
     *
     * @param pBuilder Pointer to string builder to delete. If NULL, does nothing.
     */
    R_CSTL_API void r_cstl_delete_string_builder (struct r_cstl_string_builder* pBuilder);

    /**
     * @brief Get builder length
     *
     * Returns the current length of the string being built.
     *
     * @param pBuilder Pointer to string builder.
     * @return Length in bytes, or 0 if builder is NULL or invalid.
     */
    R_CSTL_API size_t r_cstl_string_builder_length (const struct r_cstl_string_builder* pBuilder);

    /**
     * @brief Get builder capacity
     *
     * Returns the current capacity of the string builder.
     *
     * @param pBuilder Pointer to string builder.
     * @return Capacity in bytes, or 0 if builder is NULL or invalid.
     */
    R_CSTL_API size_t r_cstl_string_builder_capacity (const struct r_cstl_string_builder* pBuilder);

    /**
     * @brief Ensure builder has minimum capacity
     *
     * Ensures the builder has at least the specified capacity, reallocating if needed.
     *
     * @param pBuilder Pointer to string builder.
     * @param requiredCapacity Minimum required capacity.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int
    r_cstl_string_builder_ensure_capacity (struct r_cstl_string_builder* pBuilder, const size_t requiredCapacity);

    /**
     * @brief Append string to builder
     *
     * Appends a string to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param pString Pointer to string to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int
    r_cstl_string_builder_append (struct r_cstl_string_builder* pBuilder, const struct r_cstl_string* pString);

    /**
     * @brief Append C string to builder
     *
     * Appends a null-terminated C string to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param pCString Null-terminated string to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_emplace (struct r_cstl_string_builder* pBuilder, const char* pCString);

    /**
     * @brief Append sized C string to builder
     *
     * Appends a C string with explicit length to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param pCString String data to append.
     * @param size Length of string data.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_emplace_sized (
        struct r_cstl_string_builder* pBuilder,
        const char*                  pCString,
        const size_t                 size);

    /**
     * @brief Append raw data to builder
     *
     * Appends raw byte data to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param pData Pointer to data to append.
     * @param length Number of bytes to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_append_data (
        struct r_cstl_string_builder* pBuilder,
        const char*                  pData,
        const size_t                 length);

    /**
     * @brief Append character to builder
     *
     * Appends a single character to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param value Character to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_append_char (struct r_cstl_string_builder* pBuilder, char value);

    /**
     * @brief Append integer to builder
     *
     * Appends a string representation of an integer to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param value Integer value to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_append_int (struct r_cstl_string_builder* pBuilder, int value);

    /**
     * @brief Append long to builder
     *
     * Appends a string representation of a long integer to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param value Long integer value to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_append_long (struct r_cstl_string_builder* pBuilder, long long value);

    /**
     * @brief Append double to builder
     *
     * Appends a string representation of a double to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param value Double value to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_append_double (struct r_cstl_string_builder* pBuilder, double value);

    /**
     * @brief Append boolean to builder
     *
     * Appends "true" or "false" to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param value Boolean value to append.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_append_bool (struct r_cstl_string_builder* pBuilder, bool value);

    /**
     * @brief Append repeated data to builder
     *
     * Appends the specified data repeated count times to the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param pData Pointer to data to repeat.
     * @param count Number of times to repeat.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int
    r_cstl_string_builder_append_repeat (struct r_cstl_string_builder* pBuilder, const char* pData, size_t count);

    /**
     * @brief Append formatted string to builder
     *
     * Appends a formatted string to the builder's contents using printf-style formatting.
     *
     * @param pBuilder Pointer to string builder.
     * @param pFormat Printf-style format string.
     * @param ... Variable arguments for formatting.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int
    r_cstl_string_builder_appendf (struct r_cstl_string_builder* pBuilder, const char* pFormat, ...);

    /**
     * @brief Insert string at offset
     *
     * Inserts a string at the specified offset in the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param offset Offset at which to insert.
     * @param pString Pointer to string to insert.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_insert (
        struct r_cstl_string_builder* pBuilder,
        size_t                       offset,
        const struct r_cstl_string*  pString);

    /**
     * @brief Insert C string at offset
     *
     * Inserts a null-terminated C string at the specified offset.
     *
     * @param pBuilder Pointer to string builder.
     * @param offset Offset at which to insert.
     * @param pCString Null-terminated string to insert.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_emplace_insert (
        struct r_cstl_string_builder* pBuilder,
        size_t                       offset,
        const char*                  pCString);

    /**
     * @brief Insert character at offset
     *
     * Inserts a character at the specified offset.
     *
     * @param pBuilder Pointer to string builder.
     * @param offset Offset at which to insert.
     * @param value Character to insert.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int
    r_cstl_string_builder_insert_char (struct r_cstl_string_builder* pBuilder, size_t offset, char value);

    /**
     * @brief Delete range from builder
     *
     * Removes characters in the range [start, end) from the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @param start Starting index (inclusive).
     * @param end Ending index (exclusive).
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int
    r_cstl_string_builder_delete (struct r_cstl_string_builder* pBuilder, size_t start, size_t end);

    /**
     * @brief Delete character at index
     *
     * Removes the character at the specified index.
     *
     * @param pBuilder Pointer to string builder.
     * @param index Index of character to delete.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_delete_char_at (struct r_cstl_string_builder* pBuilder, size_t index);

    /**
     * @brief Replace range with string
     *
     * Replaces characters in the range [start, end) with the specified string.
     *
     * @param pBuilder Pointer to string builder.
     * @param start Starting index (inclusive).
     * @param end Ending index (exclusive).
     * @param pString Pointer to replacement string.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_replace (
        struct r_cstl_string_builder* pBuilder,
        size_t                       start,
        size_t                       end,
        const struct r_cstl_string*  pString);

    /**
     * @brief Replace range with C string
     *
     * Replaces characters in the range [start, end) with the specified C string.
     *
     * @param pBuilder Pointer to string builder.
     * @param start Starting index (inclusive).
     * @param end Ending index (exclusive).
     * @param pString Null-terminated replacement string.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_emplace_replace (
        struct r_cstl_string_builder* pBuilder,
        size_t                       start,
        size_t                       end,
        const char*                  pString);

    /**
     * @brief Set character at index
     *
     * Sets the character at the specified index.
     *
     * @param pBuilder Pointer to string builder.
     * @param index Index of character to set.
     * @param value Character value to set.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int
    r_cstl_string_builder_set_char_at (struct r_cstl_string_builder* pBuilder, size_t index, char value);

    /**
     * @brief Set builder length
     *
     * Sets the length of the string being built, truncating or extending as needed.
     *
     * @param pBuilder Pointer to string builder.
     * @param newLength New length to set.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_set_length (struct r_cstl_string_builder* pBuilder, size_t newLength);

    /**
     * @brief Reverse builder contents
     *
     * Reverses the characters in the builder's contents.
     *
     * @param pBuilder Pointer to string builder.
     * @return R_CSTL_OK on success, error code on failure.
     */
    R_CSTL_API int r_cstl_string_builder_reverse (struct r_cstl_string_builder* pBuilder);

    /**
     * @brief Clear builder contents
     *
     * Clears the builder's contents, setting length to 0.
     *
     * @param pBuilder Pointer to string builder.
     */
    R_CSTL_API void r_cstl_string_builder_clear (struct r_cstl_string_builder* pBuilder);

    /**
     * @brief Convert builder to string
     *
     * Creates a new string from the builder's current contents.
     *
     * @param pBuilder Pointer to string builder.
     * @return Pointer to new string, or NULL on failure.
     *
     * @note The builder is not modified by this operation.
     */
    R_CSTL_API struct r_cstl_string*
    r_cstl_string_builder_to_string (const struct r_cstl_string_builder* pBuilder);

#ifdef __cplusplus
}
#endif
