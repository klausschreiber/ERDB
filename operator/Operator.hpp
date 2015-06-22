#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <vector>
#include "Register.hpp"

class Operator {
public:
    virtual void open() = 0;
    virtual bool next() = 0;
    virtual std::vector<Register*> getOutput() = 0;
    virtual void close() = 0; 
};

#endif
