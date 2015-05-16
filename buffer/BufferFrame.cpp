#include "BufferFrame.hpp"
#include "constants.hpp"
#include <stdlib.h>
#include <iostream>

void * BufferFrame::getData() {
    return data;
}

BufferFrame::BufferFrame() : BufferFrame(0) { }

BufferFrame::BufferFrame(uint64_t pageId) {
    data = malloc(pageSize);
    this->pageId = pageId;
    isDirty = false;
    int res = pthread_rwlock_init(&rwlock, NULL);
    if (res != 0) {
        std::cout << "could not initiate rwlock! Aborting!" << std::endl;
        exit(1);
    }
    res = pthread_rwlock_init(&latch, NULL);
    if (res != 0) {
        std::cout << "could not initiate latch! Aborting!" << std::endl;
        exit(1);
    }
}

BufferFrame::~BufferFrame() {
    //cast to char* to avoid warning!
    free((char*) data);
    int res = pthread_rwlock_destroy(&rwlock);
    if (res != 0) {
        std::cout << "could not destroy rwlock! Aborting!" << std::endl;
        exit(1);
    }
    res = pthread_rwlock_destroy(&latch);
    if (res != 0) {
        std::cout << "could not destroy latch! Aborting!" << std::endl;
        exit(1);
    }
}
