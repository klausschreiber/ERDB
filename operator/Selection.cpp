#include "Selection.hpp"

Selection::Selection( Operator &in_op, unsigned int reg_id, char * value) :
        input(in_op), id(reg_id) {
    compare.setString(value);
};

Selection::Selection( Operator &in_op, unsigned int reg_id, int value) :
        input(in_op), id(reg_id) {
    compare.setInteger(value);
};

Selection::Selection( Operator &in_op, unsigned int reg_id, Register value) : 
        input(in_op), id(reg_id), compare(value) {
};

void Selection::open(){
    input.open();
}

bool Selection::next(){
    while (input.next()) {
        registers = input.getOutput();
        if (*(registers[id]) == compare)
            return true;
    }
    return false;
}

std::vector<std::shared_ptr<Register>> Selection::getOutput(){
    return registers;
}

void Selection::close() {
    input.close();
}
