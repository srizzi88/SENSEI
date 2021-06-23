#include "svtkExodusIICache.h"

#include "svtkDataArray.h"
#include "svtkObjectFactory.h"

// Define SVTK_EXO_DBG_CACHE to print cache adds, drops, and replacements.
//#undef SVTK_EXO_DBG_CACHE

#define SVTK_EXO_PRT_KEY(ckey)                                                                      \
  "(" << (ckey).Time << ", " << (ckey).ObjectType << ", " << (ckey).ObjectId << ", "               \
      << (ckey).ArrayId << ")"
#define SVTK_EXO_PRT_ARR(cval)                                                                      \
  " [" << (cval) << "," << ((cval) ? (cval)->GetActualMemorySize() / 1024. : 0.) << "/"            \
       << this->Size << "/" << this->Capacity << "]"
#define SVTK_EXO_PRT_ARR2(cval)                                                                     \
  " [" << (cval) << ", " << ((cval) ? (cval)->GetActualMemorySize() / 1024. : 0.) << "]"

#if 0
static void printCache( svtkExodusIICacheSet& cache, svtkExodusIICacheLRU& lru )
{
  cout << "Cache\n";
  svtkExodusIICacheRef cit;
  for ( cit = cache.begin(); cit != cache.end(); ++cit )
  {
    cout << SVTK_EXO_PRT_KEY( cit->first ) << SVTK_EXO_PRT_ARR2( cit->second->GetValue() ) << "\n";
  }
  cout << "LRU\n";
  svtkExodusIICacheLRURef lit;
  for ( lit = lru.begin(); lit != lru.end(); ++lit )
  {
    cout << SVTK_EXO_PRT_KEY( (*lit)->first ) << "\n";
  }
}
#endif // 0

// ============================================================================
svtkExodusIICacheEntry::svtkExodusIICacheEntry()
{
  this->Value = nullptr;
}

svtkExodusIICacheEntry::svtkExodusIICacheEntry(svtkDataArray* arr)
{
  this->Value = arr;
  if (arr)
    this->Value->Register(nullptr);
}
svtkExodusIICacheEntry::~svtkExodusIICacheEntry()
{
  if (this->Value)
    this->Value->Delete();
}

svtkExodusIICacheEntry::svtkExodusIICacheEntry(const svtkExodusIICacheEntry& other)
{
  this->Value = other.Value;
  if (this->Value)
    this->Value->Register(nullptr);
}

#if 0
void printLRUBack( svtkExodusIICacheRef& cit )
{
  cout << "Key is " << SVTK_EXO_PRT_KEY( cit->first ) << "\n";
}
#endif // 0

// ============================================================================

svtkStandardNewMacro(svtkExodusIICache);

svtkExodusIICache::svtkExodusIICache()
{
  this->Size = 0.;
  this->Capacity = 2.;
}

svtkExodusIICache::~svtkExodusIICache()
{
  this->ReduceToSize(0.);
}

void svtkExodusIICache::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Capacity: " << this->Capacity << " MiB\n";
  os << indent << "Size: " << this->Size << " MiB\n";
  os << indent << "Cache: " << &this->Cache << " (" << this->Cache.size() << ")\n";
  os << indent << "LRU: " << &this->LRU << "\n";
}

void svtkExodusIICache::Clear()
{
  // printCache( this->Cache, this->LRU );
  this->ReduceToSize(0.);
}

void svtkExodusIICache::SetCacheCapacity(double sizeInMiB)
{
  if (sizeInMiB == this->Capacity)
    return;

  if (this->Size > sizeInMiB)
  {
    this->ReduceToSize(sizeInMiB);
  }

  this->Capacity = sizeInMiB < 0 ? 0 : sizeInMiB;
}

int svtkExodusIICache::ReduceToSize(double newSize)
{
  int deletedSomething = 0;
  while (this->Size > newSize && !this->LRU.empty())
  {
    svtkExodusIICacheRef cit(this->LRU.back());
    svtkDataArray* arr = cit->second->Value;
    if (arr)
    {
      deletedSomething = 1;
      double arrSz = (double)arr->GetActualMemorySize() / 1024.;
      this->Size -= arrSz;
#ifdef SVTK_EXO_DBG_CACHE
      cout << "Dropping " << SVTK_EXO_PRT_KEY(cit->first) << SVTK_EXO_PRT_ARR(arr) << "\n";
#endif // SVTK_EXO_DBG_CACHE
      if (this->Size <= 0)
      {
        if (this->Cache.empty())
          this->Size = 0.;
        else
          this->RecomputeSize(); // oops, FP roundoff
      }
    }
    else
    {
#ifdef SVTK_EXO_DBG_CACHE
      cout << "Dropping " << SVTK_EXO_PRT_KEY(cit->first) << SVTK_EXO_PRT_ARR(arr) << "\n";
#endif // SVTK_EXO_DBG_CACHE
    }

    delete cit->second;
    this->Cache.erase(cit);
    this->LRU.pop_back();
  }

  if (this->Cache.empty())
  {
    this->Size = 0;
  }

  return deletedSomething;
}

