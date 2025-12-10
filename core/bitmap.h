#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>
#include <cstring>
#include "types.h"

class Bitmap {
private:
    uint8_t* data;
    size_t numBlocks;

public:
    Bitmap(size_t totalBlocks) : numBlocks(totalBlocks) {
        size_t byteSize = numBlocks / 8;
        if (numBlocks % 8 != 0)
            byteSize++;

        data = new uint8_t[byteSize];
        memset(data, 0, byteSize);

        setBit(0);
    }

    ~Bitmap() {
        delete[] data;
    }

    void setBit(size_t blockNum) {
        size_t byteIndex = blockNum / 8;
        size_t bitIndex = blockNum % 8;

        switch (bitIndex) {
        case 7: data[byteIndex] = data[byteIndex] | 0x80; break;
        case 6: data[byteIndex] = data[byteIndex] | 0x40; break;
        case 5: data[byteIndex] = data[byteIndex] | 0x20; break;
        case 4: data[byteIndex] = data[byteIndex] | 0x10; break;
        case 3: data[byteIndex] = data[byteIndex] | 0x08; break;
        case 2: data[byteIndex] = data[byteIndex] | 0x04; break;
        case 1: data[byteIndex] = data[byteIndex] | 0x02; break;
        case 0: data[byteIndex] = data[byteIndex] | 0x01; break;
        }
    }



    void clearBit(size_t blockNum) {
        size_t byteIndex = blockNum / 8;
        size_t bitIndex = blockNum % 8;

        switch (bitIndex) {
        case 7: data[byteIndex] = data[byteIndex] & (~0x80); break;
        case 6: data[byteIndex] = data[byteIndex] & (~0x40); break;
        case 5: data[byteIndex] = data[byteIndex] & (~0x20); break;
        case 4: data[byteIndex] = data[byteIndex] & (~0x10); break;
        case 3: data[byteIndex] = data[byteIndex] & (~0x08); break;
        case 2: data[byteIndex] = data[byteIndex] & (~0x04); break;
        case 1: data[byteIndex] = data[byteIndex] & (~0x02); break;
        case 0: data[byteIndex] = data[byteIndex] & (~0x01); break;
        }
    }



    bool isFree(size_t blockNum) const {
        size_t byteIndex = blockNum / 8;
        size_t bitIndex = blockNum % 8;

        uint8_t byte = data[byteIndex];

        switch (bitIndex) {
        case 7: return (byte & 0x80) == 0;
        case 6: return (byte & 0x40) == 0;
        case 5: return (byte & 0x20) == 0;
        case 4: return (byte & 0x10) == 0;
        case 3: return (byte & 0x08) == 0;
        case 2: return (byte & 0x04) == 0;
        case 1: return (byte & 0x02) == 0;
        case 0: return (byte & 0x01) == 0;
        }

        return false;
    }


    size_t findFreeBlock() const {
        for (size_t i = 1; i < numBlocks; i++) {
            if (isFree(i)) return i;
        }
        return numBlocks;
    }

    void serialize(char* buffer) const {
        size_t byteSize = numBlocks / 8;
        if (numBlocks % 8 != 0)
            byteSize++;

        memcpy(buffer, data, byteSize);
    }

    void deserialize(const char* buffer) {
        size_t byteSize = numBlocks / 8;
        if (numBlocks % 8 != 0)
            byteSize++;

        memcpy(data, buffer, byteSize);
    }

    size_t getByteSize() const {
        size_t byteSize = numBlocks / 8;
        if (numBlocks % 8 != 0)
            byteSize++;
        return byteSize;
    }
};

#endif