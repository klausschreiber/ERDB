#include <string.h>
#include <iostream>
#include "../schema/SchemaManager.hpp"
#include "../slottedpages/SPSegment.hpp"

int main(int argc, char ** argv) {
    std::cout << "Hello :-)" << std::endl;

    BufferManager* bm = new BufferManager(20);
    SchemaManager* sm = new SchemaManager(*bm);

    int size = sizeof(struct Schema) + 2*sizeof(struct Schema::Attribute);
    struct Schema * schema = reinterpret_cast<struct Schema *>(malloc(size));
    schema->attr_count = 2;
    schema->primary_key_index[0] = 0;
    schema->primary_key_index[1] = 5;
    schema->primary_key_index[2] = 5;
    schema->primary_key_index[3] = 5;
    schema->primary_key_index[4] = 5;
    strncpy(schema->name, "test-table", 11);
    schema->attributes[0].type = Schema::Attribute::Type::Integer;
    strncpy(schema->attributes[0].name, "id", 3);
    schema->attributes[0].notNull = true;
    schema->attributes[1].type = Schema::Attribute::Type::Varchar;
    schema->attributes[1].length = 20;
    strncpy(schema->attributes[1].name, "name", 5);
    schema->attributes[1].notNull = true;
    
    sm->addSchema(*schema);

    SPSegment* sps = new SPSegment(*bm, *sm, "test-table");

    char * data = "my test data";
    Record *r = new Record(13, data);
    sps->insert(*r);


    delete sm;
    delete bm;

    return 0;

}
