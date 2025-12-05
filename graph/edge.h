#ifndef EDGE_H
#define EDGE_H

#include <cstdint>
#include <ctime>
#include "core/types.h"

struct RatingEntry {
    uint32_t movieID;
    float rating;
    uint64_t timestamp;

    RatingEntry() : movieID(0), rating(0.0f), timestamp(0) {}

    RatingEntry(uint32_t mID, float r, uint64_t t = 0)
        : movieID(mID), rating(r), timestamp(t) {
        if (t == 0) {
            timestamp = static_cast<uint64_t>(time(nullptr));
        }
    }

};

class Edge {
public:
    uint32_t fromID;
    uint32_t toID;
    EdgeType type;
    float weight;
    uint64_t timestamp;
    uint64_t nextEdgeOffset;

    Edge()
        : fromID(0), toID(0), type(EdgeType::RATED),
        weight(0.0f), timestamp(0), nextEdgeOffset(0) {
    }

    Edge(uint32_t from, uint32_t to, EdgeType t, float w)
        : fromID(from), toID(to), type(t), weight(w),
        nextEdgeOffset(0) {
        timestamp = static_cast<uint64_t>(time(nullptr));
    }


    void serialize(char* buffer) const;
    static Edge deserialize(const char* buffer);
    static const size_t SERIALIZED_SIZE = 33;
};

#endif // EDGE_H