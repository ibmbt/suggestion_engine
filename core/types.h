#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cstddef>


const size_t PAGE_SIZE = 4096;
const int BTREE_DEGREE = 64;
const size_t HASH_TABLE_SIZE = 10007;
const float MIN_RATING = 1.0f;
const float MAX_RATING = 5.0f;


enum class NodeType : uint8_t {
    USER = 0,
    MOVIE = 1
};

enum class EdgeType : uint8_t {
    RATED = 0,
    SIMILAR = 1
};

#endif // TYPES_H