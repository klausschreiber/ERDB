#ifndef PID_HPP
#define PID_HPP


//layout Bytes: 1 2 3 4 5 6 7 8
//usage:        sid r pid------
//thus 0xffffaa1111111111 is interpreted as:
//sid: ffff
//reserved: aa
//pid: 1111111111
//
//experiments forced it this way!
struct PID {
    uint64_t pid:40;
    uint8_t reserved:8;
    uint16_t sid:16;
};

#endif
