#ifndef SCHEMA_MANAGER_HPP
#define SCHEMA_MANAGER_HPP

#include "Schema.hpp"
#include <pthread.h>
#include <unordered_map>
#include <queue>
#include <stdint.h>
#include <string>
#include <iostream>
#include <list>

#include "../buffer/BufferManager.hpp"


//this class is NOT Thread safe (as it was not requested!)
class SchemaManager {
public:

    //get the Schema (for reading only! any modification is undefined)
    const struct Schema* const getSchema(const std::string name);

    //add a Schema to used and stored Schemas.
    //This automatically assigns a new Segment id to this schema and resets the
    //page_count, thus restarting with an empty schema
    //readding a schema === truncate table :-)
    int addSchema( struct Schema & schema);

    //remove a schema from stored Schemas
    int dropSchema(const std::string name);

    //update the schema pages count (needs to be updated after insert of data)
    int incrementPagesCount(const std::string name);

    const std::list<std::string> listTables();

    //Constructor. Needs to be called with a valid BufferManager.
    SchemaManager( BufferManager &bufferManager);
private:
   
    BufferManager &bufferManager;
    //TODO: think about buffering schemas in memory vs loading from disk all the time
    std::unordered_map<std::string, struct Schema *> schema_map;
    std::unordered_map<std::string, uint64_t> schema_page_map;
    uint64_t next_page;
    uint16_t next_segment;
};

#endif
