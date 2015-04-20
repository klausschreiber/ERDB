#include <iostream>
#include <sstream>
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

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "You need to specify all Parameters!" << std::endl;
        std::cout << "try: sort <inputFile> <outputFile> <memoryBufferInMB>" << std::endl;
    }
    else {
        int fdInput, fdOutput;
        uint64_t memSize;
        
        //open input File
        if ((fdInput = open(argv[1], O_RDONLY)) < 0) {
            std::cerr << "cannot open inputFile: " << strerror(errno) << std::endl;
            return -1;
        }

        //open output File
        if ((fdOutput = open(argv[2], O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR)) < 0) {
            std::cerr << "cannot open outputFile: " << strerror(errno) << std::endl;
            if (close(fdInput) < 0) {
                std::cerr << "cannot close inputFile: " << strerror(errno) << std::endl;
            }
            return -1;
        }

        //get memory size (in Byte)
        std::stringstream s(argv[3]);
        s >> memSize;
        memSize *= 1024*1024;

        //do the test run
        
        std::cout << memSize << "Bytes used as buffer" << std::endl;

        //cleanup (close file descriptors)
        
        if (close(fdInput) < 0) {
            std::cerr << "cannot close inputFile: " << strerror(errno) << std::endl;
        }
        if (close(fdOutput) < 0) {
            std::cerr << "cannot close OutputFile: " << strerror(errno) << std::endl;
        }
    }
    std::cout << "Program was run!" << argc << std::endl;
    return 0;
}
