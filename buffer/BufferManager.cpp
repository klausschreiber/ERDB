#include "BufferManager.hpp"
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

static const uint64_t pageSize = 16*1024;

//Definition of segment-id and page-id within pageId (externally given)
//struct Pid {
//    uint16_t sid:16;
//    uint64_t pid:48;
//};

BufferManager::BufferManager( const unsigned int pageCount ) {
    this->pageCount = pageCount;
    std::cout << "BufferFrame constructor" << std::endl;
}

BufferFrame& BufferManager::fixPage( const uint64_t pageId, const bool exclusive ) {
    std::cout << "fixPage called with pageId: " << pageId << " requesting exclusive: " << exclusive<< std::endl;
    
    std::cout << "Pages.size(): " << pages.size() << std::endl;
    
    //Test if we got the frame in the buffer
    BufferFrame * frame;
    std::unordered_map<uint64_t, BufferFrame *>::const_iterator got = pages.find(pageId);
    if (got == pages.end()) {
//        //Get pid and sid from pageId
//        const struct Pid * pid = reinterpret_cast<const struct Pid *>(&pageId);
//        //calculate the file name
//        std::stringstream sstm;
//        sstm << pid->sid;
//        std::string fname = sstm.str();
//        //check if this file was opened before and is still valid
//        std::unordered_map<std::string, int>::const_iterator files_it = files.find(fname);
//        int fd;
//        if (files_it == files.end()) {
//            //it was not opened before, so open it
//            fd = open(fname.c_str(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
//            if (fd < 0) {
//                perror("Could not open file within fixPage!");
//                exit(1);
//            }
//            files.insert(std::pair<std::string, int> (fname, fd));
//        }
//        else {
//            //TODO: Think about it: Can the file descriptor get invalid at some point?
//            fd = files_it->second;
//        }
//
        frame = new BufferFrame();
        frame->data = malloc(pageSize);
        frame->pageId = pageId;
        frame->isDirty = false;
//
//        //read the data from the file
//        ssize_t rsize = pread(fd, frame->data, pageSize*(pid->pid), pageSize);
//        if (rsize < 0) {
//            perror ("Could not read from file within fixPage!");
//            exit(1);
//        }
//        else if (rsize != pageSize) {
//            //TODO: Think about it: Do we get 0 Bytes read if reading beyond end of file?
//            std::cout << "Could not read enough bytes from file within fixPage. Creating it empty." << std::endl;
//            //memset(frame->data, '\0', pageSize);
//        }
//
        std::pair<uint64_t, BufferFrame*> element(pageId, frame);
        pages.insert(element);
        std::cout << "fixPage of page: " << pageId << " (empty page used. Load not implemented!)" << std::endl;
    }
    else {
        frame = got->second;
        std::cout << "fixPage of page: " << pageId << " (Page loaded from map)" << std::endl;
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
