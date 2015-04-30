#include "BufferManager.hpp"
#include "constants.hpp"
#include <iostream>
#include <utility>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


//Definition of segment-id and page-id within pageId (externally given)
struct Pid {
    uint16_t sid:16;
    uint64_t pid:48;
};

BufferManager::BufferManager( const unsigned int pageCount ) {
    this->pageCount = pageCount;
//    pages_lock = PTHREAD_MUTEX_INITIALIZER;
//    files_lock = PTHREAD_MUTEX_INITIALIZER;
    int res = pthread_mutex_init(&pages_lock, NULL);
    if (res != 0) {
        std::cout << "could not init mutex. Aborting!" << std::endl;
    }
    res = pthread_mutex_init(&files_lock, NULL);
    if (res != 0) {
        std::cout << "could not init mutex. Aborting!" << std::endl;
    }
}

BufferFrame& BufferManager::fixPage( const uint64_t pageId, const bool exclusive ) {
    // some flags we set while processing to generate log output later on
    bool exists = true;
    bool cached = false;
 
    //Test if we got the frame in the buffer
    BufferFrame * frame;

    //lock the pages
    if (pthread_mutex_lock(&pages_lock) != 0) {
        std::cout << "error while locking pages table. Aborting!" << std::endl;
        exit(1);
    }
    std::unordered_map<uint64_t, BufferFrame *>::const_iterator got = pages.find(pageId);
    if (got == pages.end()) {
        //Get pid and sid from pageId
        const struct Pid * pid = reinterpret_cast<const struct Pid *>(&pageId);
        //calculate the file name
        std::stringstream sstm;
        sstm << pid->sid;
        std::string fname = sstm.str();
        //check if this file was opened before and is still valid
        std::unordered_map<std::string, int>::const_iterator files_it = files.find(fname);
        int fd;
        if (files_it == files.end()) {
            //it was not opened before, so open it
            fd = open(fname.c_str(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
            if (fd < 0) {
                perror("Could not open file within fixPage!");
                exit(1);
            }
            files.insert(std::pair<std::string, int> (fname, fd));
        }
        else {
            //TODO: Think about it: Can the file descriptor get invalid at some point?
            fd = files_it->second;
        }

        frame = new BufferFrame(pageId);

        //read the data from the file
        ssize_t rsize = pread(fd, frame->data, pageSize*(pid->pid), pageSize);
        if (rsize < 0) {
            perror ("Could not read from file within fixPage!");
            exit(1);
        }
        else if (rsize != pageSize) {
            //TODO: Think about it: Do we get 0 Bytes read if reading beyond end of file?
            exists = false;
            memset(frame->data, '\0', pageSize);
        }

        std::pair<uint64_t, BufferFrame*> element(pageId, frame);
        pages.insert(element);
    }
    else {
        cached = true;
        frame = got->second;
    }
    int res;
    if (exclusive) {
        //exclusive lock
        res = pthread_rwlock_wrlock(&(frame->rwlock));
    }
    else {
        //non exclusive lock
        res = pthread_rwlock_rdlock(&(frame->rwlock));
    }
    if (res != 0) {
        std::cout << "could not lock rwlock! Aborting!" << std::endl;
        exit(1);
    }
    //unlock the pages
    if (pthread_mutex_unlock(&pages_lock) != 0) {
        std::cout << "error while unlocking pages table. Aborting!" << std::endl;
        exit(1);
    }
//    std::cout << "FIX: " << pageId << " exclusive: " << exclusive << " cached: " << cached << " exists: " << exists << std::endl;
    return *frame;
}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    // set dirty flag if needed. This could always just be written but I think it is more readable this way
    if (isDirty) {
        frame.isDirty = true;
    }

    //unlock data
    int res = pthread_rwlock_unlock(&(frame.rwlock));
    if (res != 0) {
        std::cout << "could not unlock rwlock! Aborting!" << std::endl;
        exit(1);
    }
//    std::cout << "UNFIX: " << frame.pageId << " isDirty was: " << isDirty << std::endl;
    return;
}

BufferManager::~BufferManager() {
    std::cout << "BufferFrame destructor" << std::endl;
    int res = pthread_mutex_destroy(&pages_lock);
    if (res != 0) {
        std::cout << "could not destroy mutex. Aborting!" << std::endl;
    }
    res = pthread_mutex_destroy(&files_lock);
    if (res != 0) {
        std::cout << "could not destroy mutex. Aborting!" << std::endl;
    }
}
