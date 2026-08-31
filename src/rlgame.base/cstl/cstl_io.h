#pragma once

#include "rlgame.base/cstl/cstl_platform.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief File access mode flags
 *
 * Specifies how a file should be opened. Multiple flags can be combined
 * using bitwise OR (|).
 */
typedef enum R_CSTL_FileMode
{
    R_CSTL_FILE_MODE_READ = (1 << 0),
    R_CSTL_FILE_MODE_WRITE = (1 << 1),
    R_CSTL_FILE_MODE_APPEND = (1 << 2),
    R_CSTL_FILE_MODE_CREATE = (1 << 3),
    R_CSTL_FILE_MODE_TRUNCATE = (1 << 4),
    R_CSTL_FILE_MODE_BINARY = (1 << 5),
    R_CSTL_FILE_MODE_SHARE_READ = (1 << 6),
    R_CSTL_FILE_MODE_SHARE_WRITE = (1 << 7)
} R_CSTL_FileMode;

/**
 * @brief File seek origin
 *
 * Specifies the reference point for file positioning operations.
 */
typedef enum R_CSTL_SeekOrigin
{
    R_CSTL_SEEK_BEGIN = 0,
    R_CSTL_SEEK_CURRENT = 1,
    R_CSTL_SEEK_END = 2
} R_CSTL_SeekOrigin;

/**
 * @brief Opaque handle to a file
 *
 * The internal structure is opaque to maintain ABI stability and allow
 * implementation changes without breaking client code.
 */
struct R_CSTL_File;

/**
 * @brief Open a file
 *
 * Opens a file with the specified mode. The mode flags can be combined.
 *
 * @param pPath Null-terminated path to the file. Can be UTF-8 on Windows.
 * @param mode Bitwise combination of R_CSTL_FileMode flags.
 * @return Pointer to new file handle, or NULL on failure.
 *
 * @note The file must be closed with R_CSTL_FileClose when no longer needed.
 * @note On Windows, pPath is treated as UTF-8 and converted to wide string.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API struct R_CSTL_File* R_CSTL_FileOpen (const char* pPath, int mode);

/**
 * @brief Close a file
 *
 * Closes the file and releases all associated resources.
 *
 * @param pFile Pointer to file handle. If NULL, function does nothing.
 *
 * @note After this call, the pointer becomes invalid and must not be used.
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API void R_CSTL_FileClose (struct R_CSTL_File* pFile);

/**
 * @brief Read data from a file
 *
 * Reads up to the specified number of bytes from the file into the buffer.
 *
 * @param pFile Pointer to file handle.
 * @param pBuffer Pointer to buffer to receive data.
 * @param size Number of bytes to read.
 * @param pBytesRead Pointer to receive actual number of bytes read (can be NULL).
 * @return R_CSTL_OK on success, error code on failure or EOF.
 *
 * @note Returns R_CSTL_OK even on EOF if some data was read.
 * @note Returns error if no bytes read and at EOF.
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API int R_CSTL_FileRead (struct R_CSTL_File* pFile, void* pBuffer, size_t size, size_t* pBytesRead);

/**
 * @brief Write data to a file
 *
 * Writes the specified number of bytes from the buffer to the file.
 *
 * @param pFile Pointer to file handle.
 * @param pBuffer Pointer to data to write.
 * @param size Number of bytes to write.
 * @param pBytesWritten Pointer to receive actual number of bytes written (can be NULL).
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Returns error if not all bytes could be written.
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API int
R_CSTL_FileWrite (struct R_CSTL_File* pFile, const void* pBuffer, size_t size, size_t* pBytesWritten);

/**
 * @brief Get current file position
 *
 * Returns the current read/write position in the file.
 *
 * @param pFile Pointer to file handle.
 * @param pPosition Pointer to receive current position.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API int R_CSTL_FileTell (struct R_CSTL_File* pFile, int64_t* pPosition);

/**
 * @brief Set file position
 *
 * Sets the read/write position in the file relative to the specified origin.
 *
 * @param pFile Pointer to file handle.
 * @param offset Offset in bytes from the origin.
 * @param origin Reference point (R_CSTL_SeekOrigin).
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API int R_CSTL_FileSeek (struct R_CSTL_File* pFile, int64_t offset, R_CSTL_SeekOrigin origin);

/**
 * @brief Get file size
 *
 * Returns the size of the file in bytes.
 *
 * @param pFile Pointer to file handle.
 * @param pSize Pointer to receive file size.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API int R_CSTL_FileSize (struct R_CSTL_File* pFile, int64_t* pSize);

/**
 * @brief Flush file buffers
 *
 * Forces any buffered data to be written to the underlying storage.
 *
 * @param pFile Pointer to file handle.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API int R_CSTL_FileFlush (struct R_CSTL_File* pFile);

/**
 * @brief Check if file is at end
 *
 * Tests if the file position is at end-of-file.
 *
 * @param pFile Pointer to file handle.
 * @return 1 if at EOF, 0 if not at EOF, -1 on error.
 *
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
R_CSTL_API int R_CSTL_FileIsEOF (struct R_CSTL_File* pFile);

/**
 * @brief Check if file handle is valid
 *
 * Tests if the file handle is valid and open.
 *
 * @param pFile Pointer to file handle.
 * @return 1 if valid, 0 if invalid or NULL.
 *
 * @note Thread-safe: reads only, no synchronization needed.
 */
