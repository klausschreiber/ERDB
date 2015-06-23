#ifndef REGISTER_HPP
#define REGISTER_HPP

#include "cstring"
#include <iostream>
#include <functional>

class Register {
public:
    enum Type {Integer, String};

    //constructor
    Register();
    //destructor
    ~Register();


    void setInteger( const int value);
    int getInteger();
    
    // set a string as current value. Note that only Register::length length is used for now
    void setString( const char* value);
    char * getString();

    Type getType();
    

    //not they only return a useful result if type matches.
    //if not it is performed anyways, but the result will not be as desired
    bool operator< (Register b) const;
    bool operator== (Register b) const;

    std::size_t hash() const;

private:

    Type type;

    static const unsigned int length = 20;

    union {
        int int_val;
        char str_val[Register::length];
    } val;
};

//for convenience: overload the std::hash to use the Register.hash function

namespace std {
    template <>
    struct hash<Register> {
        std::size_t operator()(const Register& k) const {
            return k.hash();
        }
    };
}

#endif
