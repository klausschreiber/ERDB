#ifndef BUFFER_MANAGER_HPP
#include "BufferFrame.hpp"
#include <stdint.h>
#include <vector>

class BufferManager {

public:
    BufferManager(unsigned int pageCount);
    BufferFrame& fixPage(uint64_t pageId, bool exclusive);
    void unfixPage(BufferFrame& frame, bool isDirty);
    ~BufferManager();

private:
    uint64_t pageCount;
    std::vector<BufferFrame> pages;
};


#define BUFFER_MANAGER_HPP
#endif
