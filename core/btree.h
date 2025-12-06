#ifndef BTREE_H
#define BTREE_H

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include "types.h" 

using namespace std;

template<typename K, typename V>
struct BTreeNode {
    bool isLeaf;
    uint32_t numKeys;
    K keys[2 * BTREE_DEGREE - 1];
    V values[2 * BTREE_DEGREE - 1];
    uint64_t children[2 * BTREE_DEGREE];
    uint64_t nodeOffset;

    BTreeNode();
    void serialize(char* buffer) const;
    static BTreeNode<K, V> deserialize(const char* buffer);
    static size_t getSerializedSize();
};

template<typename K, typename V>
class BTree {
private:
    uint64_t rootOffset;
    string indexFile;
    fstream file;

    BTreeNode<K, V> readNode(uint64_t offset);
    void writeNode(const BTreeNode<K, V>& node, uint64_t offset);
    uint64_t allocateNode();
    void splitChild(BTreeNode<K, V>& parent, int index, BTreeNode<K, V>& child);
    void insertNonFull(BTreeNode<K, V>& node, K key, V value);
    bool searchInternal(const BTreeNode<K, V>& node, K key, V& value);
    void traverseInternal(uint64_t nodeOffset, vector<pair<K, V>>& result) const;
    void saveMetadata();
    void loadMetadata();

public:
    BTree(const string& filename);
    ~BTree();

    void insert(K key, V value);
    bool search(K key, V& value);
    vector<pair<K, V>> getAllPairs() const;
    bool isEmpty() const;
    uint64_t getRootOffset() const;
    size_t size() const;
    void close();
    void create();
};

#endif
