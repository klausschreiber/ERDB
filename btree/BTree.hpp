#ifndef BTREE_H
#define BTREE_H

#include "../slottedpages/TID.hpp"

template <typename KeyType, typename Comperator>
class BTree {
    public:
        void insert(KeyType key, TID &tid);
        void erase(KeyType key);
        TID* lookup(KeyType key);
    private:

};

template <typename KeyType>
struct BTreePage {
    enum class Type {INNER, LEAF};
    Type type;
};

template <typename KeyType>
struct BTreeInnerPage : BTreePage {
    
};

template <typename KeyType>
struct BTreeLeafPage : BTreePage {
    
};

#endif
