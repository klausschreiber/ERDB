#ifndef BUFFER_MANAGER_HPP
#include "BufferFrame.hpp"
#include <stdint.h>
#include <unordered_map>

class BufferManager {

public:
    BufferManager( const unsigned int pageCount );
    BufferFrame& fixPage( const uint64_t pageId, const bool exclusive );
    void unfixPage( BufferFrame& frame, const bool isDirty );
    ~BufferManager();

private:
    uint64_t pageCount;
    std::unordered_map<uint64_t, BufferFrame *> pages;
    std::unordered_map<std::string, int> files;
};


#define BUFFER_MANAGER_HPP
#endif
