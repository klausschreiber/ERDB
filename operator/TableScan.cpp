#include "TableScan.hpp"
#include <iostream>

TableScan::TableScan(SPSegment &spsegment, std::map<int, TableScan::Type> &data_schema) :
        sps(spsegment), schema(data_schema) {
}

void TableScan::open() {
    ids = sps.getTIDs();
    current = 0;
}

bool TableScan::next() {
    if (current == ids.size())
        return false;
    else {
        registers.clear();
        Record rec = sps.lookup(ids[current++]);
        const char * data = rec.getData();
        for (auto it = schema.begin(); it != schema.end(); it++) {
            std::shared_ptr<Register> r(new Register());
            switch (it->second) {
                case Integer:
                    r->setInteger(*(reinterpret_cast<const int*> (data+it->first)));
                    break;
                case String:
                    r->setString(data+it->first);
                    break;
            }
            registers.push_back(std::move(r));
        }
        return true;
    }
}

std::vector<std::shared_ptr<Register>> TableScan::getOutput() {
    return registers;
}

void TableScan::close() {
    
}
