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
        
    //LOCK(pages_lock)
    if (pthread_mutex_lock(&pages_lock) != 0) {
        std::cout << "error while locking pages table. Aborting!" << std::endl;
        exit(1);
    }
    std::unordered_map<uint64_t, BufferFrame *>::const_iterator pages_it = pages.find(pageId);
    if (pages_it == pages.end()) {
        //The page was not within the buffered pages.
        //Step 1 : create it
        //Step 2 : write lock it (even thou we still hold pages lock. This should anyways work as the page is newly created!)
        //Step 3 : add it to pages (so all others that look for this page simply wait for the lock and do not load it themselves)
        //Step 4 : check if we need to evict an element (should be fast)
        //Step 5 : if we need to evict an element, select one and remove it from pages map
        //Step 6 : unlock pages (hash-map)
        //Step 7 : evict selected element if needed (store to disk. free space)
        //Step 5 : really load page from disk
        //Step 6 : add page to evict_list
        //Step 7 : unlock page (so it can be locked as desired by caller)
        
        //Step 1 : create Page 
        frame = new BufferFrame(pageId);

        //Step 2 : write lock it
        if (pthread_rwlock_wrlock(&(frame->rwlock)) != 0) {
            std::cout << "error while locking page for writing to load it. Aborting!" << std::endl;
            exit(1);
        }

        //Step 3 : add it to pages
        std::pair<uint64_t, BufferFrame*> element(pageId, frame);
        pages.insert(element);

        //Step 4 : check if we need to evict an element
        BufferFrame * evict = NULL;
        if (pages.size() >= pageCount) {
            
            //Step 5 : select one and remove it from pages map
            //LOCK(evict_lock)
            if (pthread_mutex_lock(&evict_lock) != 0) {
                std::cout << "error while locking evict list. Aborting!" << std::endl;
                exit(1);
            }
            //check the elements within evict_list if we can evict them safely
            for (auto it = evict_list.begin(); it != evict_list.end(); it++) { 
                auto evict_cand_elem = pages.find(*it);
                if (evict_cand_elem != pages.end()) {
                    BufferFrame * evict_cand = evict_cand_elem->second;
                    if (evict_cand->pageId != *it) {
                        std::cout << "pageId does not match! looked for: " << *it << " but got: " << evict_cand->pageId << std::endl;
                    }
                    //check if it is currently unused;
                    if(pthread_rwlock_trywrlock(&(evict_cand->rwlock)) == 0) {
                        //this means we got the lock :-) -> we can evict this page
                        //thus remove it from pages and evict_list and store it to evict
                        pages.erase(evict_cand_elem);
                        evict_list.erase(it);
                        evict = evict_cand;
                        break;
                    }
                }
                else {
                    //OOPS: we got an element in our evict_list that is not within the pages map??
                    std::cout << "Inconsistent state: element in evict_list is not in pages map" << std::endl;
                    exit(1);
                }
            }

            //if at this point evict is still NULL we could not get an element to evict
            //thus we abort execution with an error (fail-stop)
            if (evict == NULL) {
                std::cout << "could not get page to evict! Aborting!" << std::endl;
                exit(1);
            }

            //UNLOCK(evict_lock)
            if (pthread_mutex_unlock(&evict_lock) != 0) {
                std::cout << "error while unlocking evict list. Aborting!" << std::endl;
                exit(1);
            }

        }

        //Step 6 : unlock pages (hash-map)
        //UNLOCK(pages_lock)
        if (pthread_mutex_unlock(&pages_lock) != 0) {
            std::cout << "error while unlocking pages table. Aborting!" << std::endl;
            exit(1);
        }

        //Step 7 : evict selected element if needed (store to disk. free space)
        if (evict != NULL) {
            if (evict->isDirty) {
                //If it is dirty we need to write it to disk

                //Get pid and sid from pageId
                const struct Pid * evict_pid = reinterpret_cast<const struct Pid *>(&(evict->pageId));
                //calculate the file name
                std::stringstream evict_sstm;
                evict_sstm << evict_pid->sid;
                std::string evict_fname = evict_sstm.str();
                
                //LOCK(files_lock)
                if (pthread_mutex_lock(&files_lock) != 0) {
                    std::cout << "error while locking files lock while evicting. Aborting!" << std::endl;
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
                ssize_t wsize = pwrite(fd_it->second, evict->data, pageSize, pageSize*(evict_pid->pid));
                if (wsize < 0) {
                    perror ("Could not write to file within fixPage! Aborting!");
                    exit(1);
                }
                else if (wsize != pageSize) {
                    std::cout << "could not write all data to file. Aborting!" << std::endl;
                    exit(1);
                }
                //std::cout << "successfully wrote back dirty page (" << evict->pageId << "). Write size was: " << wsize << ", pageSize is: " << pageSize << std::endl;
                //at this point the write was successfull
            }
            //Now as it was written to disk if needed we can securely delete it
            //Before doing it we need to unlock it (just not to break the lock :-)
            if(pthread_rwlock_unlock(&(evict->rwlock)) != 0) {
                // if this happens we could not unlock this page again, which is bad!
                std::cout << "unlock of page failed! Aborting!" << std::endl;
                exit(1);
            }
            delete evict;
            //just to be safe :-)
            evict = NULL;
        }
        
        //Step 5 : really load page from disk
        //Get pid and sid from pageId
        const struct Pid * pid = reinterpret_cast<const struct Pid *>(&pageId);
        //calculate the file name
        std::stringstream sstm;
        sstm << pid->sid;
        std::string fname = sstm.str();
        //check if this file was opened before and is still valid
        auto files_it = files.find(fname);
        int fd;
        if (files_it == files.end()) {
            //it was not opened before, so open it
            fd = open(fname.c_str(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
            if (fd < 0) {
                perror("Could not open file within fixPage. Aborting!");
                exit(1);
            }
            files.insert(std::pair<std::string, int> (fname, fd));
        }
        else {
            //if we found the file descriptor in our list, reuse it
            fd = files_it->second;
        }


        //read the data from the file
        ssize_t rsize = pread(fd, frame->data, pageSize, pageSize*(pid->pid));
        if (rsize < 0) {
            perror ("Could not read from file within fixPage. Aborting!");
            exit(1);
        }
        else if (rsize != pageSize) {
            //If we could not read the page from the segment file, we create an empty page
            exists = false;
            memset(frame->data, '\0', pageSize);
            frame->isDirty = true;
        }
        
        //Step 6 : add page to evict_list 
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
        
        //Step 7 : unlock page (so it can be locked as desired by caller)
        if(pthread_rwlock_unlock(&(frame->rwlock)) != 0) {
            // if this happens we could not unlock this page again, which is bad!
            std::cout << "unlock of page failed! Aborting!" << std::endl;
            exit(1);
        }
    }
    else {
        //The page was found within our buffer. This is the best case.
        cached = true;
        frame = pages_it->second;
        //UNLOCK(pages_lock)
        if (pthread_mutex_unlock(&pages_lock) != 0) {
            std::cout << "error while unlocking pages table. Aborting!" << std::endl;
            exit(1);
        }
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
        std::cout << "could not lock rwlock! Aborting! Error was: " << res << " as string: " << strerror(res) << " exclusive was: " << exclusive << std::endl;
        exit(1);
    }
//    std::cout << "FIX: " << pageId << " exclusive: " << exclusive << " cached: " << cached << " exists: " << exists << std::endl;
    return *frame;
}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    //set dirty flag (set it if it was set or it should be set now)
    if (isDirty) frame.isDirty |= isDirty;

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
    //before exiting we need to write all dirty pages to disk.
    //TODO: Think about it: is it the best way to ignore all locks on BufferFrames and simply write the content (of diry pages) to disk?
    //TODO: Think about it: is it safe not to lock anything within destructor?
    
    //write frames to disk
    for (auto it = pages.begin(); it != pages.end(); it++) { 
        BufferFrame * current = it->second;
        if (current->isDirty) {
            //If it is dirty we need to write it to disk

            //Get pid and sid from pageId
            const struct Pid * pid = reinterpret_cast<const struct Pid *>(&(current->pageId));
            //calculate the file name
            std::stringstream sstm;
            sstm << pid->sid;
            std::string fname = sstm.str();
            
            //get the file descriptor
            std::unordered_map<std::string, int>::const_iterator fd_it = files.find(fname);
            if (fd_it == files.end()) {
                // if this happens we have an inconsistent state
                // where did the frame come from if the file is not even opened?
                std::cout << "inconsistent state within files <-> buffered pages. Aborting!" << std::endl;
                exit(1);
            }
            //write the date to the file
            ssize_t wsize = pwrite(fd_it->second, current->data, pageSize, pageSize*(pid->pid));
            if (wsize < 0) {
                perror ("Could not write to file within fixPage! Aborting!");
                exit(1);
            }
            else if (wsize != pageSize) {
                std::cout << "could not write all data to file. Aborting!" << std::endl;
                exit(1);
            }
            //at this point the write was successfull
        }
        //Now as it was written to disk if needed we can securely delete it
        delete current;
    }
    //close file descriptors
    for (auto it = files.begin(); it != files.end(); it++) {
        //No error checking is done as there is no way out anyways
        close(it->second);
    }

    //destroy locks
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
