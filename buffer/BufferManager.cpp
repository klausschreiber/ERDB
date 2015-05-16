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

//#define PRINT_LOCK
#define PRINT_BASIC

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
    res = pthread_mutex_init(&output_lock, NULL);
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
#ifdef PRINT_LOCK
    pthread_mutex_lock(&output_lock);
    std::cout << "FIX: LOCKING(pages_lock): " << &pages_lock << std::endl;
    pthread_mutex_unlock(&output_lock);
#endif
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
        //Step 8 : really load page from disk
        //Step 9 : unlock page and relock as desired (read or write)
        //Step 10 : add page to evict_list
        
        //Step 1 : create Page 
        frame = new BufferFrame(pageId);

        //Step 2 : write lock it
#ifdef PRINT_LOCK
        pthread_mutex_lock(&output_lock);
        std::cout << "FIX: LOCKING(new frame->rwlock): " << &(frame->rwlock) << std::endl;
        pthread_mutex_unlock(&output_lock);
#endif
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
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: LOCKING(evict_lock): " << &evict_lock << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
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
#ifdef PRINT_LOCK
                    pthread_mutex_lock(&output_lock);
                    std::cout << "FIX: TRYLOCKING(evict_cand->latch): " << &(evict_cand->latch) << std::endl;
                    pthread_mutex_unlock(&output_lock);
#endif
                    if(pthread_rwlock_trywrlock(&(evict_cand->latch)) == 0) {
                        //this means it is not currently blocked by someone within fixPage, we still have to check if it is not used by someone else
#ifdef PRINT_LOCK
                        pthread_mutex_lock(&output_lock);
                        std::cout << "FIX: TRYLOCKING(evict_cand->rwlock): " << &(evict_cand->rwlock) << std::endl;
                        pthread_mutex_unlock(&output_lock);
#endif
                        if(pthread_rwlock_trywrlock(&(evict_cand->rwlock)) == 0) {
                            //this means we got the lock :-) -> we can evict this page
                            //thus remove it from pages and evict_list and store it to evict
                            pages.erase(evict_cand_elem);
                            evict_list.erase(it);
                            evict = evict_cand;
                            break;
                        }
                        else {
                            //free the latch, as we obtained it, but cannot evict this data set
#ifdef PRINT_LOCK
                            pthread_mutex_lock(&output_lock);
                            std::cout << "FIX: UNLOCK(evict_cand->latch): " << &(evict_cand->latch) << std::endl;
                            pthread_mutex_unlock(&output_lock);
#endif
                            if( pthread_rwlock_unlock(&(evict_cand->latch)) != 0) {
                                std::cout << "Cannot unlock rwlock. Aborting!" << std::endl;
                                exit(1);
                            }
                        }
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
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: UNLOCKING(evict_lock): " << &evict_lock << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            if (pthread_mutex_unlock(&evict_lock) != 0) {
                std::cout << "error while unlocking evict list. Aborting!" << std::endl;
                exit(1);
            }

        }

        //Step 6 : unlock pages (hash-map)
        //UNLOCK(pages_lock)
#ifdef PRINT_LOCK
        pthread_mutex_lock(&output_lock);
        std::cout << "FIX: UNLOCKING(pages_lock): " << &pages_lock << std::endl;
        pthread_mutex_unlock(&output_lock);
#endif
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
#ifdef PRINT_LOCK
                pthread_mutex_lock(&output_lock);
                std::cout << "FIX: LOCKING(files_lock): " << &files_lock << std::endl;
                pthread_mutex_unlock(&output_lock);
#endif
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
#ifdef PRINT_LOCK
                pthread_mutex_lock(&output_lock);
                std::cout << "FIX: UNLOCKING(files_lock): " << &files_lock << std::endl;
                pthread_mutex_unlock(&output_lock);
#endif
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
#ifdef PRINT_BASIC                
                pthread_mutex_lock(&output_lock);
                std::cout << "successfully wrote back dirty page (" << evict->pageId << "). Write size was: " << wsize << ", pageSize is: " << pageSize << std::endl;
                pthread_mutex_unlock(&output_lock);
