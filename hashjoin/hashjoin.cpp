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
        //construct hash_map data structure (this also initializes all the tbb::mutex)
        //ht = new std::pair<tbb::mutex, struct Entry*>[curr_size];
        ht = reinterpret_cast<std::pair<tbb::mutex, struct Entry*>*>(calloc(sizeof(std::pair<tbb::mutex, struct Entry*>), curr_size));
    }

    // Destructor
    ~ChainingLockingHT() {
        //do propper cleanup
        for(uint64_t i = 0; i < curr_size; i++)
            free_chain(ht[i].second);
        //delete[](ht);
        free(ht);
    }

    //returns the number of hits
    inline uint64_t lookup(uint64_t key) {
        uint64_t bucket = hashKey(key)%curr_size;
        int count = 0;
        struct Entry * curr = NULL;
        //only sync access to first element, as this hashtable does neither update nor delete
        {
            tbb::mutex::scoped_lock lock(ht[bucket].first);
            curr = ht[bucket].second;
        }
        while(curr != NULL) {
            if(curr->key == key) count++;
            curr = curr->next;            
        }
        return count;
    }

    inline void insert(Entry* entry) {
        uint64_t bucket = hashKey(entry->key)%curr_size;
        {
            tbb::mutex::scoped_lock lock(ht[bucket].first);   
            entry->next = ht[bucket].second;
            ht[bucket].second = entry;
        }
    }

    private:
    pair<tbb::mutex, struct Entry*> *ht;
    uint64_t curr_size;

    //recursive free of a chain
    void free_chain(struct Entry* entry) {
        if(entry == NULL)
            return;
        if(entry->next != NULL)
            free_chain(entry->next);
        free(entry);
    }
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
        curr_size = (uint64_t) size * 1.5;
        //ht = new std::atomic<struct Entry*>[curr_size];
        //for(uint64_t i = 0; i<curr_size; i++)
        //    ht[i].store(NULL);
        // use of calloc seems to be a factor of 2 faster than manually initializing the elements
        // and simply setting everything to 0 seems to work fine for std::atomic....
        ht = reinterpret_cast<std::atomic<struct Entry*>*>(calloc(sizeof(std::atomic<struct Entry*>*), curr_size));
    }

    // Destructor
    ~ChainingHT() {
        for(uint64_t i = 0; i < curr_size; i++)
            free_chain(ht[i].load());
        //delete[] ht;
        free(ht);
    }

    //returns the number of hits
    inline uint64_t lookup(uint64_t key) {
        uint64_t bucket = hashKey(key)%curr_size;
        int count = 0;
        //we only need to sync the firs access, as this hash table anyways does neither delete nor update
        struct Entry * curr = ht[bucket].load(std::memory_order_relaxed);
        while(curr != NULL) {
            if(curr->key == key) count++;
            curr = curr->next;
        }
        return count;
    }    

    inline void insert(Entry* entry) {
        uint64_t bucket = hashKey(entry->key)%curr_size;
        //get value for next
        entry->next = ht[bucket].load(std::memory_order_relaxed);
        //busy try to insert
        while(!ht[bucket].compare_exchange_weak(entry->next, entry, std::memory_order_release, std::memory_order_relaxed));
    }

    private:
    std::atomic<struct Entry*> *ht;
    uint64_t curr_size;

    //recursive free of a chain
    void free_chain(struct Entry* entry) {
        if(entry == NULL)
            return;
        if(entry->next != NULL)
            free_chain(entry->next);
        free(entry);
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
        curr_size = (uint64_t) size * 1.5;
        ht = reinterpret_cast<struct Entry *>(calloc(sizeof(struct Entry), curr_size));
    }

    // Destructor
    ~LinearProbingHT() {
        free(ht);
    }

    //returns the number of hits
    inline uint64_t lookup(uint64_t key) {
        uint64_t bucket = hashKey(key)%curr_size;
        uint64_t curr_bucket = bucket;
        uint64_t count = 0;
        while(ht[curr_bucket].marker.load()){
            if(ht[curr_bucket].key == key){
                count++;
            }
            curr_bucket = (curr_bucket+1)%curr_size;
            if (curr_bucket == bucket)
                break;
        }
        return count;
    }

    inline void insert(uint64_t key, uint64_t value) {
        uint64_t bucket = hashKey(key)%curr_size;
        uint64_t curr_bucket = bucket;
        bool desired = false;
        //find first free element from here on
        while(!ht[curr_bucket].marker.compare_exchange_weak(desired, true, std::memory_order_release, std::memory_order_relaxed)){
            desired = false;
            curr_bucket = (curr_bucket+1)%curr_size;
            //fail hard if the space is not enough! In real implementations we should probably try to grow, but this is quite expansive!
            if (curr_bucket == bucket)
                exit(1);
        }
        //now we set our value to used, so we can write it safely (without overwriting anything)
        //this does not prevent that someone reads this value while we have not finished writing,
        //but it avoids two inserts writing to the same value at the same time. To also prevent
        //invalid reads, a second flag would be needed. But this case rarely happens (and especially
        //it does not happen in our test-cases and for hash-joins where we insert everything and then probe)
        ht[curr_bucket].key = key;
        ht[curr_bucket].value = value;
    }
   
    private:
    struct Entry *ht;
    uint64_t curr_size;
};

//for test cases see ../testing/hashjointest.cpp
