#ifndef TABLE_SCAN_HPP
#define TABLE_SCAN_HPP

#include "Operator.hpp"
#include "../slottedpages/SPSegment.hpp"
#include <memory>


//for now it is hardcoded to use TestSchema as schema. Of course this should be implemented as a template later on (to be able to work with different table schemas)
class TableScan : public Operator {
public:
    struct TestSchema {
        int id;
        char firstname[20];
        char secondname[20];
        int age;
        int favorite;
    };
    TableScan( SPSegment &spsegment);
    virtual void open();
    virtual bool next();
    virtual std::vector<std::shared_ptr<Register>> getOutput();
    virtual void close();
private:
    SPSegment &sps;
    std::vector<TID> ids;
    unsigned int current;
    std::vector<std::shared_ptr<Register>> registers;

};

#endif
