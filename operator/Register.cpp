#include "Register.hpp"
#include <cstring>
#include <iostream>
#include <string>

Register::Register() {
    //just to be shure the getters will return 0 terminated strings from the very fist time
    memset(val.str_val, 0, Register::length);
    type = Integer;
}

Register::~Register() {
//    std::cout << "Register constructor called" << std::endl;
}

void Register::setInteger( const int value) {
    type = Integer;
    val.int_val = value;
}

int Register::getInteger(){
    //no check done, as this will always work
    return val.int_val;
}

void Register::setString( const char* value) {
    type = String;
    //copy to a max size of Register::length
    strncpy(val.str_val, value, Register::length);
    //just to be shure the result is 0 terminated
    val.str_val[Register::length-1] = 0;
}

char * Register::getString() {
    //this will always work, as it is 0 terminated, even if we set some int it the first few bytes
    return val.str_val;
}

Register::Type Register::getType() {
    return type;
}

bool Register::operator< (Register b) const {
    switch (type){
        case Integer:
            return val.int_val < b.getInteger();
        case String:
            return (strncmp(val.str_val, b.getString(), Register::length) < 0);  
        default:
            return false;
    }
}

bool Register::operator== (Register b) const {
    switch (type){
        case Integer:
            return val.int_val == b.getInteger();
        case String:
            return (strncmp(val.str_val, b.getString(), Register::length) == 0);  
        default:
            return false;
    }
}

std::size_t Register::hash() const {
    switch (type) {
        case Integer:
            std::hash<int> int_hash;
            return int_hash(val.int_val);
        case String:
            std::hash<const char*> char_hash;
            return char_hash(val.str_val);
        default:
            return 0;
    }
}


