#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include "../graph/node.h"
#include "./types.h"

using namespace std;

template<typename NodeType>
class FixedStorage {
private:
    fstream file;
    string filename;
    int nodeSize;
    int nodesPerBlock;

public:
    FixedStorage(const string& fname, int nSize, int nPerBlock)
        : filename(fname), nodeSize(nSize), nodesPerBlock(nPerBlock) {

        file.open(filename, ios::in | ios::out | ios::binary);
        if (!file.is_open()) {
            file.open(filename, ios::out | ios::binary);
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);
        }
    }

    ~FixedStorage() {
        if (file.is_open()) {
            file.close();
        }
    }

    void writeNode(uint32_t nodeID, const NodeType& node) {
        uint32_t blockNum = nodeID / nodesPerBlock;
        uint32_t posInBlock = nodeID % nodesPerBlock;
        uint64_t offset = blockNum * BLOCK_SIZE + posInBlock * nodeSize;

        char buffer[MOVIE_NODE_SIZE];
        memset(buffer, 0, nodeSize);
        node.serialize(buffer);

        file.seekp(offset, ios::beg);
        file.write(buffer, nodeSize);
        file.flush();

        if (file.fail()) {
            throw runtime_error("Failed to write node");
        }
    }

    NodeType readNode(uint32_t nodeID) {
        uint32_t blockNum = nodeID / nodesPerBlock;
        uint32_t posInBlock = nodeID % nodesPerBlock;
        uint64_t offset = blockNum * BLOCK_SIZE + posInBlock * nodeSize;

        char buffer[MOVIE_NODE_SIZE];
        memset(buffer, 0, nodeSize);

        file.seekg(offset, ios::beg);
        file.read(buffer, nodeSize);

        if (file.fail()) {
            throw runtime_error("Failed to read node");
        }

        return NodeType::deserialize(buffer);
    }

    bool exists(uint32_t nodeID) {
        try {
            NodeType node = readNode(nodeID);
            return node.id == nodeID;
        }
        catch (...) {
            return false;
        }
    }

    void printStats() {
        file.seekg(0, ios::end);
        uint64_t fileSize = file.tellg();
        uint64_t numBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;

        cout << "File: " << filename << endl;
        cout << "  File size: " << (fileSize / 1024.0) << " KB" << endl;
        cout << "  Blocks used: " << numBlocks << endl;
        cout << "  Node size: " << nodeSize << " bytes" << endl;
        cout << "  Nodes per block: " << nodesPerBlock << endl;
        cout << "  Max nodes: " << (numBlocks * nodesPerBlock) << endl;
    }
};


class EdgeFileManager {
private:
    string baseDir;

    string getEdgeFilename(uint32_t userID) const {
        return baseDir + "user_" + to_string(userID) + ".edges";
    }

public:
    EdgeFileManager(const string& dir = "./") : baseDir(dir) {}

    void writeRatings(uint32_t userID, const vector<RatingEdge>& ratings) {
        string filename = getEdgeFilename(userID);

        // Create directory if it doesn't exist
#ifdef _WIN32
        system(("mkdir " + baseDir + " 2>NUL").c_str());
#else
        system(("mkdir -p " + baseDir + " 2>/dev/null").c_str());
#endif

        fstream file(filename, ios::out | ios::binary | ios::trunc);

        if (!file.is_open()) {
            throw runtime_error("Failed to create edge file: " + filename);
        }

        uint32_t count = static_cast<uint32_t>(ratings.size());
        file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));

        char buffer[RatingEdge::getSize()];

        for (size_t i = 0; i < ratings.size(); i++) {
            const RatingEdge& rating = ratings[i];
            rating.serialize(buffer);
            file.write(buffer, RatingEdge::getSize());
        }

        file.close();
    }

    vector<RatingEdge> readRatings(uint32_t userID) {
        string filename = getEdgeFilename(userID);
        fstream file(filename, ios::in | ios::binary);

        vector<RatingEdge> ratings;

        if (!file.is_open()) {
            return ratings;
        }

        uint32_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

        if (file.fail()) {
            file.close();
            return ratings;
        }

        ratings.reserve(count);
        char buffer[RatingEdge::getSize()];

        for (uint32_t i = 0; i < count; i++) {
            file.read(buffer, RatingEdge::getSize());
            if (file.fail()) break;

            RatingEdge edge = RatingEdge::deserialize(buffer);
            ratings.push_back(edge);
        }

        file.close();
        return ratings;
    }

    void addOrUpdateRating(uint32_t userID, uint32_t movieID, float ratingValue) {
        vector<RatingEdge> ratings = readRatings(userID);

        bool found = false;

        for (size_t i = 0; i < ratings.size(); i++) {
            RatingEdge& edge = ratings[i];
            if (edge.movieID == movieID) {
                edge.setRating(ratingValue);
                found = true;
                break;
            }
        }

        if (!found) {
            ratings.push_back(RatingEdge(movieID, ratingValue));
        }

        writeRatings(userID, ratings);
    }

    bool getRating(uint32_t userID, uint32_t movieID, float& ratingValue) {
        vector<RatingEdge> ratings = readRatings(userID);

        for (size_t i = 0; i < ratings.size(); i++) {
            const RatingEdge& edge = ratings[i];
            if (edge.movieID == movieID) {
                ratingValue = edge.getRating();
                return true;
            }
        }

        return false;
    }

    bool hasRating(uint32_t userID, uint32_t movieID) {
        vector<RatingEdge> ratings = readRatings(userID);

        for (size_t i = 0; i < ratings.size(); i++) {
            const RatingEdge& edge = ratings[i];
            if (edge.movieID == movieID) {
                return true;
            }
        }

        return false;
    }

    void deleteUserEdges(uint32_t userID) {
        string filename = getEdgeFilename(userID);
        remove(filename.c_str());
    }
};

#endif