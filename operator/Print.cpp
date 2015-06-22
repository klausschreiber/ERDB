#include "Print.hpp"

Print::Print( Operator &in_op, std::ostream &ostr) :
        input(in_op), ostream(ostr) {

    }

void Print::open() {
    input.open();
}

bool Print::next() {
    if (!input.next())
        return false;
    registers = input.getOutput();
    ostream << "Tuple:\t";
    for (auto it = registers.begin(); it != registers.end(); it++) {
        switch ((*it)->getType()){
            case Register::Integer:
                ostream << "(int): " << (*it)->getInteger() << "\t";
                break;
            case Register::String:
                ostream << "(str): " << (*it)->getString() << "\t";
                break;
            default:
                ostream << "(unknown)\t";
        }
    }
    ostream << std::endl;
    return true;
}

std::vector<Register*> Print::getOutput() {
    return registers;
}

void Print::close() {
    input.close();
}

