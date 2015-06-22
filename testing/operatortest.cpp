#include "../operator/Register.hpp"
#include "../operator/TableScan.hpp"
#include <iostream>

int main( int argc, char** argv){
    std::cout << "OPERATOR TEST RUN" << std::endl;

    std::cout << "Register tests..." << std::endl;
    Register r;
    std::cout << r.getInteger() << std::endl;
    std::cout << r.getString() << std::endl;
    r.setInteger(42);
    std::cout << r.getInteger() << std::endl;
    r.setString("Test String");
    std::cout << r.getString() << std::endl;
    r.setInteger(42);
    std::cout << r.getInteger() << std::endl;
    r.setString("too long string to fit into the 20 charcter limit");
    std::cout << r.getString() << std::endl;

    std::cout << "Comparisons" << std::endl;

    r.setInteger(42);
    Register s;
    s.setInteger(42);
    std::cout << r.getInteger() << "<" << s.getInteger() << ":" << (r < s) << "  " << r.getInteger() << "==" << s.getInteger() << ":" << (r==s) << std::endl;
    s.setInteger(50);
    std::cout << r.getInteger() << "<" << s.getInteger() << ":" << (r < s) << "  " << r.getInteger() << "==" << s.getInteger() << ":" << (r==s) << std::endl;
    s.setInteger(40);
    std::cout << r.getInteger() << "<" << s.getInteger() << ":" << (r < s) << "  " << r.getInteger() << "==" << s.getInteger() << ":" << (r==s) << std::endl;
   
    s.setString("string 1");
    r.setString("string 1");
    std::cout << r.getString() << "<" << s.getString() << ":" << (r < s) << "  "<< r.getString() << "==" << s.getString() << ":" << (r==s) << std::endl;
    s.setString("string 2");
    std::cout << r.getString() << "<" << s.getString() << ":" << (r < s) << "  "<< r.getString() << "==" << s.getString() << ":" << (r==s) << std::endl;
    s.setString("string 2");
    s.setString("string 0");
    std::cout << r.getString() << "<" << s.getString() << ":" << (r < s) << "  "<< r.getString() << "==" << s.getString() << ":" << (r==s) << std::endl;
    s.setString("string 2");
    s.setString("string 1 but longer");
    std::cout << r.getString() << "<" << s.getString() << ":" << (r < s) << "  "<< r.getString() << "==" << s.getString() << ":" << (r==s) << std::endl;
    s.setString("string 2");


    std::cout << std::endl << "TableScan test..." << std::endl;
    
    BufferManager* bm = new BufferManager(20);
    SchemaManager* sm = new SchemaManager(*bm);
    

    //add the schema for all further tests
    int size = sizeof(struct Schema) + 2*sizeof(struct Schema::Attribute);
    struct Schema * schema = reinterpret_cast<struct Schema *>(malloc(size));
    schema->attr_count = 2;
    schema->primary_key_index[0] = 0;
    schema->primary_key_index[1] = 5;
    schema->primary_key_index[2] = 5;
    schema->primary_key_index[3] = 5;
    schema->primary_key_index[4] = 5;
    strncpy(schema->name, "operator-table", 15);
    schema->attributes[0].type = Schema::Attribute::Type::Integer;
    strncpy(schema->attributes[0].name, "id", 3);
    schema->attributes[0].notNull = true;
    schema->attributes[1].type = Schema::Attribute::Type::Varchar;
    schema->attributes[1].length = 20;
    strncpy(schema->attributes[1].name, "name", 5);
    schema->attributes[1].notNull = true;

    sm->addSchema(*schema); 
    

    SPSegment* sps = new SPSegment(*bm, *sm, "operator-table");

    //add some data

    char* strings[5] = {"test1", "test2", "test very long", "short", "blork"};
    struct TableScan::TestSchema row = {};        

    for( int i = 0 ; i<5; i++) {
        row.id = i;
        memcpy(row.name, strings[i], strlen(strings[i]));
        Record *r = new Record(sizeof(struct TableScan::TestSchema), (char *) &row);
        sps->insert(*r);
    }

    //initiate a table scan
   
    std::cout << "initializing table scan... " << std::endl;
 
    TableScan ts(*sps);

    std::cout << "open table scan" << std::endl;
    ts.open();
    
    while(ts.next()) {
        std::vector<Register*> regs = ts.getOutput();
        std::cout << "got output:" <<std::endl;
        for(auto it = regs.begin(); it != regs.end(); it++) {
            if ((*it)->getType() == Register::Integer) {
                std::cout << "Integer: " << (*it)->getInteger() << std::endl;
            }
            else {
                std::cout << "String: " << (*it)->getString() << std::endl;
            }   
        }
    }    

    delete (sps);
    delete (sm);
    delete (bm);
 
 
};
