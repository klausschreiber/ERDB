#ifndef SCHEMA_MANAGER_HPP
#define SCHEMA_MANAGER_HPP

//#include "Schema.hpp"
#include <pthread.h>
#include <unordered_map>
#include <queue>
#include <stdint.h>
#include <string>
#include <iostream>

#include "../buffer/BufferManager.hpp"


struct Schema {
    uint16_t segmentId;
    uint32_t pageCount;
    std::string name;
};

class SchemaManager {
public:

    //fix a Schema to read / write it or its data. A schema has to be returned.
    //Only one process can write a Schema at a time, multiple readers possible.
    Schema& fixSchema(std::string name, const bool exclusive);

    //return a schema. This is important as it gives a possible writer the
    //oportunity to perform its action (and if you are the writer it gives the
    //others the oportunity to use the schema)
    void unfixSchema(Schema& schema, const bool isDirty);

    //Constructor. Needs to be called with a valid BufferManager.
    SchemaManager( BufferManager &bufferManager) : bufferManager(bufferManager)
        {};
private:
   
    pthread_mutex_t schema_lock;
    pthread_mutex_t free_page_lock;

    BufferManager &bufferManager;
    std::unordered_map<std::string, Schema *> schema;
    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> free_pages;   
};


int main(int argc, char ** argv) {
    std::cout << "Hello :-)" << std::endl;

}


#endif
