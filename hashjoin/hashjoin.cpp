#include <iostream>
#include <cstdlib>
#include <atomic>
#include <tbb/tbb.h>
#include <unordered_map>

using namespace tbb;
using namespace std;

inline uint64_t hashKey(uint64_t k) {
   // MurmurHash64A
   const uint64_t m = 0xc6a4a7935bd1e995;
   const int r = 47;
   uint64_t h = 0x8445d61a4e774912 ^ (8*m);
   k *= m;
   k ^= k >> r;
   k *= m;
   h ^= k;
   h *= m;
   h ^= h >> r;
   h *= m;
   h ^= h >> r;
   return h|(1ull<<63);
}

class ChainingLockingHT {
    public:
    // Chained tuple entry
    struct Entry {
        uint64_t key;
        uint64_t value;
        Entry* next;
    };

    // Constructor
    ChainingLockingHT(uint64_t size) {
        //just to avoid having to many colisions
        curr_size = (uint64_t) size * 1.5; 
        //construct hash_map data structure (this also initializes all the tbb::mutex
        ht = new std::pair<tbb::mutex, struct Entry*>[curr_size];
    }

    // Destructor
    ~ChainingLockingHT() {
        //do propper cleanup (free all entries!);
        delete[](ht);
    }

    inline uint64_t lookup(uint64_t key) {
        uint64_t bucket = hashKey(key)%curr_size;
        {
            tbb::mutex::scoped_lock lock(ht[bucket].first);
            struct Entry * curr = ht[bucket].second;
            while(curr != NULL) {
                if(curr->key == key)
                    return curr->value;
                curr = curr->next;
            }
        }
        return std::numeric_limits<uint64_t>::min();
    }

    inline void insert(Entry* entry) {
        uint64_t bucket = hashKey(entry->key)%curr_size;
        struct Entry * curr = reinterpret_cast<struct Entry*>(malloc(sizeof(struct Entry)));
        curr->key = entry->key;
        curr->value = entry->value;
        {
            tbb::mutex::scoped_lock lock(ht[bucket].first);   
            curr->next = ht[bucket].second;
            ht[bucket].second = curr;
        }
    }

    private:
    pair<tbb::mutex, struct Entry*> *ht;
    uint64_t curr_size;
};

class ChainingHT {
   public:
   // Chained tuple entry
   struct Entry {
      uint64_t key;
      uint64_t value;
      Entry* next;
   };

   // Constructor
   ChainingHT(uint64_t size) {
   }

   // Destructor
   ~ChainingHT() {
   }

   inline uint64_t lookup(uint64_t key) {
        return 0;
   }

   inline void insert(Entry* entry) {
   }
};

class LinearProbingHT {
   public:
   // Entry
   struct Entry {
      uint64_t key;
      uint64_t value;
      std::atomic<bool> marker;
   };

   // Constructor
   LinearProbingHT(uint64_t size) {
   }

   // Destructor
   ~LinearProbingHT() {
   }

   inline uint64_t lookup(uint64_t key) {
        return 0;
   }

   inline void insert(uint64_t key) {
   }
};
