#include "SchemaManager.hpp"

Schema* SchemaManager::fixSchema(std::string name, const bool exclusive) {
    return NULL;
}

void SchemaManager::unfixSchema(Schema& schema, const bool isDirty) {

}

Schema& SchemaManager::newSchema(std::string name) {
    struct Schema * schema;
    schema = reinterpret_cast<Schema*>(malloc(sizeof(struct Schema)));
    return * schema;
}

int SchemaManager::dropSchema(std::string name) {
    return 0;
}

const std::list<std::string> SchemaManager::listTables() {
    std::list<std::string> * list = new std::list<std::string>();
    return *list;
}