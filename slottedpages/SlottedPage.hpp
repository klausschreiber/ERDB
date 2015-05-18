#ifndef SLOTTED_PAGE_HPP
#define SLOTTED_PAGE_HPP

#include <stdint.h>
#include "../buffer/constants.hpp"
#include "TID.hpp"

//Meaning of Slot:
// if T == 0xff: local tuple
//      if S == 0x00: regular tuple
//      if ? == 0xff: moved from other page to here
// if T == 0x00: invalid / removed tuple
// if T == something else: use TID as indirecton

#define T_LOCAL 0xff
#define T_INVAL 0x00
#define T_INDIR 0x42

#define S_REGULAR 0x00
#define S_MOVED 0xff


struct SlottedPage {
    struct Header {
        uint16_t slot_count;
        uint16_t first_free_slot;
        uint16_t data_start;
        uint16_t free_space;
    };

    struct Slot {
        struct Local {
            uint32_t length:24 __attribute__((packed));
            uint32_t offset:24 __attribute__((packed));
            uint8_t S:8;
            uint8_t T:8;
        };
        union {
            struct Local local;
            struct TID indirection;
        };
    };

    Header header;
    union {
        Slot slot[(pageSize - sizeof(struct Header))/sizeof(struct Slot)];
        char data[(pageSize - sizeof(struct Header))];
    };
};

#endif
