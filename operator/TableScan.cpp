#include "TableScan.hpp"
#include <iostream>

TableScan::TableScan(SPSegment &spsegment) : sps(spsegment) {
}

void TableScan::open() {
    ids = sps.getTIDs();
    current = 0;
//    std::cout << "open called, got " << ids.size() << " id values" << std::endl;
}

bool TableScan::next() {
    if (current == ids.size())
        return false;
    else {
//        std::cout << "TID: " << ids[current].pid << ":" << ids[current].slot << std::endl;
        Record rec = sps.lookup(ids[current++]);
//        std::cout << "Rec (" << rec.getLen() << "): " << rec.getData() << std::endl;
        const TestSchema* row = reinterpret_cast<const TestSchema*>(rec.getData());
//        std::cout << "Integer: " << row->id << " String: " << row->name << std::endl;
        registers.clear();
        std::shared_ptr<Register> r1(new Register());
        std::shared_ptr<Register> r2(new Register());
        std::shared_ptr<Register> r3(new Register());
        std::shared_ptr<Register> r4(new Register());
        std::shared_ptr<Register> r5(new Register());
//        Register *r1, *r2, *r3, *r4, *r5;
//        r1 = new Register();
//        r2 = new Register();
//        r3 = new Register();
//        r4 = new Register();
//        r5 = new Register();
        r1->setInteger(row->id);
        r2->setString(row->firstname);
        r3->setString(row->secondname);
        r4->setInteger(row->age);
        r5->setInteger(row->favorite);
        registers.push_back(std::move(r1));
        registers.push_back(std::move(r2));
        registers.push_back(std::move(r3));
        registers.push_back(std::move(r4));
        registers.push_back(std::move(r5));
        return true;
    }
}

std::vector<std::shared_ptr<Register>> TableScan::getOutput() {
    return registers;
}

void TableScan::close() {
    
}







