#ifndef REGISTER_HPP
#define REGISTER_HPP

#include "cstring"
#include <iostream>

class Register {
public:
    enum Type {Integer, String};

    //constructor
    Register();

    void setInteger( const int value);
    int getInteger();
    
    // set a string as current value. Note that only Register::length length is used for now
    void setString( const char* value);
    char * getString();

    Type getType();
    

    //not they only return a useful result if type matches.
    //if not it is performed anyways, but the result will not be as desired
    bool operator< (Register &b);
    bool operator== (Register &b);

    char * hash();

private:

    Type type;

    static const unsigned int length = 20;

    union {
        int int_val;
        char str_val[Register::length];
    } val;
};

#endif
