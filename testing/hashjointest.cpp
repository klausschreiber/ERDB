#include "../hashjoin/hashjoin.cpp"
#include <iostream>


int main(int argc,char** argv) {
   uint64_t sizeR = atoi(argv[1]);
   uint64_t sizeS = atoi(argv[2]);
   unsigned threadCount = atoi(argv[3]);
    
   std::cout << "HashTable test" << std::endl;

   task_scheduler_init init(threadCount);

   // Init build-side relation R with random data
   uint64_t* R=static_cast<uint64_t*>(malloc(sizeR*sizeof(uint64_t)));
   parallel_for(blocked_range<size_t>(0, sizeR), [&](const blocked_range<size_t>& range) {
         unsigned int seed=range.begin();
         for (size_t i=range.begin(); i!=range.end(); ++i)
            R[i]=rand_r(&seed)%sizeR;
      });

   // Init probe-side relation S with random data
   uint64_t* S=static_cast<uint64_t*>(malloc(sizeS*sizeof(uint64_t)));
   parallel_for(blocked_range<size_t>(0, sizeS), [&](const blocked_range<size_t>& range) {
         unsigned int seed=range.begin();
         for (size_t i=range.begin(); i!=range.end(); ++i)
            S[i]=rand_r(&seed)%sizeR;
      });

   // STL
   {
        // Build hash table (single threaded)
        tick_count buildTS=tick_count::now();
        unordered_multimap<uint64_t,uint64_t> ht(sizeR);
        for (uint64_t i=0; i<sizeR; i++)
            ht.emplace(R[i],0);
        tick_count probeTS=tick_count::now();
        cout << "STL             build:" << (sizeR/1e6)/(probeTS-buildTS).seconds() << "MT/s ";

        // Probe hash table and count number of hits
        std::atomic<uint64_t> hitCounter;
        hitCounter=0;
        parallel_for(blocked_range<size_t>(0, sizeS), [&](const blocked_range<size_t>& range) {
                uint64_t localHitCounter=0;
                for (size_t i=range.begin(); i!=range.end(); ++i) {
                    auto range=ht.equal_range(S[i]);
                    for (unordered_multimap<uint64_t,uint64_t>::iterator it=range.first; it!=range.second; ++it)
                        localHitCounter++;
                }
                hitCounter+=localHitCounter;
            });
        tick_count stopTS=tick_count::now();
        cout << "probe: " << (sizeS/1e6)/(stopTS-probeTS).seconds() << "MT/s "
            << "total: " << ((sizeR+sizeS)/1e6)/(stopTS-buildTS).seconds() << "MT/s "
            << "count: " << hitCounter << endl;
    }

    // Test you implementation here... (like the STL test above)
    
    // ChainingLockingHT
    {
        //Build hash table
        tick_count buildTS=tick_count::now();
        ChainingLockingHT ht(sizeR);
        parallel_for(blocked_range<size_t>(0,sizeR), [&](const blocked_range<size_t>& range) {
                struct ChainingLockingHT::Entry *entry = NULL;
                for (size_t i = range.begin(); i!= range.end(); ++i)
                {
                    entry = reinterpret_cast<struct ChainingLockingHT::Entry*>(malloc(sizeof(struct ChainingLockingHT::Entry)));
                    entry->key = R[i];
                    entry->value = 0;
                    ht.insert(entry);
                }
            });
        tick_count probeTS=tick_count::now();
        cout << "ChainingLocking build:" << (sizeR/1e6)/(probeTS-buildTS).seconds() << "MT/s ";

        // Probe hash table and count number of hits
        std::atomic<uint64_t> hitCounter;
        hitCounter=0;
        parallel_for(blocked_range<size_t>(0, sizeS), [&](const blocked_range<size_t>& range) {
                uint64_t localHitCounter = 0;
                for (size_t i=range.begin(); i!=range.end(); ++i) {
                    localHitCounter += ht.lookup(S[i]);
                }
                hitCounter+=localHitCounter;
           });
        tick_count stopTS=tick_count::now();
        cout << "probe: " << (sizeS/1e6)/(stopTS-probeTS).seconds() << "MT/s "
            << "total: " << ((sizeR+sizeS)/1e6)/(stopTS-buildTS).seconds() << "MT/s "
            << "count: " << hitCounter << endl;

    }
   
    // ChainingHT
    {
        //Build hash table
        tick_count buildTS=tick_count::now();
        ChainingHT ht(sizeR);
        parallel_for(blocked_range<size_t>(0,sizeR), [&](const blocked_range<size_t>& range) {
                struct ChainingHT::Entry *entry = NULL;
                for (size_t i = range.begin(); i!= range.end(); ++i)
                {
                    entry = reinterpret_cast<struct ChainingHT::Entry*>(malloc(sizeof(struct ChainingHT::Entry)));
                    entry->key = R[i];
                    entry->value = 0;
                    ht.insert(entry);
                }
            });
        tick_count probeTS=tick_count::now();
        cout << "Chaining        build:" << (sizeR/1e6)/(probeTS-buildTS).seconds() << "MT/s ";

        // Probe hash table and count number of hits
        std::atomic<uint64_t> hitCounter;
        hitCounter=0;
        parallel_for(blocked_range<size_t>(0, sizeS), [&](const blocked_range<size_t>& range) {
                uint64_t localHitCounter = 0;
                for (size_t i=range.begin(); i!=range.end(); ++i) {
                    localHitCounter += ht.lookup(S[i]);
                }
                hitCounter+=localHitCounter;
           });
        tick_count stopTS=tick_count::now();
        cout << "probe: " << (sizeS/1e6)/(stopTS-probeTS).seconds() << "MT/s "
            << "total: " << ((sizeR+sizeS)/1e6)/(stopTS-buildTS).seconds() << "MT/s "
            << "count: " << hitCounter << endl;

    }
    
    // LinearProbingHT
    {
        //Build hash table
        tick_count buildTS=tick_count::now();
        LinearProbingHT ht(sizeR);
        parallel_for(blocked_range<size_t>(0,sizeR), [&](const blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i!= range.end(); ++i)
                    ht.insert(R[i], 0);
            });
        tick_count probeTS=tick_count::now();
        cout << "Linear Probing  build:" << (sizeR/1e6)/(probeTS-buildTS).seconds() << "MT/s ";

        // Probe hash table and count number of hits
        std::atomic<uint64_t> hitCounter;
        hitCounter=0;
        parallel_for(blocked_range<size_t>(0, sizeS), [&](const blocked_range<size_t>& range) {
                uint64_t localHitCounter = 0;
                for (size_t i=range.begin(); i!=range.end(); ++i) {
                    localHitCounter += ht.lookup(S[i]);
                }
                hitCounter+=localHitCounter;
           });
        tick_count stopTS=tick_count::now();
        cout << "probe: " << (sizeS/1e6)/(stopTS-probeTS).seconds() << "MT/s "
            << "total: " << ((sizeR+sizeS)/1e6)/(stopTS-buildTS).seconds() << "MT/s "
            << "count: " << hitCounter << endl;

    }

   return 0;
}
