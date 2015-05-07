#include "SchemaManager.hpp"

Schema& SchemaManager::fixSchema(std::string name, const bool exclusive) {
    struct Schema * schema;
    schema = reinterpret_cast<Schema*>(malloc(sizeof(struct Schema)));
    return *schema;
}

void SchemaManager::unfixSchema(Schema& schema, const bool isDirty) {

}