#endif
                
                //at this point the write was successfull
            }
            //Now as it was written to disk if needed we can securely delete it
            //Before doing it we need to unlock it (just not to break the lock :-)
            //UNLOCK(evict->rwlock)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: UNLOCKING(evict->rwlock): " << &(evict->rwlock) << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            if(pthread_rwlock_unlock(&(evict->rwlock)) != 0) {
                // if this happens we could not unlock this page again, which is bad!
                std::cout << "unlock of page failed (rwlock)! Aborting!" << std::endl;
                exit(1);
            }
            //UNLOCK(evict->latch)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: UNLOCKING(evict->latch): " << &(evict->latch) << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            if(pthread_rwlock_unlock(&(evict->latch)) != 0) {
                // if this happens we could not unlock this page again, which is bad!
                std::cout << "unlock of page failed (latch)! Aborting!" << std::endl;
                exit(1);
            }
#ifdef PRINT_BASIC                
            pthread_mutex_lock(&output_lock);
            std::cout << "evicting and deleting page " << evict->pageId << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            delete evict;
        }
        
        //Step 8 : really load page from disk
        //Get pid and sid from pageId
        const struct Pid * pid = reinterpret_cast<const struct Pid *>(&pageId);
        //calculate the file name
        std::stringstream sstm;
        sstm << pid->sid;
        std::string fname = sstm.str();
        //check if this file was opened before and is still valid
        //LOCK(files_lock)
#ifdef PRINT_LOCK
        pthread_mutex_lock(&output_lock);
        std::cout << "FIX: LOCKING(files_lock): " << &files_lock << std::endl;
        pthread_mutex_unlock(&output_lock);
#endif
        if (pthread_mutex_lock(&files_lock) != 0) {
            std::cout << "error while locking files lock while loading. Aborting!" << std::endl;
            exit(1);
        }
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
        //UNLOCK(files_lock)
#ifdef PRINT_LOCK
        pthread_mutex_lock(&output_lock);
        std::cout << "FIX: UNLOCKING(files_lock): " << &files_lock << std::endl;
        pthread_mutex_unlock(&output_lock);
#endif
        if (pthread_mutex_unlock(&files_lock) != 0) {
            std::cout << "error unlocking files lock. Aborting!" << std::endl;
            exit(1);
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
        
        
        //Step 9 : unlock page and lock with rdlock if needed. This does not have to be checked for the frame beeing still valid (not evicted) as it is not even in the evict list.
        if (!exclusive) {
            //UNLOCK(frame->rwlock)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: UNLOCKING(frame->rwlock): " << &(frame->rwlock) << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            if(pthread_rwlock_unlock(&(frame->rwlock)) != 0) {
                // if this happens we could not unlock this page again, which is bad!
                std::cout << "unlock of page failed! Aborting!" << std::endl;
                exit(1);
            }
            //LOCK(frame->rwlock)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: LOCKING(frame->rwlock): " << &(frame->rwlock) << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            int res = pthread_rwlock_rdlock(&(frame->rwlock));
            if (res != 0) {
                std::cout << "could not lock rwlock! Aborting! Error was: " << res << " as string: " << strerror(res) << " exclusive was: " << exclusive << std::endl;
                exit(1);
            }
        }
       
         //Step 10 : add page to evict_list 
#ifdef PRINT_LOCK
        pthread_mutex_lock(&output_lock);
        std::cout << "FIX: LOCKING(evict_lock): " << &evict_lock << std::endl;
        pthread_mutex_unlock(&output_lock);
#endif
        //LOCK(evict_lock)
        if (pthread_mutex_lock(&evict_lock) != 0) {
            std::cout << "error while locking evict list. Aborting!" << std::endl;
            exit(1);
        }
        evict_list.push_back(pageId);
        //UNLOCK(evict_lock)
#ifdef PRINT_LOCK
        pthread_mutex_lock(&output_lock);
        std::cout << "FIX: UNLOCKING(evict_lock): " << &evict_lock << std::endl;
        pthread_mutex_unlock(&output_lock);
