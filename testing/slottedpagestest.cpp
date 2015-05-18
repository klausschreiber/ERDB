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

    /*char * data = "my test data at medium size";
    Record *r = new Record(28, data);
    TID tid1 = sps->insert(*r);
    TID tid11 = sps->insert(*r);
*/
    char * data2 = reinterpret_cast<char*>(malloc(512));
    memset(data2, 'X', 512);
    data2[0] = 'A';
    data2[511] = 0;
    Record *r2 = new Record(512, data2);
    TID tid2 = sps->insert(*r2);
    

  /*  char * data3 = "my longer test data which might not fit in here...";
    Record *r3 = new Record(51, data3);
    sps->update(tid1, *r3);

    char * data4 = "my tiny test";
    Record *r4 = new Record(13, data4);
    sps->update(tid11, *r4);
*/
    char * data5 = reinterpret_cast<char*>(malloc(12000));
    memset(data5, 'O', 12000);
    data5[0] = 'a';
    data5[12000] = 0;
    Record *r5 = new Record(12000, data5);
    
    char * data6 = reinterpret_cast<char*>(malloc(3000));
    memset(data6, 'H', 3000);
    data6[0] = 'i';
    data6[3000] = 0;
    Record *r6 = new Record(3000, data5);

    sps->update(tid2, *r5);

    //TID tid6 = sps->insert(*r6);

    //sps->update(tid2, *r2);

    //std::cout << sps->lookup(tid2).getData() << std::endl;

    //sps->remove(tid2);

    //std::cout << sps->lookup(tid6).getData() << std::endl;


    delete sm;
    delete bm;

    return 0;

}
