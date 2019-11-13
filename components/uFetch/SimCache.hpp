typedef flexus_boost_set_assoc<uint64_t, int> SimCacheArray;
typedef SimCacheArray::iterator SimCacheIter;

struct SimCache {
  SimCacheArray theCache;
  int32_t theCacheSize;
  int32_t theCacheAssoc;
  int32_t theCacheBlockShift;
  int32_t theBlockSize;
  std::string theName;

  void init(int32_t aCacheSize, int32_t aCacheAssoc, int32_t aBlockSize, const std::string &aName) {
    theCacheSize = aCacheSize;
    theCacheAssoc = aCacheAssoc;
    theBlockSize = aBlockSize;
    theCacheBlockShift = LOG2(theBlockSize);
    theCache.init(theCacheSize / theBlockSize, theCacheAssoc, 0);
    theName = aName;
  }

  void loadState(std::string const &aDirName) {
    std::string fname(aDirName);
    DBG_(VVerb, (<< "Loading state: " << fname << " for ufetch order L1i cache"));
    std::ifstream ifs(fname.c_str());
    if (!ifs.good()) {
      DBG_(VVerb,
           (<< " saved checkpoint state " << fname << " not found.  Resetting to empty cache. "));
    } else {
      ifs >> std::skipws;

      if (!loadArray(ifs)) {
        DBG_(VVerb, (<< "Error loading checkpoint state from file: " << fname
                     << ".  Make sure your checkpoints match your current "
                        "cache configuration."));
        DBG_Assert(false);
      }
      ifs.close();
    }
  }

  bool loadArray(std::istream &s) {
    static const int32_t kSave_ValidBit = 1;
    int32_t tagShift = LOG2(theCache.sets());

    char paren;
    int32_t dummy;
    int32_t load_state;
    uint64_t load_tag;
    for (uint32_t i = 0; i < theCache.sets(); i++) {
      s >> paren; // {
      if (paren != '{') {
        DBG_(Crit, (<< "Expected '{' when loading checkpoint"));
        return false;
      }
      for (uint32_t j = 0; j < theCache.assoc(); j++) {
        s >> paren >> load_state >> load_tag >> paren;
        DBG_(Trace, (<< theName << " Loading block " << std::hex
                     << (((load_tag << tagShift) | i) << theCacheBlockShift) << " with state "
                     << ((load_state & kSave_ValidBit) ? "Shared" : "Invalid") << " in way " << j));
        if (load_state & kSave_ValidBit) {
          theCache.insert(std::make_pair(((load_tag << tagShift) | i), 0));
          DBG_Assert(theCache.size() <= theCache.assoc());
        }
      }
      s >> paren; // }
      if (paren != '}') {
        DBG_(Crit, (<< "Expected '}' when loading checkpoint"));
        return false;
      }

      // useless associativity information
      s >> paren; // <
      if (paren != '<') {
        DBG_(Crit, (<< "Expected '<' when loading checkpoint"));
        return false;
      }
      for (uint32_t j = 0; j < theCache.assoc(); j++) {
        s >> dummy;
      }
      s >> paren; // >
      if (paren != '>') {
        DBG_(Crit, (<< "Expected '>' when loading checkpoint"));
        return false;
      }
    }
    return true;
  }

  uint64_t insert(uint64_t addr) {
    uint64_t ret_val = 0;
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.move_back(iter);
      return ret_val; // already present
    }
    if ((int)theCache.size() >= theCacheAssoc) {
      ret_val = theCache.front_key() << theCacheBlockShift;
      theCache.pop_front();
    }
    theCache.insert(std::make_pair(addr, 0));
    return ret_val;
  }

  bool lookup(uint64_t addr) {
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.move_back(iter);
      return true; // present
    }
    return false; // not present
  }

  bool inval(uint64_t addr) {
    addr = addr >> theCacheBlockShift;
    SimCacheIter iter = theCache.find(addr);
    if (iter != theCache.end()) {
      theCache.erase(iter);
      return true; // invalidated
    }
    return false; // not present
  }
};
