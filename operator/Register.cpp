#include "Register.hpp"
#include <cstring>

Register::Register() {
    //just to be shure the getters will return 0 terminated strings from the very fist time
    memset(val.str_val, 0, Register::length);
    type = Integer;
}

void Register::setInteger( const int value) {
    type = Integer;
    val.int_val = value;
}

int Register::getInteger() {
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

bool Register::operator< (Register &b) {
    switch (type){
        case Integer:
            return val.int_val < b.getInteger();
        case String:
            return (strncmp(val.str_val, b.getString(), Register::length) < 0);  
        default:
            return false;
    }
}

bool Register::operator== (Register &b) {
    switch (type){
        case Integer:
            return val.int_val == b.getInteger();
        case String:
            return (strncmp(val.str_val, b.getString(), Register::length) == 0);  
        default:
            return false;
    }
}

char * Register::hash() {
    //TODO: implement real hash!
    return val.str_val;
}


