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

    std::cout << "insert called with record of size: " << r.getLen() << std::endl;
    
    struct PID pid = {};
    pid.pid = 0;
    pid.sid = schema->segment;
    pid.reserved = 0;

    BufferFrame * frame = &bm.fixPage(pid, true);
    SlottedPage * spage = reinterpret_cast<SlottedPage *>(frame->getData());

    //check if the page is uninitialized and do so if needed
    if(spage->header.data_start == 0) {
        init(spage);
    }

    //find first page with enough space
    while( spage->header.free_space < r.getLen() + sizeof(SlottedPage::Slot)) {
        //unfix Page
        bm.unfixPage(*frame, false);
        pid.pid++;
        std::cout << pid.pid << std::endl;
        frame = &bm.fixPage(pid, true);
        std::cout << "alive" << std::endl;
        spage = reinterpret_cast<SlottedPage *>(frame->getData());
        //check if the page is uninitialized and do so if needed
        if(spage->header.data_start == 0) {
            init(spage);
        }
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
    
    //check if we need to compact page
    if( spage->header.data_start - 
            (spage->header.slot_count) * sizeof(SlottedPage::Slot) <
            r.getLen() + sizeof(SlottedPage::Slot)) {
        //compact page
        compact(spage);
    }
    
    //generate TID to return (where did we store)
    struct TID tid = {};
    tid.slot = spage->header.first_free_slot;
    tid.reserved = 0;
    tid.pid = pid.pid;

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
    
    //done, unfix page
    bm.unfixPage(*frame, true);
    return tid;
}


bool SPSegment::remove(TID tid) {
    struct PID pid = {};
    pid.pid = tid.pid;
    pid.sid = schema->segment;
    BufferFrame * frame = &bm.fixPage(pid, true);
    SlottedPage * spage = reinterpret_cast<SlottedPage *>(frame->getData());
    if (spage->header.slot_count <= tid.slot) {
        
        std::cout << "remove Failed: tid.slot " << tid.slot <<
            " invalid for slot_count " << spage->header.slot_count << 
            "! Aborting!" << std::endl;
        exit(1);
    }
    if (spage->slot[tid.slot].local.T != T_INVAL) {
        spage->slot[tid.slot].local.T = T_INVAL;
        spage->header.free_space += spage->slot[tid.slot].local.length;
        bm.unfixPage(*frame, true);
        return true;
    }
    bm.unfixPage(*frame, false);
    return false;
}


Record SPSegment::lookup(TID tid) {
    PID pid = {};
    pid.pid = tid.pid;
    pid.sid = schema->segment;
    BufferFrame * frame = &bm.fixPage(pid, false);
    SlottedPage * spage = reinterpret_cast<SlottedPage *>(frame->getData());
    if (spage->slot[tid.slot].local.T == T_LOCAL) {
        //no indirecton
        char *data = reinterpret_cast<char *>(
                malloc(spage->slot[tid.slot].local.length));
        memcpy(data,
               spage->data + spage->slot[tid.slot].local.offset,
               spage->slot[tid.slot].local.length);
        Record record(spage->slot[tid.slot].local.length, data);
        bm.unfixPage(*frame, false);
        return std::move(record);
    }
    else {
        //indirecton
        tid = spage->slot[tid.slot].indirection;
        pid.pid = tid.pid;
        bm.unfixPage(*frame, false);
        //get new page
        frame = &bm.fixPage(pid, false);
        spage = reinterpret_cast<SlottedPage *>(frame->getData());
        //now handle it as if it is local
        char * data = reinterpret_cast<char *> (
                malloc(spage->slot[tid.slot].local.length));
        memcpy(data,
                spage->data + spage->slot[tid.slot].local.offset + sizeof(TID),
                spage->slot[tid.slot].local.length);
        Record record(spage->slot[tid.slot].local.length, data);
        bm.unfixPage(*frame, false);
        return std::move(record);
    }
}

