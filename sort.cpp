#include <iostream>
#include <sstream>
#include <cstring>
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
    /// First run, do the sort of individual chuncks of size <= memSize
    uint64_t * buffer = (uint64_t *) malloc(memSize);
    uint64_t readSize = read(fdInput, buffer, memSize);
    uint64_t elementCount = readSize/sizeof(uint64_t);
    std::sort(buffer, buffer + (elementCount-1)*sizeof(uint64_t));
    int fd, ret;
    if ((fd = open("__sort_chunk_1", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)) < 0) {
        std::cerr << "cannot open temporary file '__sort_chunk_1': " << strerror(errno) << std::endl;
        return;
    }
    if ((ret = posix_fallocate(fd, 0, elementCount*sizeof(uint64_t))) != 0)
        std::cerr << "warning: could not allowate file space: " << strerror(ret) << std::endl;
    if ((ret = write(fd, buffer, elementCount*sizeof(uint64_t))) != elementCount*sizeof(uint64_t)) {
        if (ret < 0)
            std::cerr << "cannot write temporary file: " << strerror(ret) << std::endl;
        else
            std::cerr << "less data written than expected: wrote " << ret << "/" << elementCount*sizeof(uint64_t) << " Bytes" << std::endl;
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
        //externalSort(fdInput, size, fdOutput, memSize);

        //cleanup (close file descriptors)
        cleanupFd(fdInput);
        cleanupFd(fdOutput);

        //-------------------------
        //CHECK SUCCESS OF TEST RUN
        //-------------------------
        int fdResult;

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
        }

        //next test: check if elements are in ascending order
        uint64_t leftover = 0;
        uint64_t * buffer = (uint64_t *) malloc(memSize);
        ssize_t ret;

        while ((ret = read(fdResult, buffer, memSize)) > 0) {
            for (int i = 0; i < (ret/8); i++) {
                //first element of current buffer
                if (i == 0 && buffer[i] < leftover) {
                    std::cout << "Error in result found: " << buffer[i] << " is following " << leftover << ", but it is smaller!" << std::endl;
                }
                else {
                    if (buffer[i] < buffer[i-1]) {
                        std::cout << "Error in result found: " << buffer[i] << " is following " << buffer[i-1] << ", but it is smaller!" << std::endl;
                    }
                    if (i == (ret/8 - 1)) {
                        leftover = buffer[i];
                    }
                }
            }
        }

        free(buffer);

    }
    return 0;
}
