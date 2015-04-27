#ifndef BUFFER_FRAME_HPP
#include <stdint.h>

class BufferFrame {
    friend class BufferManager;

public:

    void* getData();

private:

    void * data;
    uint64_t pageId, LSN;
    bool isDirty;
};


#define BUFFER_FRAME_HPP
#endif
