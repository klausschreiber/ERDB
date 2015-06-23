#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include "Operator.hpp"
#include <memory>

//print operator. It prints its input to stdout and forwards it.
//thus it is safe to use its getOutput() which simply passe th eretreived data on to the next levl
class Projection : public Operator {
public:

    Projection( Operator &in_op, std::vector<int> reg_ids);

    virtual void open();
    virtual bool next();
    virtual std::vector<std::shared_ptr<Register>> getOutput();
    virtual void close();

private:

    std::vector<std::shared_ptr<Register>> registers;
    Operator &input;
    std::vector<int> register_ids;
};

#endif
