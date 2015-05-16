#include "Schema.hpp"

#include <sstream>

std::string Schema::toString() const {
    std::stringstream out;
    out << name << std::endl;
    out << "Primary Key:";
    for (int i = 0; i < 5; i++) {
        //if it is a valid attribute, it is assumed to be part of the key
        if (primary_key_index[i] < attr_count) {
            out << ' ' << attributes[primary_key_index[i]].name;
        }
    }
    out << std::endl;
    for (int i = 0; i < attr_count; i++) {
        out << attributes[i].name << '\t';
        switch(attributes[i].type) {
            case Schema::Attribute::Type::Integer: {
                out << "Integer";
                break;
            }
            case Schema::Attribute::Type::Varchar: {
                out << "Varchar(" << attributes[i].length << ")";
                break;
            }
        }
        if (attributes[i].notNull)
            out << " not null";
        out << std::endl;
    }
    return out.str();
}

//static std::string type(const Schema::Relation::Attribute& attr) {
//   Types::Tag type = attr.type;
//   switch(type) {
//      case Types::Tag::Integer:
//         return "Integer";
//      /*case Types::Tag::Numeric: {
//         std::stringstream ss;
//         ss << "Numeric(" << attr.len1 << ", " << attr.len2 << ")";
//         return ss.str();
//      }*/
//      case Types::Tag::Char: {
//         std::stringstream ss;
//         ss << "Char(" << attr.len << ")";
//         return ss.str();
//      }
//   }
//   throw;
//}
//
//std::string Schema::toString() const {
//   std::stringstream out;
//   for (const Schema::Relation& rel : relations) {
//      out << rel.name << std::endl;
//      out << "\tPrimary Key:";
//      for (unsigned keyId : rel.primaryKey)
//         out << ' ' << rel.attributes[keyId].name;
//      out << std::endl;
//      for (const auto& attr : rel.attributes)
//         out << '\t' << attr.name << '\t' << type(attr) << (attr.notNull ? " not null" : "") << std::endl;
//   }
//   return out.str();
//}
