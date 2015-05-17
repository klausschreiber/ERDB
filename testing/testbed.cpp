#include <iostream>
#include <stdint.h>

union TEST {
    uint64_t c:40;
    uint8_t b:8;
    uint16_t a:16;
};

int main(int argc, char** argv) {
    uint64_t testval = 0xffffaa1111111111;
    struct TEST *test = reinterpret_cast<struct TEST*>(&testval);
    std::cout << "original: " << testval << std::hex << testval << std::dec << std::endl;
    std::cout << "a: " << std::hex << test->a << " b: " << (int)test->b << " c: " << test->c <<std::endl;
    return 0;
};
