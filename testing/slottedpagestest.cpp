#include <string.h>
#include <iostream>
#include "../schema/SchemaManager.hpp"
#include "../slottedpages/SPSegment.hpp"
#include "../buffer/PID.hpp"

int main(int argc, char ** argv) {
    std::cout << "Hello :-)" << std::endl;

    std::cout << "slottedPage: " << sizeof(SlottedPage) << std::endl;
    std::cout << "Header: " << sizeof(SlottedPage::Header) << std::endl;
    std::cout << "Slot: " << sizeof(SlottedPage::Slot) << std::endl;
    std::cout << "TID: " << sizeof(TID) << std::endl;
    std::cout << "Local: " << sizeof(SlottedPage::Slot::Local) << std::endl;
    std::cout << "PID: " << sizeof(PID) << std::endl;

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
    TID tid1 = sps->insert(*r);

    char * data2 = reinterpret_cast<char*>(malloc(512));
    memset(data2, 'X', 512);
    data2[0] = 'A';
    data2[511] = 'O';
    Record *r2 = new Record(512, data2);
    TID tid2 = sps->insert(*r2);

    char * data3 = "my test";
    Record *r3 = new Record(8, data3);
    TID tid3 = sps->insert(*r3);

    sps->remove(tid2);

    std::cout << sps->lookup(tid1).getData() << std::endl;

    sps->update(tid3, *r3);
    std::cout << sps->lookup(tid3).getData() << std::endl;

    delete sm;
    delete bm;

    return 0;

}
