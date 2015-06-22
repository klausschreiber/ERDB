#ifndef PRINT_HPP
#define PRINT_HPP

#include "Operator.hpp"
#include <iostream>

//print operator. It prints its input to stdout and forwards it.
//thus it is safe to use its getOutput() which simply passe th eretreived data on to the next levl
class Print : public Operator {
public:

    Print( Operator &in_op, std::ostream &ostr);

    virtual void open();
    virtual bool next();
    virtual std::vector<Register*> getOutput();
    virtual void close();

private:

    std::vector<Register*> registers;
    Operator &input;
    std::ostream &ostream;
};

#endif
