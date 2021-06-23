#ifndef svtkExodusIICache_h
#define svtkExodusIICache_h

// ============================================================================
// The following classes define an LRU cache for data arrays
// loaded by the Exodus reader. Here's how they work:
//
// The actual cache consists of two STL containers: a set of
// cache entries (svtkExodusIICacheEntry) and a list of
// cache references (svtkExodusIICacheRef). The entries in
// these containers are sorted for fast retrieval:
// 1. The cache entries are indexed by the timestep, the
//    object type (edge block, face set, ...), and the
//    object ID (if one exists). When you call Find() to
//    retrieve a cache entry, you provide a key containing
//    this information and the array is returned if it exists.
// 2. The list of cache references are stored in "least-recently-used"
//    order. The least recently referenced array is the first in
//    the list. Whenever you request an entry with Find(), it is
//    moved to the back of the list if it exists.
// This makes retrieving arrays O(n log n) and popping LRU
// entries O(1). Each cache entry stores an iterator into
// the list of references so that it can be located quickly for
// removal.

#include "svtkIOExodusModule.h" // For export macro
#include "svtkObject.h"

#include <list> // use for LRU ordering
#include <map>  // used for cache storage

class SVTKIOEXODUS_EXPORT svtkExodusIICacheKey
{
public:
  int Time;
  int ObjectType;
  int ObjectId;
  int ArrayId;
  svtkExodusIICacheKey()
  {
    Time = -1;
    ObjectType = -1;
    ObjectId = -1;
    ArrayId = -1;
  }
  svtkExodusIICacheKey(int time, int objType, int objId, int arrId)
  {
    Time = time;
    ObjectType = objType;
    ObjectId = objId;
    ArrayId = arrId;
  }
  svtkExodusIICacheKey(const svtkExodusIICacheKey& src)
  {
    Time = src.Time;
    ObjectType = src.ObjectType;
    ObjectId = src.ObjectId;
    ArrayId = src.ArrayId;
  }
  svtkExodusIICacheKey& operator=(const svtkExodusIICacheKey& src)
  {
    Time = src.Time;
    ObjectType = src.ObjectType;
    ObjectId = src.ObjectId;
    ArrayId = src.ArrayId;
    return *this;
  }
  bool match(const svtkExodusIICacheKey& other, const svtkExodusIICacheKey& pattern) const
  {
    if (pattern.Time && this->Time != other.Time)
      return false;
    if (pattern.ObjectType && this->ObjectType != other.ObjectType)
      return false;
    if (pattern.ObjectId && this->ObjectId != other.ObjectId)
      return false;
    if (pattern.ArrayId && this->ArrayId != other.ArrayId)
      return false;
    return true;
  }
  bool operator<(const svtkExodusIICacheKey& other) const
  {
    if (this->Time < other.Time)
      return true;
    else if (this->Time > other.Time)
      return false;
    if (this->ObjectType < other.ObjectType)
      return true;
    else if (this->ObjectType > other.ObjectType)
      return false;
    if (this->ObjectId < other.ObjectId)
      return true;
    else if (this->ObjectId > other.ObjectId)
      return false;
    if (this->ArrayId < other.ArrayId)
      return true;
    return false;
  }
};

class svtkExodusIICacheEntry;
class svtkExodusIICache;
class svtkDataArray;

typedef std::map<svtkExodusIICacheKey, svtkExodusIICacheEntry*> svtkExodusIICacheSet;
typedef std::map<svtkExodusIICacheKey, svtkExodusIICacheEntry*>::iterator svtkExodusIICacheRef;
typedef std::list<svtkExodusIICacheRef> svtkExodusIICacheLRU;
typedef std::list<svtkExodusIICacheRef>::iterator svtkExodusIICacheLRURef;

class SVTKIOEXODUS_EXPORT svtkExodusIICacheEntry
{
public:
  svtkExodusIICacheEntry();
  svtkExodusIICacheEntry(svtkDataArray* arr);
  svtkExodusIICacheEntry(const svtkExodusIICacheEntry& other);

  ~svtkExodusIICacheEntry();

  svtkDataArray* GetValue() { return this->Value; }

protected:
  svtkDataArray* Value;
  svtkExodusIICacheLRURef LRUEntry;

  friend class svtkExodusIICache;
};

class SVTKIOEXODUS_EXPORT svtkExodusIICache : public svtkObject
{
public:
  static svtkExodusIICache* New();
  svtkTypeMacro(svtkExodusIICache, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /// Empty the cache
  void Clear();

  /// Set the maximum allowable cache size. This will remove cache entries if the capacity is
  /// reduced below the current size.
  void SetCacheCapacity(double sizeInMiB);

  /** See how much cache space is left.
   * This is the difference between the capacity and the size of the cache.
   * The result is in MiB.
   */
  double GetSpaceLeft() { return this->Capacity - this->Size; }

  /** Remove cache entries until the size of the cache is at or below the given size.
   * Returns a nonzero value if deletions were required.
   */
  int ReduceToSize(double newSize);

  /// Insert an entry into the cache (this can remove other cache entries to make space).
  void Insert(svtkExodusIICacheKey& key, svtkDataArray* value);

  /** Determine whether a cache entry exists. If it does, return it -- otherwise return nullptr.
   * If a cache entry exists, it is marked as most recently used.
   */
  svtkDataArray*& Find(const svtkExodusIICacheKey&);

  /** Invalidate a cache entry (drop it from the cache) if the key exists.
   * This does nothing if the cache entry does not exist.
   * Returns 1 if the cache entry existed prior to this call and 0 otherwise.
   */
  int Invalidate(const svtkExodusIICacheKey& key);

  /** Invalidate all cache entries matching a specified pattern, dropping all matches from the
   * cache. Any nonzero entry in the \a pattern forces a comparison between the corresponding value
   * of \a key. Any cache entries satisfying all the comparisons will be dropped. If pattern is
   * entirely zero, this will empty the entire cache. This is useful for invalidating all entries of
   * a given object type.
   *
   * Returns the number of cache entries dropped.
   * It is not an error to specify an empty range -- 0 will be returned if one is given.
   */
  int Invalidate(const svtkExodusIICacheKey& key, const svtkExodusIICacheKey& pattern);

protected:
  /// Default constructor
  svtkExodusIICache();

  /// Destructor.
  ~svtkExodusIICache() override;

  /// Avoid (some) FP problems
  void RecomputeSize();

  /// The capacity of the cache (i.e., the maximum size of all arrays it contains) in MiB.
  double Capacity;

  /// The current size of the cache (i.e., the size of the all the arrays it currently contains) in
  /// MiB.
  double Size;

  /** A least-recently-used (LRU) cache to hold arrays.
   * During RequestData the cache may contain more than its maximum size since
   * the user may request more data than the cache can hold. However, the cache
   * is expunged whenever a new array is loaded. Never count on the cache holding
   * what you request for very long.
   */
  svtkExodusIICacheSet Cache;

  /// The actual LRU list (indices into the cache ordered least to most recently used).
  svtkExodusIICacheLRU LRU;

private:
  svtkExodusIICache(const svtkExodusIICache&) = delete;
  void operator=(const svtkExodusIICache&) = delete;
};
#endif // svtkExodusIICache_h
