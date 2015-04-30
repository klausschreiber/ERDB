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
    uint16_t pid:16;
    uint64_t sid:48;
};

BufferManager::BufferManager( const unsigned int pageCount ) {
    this->pageCount = pageCount;
    int res = pthread_mutex_init(&pages_lock, NULL);
    if (res != 0) {
        std::cout << "could not init mutex. Aborting!" << std::endl;
        exit(1);
    }
    res = pthread_mutex_init(&files_lock, NULL);
    if (res != 0) {
        std::cout << "could not init mutex. Aborting!" << std::endl;
        exit(1);
    }
    res = pthread_mutex_init(&evict_lock, NULL);
    if (res != 0) {
        std::cout << "could not init mutex. Aborting!" << std::endl;
        exit(1);
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
    std::unordered_map<uint64_t, BufferFrame *>::const_iterator pages_it = pages.find(pageId);
    if (pages_it == pages.end()) {
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
            //if we found the file descriptor in our list, reuse it
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
            frame->isDirty = true;
        }
        
        std::cout << "pages" ;
        for ( std::unordered_map<uint64_t, BufferFrame*>::iterator it = pages.begin(); it != pages.end(); it++) {
            std::cout << " : " << it->first << " - " << it->second->pageId;
        }
        std::cout << std::endl;

        std::cout << "evict_list" ;
        for ( std::list<uint64_t>::iterator it = evict_list.begin(); it != evict_list.end(); it++) {
            std::cout << " : " << *it;
        }
        std::cout << std::endl;
        //check if we have free space within our buffer
        if (pages.size() >= pageCount)
        {
//            std::cout << "evicting... buffer size is:" << pages.size() << std::endl;
            //we need to evict a page!
            //LOCK(evict_lock)
            if (pthread_mutex_lock(&evict_lock) != 0) {
                std::cout << "error while locking evict list. Aborting!" << std::endl;
                exit(1);
            }
            bool evict_success = false;
            for (std::list<uint64_t>::iterator it = evict_list.begin(); it != evict_list.end(); it++) { 
                std::cout << "trying to evict pageId: " << *it << std::endl;
                std::unordered_map<uint64_t, BufferFrame *>::const_iterator elem = pages.find(*it);
                if (elem != pages.end()) {
                    BufferFrame * candidate = elem->second;
                    if (candidate->pageId != *it) {
                        std::cout << "pageId does not match! looked for: " << *it << " but got: " << candidate->pageId << std::endl;
                    }
//                    std::cout << "found candidate for pageId: " << *it << std::endl;
                    //check if it is currently unused;
                    if(pthread_rwlock_trywrlock(&(candidate->rwlock)) == 0) {
                        //this means we got the lock :-) -> we can evict this page
//                        std::cout << "candidate could be locked :-)" << std::endl;
                        pages.erase(elem);
                        evict_list.erase(it);
                    
                        //UNLOCK(evict_lock) success case
                        if (pthread_mutex_unlock(&evict_lock) != 0) {
//                            std::cout << "error while unlocking evict list. Aborting!" << std::endl;
                            exit(1);
                        }
                        //we removed it from both lists, now we need to store it in case it is dirty
                        if (candidate->isDirty) {
                            //write it to disk
                            //Get pid and sid from pageId
                            const struct Pid * evict_pid = reinterpret_cast<const struct Pid *>(&(candidate->pageId));
                            //calculate the file name
                            std::stringstream evict_sstm;
                            evict_sstm << evict_pid->sid;
                            std::string evict_fname = evict_sstm.str();
                            
                            //LOCK(files_lock)
                            if (pthread_mutex_lock(&files_lock) != 0) {
                                std::cout << "error while locking files lock. Aborting!" << std::endl;
                                exit(1);
                            }
                            //get the file descriptor
                            std::unordered_map<std::string, int>::const_iterator fd_it = files.find(evict_fname);
                            if (fd_it == files.end()) {
                                // if this happens we have an inconsistent state
                                // where did the frame come from if the file is not even opened?
                                std::cout << "inconsistent state within files <-> buffered pages. Aborting!" << std::endl;
                                exit(1);
                            }
                            //UNLOCK(files_lock)
                            if (pthread_mutex_unlock(&files_lock) != 0) {
                                std::cout << "error unlocking files lock. Aborting!" << std::endl;
                                exit(1);
                            }

                            //write the date to the file
                            ssize_t wsize = pwrite(fd_it->second, candidate->data, pageSize, pageSize*(evict_pid->pid));
                            if (wsize < 0) {
                                perror ("Could not write to file within fixPage! Aborting!");
                                exit(1);
                            }
                            else if (wsize != pageSize) {
                                std::cout << "could not write all data to file. Aborting!" << std::endl;
                                exit(1);
                            }
                            std::cout << "successfully wrote back dirty page. Write size was: " << 0 << ", pageSize is: " << pageSize << std::endl;
                            //at this point the write was successfull
                        }
                        //unlock and delete the page
                        if(pthread_rwlock_unlock(&(candidate->rwlock)) != 0) {
                            // if this happens we could not unlock this page again, which is bad!
                            std::cout << "unlock of page failed! Aborting!" << std::endl;
                            exit(1);
                        }
                        std::cout << "successfully evicted pageId: " << candidate->pageId << " original pageId: " << *it << std::endl;
                        delete candidate;
                        //we found one element and successfully deleted it, stop evicting now
                        evict_success = true;
                        break;
                    }
                    else {
                        //do nothing here, as the page was locked. continue with next page
                        continue;
                    }
                }
                else {
                    // this should never happen as it means inconsistency within our internal data
                    std::cout << "error while looking for evict page. Inconsistent data! Aborting!" << std::endl;
                    exit(1);
                }
            }            
            //UNLOCK(evict_lock) if not happened before
            if (!evict_success && pthread_mutex_unlock(&evict_lock) != 0) {
                std::cout << "error while unlocking evict list. Aborting!" << std::endl;
                exit(1);
            }

        }
        //enough space should be free now, add the element to the pages map
        std::cout << "adding data to pages: pageId: " << pageId << " frame->pageId: " << frame->pageId << std::endl;
        std::pair<uint64_t, BufferFrame*> element(pageId, frame);
        pages.insert(element);
        
        //add element to evict_list

        //LOCK(evict_lock)
        if (pthread_mutex_lock(&evict_lock) != 0) {
            std::cout << "error while locking evict list. Aborting!" << std::endl;
            exit(1);
        }
        evict_list.push_back(pageId);
        //UNLOCK(evict_lock)
        if (pthread_mutex_unlock(&evict_lock) != 0) {
            std::cout << "error while unlocking evict list. Aborting!" << std::endl;
            exit(1);
        }
    }
    else {
        cached = true;
        frame = pages_it->second;
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
    std::cout << "FIX: " << pageId << " exclusive: " << exclusive << " cached: " << cached << " exists: " << exists << std::endl;
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
    std::cout << "UNFIX: " << frame.pageId << " isDirty was: " << isDirty << std::endl;
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
    res = pthread_mutex_destroy(&evict_lock);
    if (res != 0) {
        std::cout << "could not destroy mutex. Aborting!" << std::endl;
    }
}
