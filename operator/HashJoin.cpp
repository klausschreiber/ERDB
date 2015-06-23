#include "HashJoin.hpp"
#include <utility>
#include <iostream>

HashJoin::HashJoin( Operator &in_l, Operator &in_r, unsigned int id_l, unsigned int id_r) :
        left(in_l), right(in_r), left_id(id_l), right_id(id_r){

};

void HashJoin::open() {
    left.open();
    right.open();
}

bool HashJoin::next() {
    //is this the first run?
    if (hash_map.empty())
    {
        //initialize hash map
        while( left.next()) {
            std::vector<std::shared_ptr<Register>> in = left.getOutput();
            hash_map.insert(std::make_pair((*in[left_id]), in));
        }
    }
    //if the left input is empty, we do not produce any output
    if (hash_map.empty())
        return false;
    //check if we still got another element for previous right hand side
    if (cur != end) {
        registers.clear();
        //add the left tuple first
        for( auto lit = (cur->second).begin(); lit != (cur->second).end(); lit++) {
            registers.push_back(*lit);
        }
        //now add the rigth tuple
        for (auto rit = cur_right.begin(); rit != cur_right.end(); rit++) {
            registers.push_back(*rit);
        }
        cur++;
        return true;
    }
    //get the next element of the right input until we have a hit
    //for now we assume we always match exactly once, so we have a 1:N join
    while (right.next()) {
        cur_right = right.getOutput();
        auto range = hash_map.equal_range((*cur_right[right_id]));
        if (range.first != range.second) {
            //we got a range
            cur = range.first;
            end = range.second;
            registers.clear();
            //add the left tuple first
            for( auto lit = (cur->second).begin(); lit != (cur->second).end(); lit++) {
                registers.push_back(*lit);
            }
            //now add the rigth tuple
            for (auto rit = cur_right.begin(); rit != cur_right.end(); rit++) {
                registers.push_back(*rit);
            }
            cur++;
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<Register>> HashJoin::getOutput(){
    return registers;
}
void HashJoin::close(){
    left.close();
    right.close();
}