void svtkExodusIICache::Insert(svtkExodusIICacheKey& key, svtkDataArray* value)
{
  double vsize = value ? value->GetActualMemorySize() / 1024. : 0.;

  svtkExodusIICacheRef it = this->Cache.find(key);
  if (it != this->Cache.end())
  {
    if (it->second->Value == value)
      return;

    // Remove existing array and put in our new one.
    this->Size -= vsize;
    if (this->Size <= 0)
    {
      this->RecomputeSize();
    }
    this->ReduceToSize(this->Capacity - vsize);
    it->second->Value->Delete();
    it->second->Value = value;
    it->second->Value->Register(
      nullptr); // Since we re-use the cache entry, the constructor's Register won't get called.
    this->Size += vsize;
#ifdef SVTK_EXO_DBG_CACHE
    cout << "Replacing " << SVTK_EXO_PRT_KEY(it->first) << SVTK_EXO_PRT_ARR(value) << "\n";
#endif // SVTK_EXO_DBG_CACHE
    this->LRU.erase(it->second->LRUEntry);
    it->second->LRUEntry = this->LRU.insert(this->LRU.begin(), it);
  }
  else
  {
    this->ReduceToSize(this->Capacity - vsize);
    std::pair<const svtkExodusIICacheKey, svtkExodusIICacheEntry*> entry(
      key, new svtkExodusIICacheEntry(value));
    std::pair<svtkExodusIICacheSet::iterator, bool> iret = this->Cache.insert(entry);
    this->Size += vsize;
#ifdef SVTK_EXO_DBG_CACHE
    cout << "Adding " << SVTK_EXO_PRT_KEY(key) << SVTK_EXO_PRT_ARR(value) << "\n";
#endif // SVTK_EXO_DBG_CACHE
    iret.first->second->LRUEntry = this->LRU.insert(this->LRU.begin(), iret.first);
  }
  // printCache( this->Cache, this->LRU );
}

svtkDataArray*& svtkExodusIICache::Find(const svtkExodusIICacheKey& key)
{
  static svtkDataArray* dummy = nullptr;

  svtkExodusIICacheRef it = this->Cache.find(key);
  if (it != this->Cache.end())
  {
    this->LRU.erase(it->second->LRUEntry);
    it->second->LRUEntry = this->LRU.insert(this->LRU.begin(), it);
    return it->second->Value;
  }

  dummy = nullptr;
  return dummy;
}

int svtkExodusIICache::Invalidate(const svtkExodusIICacheKey& key)
{
  svtkExodusIICacheRef it = this->Cache.find(key);
  if (it != this->Cache.end())
  {
#ifdef SVTK_EXO_DBG_CACHE
    cout << "Dropping " << SVTK_EXO_PRT_KEY(it->first) << SVTK_EXO_PRT_ARR(it->second->Value) << "\n";
#endif // SVTK_EXO_DBG_CACHE
    this->LRU.erase(it->second->LRUEntry);
    if (it->second->Value)
    {
      this->Size -= it->second->Value->GetActualMemorySize() / 1024.;
    }
    delete it->second;
    this->Cache.erase(it);

    if (this->Size <= 0)
    {
      if (this->Cache.empty())
        this->Size = 0.;
      else
        this->RecomputeSize(); // oops, FP roundoff
    }

    return 1;
  }
  return 0;
}

int svtkExodusIICache::Invalidate(const svtkExodusIICacheKey& key, const svtkExodusIICacheKey& pattern)
{
  svtkExodusIICacheRef it;
  int nDropped = 0;
  it = this->Cache.begin();
  while (it != this->Cache.end())
  {
    if (!it->first.match(key, pattern))
    {
      ++it;
      continue;
    }

#ifdef SVTK_EXO_DBG_CACHE
    cout << "Dropping " << SVTK_EXO_PRT_KEY(it->first) << SVTK_EXO_PRT_ARR(it->second->Value) << "\n";
#endif // SVTK_EXO_DBG_CACHE
    this->LRU.erase(it->second->LRUEntry);
    if (it->second->Value)
    {
      this->Size -= it->second->Value->GetActualMemorySize() / 1024.;
    }
    svtkExodusIICacheRef tmpIt = it++;
    delete tmpIt->second;
    this->Cache.erase(tmpIt);

    if (this->Size <= 0)
    {
      if (this->Cache.empty())
        this->Size = 0.;
      else
        this->RecomputeSize(); // oops, FP roundoff
    }

    ++nDropped;
  }
  return nDropped;
}

void svtkExodusIICache::RecomputeSize()
{
  this->Size = 0.;
  svtkExodusIICacheRef it;
  for (it = this->Cache.begin(); it != this->Cache.end(); ++it)
  {
    if (it->second->Value)
    {
      this->Size += (double)it->second->Value->GetActualMemorySize() / 1024.;
    }
  }
}
