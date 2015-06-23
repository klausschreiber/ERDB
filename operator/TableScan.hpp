#ifndef TABLE_SCAN_HPP
#define TABLE_SCAN_HPP

#include "Operator.hpp"
#include "../slottedpages/SPSegment.hpp"
#include <memory>
#include <map>


class TableScan : public Operator {
public:
    enum Type {Integer, String};
;
    //initialize table scan using a SPSegment and a map representing the schema.
    //This can be interpreted as: (first, second) -> at offset <first> there is a value of type second, where second is either an int or a char[20].
    TableScan( SPSegment &spsegment, std::map<int, TableScan::Type> &data_schema);
    virtual void open();
    virtual bool next();
    virtual std::vector<std::shared_ptr<Register>> getOutput();
    virtual void close();
private:
    SPSegment &sps;
    std::vector<TID> ids;
    unsigned int current;
    std::vector<std::shared_ptr<Register>> registers;
    std::map<int,Type> schema;

};

#endif
