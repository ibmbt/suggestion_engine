#ifndef BTREE_H
#define BTREE_H

#include <cstdint>
#include <vector>
#include <fstream>
#include <cstring>
#include <stdexcept>
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

    BTreeNode() : isLeaf(true), numKeys(0), nodeOffset(0) {
        memset(keys, 0, sizeof(keys));
        memset(values, 0, sizeof(values));
        memset(children, 0, sizeof(children));
    }

    void serialize(char* buffer) const {
        size_t offset = 0;

        if (isLeaf) {
            buffer[offset++] = 1;
        }
        else {
            buffer[offset++] = 0;
        }


        memcpy(buffer + offset, &numKeys, sizeof(uint32_t));
        offset = offset + sizeof(uint32_t);

        memcpy(buffer + offset, keys, sizeof(K) * (2 * BTREE_DEGREE - 1));
        offset = offset + sizeof(K) * (2 * BTREE_DEGREE - 1);

        memcpy(buffer + offset, values, sizeof(V) * (2 * BTREE_DEGREE - 1));
        offset = offset + sizeof(V) * (2 * BTREE_DEGREE - 1);

        memcpy(buffer + offset, children, sizeof(uint64_t) * (2 * BTREE_DEGREE));
        offset = offset + sizeof(uint64_t) * (2 * BTREE_DEGREE);

        memcpy(buffer + offset, &nodeOffset, sizeof(uint64_t));
    }

    static BTreeNode<K, V> deserialize(const char* buffer) {
        BTreeNode<K, V> node;
        size_t offset = 0;

        node.isLeaf = (buffer[offset++] == 1);

        memcpy(&node.numKeys, buffer + offset, sizeof(uint32_t));
        offset = offset + sizeof(uint32_t);

        memcpy(node.keys, buffer + offset, sizeof(K) * (2 * BTREE_DEGREE - 1));
        offset = offset + sizeof(K) * (2 * BTREE_DEGREE - 1);

        memcpy(node.values, buffer + offset, sizeof(V) * (2 * BTREE_DEGREE - 1));
        offset = offset + sizeof(V) * (2 * BTREE_DEGREE - 1);

        memcpy(node.children, buffer + offset, sizeof(uint64_t) * (2 * BTREE_DEGREE));
        offset = offset + sizeof(uint64_t) * (2 * BTREE_DEGREE);

        memcpy(&node.nodeOffset, buffer + offset, sizeof(uint64_t));

        return node;
    }

    static size_t getSerializedSize() {
        return 1 + sizeof(uint32_t) +
            sizeof(K) * (2 * BTREE_DEGREE - 1) +
            sizeof(V) * (2 * BTREE_DEGREE - 1) +
            sizeof(uint64_t) * (2 * BTREE_DEGREE) +
            sizeof(uint64_t);
    }

};

template<typename K, typename V>
class BTree {
private:
    uint64_t rootOffset;
    string indexFile;
    fstream file;
    uint64_t nextFreeOffset;

    BTreeNode<K, V> readNode(uint64_t offset) {
        if (offset == 0) {
            throw runtime_error("Cannot read at offset 0");
        }

        size_t serialSize = BTreeNode<K, V>::getSerializedSize();
        char* buffer = new char[serialSize];
        memset(buffer, 0, serialSize);

        file.seekg(offset, ios::beg);
        file.read(buffer, serialSize);

        if (file.fail() || file.gcount() != static_cast<streamsize>(serialSize)) {
            file.clear();
            delete[] buffer;
            throw runtime_error("Failed to read from disk");
        }

        BTreeNode<K, V> node = BTreeNode<K, V>::deserialize(buffer);
        delete[] buffer;
        return node;
    }

    void writeNode(BTreeNode<K, V>& node, uint64_t offset) {
        if (offset == 0) {
            throw runtime_error("Cannot write at offset 0");
        }

        node.nodeOffset = offset;

        size_t serialSize = BTreeNode<K, V>::getSerializedSize();
        char* buffer = new char[serialSize];
        memset(buffer, 0, serialSize);

        node.serialize(buffer);

        file.seekp(offset, ios::beg);
        file.write(buffer, serialSize);
        file.flush();

        if (file.fail()) {
            delete[] buffer;
            throw runtime_error("Failed to write to disk");
        }

        delete[] buffer;
    }

