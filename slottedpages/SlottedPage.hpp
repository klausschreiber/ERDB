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


struct SlottedPage {
    struct Header {
        uint16_t slot_count;
        uint16_t first_free_slot;
        uint16_t data_start;
        uint16_t free_space;
    };

    struct Slot {
        struct Local {
            uint32_t length:24;
            uint32_t offset:24;
            uint8_t S;
            uint8_t T;
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
