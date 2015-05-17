#ifndef H_Schema_hpp
#define H_Schema_hpp

#include <string>

struct Schema {
    struct Attribute {
        enum class Type: uint8_t {Integer, Varchar};
        uint16_t length; //length in case it is a Varchar
        char name[50]; //column name
        Type type; //column type
        bool notNull; //is null allowed (ignored for now)
    };
    uint64_t page_count;
    uint16_t segment; //segment id
    uint16_t attr_count; //number of attributes
    uint16_t primary_key_index[5]; //index of primary key elements. This is assumed to be valid if it is within the valid attributes.
    char name[50]; //Schema name = Table name (we define only one database, thus each schema is a table
    Attribute attributes[]; //array of attributes

    //functions as helpers
    std::string toString() const;
};

struct SchemaFrame {
    struct Header {
        uint16_t valid;
        uint16_t has_next;
        uint32_t reserved;
        uint64_t reserved2;
    } header;
    struct Schema schema;
};

#endif
