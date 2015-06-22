#include "TableScan.hpp"
#include <iostream>

TableScan::TableScan(SPSegment &spsegment) : sps(spsegment) {
}

void TableScan::open() {
    ids = sps.getTIDs();
    current = 0;
    std::cout << "open called, got " << ids.size() << " id values" << std::endl;
}

bool TableScan::next() {
    if (current == ids.size())
        return false;
    else {
        const TestSchema* row = reinterpret_cast<const TestSchema*>(sps.lookup(ids[current++]).getData());
        std::cout << "Integer: " << row->id << " String: " << row->name << std::endl;
        registers.clear();
        Register *r1, *r2;
        r1 = new Register();
        r2 = new Register();
        r1->setInteger(row->id);
        r2->setString(row->name);
        registers.push_back(r1);
        registers.push_back(r2);
        return true;
    }
}

std::vector<Register*> TableScan::getOutput() {
    return registers;
}

void TableScan::close() {
    
}







