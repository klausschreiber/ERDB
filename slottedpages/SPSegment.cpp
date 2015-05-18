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
    pid.pid = 0;
    pid.sid = schema->segment;
    pid.reserved = 0;

    BufferFrame * frame = &bm.fixPage(pid, true);
    SlottedPage * spage = reinterpret_cast<SlottedPage *>(frame->getData());

    //find first page with enough space
    while( spage->header.free_space - sizeof(SlottedPage::Slot) < r.getLen()) {
        //unfix Page
        bm.unfixPage(*frame, false);
        pid.pid++;
        frame = &bm.fixPage(pid, true);
        spage = reinterpret_cast<SlottedPage *>(frame->getData());
    }

    //set the page_count of the schema if needed
    if( pid.pid > schema->page_count) {
        schema = sm.incrementPagesCount(schema->name);
        if (schema == NULL) {
            //our schema is gone!?!
            std::cout << "insert Failed: Schema invalid! Aborting!" << std::endl;
            exit(1);
        }
    }

    //check if the page is uninitialized
    if(spage->header.data_start == 0) {
        // this should only happen for page 0 if it has never been written before
        spage->header.slot_count = 0;
        spage->header.first_free_slot = 0;
        //point at element beyond last possible value
        spage->header.data_start = pageSize - sizeof(SlottedPage::Header);
        //only header is used
        spage->header.free_space = pageSize - sizeof(SlottedPage::Header);
    }
    
    //check if we need to compact page
    if( spage->header.data_start - 
            spage->header.slot_count * sizeof(SlottedPage::Slot) < r.getLen()) {
        //compact page
        compact(spage);
    }

    //copy data to page
    uint16_t start_offset = spage->header.data_start - r.getLen();
    memcpy(spage->data + start_offset, r.getData(), r.getLen());
    //set up header and slot
    
    spage->slot[spage->header.first_free_slot].local.length =
        r.getLen();
    spage->slot[spage->header.first_free_slot].local.offset =
        start_offset;
    spage->slot[spage->header.first_free_slot].local.T = T_LOCAL;
    spage->slot[spage->header.first_free_slot].local.S = S_REGULAR;

    spage->header.data_start = start_offset;
    //if we added at end, we have to increment the slot_count and the space
    //consumed by the slots (header)
    if (spage->header.first_free_slot == spage->header.slot_count) {
        spage->header.slot_count++;
        spage->header.free_space -= sizeof(SlottedPage::Slot);
    }
    //find next free slot by searching from current slot onwards
    spage->header.first_free_slot++;
    while ( spage->header.first_free_slot != spage->header.slot_count && 
            spage->slot[spage->header.first_free_slot].local.T != T_INVAL )
        spage->header.first_free_slot++;
    
    //update the free space according to added length
    spage->header.free_space -= r.getLen();
    
    //generate TID to return (where did we store)
    struct TID tid = {};
    tid.slot = spage->header.first_free_slot -1;
    tid.reserved = 0;
    tid.pid = pid.pid;
    //done, unfix page
    bm.unfixPage(*frame, true);
    return tid;
}

bool SPSegment::remove(TID tid) {
    BufferFrame * frame = &bm.fixPage(tid.pid, true);
    SlottedPage * spage = reinterpret_cast<SlottedPage *>(frame->getData());
    if (spage->header.slot_count <= tid.slot) {
        //our !?!
        std::cout << "remove Failed: tid.slot " << tid.slot << "invalid  for slot_count " << spage->header.slot_count
        << "! Aborting!" << std::endl;
        exit(1);
    }
    if (spage->slot[tid.slot].local.T =! 0) {
        spage->slot->local.T = 0;
        bm.unfixPage(*frame, true);
        return true;
    }
    bm.unfixPage(*frame, false);
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

void SPSegment::compact(SlottedPage * page) {
    SlottedPage * tmppage =
        reinterpret_cast<SlottedPage*>(malloc(sizeof(SlottedPage)));
    //copy the header part that does not change
    tmppage->header.slot_count = page->header.slot_count;
    tmppage->header.first_free_slot = page->header.first_free_slot;
    tmppage->header.free_space = page->header.free_space;
    //set the initial data_start as if page is empty
    tmppage->header.data_start = pageSize - sizeof(SlottedPage::Header);
    //copy all used slots, set gaps to invalid (in header)
    for (int i = 0; i < page->header.slot_count ; i++) {
        if (page->slot[i].local.T == T_LOCAL) {
            //copy slot metadata (and change accordingly
            tmppage->slot[i].local.T = T_LOCAL;
            tmppage->slot[i].local.S = page->slot[i].local.S;
            tmppage->slot[i].local.length = page->slot[i].local.length;
            tmppage->slot[i].local.offset =
                tmppage->header.data_start - tmppage->slot[i].local.length;
            //copy real data
            memcpy(tmppage->data + tmppage->slot[i].local.offset,
                    page->data + page->slot[i].local.offset,
                    tmppage->slot[i].local.length);
            //change header.data_start
            tmppage->header.data_start = tmppage->slot[i].local.offset;
        }
        else if (page->slot[i].local.T == T_INDIR) {
            //moved page, only slot meta data has to be copied
            tmppage->slot[i] = page->slot[i];
        }
        else {
            //all other slots are assumed invalid and simply set as invalid
            //all other fileds do not matter in this case :-)
            tmppage->slot[i].local.T = T_INVAL;
        }
    }
    //copy tmppage to original page
    memcpy(page, tmppage, sizeof(SlottedPage));
    //free temporary buffer
    free(tmppage);
}
