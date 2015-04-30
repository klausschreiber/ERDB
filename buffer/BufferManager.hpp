#ifndef BUFFER_MANAGER_HPP
#define BUFFER_MANAGER_HPP
#include "BufferFrame.hpp"
#include <stdint.h>
#include <unordered_map>
#include <list>
#include <pthread.h>

class BufferManager {

public:
    BufferManager( const unsigned int pageCount );
    BufferFrame& fixPage( const uint64_t pageId, const bool exclusive );
    void unfixPage( BufferFrame& frame, const bool isDirty );
    ~BufferManager();

private:
    uint64_t pageCount;

    pthread_mutex_t files_lock;
    pthread_mutex_t pages_lock;
    pthread_mutex_t evict_lock;

    std::unordered_map<uint64_t, BufferFrame *> pages;
    std::unordered_map<std::string, int> files;
    std::list<uint64_t> evict_list;
};


#endif
