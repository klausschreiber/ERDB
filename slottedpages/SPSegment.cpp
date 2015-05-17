#include "SPSegment.hpp"
#include <iostream>
#include "SlottedPage.hpp"
#include "../buffer/PID.hpp"
#include <string.h>

TID SPSegment::insert(const Record& r) {
    //check for oversized Records
    if (r.getLen() > pageSize - sizeof(SlottedPage::Header) -
            sizeof(SlottedPage::Slot)) {
        std::cout << "data to big to fit on single page! Aborting!" << std::endl;
        exit(1);
    }
    
    struct PID pid = {};
    if( schema->page_count == 0 )
        pid.pid = 0;
    else
        pid.pid = schema->page_count-1;
    pid.sid = schema->segment;
    pid.reserved = 0;

    std::cout << *reinterpret_cast<uint64_t*>(&pid) << std::endl;

    BufferFrame& frame = bm.fixPage(pid, true);
    SlottedPage * spage = reinterpret_cast<SlottedPage *>(frame.getData());

    if(spage->header.data_start == 0) {
        // this should only happen for page 0 if it has never been written before
        spage->header.slot_count = 0;
        spage->header.first_free_slot = 0;
        //point at element beyond last possible value
        spage->header.data_start = pageSize - sizeof(SlottedPage::Header);
        //only header is used
        spage->header.free_space = pageSize - sizeof(SlottedPage::Header);
    }
    
    if(spage->header.free_space - sizeof(SlottedPage::Slot) > r.getLen()) {
        // it fits on this page, but compactificaton might be needed
        std::cout << "fits" << std::endl;
        if(spage->header.slot_count == 0) {
            //unusend page.
            std::cout << "unused page" << std::endl;

            //copy data to page
            uint16_t start_offset = spage->header.data_start - r.getLen();
            memcpy(spage->data, r.getData(), r.getLen());
            //set up header and slot
            
            spage->slot[spage->header.first_free_slot].local.length =
                r.getLen();
            spage->slot[spage->header.first_free_slot].local.offset =
                start_offset;
            spage->slot[spage->header.first_free_slot].local.T = 0xff;
            spage->slot[spage->header.first_free_slot].local.S = 0;

            spage->header.data_start = start_offset;
            //TODO: if we want to reuse gaps, change this!
            spage->header.first_free_slot++;
            spage->header.slot_count++;
            spage->header.free_space -= 
                (sizeof(SlottedPage::Slot) + r.getLen());
            
            //generate TID to return (where did we store)
            struct TID tid = {};
            tid.slot = spage->header.first_free_slot -1;
            tid.reserved = 0;
            tid.pid = pid.pid;
            //done, unfix page
            bm.unfixPage(frame, true);
            return tid;
        }
    }
    else {
        // in this case it does not fit on this page, so we need to load the next one
        std::cout << "does not fit" << std::endl;
    }
    struct TID ret;
    return ret;
}

bool SPSegment::remove(TID tid) {
    return false;
}

Record SPSegment::lookup(TID tid) {
    Record* r = reinterpret_cast<Record*>(malloc (sizeof(Record)));
    return std::move(*r);
}

bool update(TID tid, const Record& r) {
    return false;
}

SPSegment::SPSegment( BufferManager& bm, SchemaManager& sm, std::string schema_name) 
        : sm(sm), bm(bm) {
    schema = sm.getSchema(schema_name);
    if (schema == NULL) {
        //oops we got called with non existent schema, thus we cannot do
        //anything! Abort.
        std::cerr << "SPSegment::constructor: could not load schema: "
            << schema_name << std::endl;
        exit(1);
    }
}
