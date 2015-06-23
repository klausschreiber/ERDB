#ifndef SELECTION_HPP
#define SELECTION_HPP

#include "Operator.hpp"
#include <memory>

//print operator. It prints its input to stdout and forwards it.
//thus it is safe to use its getOutput() which simply passe th eretreived data on to the next levl
class Selection : public Operator {
public:

    Selection( Operator &in_op, unsigned int reg_id, char * value);
    Selection( Operator &in_op, unsigned int reg_id, int value);
    Selection( Operator &in_op, unsigned int reg_id, Register value);

    virtual void open();
    virtual bool next();
    virtual std::vector<std::shared_ptr<Register>> getOutput();
    virtual void close();

private:

    std::vector<std::shared_ptr<Register>> registers;
    Operator &input;
    unsigned int id;
    Register compare;
};

#endif
