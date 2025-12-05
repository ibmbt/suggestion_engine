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

    KeyValuePair() : key(K()), value(V()), occupied(false) {}
    KeyValuePair(K k, V v) : key(k), value(v), occupied(true) {}
};

template<typename K, typename V>
class HashTable {
private:
    KeyValuePair<K, V>* table;
    size_t capacity;
    size_t size;

    size_t hash(uint32_t key) const {
        return key % capacity;
    }

    size_t hash(const string& key) const {
        size_t hashVal = 0;
        for (size_t i = 0; i < key.length(); i++) {
            hashVal = (hashVal * 31 + static_cast<size_t>(key[i])) % capacity;
        }
        return hashVal;
    }

    size_t findSlot(const K& key) const {
        size_t index = hash(key);
        size_t originalIndex = index;

        while (table[index].occupied) {
            if (table[index].key == key) {
                return index;
            }
            index = (index + 1) % capacity;

            if (index == originalIndex) {
                return capacity;
            }
        }
        return index;
    }

public:

    HashTable(size_t cap = HASH_TABLE_SIZE)
        : capacity(cap), size(0) {
        table = new KeyValuePair<K, V>[capacity];
    }

    ~HashTable() {
        delete[] table;
    }


    void insert(const K& key, const V& value) {
        size_t index = findSlot(key);

        if (index >= capacity) {
            return;
        }

        if (!table[index].occupied) {
            size++;
        }

        table[index] = KeyValuePair<K, V>(key, value);
    }

    bool find(const K& key, V& value) const {
        size_t index = findSlot(key);

        if (index < capacity && table[index].occupied) {
            value = table[index].value;
            return true;
        }
        return false;
    }

    bool contains(const K& key) const {
        size_t index = findSlot(key);
        return (index < capacity && table[index].occupied);
    }

    void remove(const K& key) {
        size_t index = findSlot(key);

        if (index < capacity && table[index].occupied) {
            table[index].occupied = false;
            size--;
        }
    }

    size_t getSize() const {
        return size;
    }

    vector<K> getAllKeys() const {
        vector<K> keys;
        keys.reserve(size);

        for (size_t i = 0; i < capacity; i++) {
            if (table[i].occupied) {
                keys.push_back(table[i].key);
            }
        }
        return keys;
    }

    vector<KeyValuePair<K, V>> getAllPairs() const {
        vector<KeyValuePair<K, V>> pairs;
        pairs.reserve(size);

        for (size_t i = 0; i < capacity; i++) {
            if (table[i].occupied) {
                pairs.push_back(table[i]);
            }
        }
        return pairs;
    }

    void clear() {
        for (size_t i = 0; i < capacity; i++) {
            table[i].occupied = false;
        }
        size = 0;
    }

};

#endif