#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <vector>
#include <string>
#include "types.h"
using namespace std;

template<typename K, typename V>
struct KeyValuePair {
    K key;
    V value;
    bool occupied;

    KeyValuePair();
    KeyValuePair(K k, V v);
};

template<typename K, typename V>
class HashTable {
private:
    KeyValuePair<K, V>* table;
    size_t capacity;
    size_t size;

    size_t hash(uint32_t key) const;
    size_t hash(const string& key) const;
    size_t findSlot(const K& key) const;

public:
    HashTable(size_t cap = HASH_TABLE_SIZE);
    ~HashTable();

    void insert(const K& key, const V& value);
    bool find(const K& key, V& value) const;
    bool contains(const K& key) const;
    void remove(const K& key);

    size_t getSize() const;
    vector<K> getAllKeys() const;
    vector<KeyValuePair<K, V>> getAllPairs() const;

    void clear();
};

#endif