#ifndef RL_CHUNK_CHUNK_BUFFER_H
#define RL_CHUNK_CHUNK_BUFFER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <vector>

namespace rl
{

/** Buffer for storing chunk data with GPU-friendly memory layout
 *
 * This class provides a 3D buffer for storing chunk data with aligned memory
 * suitable for GPU operations. It supports efficient data transfer between CPU and GPU,
 * bounds-checked access, and various data manipulation operations.
 *
 * @tparam IdType The type of data stored in the buffer (e.g., uint32_t for block IDs)
 */
template <typename IdType> class ChunkBuffer
{
        public:
                /** Forward declaration of the Heap structure */
                struct Heap;

        private:
                Heap heap;
        public:
                /** Constructs a ChunkBuffer with specified dimensions
                 * @tparam W Width of the chunk (x-axis)
                 * @tparam H Height of the chunk (y-axis)
                 * @tparam D Depth of the chunk (z-axis)
                 */
                template <size_t W, size_t H, size_t D> 
                ChunkBuffer() : heap(W, H, D)
                {
                        
                }

                /** Default destructor */
                ~ChunkBuffer()                             = default;
                ChunkBuffer(const ChunkBuffer&)            = delete;
                ChunkBuffer& operator=(const ChunkBuffer&) = delete;

                /** Move constructor
                 * @param other The ChunkBuffer to move from */
                ChunkBuffer(ChunkBuffer&& other) noexcept : heap(std::move(other.heap))
                {
                }

                /** Move assignment operator
                 * @param other The ChunkBuffer to move from
                 * @return Reference to this ChunkBuffer */
                ChunkBuffer& operator=(ChunkBuffer&& other) noexcept
                {
                        if (this != &other)
                        {
                                heap = std::move(other.heap);
                        }
                        return *this;
                }

                /** Returns the underlying heap structure
                 * @return Reference to the heap */
                Heap& getHeap() noexcept
                {
                        return heap;
                }
                /** Returns the underlying heap structure (const overload)
                 * @return Const reference to the heap */
                const Heap& getHeap() const noexcept
                {
                        return heap;
                }

                /** Fetches data at 3D coordinates
                 * @param x X coordinate
                 * @param y Y coordinate
                 * @param z Z coordinate
                 * @return The data at the specified coordinates */
                IdType fetchXYZ(size_t x, size_t y, size_t z) const noexcept
                {
                        return heap.fetchXYZ(x, y, z);
                }

                /** Fetches data at linear index with bounds checking
                 * @param index Linear index into the buffer
                 * @return Optional containing the data if index is valid, nullopt
                 * otherwise */
                std::optional<IdType> fetchIndex(size_t index) const noexcept
                {
                        return heap.fetchIndex(index);
                }

                /** Fetches a block of data starting at an index
                 * @param index Starting linear index
                 * @param count Number of elements to fetch
                 * @return Vector containing the fetched data */
                std::vector<IdType> fetchBlock(size_t index, size_t count) noexcept
                {
                        return heap.fetchBlock(index, count);
                }

                /** Sets data at 3D coordinates
                 * @param x X coordinate
                 * @param y Y coordinate
                 * @param z Z coordinate
                 * @param value The value to set */
                void setXYZ(size_t x, size_t y, size_t z, IdType value) noexcept
                {
                        heap.setXYZ(x, y, z, value);
                }

                /** Checks if 3D coordinates are within bounds
                 * @param x X coordinate
                 * @param y Y coordinate
                 * @param z Z coordinate
                 * @return true if coordinates are valid, false otherwise */
                bool isValidXYZ(size_t x, size_t y, size_t z) const noexcept
                {
                        return heap.isValidXYZ(x, y, z);
                }

                /** Checks if a linear index is within bounds
                 * @param index Linear index to check
                 * @return true if index is valid, false otherwise */
                bool isValidIndex(size_t index) const noexcept
                {
                        return heap.isValidIndex(index);
                }

                /** Returns the raw pointer to the buffer data (GPU-friendly)
                 * @return Pointer to the buffer data */
                IdType* getRaw() noexcept
                {
                        return heap.begin();
                }
                /** Returns the raw pointer to the buffer data (const overload)
                 * @return Const pointer to the buffer data */
                const IdType* getRaw() const noexcept
                {
                        return heap.begin();
                }

                /** Returns the size of the buffer in bytes (GPU allocation)
                 * @return Size in bytes */
                size_t getSizeInBytes() const noexcept
                {
                        return heap.getSizeInBytes();
                }

                /** Returns the number of elements in the buffer
                 * @return Number of elements */
                size_t getElementCount() const noexcept
                {
                        return heap.getElementCount();
                }

                /** Returns the aligned dimensions of the buffer
                 * @return Tuple of (width, height, depth) */
                std::tuple<size_t, size_t, size_t> getDimensions() const noexcept
                {
                        return heap.getDimensions();
                }

                /** Fills the entire buffer with a specified value
                 * @param value The value to fill with */
                void fill(IdType value) noexcept
                {
                        heap.fill(value);
                }

                /** Clears the buffer by setting all elements to zero
                 */
                void clear() noexcept
                {
                        heap.fill(static_cast<IdType>(0));
                }

                /** Copies data from another buffer
                 * @param source Pointer to source data
                 * @param count Number of elements to copy */
                void copyFrom(const IdType* source, size_t count) noexcept
                {
                        heap.copyFrom(source, count);
                }

                /** Copies data to an external buffer
                 * @param destination Pointer to destination buffer
                 * @param count Number of elements to copy */
                void copyTo(IdType* destination, size_t count) const noexcept
                {
                        heap.copyTo(destination, count);
                }

                /** Returns iterator to the beginning of the buffer
                 * @return Iterator to begin */
                IdType* begin() noexcept
                {
                        return heap.begin();
                }
                /** Returns iterator to the beginning of the buffer (const overload)
                 * @return Const iterator to begin */
                const IdType* begin() const noexcept
                {
                        return heap.begin();
                }
                /** Returns iterator to the end of the buffer
                 * @return Iterator to end */
                IdType* end() noexcept
                {
                        return heap.end();
                }
                /** Returns iterator to the end of the buffer (const overload)
                 * @return Const iterator to end */
                const IdType* end() const noexcept
                {
                        return heap.end();
                }
};

/** internal heap structure managing the aligned buffer memory
 *
 * This structure handles the actual memory allocation with 16-byte alignment
 * for optimal GPU transfer performance. It provides low-level data access
 * and manipulation methods.
 */
template <typename IdType> struct ChunkBuffer<IdType>::Heap
{
                IdType*     pHeapBase; /**< Pointer to the start of the buffer */
                IdType*     pHeapEnd; /**< Pointer to the end of the buffer */
                size_t wSize; /**< Aligned width (x-axis dimension) */
                size_t hSize; /**< Aligned height (y-axis dimension) */
                size_t dSize; /**< Aligned depth (z-axis dimension) */

                /** Constructs a heap with specified dimensions (aligned to 16-byte
                 * boundaries)
                 * @param wSize Requested width
                 * @param hSize Requested height
                 * @param dSize Requested depth
                 */
                Heap(size_t wSize, size_t hSize, size_t dSize) noexcept
                {
                        this->wSize     = (wSize + 15) & ~15; // Align to 16
                        this->hSize     = (hSize + 15) & ~15;
                        this->dSize     = (dSize + 15) & ~15;
                        size_t aligned  = wSize * hSize * dSize;
                        this->pHeapBase = new IdType[aligned];
                        this->pHeapEnd  = pHeapBase + aligned;
                }

                /** Default constructor for empty heap */
                Heap() noexcept : pHeapBase(nullptr), pHeapEnd(nullptr), wSize(0), hSize(0), dSize(0)
                {
                }

                /** Move constructor
                 * @param other The heap to move from */
                Heap(Heap&& other) noexcept :
                    pHeapBase(other.pHeapBase), pHeapEnd(other.pHeapEnd), wSize(other.wSize), hSize(other.hSize),
                    dSize(other.dSize)
                {
                        other.pHeapBase = nullptr;
                        other.pHeapEnd  = nullptr;
                        other.wSize = other.hSize = other.dSize = 0;
                }

                /** Move assignment operator
                 * @param other The heap to move from
                 * @return Reference to this heap */
                Heap& operator=(Heap&& other) noexcept
                {
                        if (this != &other)
                        {
                                delete[] pHeapBase;
                                pHeapBase       = other.pHeapBase;
                                pHeapEnd        = other.pHeapEnd;
                                wSize               = other.wSize;
                                hSize               = other.hSize;
                                dSize               = other.dSize;
                                other.pHeapBase = nullptr;
                                other.pHeapEnd  = nullptr;
                                other.wSize = other.hSize = other.dSize = 0;
                        }
                        return *this;
                }

                /** Deleted copy constructor (Heap manages unique resources) */
                Heap(const Heap& other) = delete;
                /** Deleted copy assignment (Heap manages unique resources) */
                Heap& operator=(const Heap& other) = delete;

                /** Destructor - releases allocated memory */
                ~Heap()
                {
                        delete[] pHeapBase;
                        pHeapBase = pHeapEnd = nullptr;
                }

                /** Returns iterator to the beginning of the buffer
                 * @return Pointer to the start of the buffer */
                IdType* begin() const noexcept
                {
                        return pHeapBase;
                }

                /** Returns iterator to the end of the buffer
                 * @return Pointer to the end of the buffer */
                IdType* end() const noexcept
                {
                        return pHeapEnd;
                }

                /** Fetches data at 3D coordinates
                 * @param x X coordinate (0 to w-1)
                 * @param y Y coordinate (0 to h-1)
                 * @param z Z coordinate (0 to d-1)
                 * @return The data at the specified coordinates */
                IdType fetchXYZ(size_t x, size_t y, size_t z) const noexcept;

                /** Fetches data at linear index with bounds checking
                 * @param index Linear index into the buffer
                 * @return Optional containing the data if index is valid, nullopt
                 * otherwise */
                std::optional<IdType> fetchIndex(size_t index) const noexcept;

                /** Fetches a block of data starting at an index
                 * @param index Starting linear index
                 * @param count Number of elements to fetch
                 * @return Vector containing the fetched data (may be smaller if near end)
                 */
                std::vector<IdType> fetchBlock(size_t index, size_t count) noexcept;

                /** Sets data at 3D coordinates
                 * @param x X coordinate (0 to w-1)
                 * @param y Y coordinate (0 to h-1)
                 * @param z Z coordinate (0 to d-1)
                 * @param value The value to set */
                void setXYZ(size_t x, size_t y, size_t z, IdType value) noexcept;

                /** Checks if 3D coordinates are within bounds
                 * @param x X coordinate
                 * @param y Y coordinate
                 * @param z Z coordinate
                 * @return true if coordinates are valid, false otherwise */
                bool isValidXYZ(size_t x, size_t y, size_t z) const noexcept;

                /** Checks if a linear index is within bounds
                 * @param index Linear index to check
                 * @return true if index is valid, false otherwise */
                bool isValidIndex(size_t index) const noexcept;

                /** Returns the size of the buffer in bytes
                 * @return Size in bytes (useful for GPU buffer allocation) */
                size_t getSizeInBytes() const noexcept;

                /** Returns the number of elements in the buffer
                 * @return Number of elements */
                size_t getElementCount() const noexcept;

                /** Returns the aligned dimensions of the buffer
                 * @return Tuple of (width, height, depth) */
                std::tuple<size_t, size_t, size_t> getDimensions() const noexcept;

                /** Fills the entire buffer with a specified value
                 * @param value The value to fill with */
                void fill(IdType value) noexcept;

                /** Copies data from an external buffer
                 * @param source Pointer to source data
                 * @param count Number of elements to copy */
                void copyFrom(const IdType* source, size_t count) noexcept;

                /** Copies data to an external buffer
                 * @param destination Pointer to destination buffer
                 * @param count Number of elements to copy */
                void copyTo(IdType* destination, size_t count) const noexcept;
};

} // namespace rl

#endif