    uint64_t allocateNode() {
        uint64_t offset = nextFreeOffset;
        size_t nodeSize = BTreeNode<K, V>::getSerializedSize();
        nextFreeOffset = nextFreeOffset + nodeSize;

        if (nextFreeOffset % 64 != 0) {
            nextFreeOffset = ((nextFreeOffset / 64) + 1) * 64;
        }

        return offset;
    }

    void splitChild(BTreeNode<K, V>& parent, uint32_t childIndex) {
        uint64_t fullChildOffset = parent.children[childIndex];
        BTreeNode<K, V> fullChild = readNode(fullChildOffset);

        if (fullChild.numKeys != 2 * BTREE_DEGREE - 1) {
            throw runtime_error("Trying to split non-full child");
        }

        BTreeNode<K, V> newChild;
        newChild.isLeaf = fullChild.isLeaf;
        newChild.numKeys = BTREE_DEGREE - 1;

        for (uint32_t j = 0; j < BTREE_DEGREE - 1; j++) {
            newChild.keys[j] = fullChild.keys[j + BTREE_DEGREE];
            newChild.values[j] = fullChild.values[j + BTREE_DEGREE];
        }

        if (!fullChild.isLeaf) {
            for (uint32_t j = 0; j < BTREE_DEGREE; j++) {
                newChild.children[j] = fullChild.children[j + BTREE_DEGREE];
            }
        }

        fullChild.numKeys = BTREE_DEGREE - 1;

        K midKey = fullChild.keys[BTREE_DEGREE - 1];
        V midValue = fullChild.values[BTREE_DEGREE - 1];

        uint64_t newChildOffset = allocateNode();
        writeNode(newChild, newChildOffset);
        writeNode(fullChild, fullChildOffset);

        for (int32_t j = parent.numKeys - 1; j >= static_cast<int32_t>(childIndex); j--) {
            parent.keys[j + 1] = parent.keys[j];
            parent.values[j + 1] = parent.values[j];
        }
        parent.keys[childIndex] = midKey;
        parent.values[childIndex] = midValue;

        for (int32_t j = parent.numKeys; j >= static_cast<int32_t>(childIndex) + 1; j--) {
            parent.children[j + 1] = parent.children[j];
        }
        parent.children[childIndex + 1] = newChildOffset;
        parent.numKeys++;
    }

    void insertNonFull(uint64_t nodeOffset, K key, V value) {
        BTreeNode<K, V> node = readNode(nodeOffset);

        if (node.isLeaf) {
            for (uint32_t i = 0; i < node.numKeys; i++) {
                if (node.keys[i] == key) {
                    node.values[i] = value;
                    writeNode(node, nodeOffset);
                    return;
                }
            }

            int32_t i = static_cast<int32_t>(node.numKeys) - 1;
            while (i >= 0 && key < node.keys[i]) {
                node.keys[i + 1] = node.keys[i];
                node.values[i + 1] = node.values[i];
                i--;
            }

            node.keys[i + 1] = key;
            node.values[i + 1] = value;
            node.numKeys++;

            writeNode(node, nodeOffset);
        }
        else {
            uint32_t i = 0;
            while (i < node.numKeys && key > node.keys[i]) {
                i++;
            }

            if (i < node.numKeys && key == node.keys[i]) {
                node.values[i] = value;
                writeNode(node, nodeOffset);
                return;
            }

            uint64_t childOffset = node.children[i];
            BTreeNode<K, V> child = readNode(childOffset);

            if (child.numKeys == 2 * BTREE_DEGREE - 1) {
                splitChild(node, i);
                writeNode(node, nodeOffset);

                node = readNode(nodeOffset);
                if (key > node.keys[i]) {
                    i++;
                }
                else if (key == node.keys[i]) {
                    node.values[i] = value;
                    writeNode(node, nodeOffset);
                    return;
                }
            }

            insertNonFull(node.children[i], key, value);
        }
    }

    bool searchInternal(uint64_t nodeOffset, K key, V& value) {
        if (nodeOffset == 0) {
            return false;
        }

        BTreeNode<K, V> node = readNode(nodeOffset);

        uint32_t i = 0;
        while (i < node.numKeys && key > node.keys[i]) {
            i++;
        }

        if (i < node.numKeys && key == node.keys[i]) {
            value = node.values[i];
            return true;
        }

        if (node.isLeaf) {
            return false;
        }

        return searchInternal(node.children[i], key, value);
    }

