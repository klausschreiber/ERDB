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
    const struct Schema* const getSchema(std::string name);

    //add a Schema to used and stored Schemas.
    //This automatically assigns a new Segment id to this schema.
    int addSchema( struct Schema & schema);

    //remove a schema from stored Schemas
    int dropSchema(std::string name);

    const std::list<std::string> listTables();

    //Constructor. Needs to be called with a valid BufferManager.
    SchemaManager( BufferManager &bufferManager);
private:
   
    BufferManager &bufferManager;
    std::unordered_map<std::string, struct Schema *> schema_map;
    std::unordered_map<std::string, uint64_t> schema_page_map;
    uint64_t next_page;
    uint16_t next_segment;
};

#include <string.h>

int main(int argc, char ** argv) {
    std::cout << "Hello :-)" << std::endl;

    BufferManager* bm = new BufferManager(20);
    SchemaManager* sm = new SchemaManager(*bm);

    sm->dropSchema("test");

    int size = sizeof(struct Schema) + 1*sizeof(struct Schema::Attribute);
    struct Schema * schema = reinterpret_cast<struct Schema *>(malloc(size));
    schema->attr_count = 1;
    schema->primary_key_index[0] = 0;
    schema->primary_key_index[1] = 5;
    schema->primary_key_index[2] = 5;
    schema->primary_key_index[3] = 5;
    schema->primary_key_index[4] = 5;
    strncpy(schema->name, "test", 11);
    schema->attributes[0].type = Schema::Attribute::Type::Integer;
    strncpy(schema->attributes[0].name, "id", 3);
    schema->attributes[0].notNull = true;
 
    sm->addSchema(*schema);

    int size2 = sizeof(struct Schema) + 2*sizeof(struct Schema::Attribute);
    struct Schema * schema2 = reinterpret_cast<struct Schema *>(malloc(size2));
    schema2->attr_count = 2;
    schema2->primary_key_index[0] = 0;
    schema2->primary_key_index[1] = 5;
    schema2->primary_key_index[2] = 5;
    schema2->primary_key_index[3] = 5;
    schema2->primary_key_index[4] = 5;
    strncpy(schema2->name, "test-table", 11);
    schema2->attributes[0].type = Schema::Attribute::Type::Integer;
    strncpy(schema2->attributes[0].name, "id", 3);
    schema2->attributes[0].notNull = true;
    schema2->attributes[1].type = Schema::Attribute::Type::Varchar;
    schema2->attributes[1].length = 20;
    strncpy(schema2->attributes[1].name, "name", 5);
    schema2->attributes[1].notNull = true;
    
    sm->addSchema(*schema2);

    delete sm;
    delete bm;

    return 0;

}


#endif