#endif
        if (pthread_mutex_unlock(&evict_lock) != 0) {
            std::cout << "error while unlocking evict list. Aborting!" << std::endl;
            exit(1);
        }
    }
    else {
        //The page was found within our buffer. This is the best case.
        // Step 1: Obtain latch so noone can evict this page from now on.
        // Step 2: Free the lock on pages hash
        // Step 3: Obtain rwlock. This might block for a while if someone else
        //          currently uses it, which does not matter as we do not hold
        //          any critical locks at this point
        
        cached = true;
        frame = pages_it->second;
        //LOCK(frame->latch)
#ifdef PRINT_LOCK
        pthread_mutex_lock(&output_lock);
        std::cout << "FIX: LOCKING(frame->latch): " << &(frame->latch) << std::endl;
        pthread_mutex_unlock(&output_lock);
#endif
        if (pthread_rwlock_tryrdlock(&(frame->latch)) != 0) {
            //OOPS we did not get the lock. This means it is currently write
            //locked by someone and will be evicted.
            //Thus we need to start over again and eventually have to load it
            //from disk again
            
            //UNLOCK(pages_lock)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: UNLOCKING(pages_lock): " << &pages_lock << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            if (pthread_mutex_unlock(&pages_lock) != 0) {
                std::cout << "Could not unlock pages_lock. Aborting!" << std::endl;
                exit(1);
            }
            //restart operation (recursive call to same function)
            return fixPage( pageId, exclusive);
        }
        else {
            //we got the latch at this point
            
            //UNLOCK(pages_lock)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: UNLOCKING(pages_lock): " << &pages_lock << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            if (pthread_mutex_unlock(&pages_lock) != 0) {
                std::cout << "error while unlocking pages table. Aborting!" << std::endl;
                exit(1);
            }

            //LOCK(frame->rwlock)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: LOCKING(frame->rwlock): " << &(frame->rwlock) << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            int res;
            if (exclusive) {
                //exclusive = wr-lock
                res = pthread_rwlock_wrlock(&(frame->rwlock));
            }
            else {
                //read only access
                res = pthread_rwlock_rdlock(&(frame->rwlock));
            }
            if (res != 0) {
                std::cout << "could not lock rwlock! Aborting! Error was: " << res << " as string: " << strerror(res) << " exclusive was: " << exclusive << std::endl;
                exit(1);
            }
            //UNLOCK(frame->latch)
#ifdef PRINT_LOCK
            pthread_mutex_lock(&output_lock);
            std::cout << "FIX: UNLOCKING(frame->latch): " << &(frame->latch) << std::endl;
            pthread_mutex_unlock(&output_lock);
#endif
            if (pthread_rwlock_unlock(&(frame->latch)) != 0) {
                std::cout << "error while unlocking latch. Aborting!" << std::endl;
                exit(1);
            }
        }
    }
    //we are done at this point. frame contains a locked (according to exclusive)
    //frame that can be returned and used savely elsewhere
#ifdef PRINT_BASIC
    pthread_mutex_lock(&output_lock);
    std::cout << "FIX: " << pageId << " exclusive: " << exclusive << " cached: " << cached << " exists: " << exists << std::endl;
    pthread_mutex_unlock(&output_lock);
#endif
    return *frame;
}

void BufferManager::unfixPage(BufferFrame& frame, bool isDirty) {
    //set dirty flag (set it if it was set or it should be set now)
    if (isDirty) frame.isDirty |= isDirty;

    //unlock data
#ifdef PRINT_LOCK
    pthread_mutex_lock(&output_lock);
    std::cout << "UNFIX: UNLOCKING(frame->rwlock): " << &(frame.rwlock) << std::endl;
    pthread_mutex_unlock(&output_lock);
#endif
    int res = pthread_rwlock_unlock(&(frame.rwlock));
    if (res != 0) {
        std::cout << "could not unlock rwlock! Aborting!" << std::endl;
        exit(1);
    }
    pthread_mutex_lock(&output_lock);
    std::cout << "UNFIX: " << frame.pageId << " isDirty was: " << isDirty << std::endl;
    pthread_mutex_unlock(&output_lock);
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
        std::cout << "Destructor: clearing page " << current->pageId << std::endl;
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
            std::cout <<  "Destructor: writing page " << current->pageId << " to disk. "  << wsize << " Bytes written" << std::endl;
        }
        //Now as it was written to disk if needed we can securely delete it
        //delete current;
    }
    //close file descriptors
    for (auto it = files.begin(); it != files.end(); it++) {
        //No error checking is done as there is no way out anyways
        close(it->second);
    }
    std::cout << "Destructor: file Handles closed" << std::endl;

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
    res = pthread_mutex_destroy(&output_lock);
    if (res != 0) {
        std::cout << "could not destroy mutex. Aborting!" << std::endl;
    }
}
