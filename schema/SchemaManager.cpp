#include "SchemaManager.hpp"
#include "../buffer/constants.hpp"
#include <string.h>
#include <utility>
#include <iostream>

const struct Schema* const SchemaManager::getSchema(const std::string name) {
    auto result = schema_map.find(name);
    if (result != schema_map.end()) {
        //we found it
        return result->second;
    }
    else {
        return NULL;
    }
}

int SchemaManager::addSchema(Schema& schema) {
    size_t size = sizeof(struct Schema) + 
        schema.attr_count * sizeof(struct Schema::Attribute);
    if( size > pageSize - sizeof(struct SchemaFrame::Header)) {
        //in this case the schema cannot be stored on a single page!
        return -1;
    }
    else if ( schema_map.find(schema.name) != schema_map.end() ) {
        //another schema with the same name exists!
        return -2;
    }
    else {
        //set the segment id to use for this schema
        schema.segment = next_segment;
        next_segment++;
        //set the schemas page_count to 0;
        schema.page_count = 0;
        //get the next (empty) page
        BufferFrame& frame = bufferManager.fixPage(next_page, true);
        //copy the schema to the new page
        struct SchemaFrame* schemaFrame =
            reinterpret_cast<struct SchemaFrame*>(frame.getData());
        memcpy( &(schemaFrame->schema), &schema, size);
        //set the header accordingly
        schemaFrame->header.valid = 1;
        schemaFrame->header.has_next = 0;
        //release page
        bufferManager.unfixPage(frame, true);
        //get previous page and update header (has_next = true)
        //except if we write first page (no page-1 exists)
        if(next_page > 0)
        {
            BufferFrame &previous = bufferManager.fixPage(next_page-1, true);
            schemaFrame =
                reinterpret_cast<struct SchemaFrame*>(previous.getData());
            schemaFrame->header.has_next = 1;
            bufferManager.unfixPage(previous, true);
        }
        //add it to schema map
        schema_map.insert( 
                std::pair<std::string, struct Schema *> (
                    schema.name, &schema));
        schema_page_map.insert(
                std::pair<std::string, uint64_t>(
                    schema.name, next_page));
        //increment next_page counter
        next_page++;
        
        return 0;
    }
}

const struct Schema* SchemaManager::incrementPagesCount(const std::string name) {
    auto result = schema_map.find(name);
    if (result != schema_map.end()) {
        auto page_res = schema_page_map.find(name);
        if(page_res == schema_page_map.end()) {
            std::cerr << "incrempntPagesCount: This should not happen!"
                << std::endl;
            exit(1);
        }
        //change it on disk
        BufferFrame& frame = bufferManager.fixPage(page_res->second, true);
        struct SchemaFrame* schemaFrame =
            reinterpret_cast<struct SchemaFrame*>(frame.getData());
        schemaFrame->schema.page_count++;
        bufferManager.unfixPage(frame, true);
        //change it in memory
        result->second->page_count++;
        return &(schemaFrame->schema);
    }
    else {
        return NULL;
    }
}

int SchemaManager::dropSchema(const std::string name) {
    auto result = schema_map.find(name);
    if (result != schema_map.end()) {
        //we found it
        auto page_res = schema_page_map.find(name);
        if (page_res == schema_page_map.end()) {
            std::cerr << "dropSchema: This should not happen!" << std::endl;
            exit(1);
        }
        BufferFrame& frame = bufferManager.fixPage(page_res->second, true);
        struct SchemaFrame* schemaFrame =
            reinterpret_cast<struct SchemaFrame*>(frame.getData());
        schemaFrame->header.valid = 0;
        bufferManager.unfixPage(frame, true);
        //it is marked invalid on disk, now remove it from cache
        schema_map.erase(result);
        schema_page_map.erase(page_res);
        //done
        return 0;
    }
    else {
        return -1;
    }
}

const std::list<std::string> SchemaManager::listTables() {
    std::list<std::string> * list = new std::list<std::string>();
    return *list;
}

SchemaManager::SchemaManager( BufferManager & bufferManager )
            : bufferManager(bufferManager) {
        next_segment = 1;
        next_page = 0;
        uint64_t curr_page = 0;
        bool last_schema = false;
        while (!last_schema) {
            //while we have not reached the last schema
            BufferFrame& frame =
                bufferManager.fixPage(curr_page, false);
            struct SchemaFrame* schemaFrame = 
                reinterpret_cast<struct SchemaFrame*>(frame.getData());
            if( schemaFrame->header.valid ) {
                //copy the schema to own memory before unfixingPage!
                size_t size = sizeof(struct Schema) + 
                    schemaFrame->schema.attr_count *
                    sizeof(struct Schema::Attribute);
                struct Schema * schema =
                    reinterpret_cast<struct Schema*>(malloc(size));
                memcpy( schema, &(schemaFrame->schema), size);
                //add it to the known relations
                schema_map.insert( 
                        std::pair<std::string, struct Schema *> (
                            schema->name, schema));
                schema_page_map.insert(
                        std::pair<std::string, uint64_t>(
                            schema->name, curr_page));
                //set the next_segment id if needed
                if (next_segment <= schema->segment)
                    next_segment = schema->segment + 1;
                std::cout << "Loadad: \n" << schema->toString() << std::endl;
            }
            //check if this was the last frame to read
            if( !schemaFrame->header.has_next ) 
                last_schema = true;
                next_page = curr_page + 1;

            bufferManager.unfixPage(frame, false);
            curr_page++;
        }
    }
