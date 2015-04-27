#include "BufferManager.hpp"
#include <iostream>

static const uint64_t pageSize = 16*1024;

BufferManager::BufferManager(unsigned int pageCount) {
    this->pageCount = pageCount;
    pages.reserve(pageCount);
    std::cout << "BufferFrame constructor" << std::endl;
}

BufferFrame& BufferManager::fixPage(uint64_t pageId, bool exclusive) {
    std::cout << "fixPage" << std::endl;
    if (pages.empty()) {
        pages.push_back(BufferFrame());
    }
    return pages[0];
}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    std::cout << "unfixPage" << std::endl;
    return;
}

BufferManager::~BufferManager() {
    std::cout << "BufferFrame destructor" << std::endl;
}
