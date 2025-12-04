#ifndef EDGE_H
#define EDGE_H

#include <cstdint>
#include "core/types.h"

struct RatingEntry {
    uint32_t movieID;
    float rating;
    uint64_t timestamp;

    RatingEntry();
    RatingEntry(uint32_t mid, float r, uint64_t t = 0);
};

class Edge {
public:
    uint32_t fromID;
    uint32_t toID;
    EdgeType type;
    float weight;
    uint64_t timestamp;
    uint64_t nextEdgeOffset;

    Edge();
    Edge(uint32_t from, uint32_t to, EdgeType t, float w);

    void serialize(char* buffer) const;
    static Edge deserialize(const char* buffer);
    static constexpr size_t SERIALIZED_SIZE = 33;
};

#endif // EDGE_H