R_CSTL_API int R_CSTL_FileIsValid (const struct R_CSTL_File* pFile);

/**
 * @brief Read entire file into a string
 *
 * Convenience function that opens a file, reads all contents, and closes it.
 *
 * @param pPath Null-terminated path to the file.
 * @param pOutString Pointer to receive the string handle.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note The returned string must be freed with R_CSTL_StringDelete.
 * @note Reads file as binary; no newline translation is performed.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_FileReadAllText (const char* pPath, struct R_CSTL_String** pOutString);

/**
 * @brief Write string to file
 *
 * Convenience function that opens a file, writes the string, and closes it.
 *
 * @param pPath Null-terminated path to the file.
 * @param pString Pointer to string to write.
 * @param mode File mode flags (must include WRITE). Defaults to CREATE | TRUNCATE.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_FileWriteAllText (const char* pPath, const struct R_CSTL_String* pString, int mode);

/**
 * @brief Read file in chunks with callback
 *
 * Reads a file in chunks and calls a callback for each chunk. Useful for
 * processing large files without loading them entirely into memory.
 *
 * @param pFile Pointer to file handle.
 * @param pCallback Callback function called for each chunk.
 * @param pUserData User data passed to callback.
 * @param chunkSize Size of each chunk to read (0 for default 64KB).
 * @return R_CSTL_OK on success, error code on failure or if callback returns non-zero.
 *
 * @note The callback receives (buffer, bytesRead, pUserData).
 * @note If callback returns non-zero, reading stops and that value is returned.
 * @note Thread-safe: can be called concurrently, but not on the same file handle.
 */
typedef int (*R_CSTL_FileReadChunkCallback) (const void* pBuffer, size_t bytesRead, void* pUserData);

R_CSTL_API int R_CSTL_FileReadChunks (
    struct R_CSTL_File*          pFile,
    R_CSTL_FileReadChunkCallback pCallback,
    void*                        pUserData,
    size_t                       chunkSize);

/**
 * @brief Check if a file exists
 *
 * Tests if a file exists at the given path.
 *
 * @param pPath Null-terminated path to check.
 * @return 1 if file exists, 0 if not, -1 on error.
 *
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_FileExists (const char* pPath);

/**
 * @brief Delete a file
 *
 * Deletes the file at the given path.
 *
 * @param pPath Null-terminated path to the file to delete.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_FileDelete (const char* pPath);

/**
 * @brief Rename a file
 *
 * Renames or moves a file from oldPath to newPath.
 *
 * @param pOldPath Null-terminated current path.
 * @param pNewPath Null-terminated new path.
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note On Windows, fails if newPath already exists unless MOVEFILE_REPLACE_EXISTING is used.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_FileRename (const char* pOldPath, const char* pNewPath);

/**
 * @brief Get file modification time
 *
 * Returns the last modification time of the file.
 *
 * @param pPath Null-terminated path to the file.
 * @param pOutTime Pointer to receive modification time (Unix timestamp in seconds).
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_FileGetModTime (const char* pPath, int64_t* pOutTime);

/**
 * @brief Get file creation time
 *
 * Returns the creation time of the file.
 *
 * @param pPath Null-terminated path to the file.
 * @param pOutTime Pointer to receive creation time (Unix timestamp in seconds).
 * @return R_CSTL_OK on success, error code on failure.
 *
 * @note On Linux, this may return the last metadata change time if creation time is unavailable.
 * @note Thread-safe: can be called concurrently from multiple threads.
 */
R_CSTL_API int R_CSTL_FileGetCreationTime (const char* pPath, int64_t* pOutTime);

/**
 * @brief Open standard input
 *
 * Returns a file handle for standard input.
 *
 * @return Pointer to file handle for stdin, or NULL on failure.
 *
 * @note The returned handle should not be closed with R_CSTL_FileClose.
 * @note Thread-safe: returns shared handle.
 */
R_CSTL_API struct R_CSTL_File* R_CSTL_FileStdin (void);

/**
 * @brief Open standard output
 *
 * Returns a file handle for standard output.
 *
 * @return Pointer to file handle for stdout, or NULL on failure.
 *
 * @note The returned handle should not be closed with R_CSTL_FileClose.
 * @note Thread-safe: returns shared handle.
 */
R_CSTL_API struct R_CSTL_File* R_CSTL_FileStdout (void);

/**
 * @brief Open standard error
 *
 * Returns a file handle for standard error.
 *
 * @return Pointer to file handle for stderr, or NULL on failure.
 *
 * @note The returned handle should not be closed with R_CSTL_FileClose.
 * @note Thread-safe: returns shared handle.
 */
R_CSTL_API struct R_CSTL_File* R_CSTL_FileStderr (void);
