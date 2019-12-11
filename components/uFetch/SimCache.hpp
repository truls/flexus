#ifndef SIMCACHE_H
#define SIMCACHE_H

#include <components/CommonQEMU/seq_map.hpp>
#include <core/types.hpp>

namespace nuFetch {

typedef flexus_boost_set_assoc<uint64_t, int> SimCacheArray;
typedef SimCacheArray::iterator SimCacheIter;

class SimCache {
private:
  SimCacheArray theCache;     // The cache storage
  int32_t theCacheSize;       // The size of the cache
  int32_t theCacheAssoc;      // The cache associativity
  int32_t theCacheBlockShift; //
  int32_t theBlockSize;       // The cache block size
  std::string theName;        // The statistics name of the cache

public:
  SimCache(int32_t aCacheSize, int32_t aCacheAssoc, int32_t aBlockSize, const std::string &aName);
  void loadState(std::string const &aDirName);
  bool loadArray(std::istream &s);
  uint64_t insert(uint64_t addr);
  bool lookup(uint64_t addr);
  bool inval(uint64_t addr);
};

} // namespace nuFetch
#endif /* SIMCACHE_H */