    void traverseInternal(uint64_t nodeOffset, vector<pair<K, V>>& result) {
        if (nodeOffset == 0) {
            return;
        }

        BTreeNode<K, V> node = readNode(nodeOffset);

        for (uint32_t i = 0; i < node.numKeys; i++) {
            if (!node.isLeaf && node.children[i] != 0) {
                traverseInternal(node.children[i], result);
            }
            result.push_back(make_pair(node.keys[i], node.values[i]));
        }

        if (!node.isLeaf && node.children[node.numKeys] != 0) {
            traverseInternal(node.children[node.numKeys], result);
        }
    }

    void saveMetadata() {
        file.seekp(0, ios::beg);
        file.write(reinterpret_cast<const char*>(&rootOffset), sizeof(uint64_t));
        file.write(reinterpret_cast<const char*>(&nextFreeOffset), sizeof(uint64_t));
        file.flush();

        if (file.fail()) {
            throw runtime_error("Failed to save metadata");
        }
    }

    void loadMetadata() {
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(&rootOffset), sizeof(uint64_t));
        file.read(reinterpret_cast<char*>(&nextFreeOffset), sizeof(uint64_t));

        if (file.fail()) {
            file.clear();
            rootOffset = 0;
            nextFreeOffset = METADATA_SIZE;
        }

        if (nextFreeOffset < METADATA_SIZE) {
            nextFreeOffset = METADATA_SIZE;
        }
    }

public:
    BTree(const string& filename) : indexFile(filename), rootOffset(0), nextFreeOffset(METADATA_SIZE) {
        file.open(indexFile, ios::in | ios::out | ios::binary);

        if (!file.is_open()) {
            file.open(indexFile, ios::out | ios::binary);
            if (!file.is_open()) {
                throw runtime_error("Failed to create index file");
            }
            saveMetadata();
            file.close();

            file.open(indexFile, ios::in | ios::out | ios::binary);
            if (!file.is_open()) {
                throw runtime_error("Failed to open index file");
            }
        }
        else {
            loadMetadata();
        }
    }

    ~BTree() {
        close();
    }

    void insert(K key, V value) {
        if (rootOffset == 0) {
            BTreeNode<K, V> root;
            root.isLeaf = true;
            root.numKeys = 1;
            root.keys[0] = key;
            root.values[0] = value;

            rootOffset = allocateNode();
            writeNode(root, rootOffset);
            saveMetadata();
            return;
        }

        BTreeNode<K, V> root = readNode(rootOffset);

        if (root.numKeys == 2 * BTREE_DEGREE - 1) {
            BTreeNode<K, V> newRoot;
            newRoot.isLeaf = false;
            newRoot.numKeys = 0;

            uint64_t oldRootOffset = rootOffset;
            rootOffset = allocateNode();

            newRoot.children[0] = oldRootOffset;
            writeNode(newRoot, rootOffset);

            splitChild(newRoot, 0);
            writeNode(newRoot, rootOffset);
            saveMetadata();

            insertNonFull(rootOffset, key, value);
        }
        else {
            insertNonFull(rootOffset, key, value);
        }
    }

    bool search(K key, V& value) {
        return searchInternal(rootOffset, key, value);
    }

    vector<pair<K, V>> getAllPairs() {
        vector<pair<K, V>> result;
        if (rootOffset != 0) {
            traverseInternal(rootOffset, result);
        }
        return result;
    }

    void close() {
        if (file.is_open()) {
            saveMetadata();
            file.close();
        }
    }

    bool isEmpty() {
        return rootOffset == 0;
    }

    uint64_t getRootOffset() {
        return rootOffset;
    }

    size_t size() {
        return getAllPairs().size();
    }

    void create() {
        file.open(indexFile, ios::out | ios::binary);
        file.close();
        file.open(indexFile, ios::in | ios::out | ios::binary);

        rootOffset = 0;
        nextFreeOffset = sizeof(uint64_t) * 2;

        file.seekp(0);
        file.write(reinterpret_cast<const char*>(&rootOffset), sizeof(uint64_t));
        file.write(reinterpret_cast<const char*>(&nextFreeOffset), sizeof(uint64_t));
        file.flush();
    }

};

#endif
