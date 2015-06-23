#include "Projection.hpp"

Projection::Projection( Operator &in_op, std::vector<int> reg_ids) :
        input(in_op), register_ids(reg_ids) {

};

void Projection::open() {
    input.open();
}

bool Projection::next() {
    if (!input.next())
        return false;
    std::vector<std::shared_ptr<Register>> in = input.getOutput();
    registers.clear();
    //no check of out of range is done as we assume the caller knows what he is doing
    for( auto it = register_ids.begin(); it != register_ids.end(); it++) {
        registers.push_back(in[*it]);
    }
    return true;
}

std::vector<std::shared_ptr<Register>> Projection::getOutput() {
    return registers;
}

void Projection::close() {
    input.close();
}


