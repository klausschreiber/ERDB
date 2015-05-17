#include <string.h>
#include <iostream>
#include "../schema/SchemaManager.hpp"

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

    sm->incrementPagesCount("test-table");
    sm->incrementPagesCount("test");

    const struct Schema * schema3 = sm->getSchema("test");
    std::cout << schema3->name << ": " << schema3->page_count << std::endl;

    const struct Schema * schema4 = sm->getSchema("test-table");
    std::cout << schema4->name << ": " << schema4->page_count << std::endl;
    
    const struct Schema * nullSchema = sm->getSchema("nonexistent");
    std::cout << (nullSchema == NULL) << std::endl;

    delete sm;
    delete bm;

    return 0;

}