bool SPSegment::update(TID tid, const Record& r) {
    PID pid = {};
    pid.pid = tid.pid;
    pid.sid = schema->segment;
    BufferFrame * frame = &bm.fixPage(pid, true);
    SlottedPage * spage = reinterpret_cast<SlottedPage *>(frame->getData());
    //check if it is a valid slot
    if (tid.slot > spage->header.slot_count ||
            spage->slot[tid.slot].local.T == T_INVAL ||
            (spage->slot[tid.slot].local.T == T_LOCAL && 
                spage->slot[tid.slot].local.S == S_MOVED)) {
        std::cout << "Requesting update of invalid slot! Try insert! Aborting!"
            << std::endl;
        exit(1);
    }

    //distinguish: local vs indirected tuple
    if (spage->slot[tid.slot].local.T == T_LOCAL) {
        std::cout << "update: local slot" << std::endl;
        //local slot
        //distinguish: does it fit in slot / on page / not
        if (spage->slot[tid.slot].local.length >= r.getLen()) {
            //fits in slot
            std::cout << "update: same area" << std::endl;
            //update free_space
            spage->header.free_space += 
                (spage->slot[tid.slot].local.length - r.getLen());
            //set new slot length
            spage->slot[tid.slot].local.length = r.getLen();
            //copy data into page
            memcpy(spage->data + spage->slot[tid.slot].local.offset,
                    r.getData(),
                    r.getLen());
            bm.unfixPage(*frame, true);
            return true;
        }
        else if (spage->header.free_space + 
                (uint32_t) spage->slot[tid.slot].local.length
                >= r.getLen()) {
            std::cout << "update: same page" << std::endl;
            //fits on page
            //check if we need to compact the page
            std::cout << "compact precond: " << std::endl;
            std::cout << "data start: " << spage->header.data_start << std::endl;
            std::cout << "slot count: " << spage->header.slot_count << std::endl;
            std::cout << "slot length: " << spage->slot[tid.slot].local.length << std::endl;
            std::cout << "left: " << spage->header.data_start - 
                    (spage->header.slot_count) * sizeof(SlottedPage::Slot)  << std::endl;
            std::cout << "r.getLen: " << r.getLen() << std::endl;
            uint32_t old_len = spage->slot[tid.slot].local.length;
            if( spage->header.data_start - 
                    (spage->header.slot_count) * sizeof(SlottedPage::Slot)<
                    r.getLen()) {
                //set the length of this slot to 0 (so compact works as desired)
                spage->slot[tid.slot].local.length = 0;
                //compact page
                std::cout << "update: compact" << std::endl;
                compact(spage);
            }

            std::cout << "compact postcond: " << std::endl;
            std::cout << "data start: " << spage->header.data_start << std::endl;
            std::cout << "free space: " << spage->header.free_space << std::endl;
            //update free_space
            spage->header.free_space -= 
                (r.getLen() - old_len);
            //generate now position and length
            std::cout << "old data (len|off): " << spage->slot[tid.slot].local.length << "|" << spage->slot[tid.slot].local.offset << std::endl;
            spage->slot[tid.slot].local.length = r.getLen();
            spage->slot[tid.slot].local.offset =
                spage->header.data_start - r.getLen();
            std::cout << "new data (len|off): " << spage->slot[tid.slot].local.length << "|" << spage->slot[tid.slot].local.offset << std::endl;
            //copy the data onty the page
            memcpy(spage->data + spage->slot[tid.slot].local.offset,
                    r.getData(),
                    r.getLen());
            bm.unfixPage(*frame, true);
            return true;
        }
        else {
            //does not fit -> introduce indirection
            //add the original tid in front of the Record (as new record)
            std::cout << "update: introducing indirection" << std::endl;
            char * indir_data = reinterpret_cast<char*>(
                    malloc(r.getLen() + sizeof(TID)));
            memcpy(indir_data,
                    &(spage->slot[tid.slot].indirection),
                    sizeof(TID));
            memcpy(indir_data + sizeof(TID),
                    r.getData(),
                    r.getLen());
            Record indir_r(r.getLen() + sizeof(TID), indir_data);
            free(indir_data);
            std::cout << "free" << std::endl;
            //insert this record therefore we need to release the page first
            bm.unfixPage(*frame, false);
            TID indir_tid = insert(indir_r);
            frame = &bm.fixPage(pid, true);
            spage = reinterpret_cast<SlottedPage *>(frame->getData());
            //load the page it is stored on
            std::cout << "still alive" << std::endl;
            PID indir_pid = {};
            indir_pid.pid = indir_tid.pid;
            indir_pid.sid = schema->segment;
            BufferFrame * indir_frame = &bm.fixPage(indir_pid, true);
            SlottedPage * indir_spage = reinterpret_cast<SlottedPage *>(
                    indir_frame->getData());
            //change the header accordingly
            indir_spage->slot[indir_tid.slot].local.S = S_MOVED;
            //update free_space
            spage->header.free_space += spage->slot[tid.slot].local.length;
            //also change the header of the original tuple
            spage->slot[tid.slot].indirection = indir_tid;
            spage->slot[tid.slot].local.T = T_INDIR;
            //unfix the pages
            bm.unfixPage(*frame, true);
            bm.unfixPage(*indir_frame, true);
            std::cout << "survived update" << std::endl;
            return true;
        }
    }
    else {
        std::cout << "update: indirected slot" << std::endl;
        //indirected slot

        TID indir_tid = spage->slot[tid.slot].indirection;

        PID indir_pid = {};
        indir_pid.pid = indir_tid.pid;
        indir_pid.sid = schema->segment;
        BufferFrame * indir_frame = &bm.fixPage(indir_pid , true);
        SlottedPage * indir_spage = reinterpret_cast<SlottedPage *>(
                indir_frame->getData());
        
        //distinguish: does it fit on this page / in indirected slot /
        //on indirected page / not
        if ( spage->header.free_space >= r.getLen()) {
            std::cout << "update: removing indirection" << std::endl;
            //fits on this page -> remove indirection
            //unfix indirection page and remove entry
            bm.unfixPage(*indir_frame, false);
            remove(spage->slot[tid.slot].indirection);
            //check if we need to compact the page
            if( spage->header.data_start - 
                    (spage->header.slot_count) * sizeof(SlottedPage::Slot) <
                    r.getLen()) {
                //compact page
                compact(spage);
            }
            //generate now position and length
            spage->slot[tid.slot].local.length = r.getLen();
            spage->slot[tid.slot].local.offset =
                spage->header.data_start - r.getLen();
            //set T and S slot fields:
            spage->slot[tid.slot].local.T = T_LOCAL;
            spage->slot[tid.slot].local.S = S_REGULAR;
            //copy the data onty the page
            memcpy(spage->data + spage->slot[tid.slot].local.offset,
                    r.getData(),
                    r.getLen());
            //update free_space
            spage->header.free_space -= spage->slot[tid.slot].local.length;
            bm.unfixPage(*frame, true);
            //test
            std::cout << "end of removing indirection" << std::endl;
            char * test = (char*) malloc(100);
            free(test);
            std::cout << "I survived a malloc :-)" << std::endl;
            return true;
        }
        else if (indir_spage->slot[indir_tid.slot].local.length >=
                r.getLen() + sizeof(TID)) {
            //fits in original indirected slot
            //update free_space
            indir_spage->header.free_space += 
                (spage->slot[indir_tid.slot].local.length - r.getLen() +
                 sizeof(TID));
            //set new slot length
            indir_spage->slot[indir_tid.slot].local.length =
                r.getLen() + sizeof(TID);
            //copy data into page
            memcpy(indir_spage->data + 
                    indir_spage->slot[indir_tid.slot].local.offset,
                    &indir_tid,
                    sizeof(TID));
            memcpy(indir_spage->data +
                    indir_spage->slot[indir_tid.slot].local.offset + sizeof(TID),
                    r.getData(),
                    r.getLen());
            bm.unfixPage(*frame, true);
            return true;
        }
        else if (indir_spage->header.free_space >= r.getLen() + sizeof(TID)) {
            //fits on original indirected page
            //we do not touch the original page
            bm.unfixPage(*frame, false);
            //update free_space
            indir_spage->header.free_space -= 
                (r.getLen() + sizeof(TID) -
                 spage->slot[indir_tid.slot].local.length);
            //check if we need to compact the indir_page
            if( indir_spage->header.data_start - 
                    (indir_spage->header.slot_count) * sizeof(SlottedPage::Slot) <
                    r.getLen() + sizeof(TID)) {
                //set the length of this slot to 0 (so compact works as desired)
                indir_spage->slot[indir_tid.slot].local.length = 0;
                //compact page
                compact(spage);
            }
            //generate new position and length
            indir_spage->slot[indir_tid.slot].local.length = 
                r.getLen() + sizeof(TID);
            indir_spage->slot[indir_tid.slot].local.offset =
                spage->header.data_start - r.getLen() - sizeof(TID);
            //copy the data onty the page
            memcpy(indir_spage->data + 
                    indir_spage->slot[indir_tid.slot].local.offset,
                    &indir_tid,
                    sizeof(TID));
            memcpy(spage->data + spage->slot[indir_tid.slot].local.offset +
                    sizeof(TID),
                    r.getData(),
                    r.getLen());
            bm.unfixPage(*indir_frame, true);
            return false;
        }
        else {
            std::cout << "update: changing indirection" << std::endl;
            //does not fit -> select new page for indirection
            //close indirecton page and remove entry
            bm.unfixPage(*indir_frame, false);
            remove(spage->slot[tid.slot].indirection);
            //add the original tid in front of the Record (as new record)
            char * indir_data = reinterpret_cast<char*>(
                    malloc(r.getLen() + sizeof(TID)));
            memcpy(indir_data,
                    &(spage->slot[tid.slot].indirection),
                    sizeof(TID));
            memcpy(indir_data + sizeof(TID),
                    r.getData(),
                    r.getLen());
            Record indir_r(r.getLen() + sizeof(TID), indir_data);
            //insert this record
            indir_tid = insert(indir_r);
            //load the page it is stored on
            indir_pid = {};
            indir_pid.pid = indir_tid.pid;
            indir_pid.sid = schema->segment;
            indir_frame = &bm.fixPage(indir_pid, true);
            indir_spage = reinterpret_cast<SlottedPage *>(
                    indir_frame->getData());
            //change the header accordingly
            indir_spage->slot[indir_tid.slot].local.S = S_MOVED;
            //also change the header of the original tuple
            spage->slot[tid.slot].indirection = indir_tid;
            spage->slot[tid.slot].local.T = T_INDIR;
            //unfix the pages
            bm.unfixPage(*frame, true);
            bm.unfixPage(*indir_frame, true);
            return true;
        }
    }
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


void SPSegment::init(struct SlottedPage * page) {
    page->header.slot_count = 0;
    page->header.first_free_slot = 0;
    //point at element beyond last possible value
    page->header.data_start = pageSize - sizeof(SlottedPage::Header);
    //only header is used
    page->header.free_space = pageSize - sizeof(SlottedPage::Header);
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
            if ( i < tmppage->header.first_free_slot )
                tmppage->header.first_free_slot = i;
        }
    }
    //copy tmppage to original page
    memcpy(page, tmppage, sizeof(SlottedPage));
    //free temporary buffer
    free(tmppage);
}
