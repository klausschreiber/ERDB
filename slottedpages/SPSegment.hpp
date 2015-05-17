#ifndef SPSEGMENT_HPP
#define SPSEGMENT_HPP

#include <string>

#include "../schema/Schema.hpp"
#include "../schema/SchemaManager.hpp"
#include "Record.hpp"
#include "TID.hpp"

class SPSegment {
public:
    //insert record    
    TID insert(const Record& r);

    //remove record
    bool remove(TID tid);

    //lookup record (read only)
    Record lookup(TID tid);

    //update record at TID with content of r
    bool update(TID tid, const Record& r);

    //constructor. Needs a SchemaManager to work with the schema and the name
    //of the schema to handle
    SPSegment ( BufferManager& bm, SchemaManager& sm, std::string schema_name);

private:

    //the used schema
    const struct Schema * schema;
    //schema manager to interact with
    SchemaManager& sm;
    //buffer manager to interact with
    BufferManager& bm;
};


#endif
