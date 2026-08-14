#ifndef RL_WORLD_PRE_REGISTRY_H
#define RL_WORLD_PRE_REGISTRY_H

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace rl
{

constexpr size_t bucketCapacity = 4; /**< Minimum entries per bucket */
constexpr size_t initialBuckets = 64; /**< Initial number of buckets */

/** Computes bucket index from hash using bitwise operations
 * @param hash The hash value
 * @param bucketCount Number of buckets
 * @return Bucket index */
inline size_t computeBucketIndex(size_t hash, size_t bucketCount)
{
  return hash & (bucketCount - 1); // Power of two assumption for bitwise
}

/** Cache-friendly hash table entry storing key-value pair and next index */
template <typename KeyType, typename ValueType> struct HashTableEntry
{
    KeyType   key{}; /**< The bucket key */
    ValueType value{}; /**< The bucket value */
    size_t    nextIndex; /**< Index of next entry in chain (SIZE_MAX if none) */
    size_t    hash; /**< Cached hash value for bucket computation */

    /** Constructs an empty entry */
    HashTableEntry() : nextIndex(SIZE_MAX), hash(0)
    {
    }
};

/** Bucket storing contiguous array of entry indices for cache efficiency */
struct HashBucket
{
    size_t indices[bucketCapacity]; /**< Array of entry indices */
    size_t count; /**< Number of entries in this bucket */

    /** Constructs an empty bucket */
    HashBucket() : count(0)
    {
      for (size_t i = 0; i < bucketCapacity; ++i)
      {
        indices[i] = SIZE_MAX;
      }
    }
};

/** Cache-friendly hash table using contiguous storage and buckets */
template <typename KeyType, typename ValueType, typename HashFunc> class PreHashTable
{
  public:
    /** Constructs an empty hash table */
    PreHashTable() : entryCount(0), bucketCount(initialBuckets), hasher()
    {
      buckets.resize(initialBuckets);
      entries.reserve(initialBuckets * bucketCapacity);
    }
    /** Destroys the hash table */
    ~PreHashTable() = default;

    /** Inserts or updates a key-value pair
     * @param key The bucket key
     * @param value The bucket value
     * @return Reference to the inserted/updated value */
    ValueType& insert(const KeyType& key, const ValueType& value);
    /** Finds a value by key
     * @param key The bucket key to find
     * @return Pointer to the value, or nullptr if not found */
    ValueType* find(const KeyType& key);
    /** Finds a value by key (const version)
     * @param key The bucket key to find
     * @return Pointer to the value, or nullptr if not found */
    const ValueType* find(const KeyType& key) const;
    /** Returns the number of entries in the table
     * @return Entry count */
    size_t size() const
    {
      return entryCount;
    }
    /** Clears all entries */
    void clear();
    /** Returns all keys in insertion order
     * @return Vector of keys */
    std::vector<KeyType> getAllKeys() const;

  private:
    /** Resizes the hash table when load factor is exceeded
     * @param newBucketCount New number of buckets */
    void resize(size_t newBucketCount);
    /** Finds an entry index by key
     * @param key The bucket key to find
     * @return Entry index, or SIZE_MAX if not found */
    size_t findEntryIndex(const KeyType& key) const;

    std::vector<HashTableEntry<KeyType, ValueType>> entries; /**< Contiguous array of entries */
    std::vector<HashBucket>                         buckets; /**< Array of buckets */
    size_t                                          entryCount; /**< Total number of entries */
    size_t   bucketCount; /**< Number of buckets (power of two) */
    HashFunc hasher; /**< Hash function */
};

/** Template base registry for managing and querying instances grouped by conditions
 * @tparam T The type of items to register
 * @tparam BucketType The bucket type containing items
 * @tparam KeyType The key type for bucketing
 * @tparam HashFunc The hash function for keys */
template <typename T, typename BucketType, typename KeyType, typename HashFunc> class PreRegistry
{
  public:
    /** Constructs an empty PreRegistry */
    PreRegistry() = default;
    /** Destroys the registry */
    virtual ~PreRegistry() = default;

    /** Registers an item into the appropriate bucket based on its properties
     * @param item The item to register */
    virtual void registerItem(T& item) = 0;

    /** Returns the items in the bucket matching the given key
     * @param key The bucket key to look up
     * @return Raw pointers to items in the bucket */
    virtual const std::vector<T*>& getBucket(const KeyType& key) const = 0;
    /** Returns all items from all buckets in sorted order
     * @return Raw pointers to all sorted items */
    virtual std::vector<T*> getItems() const = 0;
    /** Returns the count of items in the registry
     * @return The count of items in the registry */
    virtual size_t getCount() const = 0;

    /** Returns the number of buckets in the registry
     * @return Number of buckets */
    size_t getBucketCount() const
    {
      std::scoped_lock guard(registryMutex);
      return bucketOrder.size();
    }

    /** Clears all buckets from the registry */
    void clearBuckets()
    {
      std::scoped_lock guard(registryMutex);
      clearHashBuckets();
    }

  protected:
    mutable std::mutex registryMutex; /**< Mutex for thread-safe registry operations */
    PreHashTable<KeyType, BucketType, HashFunc> hashTable; /**< Cache-friendly hash table */
    std::vector<KeyType>                        bucketOrder; /**< Order of bucket insertion */

    /** internal clear buckets without lock (call when lock is already held) */
    void clearHashBuckets()
    {
      hashTable.clear();
      bucketOrder.clear();
    }
};

// Template method implementations

template <typename KeyType, typename ValueType, typename HashFunc>
size_t PreHashTable<KeyType, ValueType, HashFunc>::findEntryIndex(const KeyType& key) const
{
  size_t hash      = hasher(key);
  size_t bucketIdx = computeBucketIndex(hash, bucketCount);

  const HashBucket& bucket = buckets[bucketIdx];
  for (size_t i = 0; i < bucket.count; ++i)
  {
    size_t entryIdx = bucket.indices[i];
    if (entryIdx < entries.size() && entries[entryIdx].key == key)
    {
      return entryIdx;
    }
  }
  return SIZE_MAX;
}

template <typename KeyType, typename ValueType, typename HashFunc>
ValueType* PreHashTable<KeyType, ValueType, HashFunc>::find(const KeyType& key)
{
  size_t entryIdx = findEntryIndex(key);
  if (entryIdx == SIZE_MAX)
    return nullptr;
  return &entries[entryIdx].value;
}

template <typename KeyType, typename ValueType, typename HashFunc>
const ValueType* PreHashTable<KeyType, ValueType, HashFunc>::find(const KeyType& key) const
{
  size_t entryIdx = findEntryIndex(key);
  if (entryIdx == SIZE_MAX)
    return nullptr;
  return &entries[entryIdx].value;
}

template <typename KeyType, typename ValueType, typename HashFunc>
void PreHashTable<KeyType, ValueType, HashFunc>::resize(size_t newBucketCount)
{
  std::vector<HashBucket> newBuckets(newBucketCount);
  for (size_t i = 0; i < newBucketCount; ++i)
  {
    newBuckets[i].count = 0;
    for (size_t j = 0; j < bucketCapacity; ++j)
    {
      newBuckets[i].indices[j] = SIZE_MAX;
    }
  }

  for (size_t i = 0; i < entries.size(); ++i)
  {
    HashTableEntry<KeyType, ValueType>& entry = entries[i];
    size_t      newBucketIdx                  = computeBucketIndex(entry.hash, newBucketCount);
    HashBucket& newBucket                     = newBuckets[newBucketIdx];

    if (newBucket.count < bucketCapacity)
    {
      newBucket.indices[newBucket.count++] = i;
    }
    else
    {
      size_t chainIdx = i;
      while (entries[chainIdx].nextIndex != SIZE_MAX)
      {
        chainIdx = entries[chainIdx].nextIndex;
      }
      entries[chainIdx].nextIndex                 = entries.size();
      HashTableEntry<KeyType, ValueType> newEntry = entry;
      newEntry.nextIndex                          = SIZE_MAX;
      size_t newEntryIdx                          = entries.size();
      entries.push_back(newEntry);

      for (size_t retry = 0; retry < newBucketCount; ++retry)
      {
        size_t probeBucketIdx = (newBucketIdx + retry) & (newBucketCount - 1);
        if (newBuckets[probeBucketIdx].count < bucketCapacity)
        {
          newBuckets[probeBucketIdx].indices[newBuckets[probeBucketIdx].count++] = newEntryIdx;
          break;
        }
      }
    }
  }

  buckets     = std::move(newBuckets);
  bucketCount = newBucketCount;
}

template <typename KeyType, typename ValueType, typename HashFunc>
ValueType& PreHashTable<KeyType, ValueType, HashFunc>::insert(const KeyType&   key,
                                                              const ValueType& value)
{
  size_t existingIdx = findEntryIndex(key);
  if (existingIdx != SIZE_MAX)
  {
    entries[existingIdx].value = value;
    return entries[existingIdx].value;
  }

  if (entryCount >= bucketCount * bucketCapacity * 3 / 4)
  {
    resize(bucketCount * 2);
  }

  size_t      hash      = hasher(key);
  size_t      bucketIdx = computeBucketIndex(hash, bucketCount);
  HashBucket& bucket    = buckets[bucketIdx];

  HashTableEntry<KeyType, ValueType> newEntry;
  newEntry.key       = key;
  newEntry.value     = value;
  newEntry.nextIndex = SIZE_MAX;
  newEntry.hash      = hash;

  size_t entryIdx = entries.size();
  entries.push_back(newEntry);
  entryCount++;

  if (bucket.count < bucketCapacity)
  {
    bucket.indices[bucket.count++] = entryIdx;
  }
  else
  {
    for (size_t i = 0; i < bucket.count; ++i)
    {
      size_t chainIdx = bucket.indices[i];
      while (entries[chainIdx].nextIndex != SIZE_MAX)
      {
        chainIdx = entries[chainIdx].nextIndex;
      }
      entries[chainIdx].nextIndex = entryIdx;
    }
  }

  return entries[entryIdx].value;
}

template <typename KeyType, typename ValueType, typename HashFunc>
void PreHashTable<KeyType, ValueType, HashFunc>::clear()
{
  entries.clear();
  for (auto& bucket : buckets)
  {
    bucket.count = 0;
    for (size_t i = 0; i < bucketCapacity; ++i)
    {
      bucket.indices[i] = SIZE_MAX;
    }
  }
  entryCount = 0;
}

template <typename KeyType, typename ValueType, typename HashFunc>
std::vector<KeyType> PreHashTable<KeyType, ValueType, HashFunc>::getAllKeys() const
{
  std::vector<KeyType> keys;
  keys.reserve(entryCount);
  for (const auto& entry : entries)
  {
    keys.emplace_back(entry.key);
  }
  return keys;
}

} // namespace rl

#endif
