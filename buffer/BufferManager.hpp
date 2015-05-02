#ifndef BUFFER_MANAGER_HPP
#define BUFFER_MANAGER_HPP
#include "BufferFrame.hpp"
#include <stdint.h>
#include <unordered_map>
#include <list>
#include <pthread.h>

class BufferManager {

public:
    //constructor. Needs to be called with a count of pages to use
    BufferManager( const unsigned int pageCount );
    
    //fixPage call. Used to get a certain Page. If exclusive is given it is locked for writing
    //else only reading is safe!
    BufferFrame& fixPage( const uint64_t pageId, const bool exclusive );
    
    //return a previously fixed Page. Returning a page twice is undefined and might lead to a
    //system failure! If isDirty is set, it is assumed that the page was changed. Else it is
    //assumed untouched
    void unfixPage( BufferFrame& frame, const bool isDirty );
    
    //Destructor
    ~BufferManager();

private:
    //pageCount to buffer in RAM
    uint64_t pageCount;

    //Lock variables for hash-maps and evict-list
    pthread_mutex_t files_lock;
    pthread_mutex_t pages_lock;
    pthread_mutex_t evict_lock;

    //hash-map for buffered frames (pageId -> BufferFrame)
    std::unordered_map<uint64_t, BufferFrame *> pages;
    //hash-map for file descriptors (file name -> fd)
    std::unordered_map<std::string, int> files;
    //list for evict order (currently used as fifo
    std::list<uint64_t> evict_list;

};


#endif
