#ifndef TABLE_SCAN_HPP
#define TABLE_SCAN_HPP

#include "Operator.hpp"
#include "../slottedpages/SPSegment.hpp"


//for now it is hardcoded to use TestSchema as schema. Of course this should be implemented as a template later on (to be able to work with different table schemas)
class TableScan : Operator {
public:
    struct TestSchema {
        int id;
        char name[20];
    };
    TableScan( SPSegment &spsegment);
    virtual void open();
    virtual bool next();
    virtual std::vector<Register*> getOutput();
    virtual void close();
private:
    SPSegment &sps;
    std::vector<TID> ids;
    unsigned int current;
    std::vector<Register*> registers;

};

#endif
