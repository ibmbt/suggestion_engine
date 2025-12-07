#ifndef EDGE_H
#define EDGE_H

#include <cstdint>
#include <ctime>
#include <cstring>
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


    void serialize(char* buffer) const {
        size_t offset = 0;

        memcpy(buffer + offset, &fromID, sizeof(uint32_t));
        offset = offset + sizeof(uint32_t);

        memcpy(buffer + offset, &toID, sizeof(uint32_t));
        offset = offset + sizeof(uint32_t);

        memcpy(buffer + offset, &type, sizeof(EdgeType));
        offset = offset + sizeof(EdgeType);

        memcpy(buffer + offset, &weight, sizeof(float));
        offset = offset + sizeof(float);

        memcpy(buffer + offset, &timestamp, sizeof(uint64_t));
        offset = offset + sizeof(uint64_t);

        memcpy(buffer + offset, &nextEdgeOffset, sizeof(uint64_t));
        offset = offset + sizeof(uint64_t);
    }

    static Edge deserialize(const char* buffer) {
        Edge edge;
        size_t offset = 0;

        memcpy(&edge.fromID, buffer + offset, sizeof(uint32_t));
        offset = offset + sizeof(uint32_t);

        memcpy(&edge.toID, buffer + offset, sizeof(uint32_t));
        offset = offset + sizeof(uint32_t);

        memcpy(&edge.type, buffer + offset, sizeof(EdgeType));
        offset = offset + sizeof(EdgeType);

        memcpy(&edge.weight, buffer + offset, sizeof(float));
        offset = offset + sizeof(float);

        memcpy(&edge.timestamp, buffer + offset, sizeof(uint64_t));
        offset = offset + sizeof(uint64_t);

        memcpy(&edge.nextEdgeOffset, buffer + offset, sizeof(uint64_t));
        offset = offset + sizeof(uint64_t);

        return edge;
    }

};

#endif // EDGE_H