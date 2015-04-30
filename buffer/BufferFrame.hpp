#ifndef BUFFER_FRAME_HPP
#define BUFFER_FRAME_HPP
#include <stdint.h>
#include <pthread.h>

class BufferFrame {
    friend class BufferManager;

public:

    void* getData();

    ~BufferFrame();

private:

    //hide constructor + create constructor for new pages
    BufferFrame();
    BufferFrame(uint64_t pageId);

    void * data;
    uint64_t pageId;
    bool isDirty;
    pthread_rwlock_t rwlock;
};


#endif
