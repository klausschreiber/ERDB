#include "../operator/Register.hpp"
#include "../operator/TableScan.hpp"
#include "../operator/Print.hpp"
#include "../operator/Projection.hpp"
#include "../operator/Selection.hpp"
#include "../operator/HashJoin.hpp"
#include <iostream>
#include <memory>

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
    

    //add the schema for all further tests.
    //This is not the real schema used, as only its name will be used to reference it
    //and identify the segment. The other values are stored elsewere
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

    //add some data using a struct for convenience

    struct TestSchema {
        int id;
        char firstname[20];
        char secondname[20];
        int age;
        int favorite;
    };

    char* firstname[5] = {"Hans", "Peter", "Dieter", "Moritz", "Max"};
    char* secondname[5] = {"Maier", "Mueller", "Huber", "Schmidt", "Gruber"};
    int age[5] = {17, 25, 17, 40, 17};
    int favorite[5] = { 7, 7, 42, 42, 9000};
    struct TestSchema row = {};

    for( int i = 0 ; i<5; i++) {
        row.id = i;
        memset(row.firstname, 0, 20);
        memset(row.secondname, 0, 20);
        memcpy(row.firstname, firstname[i], strlen(firstname[i]));
        memcpy(row.secondname, secondname[i], strlen(secondname[i]));
        row.age = age[i];
        row.favorite = favorite[i];
        Record *r = new Record(sizeof(struct TestSchema), (char *) &row);
        sps->insert(*r);
    }

    //create the corresponding schema map (for the table scan)
    
    std::map<int, TableScan::Type> tableschema;
    tableschema.insert(std::make_pair(0, TableScan::Integer));
    tableschema.insert(std::make_pair(4, TableScan::String));
    tableschema.insert(std::make_pair(24, TableScan::String));
    tableschema.insert(std::make_pair(44, TableScan::Integer));
    tableschema.insert(std::make_pair(48, TableScan::Integer));

    //initiate a table scan
   
    std::cout << "initializing table scan... " << std::endl;
 
    TableScan ts(*sps, tableschema);

    std::cout << "open table scan" << std::endl;
    ts.open();
    
    while(ts.next()) {
        std::vector<std::shared_ptr<Register>> regs = ts.getOutput();
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

    // testing Print
    
    std::cout << "Testing Print operator..." << std::endl;

    ts.close();

    Print p(ts ,std::cout);
    p.open();
    while(p.next())
        continue;

    p.close();

    std::cout << "Print testing done" << std::endl;
    
    //testing projection
    std::cout << "Testing projection" << std::endl;

    std::vector<int> project;
    project.push_back(0);
    project.push_back(3);

    Projection proj(p, project);
    Print p2(proj, std::cout);

    p2.open();

    while(p2.next())
        continue;

    p2.close();

    std::cout << "Projection testing done" << std::endl;

    //testing selection
    std::cout << "Testing selection" << std::endl;

    Selection sel(p2, 1, 17);
    Print p3(sel, std::cout);

    p3.open();

    while(p3.next())
        continue;

    p3.close();

    
    std::cout << "Selection test done" << std::endl;
    
    //hash join test
    
    std::cout << "Testing hash join" << std::endl;

    TableScan ts2(*sps, tableschema);

    HashJoin h(p3, ts2, 1, 3);

    Print p4(h, std::cout);

    p4.open();

    while(p4.next())
        continue;

    p4.close();


    delete (sps);
    delete (sm);
    delete (bm);
 
 
};
