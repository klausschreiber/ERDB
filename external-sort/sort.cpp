#include <iostream>
#include <sstream>
#include <cstring>
#include <tuple>
#include <queue>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

void externalSort(int fdInput, uint64_t size, int fdOutput, uint64_t memSize) {
    //First run, do the sort of individual chuncks of size == memSize
    uint64_t * buffer = (uint64_t *) malloc(memSize);
    uint64_t chunkNumber, chunkMemSize;
    ssize_t readSize;
    int fd, ret;
    chunkNumber = 0;
    //this gives the maximum allowed read size to have only full uint64_t loaded (8 byte each)
    chunkMemSize = memSize - (memSize % 8);
    while ((readSize = read(fdInput, buffer, chunkMemSize)) > 0) {
        //sort the read elements
        std::sort(buffer, buffer + readSize/sizeof(uint64_t));
        //get a file descriptor for the next temporary file
        if ((fd = open(("__sort_chunk_" + std::to_string(chunkNumber)).c_str(), O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)) < 0) {
            std::cerr << "externalSort: cannot open temporary file '__sort_chunk_" + std::to_string(chunkNumber) + "':" << strerror(errno) << std::endl;
            return;
        }
        //get some disk space for this file
        if ((ret = posix_fallocate(fd, 0, readSize)) != 0) {
            std::cerr << "externalSort: cannot allocite disk space for temporary file '__sort_chunk_" + std::to_string(chunkNumber) + "':" << strerror(ret) << std::endl;
            std::cerr << "externalSort: proceeding anyways, even thou this might be slow." << std::endl;
        }
        //write content to temporary file
        if ((ret = write(fd, buffer, readSize)) != readSize) {
            if (ret < 0) {
                std::cerr << "externalSort: error writing to temporary file '__sort_chunk_" + std::to_string(chunkNumber) + "':" << strerror(errno) << std::endl;
                if (close(fd) < 0) {
                    std::cerr << "externalSort: cannot close temporary file '__sort_chunk_" + std::to_string(chunkNumber) + "':" << strerror(errno) << std::endl;
                    return;
                }
            }
            else {
                std::cerr << "externalSort: write aborted prematurely after writing " << ret << " Bytes (of " << readSize << " Bytes to temporary file '__sort_chunk_" + std::to_string(chunkNumber) + "':" << std::endl;
                return;
            }
        }
        //close the temporary file
        if (close(fd) < 0) {
            std::cerr << "externalSort: cannot close temporary file '__sort_chunk_" + std::to_string(chunkNumber) + "':" << strerror(errno) << std::endl;
            return;
        }
        //increment counter for chunkNumber
        chunkNumber++;
    }
    //second run: now perform the merge sort of all the chunks
    //we assume that the number of files is relatively small and thus the queue does not use to much memory
    //i.e. this means we still can use the memory for the real buffers.
    //within the queue each element represents: <current smallest element of this buffer>,<file descriptor>,<buffer begin>,<buffer pos>,<last read size>
    struct compare  
    {  
        bool operator()(const std::tuple<uint64_t, int, uint64_t *, ssize_t, ssize_t>& l, const std::tuple<uint64_t, int, uint64_t *, ssize_t, ssize_t>& r)  
        {  
            return std::get<0>(l) > std::get<0>(r);  
        }  
    }; 

    std::priority_queue<std::tuple<uint64_t, int, uint64_t *, ssize_t, ssize_t >, std::vector<std::tuple<uint64_t, int, uint64_t *, ssize_t, ssize_t>>, compare> filequeue;
    uint64_t mergeMemSize = memSize / (chunkNumber + 1) - (memSize / (chunkNumber + 1)) % 8;
    if (mergeMemSize < 8) {
        std::cerr << "externalSort: resulting Buffer size per File of k-Way sort to small! Abortig merge sort!" << std::endl;
        return;
    }
    //reserve size on disk for output file
    if ((ret = posix_fallocate(fdOutput, 0, size * 8)) != 0) {
        std::cerr << "externalSort: cannot allocate disk space for output file: " << strerror(errno) << std::endl;
        return;
    }
    //open all the files and fill the priority_queue
    for (uint64_t i = 0; i < chunkNumber; i++)
    {
        fd = open(("__sort_chunk_" + std::to_string(i)).c_str(), O_RDONLY);
        uint64_t * bufferStart = buffer + i * mergeMemSize / 8;
        readSize = read(fd, bufferStart, mergeMemSize)/8;
        if (readSize > 0) {
            filequeue.push(std::tuple<uint64_t, int, uint64_t *, ssize_t, ssize_t>(bufferStart[0], fd, bufferStart, 0, readSize));
        }
    }
    //initialize the write buffer (this is the position we write to, until we reach buffer + (chunkNumber + 1) * mergeMemSize which should mark the end
    uint64_t * writeBuffer = buffer + chunkNumber * mergeMemSize / 8;
    uint64_t * writeBufferEnd = buffer + (chunkNumber + 1) * mergeMemSize / 8;

    //now we start to always pick one element out of the queue until we are done;
    while (!filequeue.empty())
    {
        //get the next element
        std::tuple<uint64_t, int, uint64_t *, ssize_t , ssize_t> current = filequeue.top();
        filequeue.pop();
        //write one value to writeBuffer
        *writeBuffer++ = std::get<0>(current);
        //update tuple
        fd = std::get<1>(current);
        uint64_t * bufferStart = std::get<2>(current);
        ssize_t offset = std::get<3>(current);
        readSize = std::get<4>(current);
        offset++;
        if (offset == readSize){
            //in this case we need to load more data from disk
            readSize = read(fd, bufferStart, mergeMemSize)/8;
            if (readSize > 0) {
                filequeue.push(std::tuple<uint64_t, int, uint64_t *, ssize_t, ssize_t>(bufferStart[0], fd, bufferStart, 0, readSize));
            }
            else {
                // close this fd (we are done with this one file!)
                if (close(fd) < 0) {
                    std::cerr << "externalSort: cannot close temporary file:" << strerror(errno) << std::endl;
                    std::cerr << "externalSort: ignoring this even thou this might be dangerous" << std::endl;
                }
            }
        }
        else {
            //in this case we do not have to load a new chunk of data, simply update the tuple
            filequeue.push(std::tuple<uint64_t, int, uint64_t *, ssize_t, ssize_t>(bufferStart[offset], fd, bufferStart, offset, readSize));
        }

        // check if we need to write part of the data to disk
        if (writeBuffer >= writeBufferEnd) {
            //write content to temporary file, &buffer[chunkNumber * mergeMemSize /8] represents the start of the write buffer
            if ((ret = write(fdOutput, &buffer[chunkNumber * mergeMemSize / 8], mergeMemSize)) != (ssize_t) mergeMemSize) {
                if (ret < 0) {
                    std::cerr << "externalSort: error writing to output file:" << strerror(errno) << std::endl;
                    //TODO: cleanup before this return might be needed!
                    return;
                }
                else {
                    std::cerr << "externalSort: write aborted prematurely after writing " << ret << " Bytes (of " << mergeMemSize << " Bytes to output file)" << std::endl;
                    //TODO: cleanup before this return might be needed!
                    return;
                }
            }
            //reset the buffer
            writeBuffer = buffer + chunkNumber * mergeMemSize / 8;
        }
    }
    //last write out all that is left in the write buffer
    ssize_t remainderSize = (char *) writeBuffer - (char *) (buffer + chunkNumber * mergeMemSize / 8);
    if ((ret = write(fdOutput, &buffer[chunkNumber * mergeMemSize / 8], remainderSize)) != remainderSize) { 
        if (ret < 0) {
            std::cerr << "externalSort: error writing to output file:" << strerror(errno) << std::endl;
            return;
        }
        else {
            std::cerr << "externalSort: write aborted prematurely after writing " << ret << " Bytes (of " << mergeMemSize << " Bytes to output file)" << std::endl;
            return;
        }
    }
}

