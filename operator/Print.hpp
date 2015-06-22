#ifndef PRINT_HPP
#define PRINT_HPP

#include "Operator.hpp"

class Print : Operator {
public
    virtual void open();
    virtual bool next();
    virtual std::vector<Register*> getOutput();
    virtual void close();
}

#endif
