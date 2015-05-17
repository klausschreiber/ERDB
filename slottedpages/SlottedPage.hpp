#ifndef SLOTTED_PAGE_HPP
#define SLOTTED_PAGE_HPP

#include <stdint.h>
#include "../buffer/constants.hpp"


struct SlottedPage {
    struct Header {
        uint16_t slot_count;
        uint16_t first_free_slot;
        uint16_t data_start;
        uint16_t free_space;
    };

    struct Slot {
        struct Local {
            uint8_t T;
            uint8_t S;
            uint32_t offset:24;
            uint32_t length:24;
        };
        union {
            struct Local local;
            uint64_t indirection;
        };
    };

    Header header;
    union {
        Slot slot[(pageSize - sizeof(struct Header))/sizeof(struct Slot)];
        char data[(pageSize - sizeof(struct Header))];
    };
};

#endif