/// helper to cleanup fd and print error message if it failed 
void inline cleanupFd(int fd) {
    if (close(fd) < 0) {
        std::cerr << "cannot close inputFile: " << strerror(errno) << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "usage: sort <inputFile> <outputFile> <memoryBufferInMB>" << std::endl;
        return -1;
    }
    else if (strcmp(argv[1], argv[2]) == 0){
        std::cout << "inputFile and outputFile must not be the same file!" << std::endl;
        return -1;
    }
    else {
        //-------------------
        //GENERATE INPUT DATA
        //-------------------
        int fdInput, fdOutput;
        uint64_t memSize;
        uint64_t size;

        //open input File
        if ((fdInput = open(argv[1], O_RDONLY)) < 0) {
            std::cerr << "cannot open inputFile: " << strerror(errno) << std::endl;
            return -1;
        }

        //open output File
        if ((fdOutput = open(argv[2], O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)) < 0) {
            std::cerr << "cannot open outputFile: " << strerror(errno) << std::endl;
            cleanupFd(fdInput);
            return -1;
        }

        //get memory size (in Byte)
        std::stringstream s(argv[3]);
        s >> memSize;
        memSize *= 1024*1024;
        if (memSize == 0) {
            std::cerr << "Invalid buffer size given (" << argv[3] << " Bytes)! Exiting!" << std::endl;
            cleanupFd(fdInput);
            cleanupFd(fdOutput);
            return -1;
        }

        //get the number of elements within the infile
        struct stat st;
        if (fstat(fdInput, &st)) {
            std::cerr << "Could not get stat of infile, thus sort cannot be executed!" << std::endl;
            cleanupFd(fdInput);
            cleanupFd(fdOutput);
            return -1;
        }
        size = st.st_size / 8;

        //be verbose (output the used input and output files and the buffer size)
        std::cout << "Sorting " << argv[1] << " (containing "<< size << " elements) into " << argv[2] << " using a buffer of size " << memSize << "Bytes." << std::endl;

        //----------------
        //PERFORM TEST RUN
        //----------------
        externalSort(fdInput, size, fdOutput, memSize);

        //cleanup (close file descriptors)
        cleanupFd(fdInput);
        cleanupFd(fdOutput);

        //-------------------------
        //CHECK SUCCESS OF TEST RUN
        //-------------------------
        int fdResult;
        int failCount = 0;

        //open result file
        if ((fdResult = open(argv[2], O_RDONLY)) < 0) {
            std::cerr << "cannot open resultFile: " << strerror(errno) << std::endl;
            return -1;
        }

        //first test: get stat of file to check if size is still the same
        if (fstat(fdResult, &st)) {
            std::cerr << "Could not get stat of resultFile!" << std::endl;
            cleanupFd(fdResult);
            return -1;
        }

        if (size != (uint64_t) (st.st_size /8)) {
            std::cout << "Size of result Elements (" << st.st_size / 8 << ") does not match original size(" << size << ")! Testing order anyways." << std::endl;
            failCount ++;
        }
        else {
            std::cout << "Size test done" << std::endl;
        }

        //next test: check if elements are in ascending order
        uint64_t leftover = 0;
        uint64_t * buffer = (uint64_t *) malloc(memSize);
        ssize_t ret;

        while ((ret = read(fdResult, buffer, memSize)) > 0) {
            for (int i = 0; i < (ret/8); i++) {
                //first element of current buffer
                if (i == 0 && buffer[i] < leftover) {
                    std::cout << "Error in result found: " << buffer[i] << " is following " << leftover << ", but it is smaller!LEFTOVER!!!!" << std::endl;
                    failCount++;
                }
                else {
                    if (buffer[i] < buffer[i-1]) {
                        std::cout << "Error in result found: " << buffer[i] << " is following " << buffer[i-1] << ", but it is smaller!MIDDLE!!!!" << i << std::endl;
                        failCount++;
                    }
                    if (i == (ret/8 - 1)) {
                        leftover = buffer[i];
                    }
                }
            }
        }
        std::cout << "Order test done" << std::endl;
        std::cout << "Total number of failures found: " << failCount << std::endl;
        cleanupFd(fdResult);
        free(buffer);
        if (failCount)
            return -1;
    }
    return 0;
}
