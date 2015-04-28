#include "BufferManager.hpp"
#include <iostream>
#include <utility>

static const uint64_t pageSize = 16*1024;

BufferManager::BufferManager( const unsigned int pageCount ) {
    this->pageCount = pageCount;
    std::cout << "BufferFrame constructor" << std::endl;
}

BufferFrame& BufferManager::fixPage( const uint64_t pageId, const bool exclusive ) {
    std::cout << "fixPage called with pageId: " << pageId << " requesting exclusive: " << exclusive<< std::endl;
    uint64_t segment = pageId >> ( 64 - 16 );
    uint64_t page = pageId & (( (uint64_t) 1 << 48 ) - 1 );
    BufferFrame* frame;
    std::unordered_map<uint64_t, BufferFrame*>::const_iterator got = pages.find(pageId);
    if (got == pages.end()) {
        frame = new BufferFrame();
        frame->data = malloc(pageSize);
        frame->pageId = pageId;
        frame->isDirty = true;
        frame->LSN = 42;
        std::pair<uint64_t, BufferFrame*> element(pageId, frame);
        pages.insert(element);
        std::cout << "fixPage of segment: " << segment << " with offset: " << page << " (empty page used. Load not implemented!)" << std::endl;
    }
    else {
        frame = got->second;
        std::cout << "fixPage of segment: " << segment << " with offset: " << page << " (Page loaded from map)" << std::endl;
    }
    return *frame;
}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    std::cout << "unfixPage called with pageId " << frame.pageId << std::endl;
    //TODO: free lock if we have any
    return;
}

BufferManager::~BufferManager() {
    std::cout << "BufferFrame destructor" << std::endl;
}
