#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <vector>
#include <string>
#include "types.h"
using namespace std;

template<typename K, typename V>
struct HashNode {
    K key;
    V value;
    HashNode* next;

    HashNode(K k, V v) : key(k), value(v), next(nullptr) {}
};

template<typename K, typename V>
class HashTable {
private:
    HashNode<K, V>** table;
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

public:
    HashTable(size_t cap = HASH_TABLE_SIZE)
        : capacity(cap), size(0) {
        table = new HashNode<K, V>* [capacity];
        for (size_t i = 0; i < capacity; i++) {
            table[i] = nullptr;
        }
    }

    ~HashTable() {
        clear();
        delete[] table;
    }

    void insert(const K& key, const V& value) {
        size_t index = hash(key);

        HashNode<K, V>* current = table[index];
        while (current != nullptr) {
            if (current->key == key) {
                current->value = value;
                return;
            }
            current = current->next;
        }

        HashNode<K, V>* newNode = new HashNode<K, V>(key, value);
        newNode->next = table[index];
        table[index] = newNode;
        size++;
    }

    vector<K> getAllKeys() const {
        vector<K> keys;
        keys.reserve(size);

        for (size_t i = 0; i < capacity; i++) {
            HashNode<K, V>* current = table[i];
            while (current != nullptr) {
                keys.push_back(current->key);
                current = current->next;
            }
        }
        return keys;
    }

    vector<pair<K, V>> getAllPairs() const {
        vector<pair<K, V>> pairs;
        pairs.reserve(size);

        for (size_t i = 0; i < capacity; i++) {
            HashNode<K, V>* current = table[i];
            while (current != nullptr) {
                pairs.push_back(make_pair(current->key, current->value));
                current = current->next;
            }
        }
        return pairs;
    }

    bool find(const K& key, V& value) const {
        size_t index = hash(key);
        HashNode<K, V>* current = table[index];

        while (current != nullptr) {
            if (current->key == key) {
                value = current->value;
                return true;
            }
            current = current->next;
        }
        return false;
    }

    bool contains(const K& key) const {
        size_t index = hash(key);
        HashNode<K, V>* current = table[index];

        while (current != nullptr) {
            if (current->key == key) {
                return true;
            }
            current = current->next;
        }
        return false;
    }

    void remove(const K& key) {
        size_t index = hash(key);
        HashNode<K, V>* current = table[index];
        HashNode<K, V>* prev = nullptr;

        while (current != nullptr) {
            if (current->key == key) {
                if (prev == nullptr) {
                    table[index] = current->next;
                }
                else {
                    prev->next = current->next;
                }
                delete current;
                size--;
                return;
            }
            prev = current;
            current = current->next;
        }
    }

    size_t getSize() const {
        return size;
    }

    void clear() {
        for (size_t i = 0; i < capacity; i++) {
            HashNode<K, V>* current = table[i];
            while (current != nullptr) {
                HashNode<K, V>* temp = current;
                current = current->next;
                delete temp;
            }
            table[i] = nullptr;
        }
        size = 0;
    }
};

#endif
