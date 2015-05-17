#ifndef TID_HPP
#define TID_HPP

//bytes used as: 1 Byte reserved, 5 Bytes pid, 2 Bytes slot id

struct TID {
    uint16_t slot:16;
    uint64_t pid:40;
    uint8_t reserved:8;
};

#endif
