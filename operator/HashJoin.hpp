#ifndef HASH_JOIN_HPP
#define HASH_JOIN_HPP

#include "Operator.hpp"
#include <memory>
#include <unordered_map>

//print operator. It prints its input to stdout and forwards it.
//thus it is safe to use its getOutput() which simply passe th eretreived data on to the next levl
class HashJoin : public Operator {
public:

    HashJoin( Operator &in_l, Operator &in_r, unsigned int id_l, unsigned int id_r);

    virtual void open();
    virtual bool next();
    virtual std::vector<std::shared_ptr<Register>> getOutput();
    virtual void close();

private:

    std::vector<std::shared_ptr<Register>> registers;
    Operator &left;
    Operator &right;
    unsigned int left_id, right_id;
    std::unordered_multimap<Register, std::vector<std::shared_ptr<Register>>> hash_map;
    std::unordered_multimap<Register, std::vector<std::shared_ptr<Register>>>::iterator cur, end;
    std::vector<std::shared_ptr<Register>> cur_right;
};

#endif
