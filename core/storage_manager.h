#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include "bitmap.h"
#include "types.h"
#include "../graph/node.h"
#include "../graph/edge.h"

using namespace std;

class StorageManager {
private:
    fstream file;
    Bitmap* bitmap;
    size_t totalBlocks;
    string dataFile;

    void initializeStorage() {
        char* block = new char[BLOCK_SIZE];
        memset(block, 0, BLOCK_SIZE);

        file.clear();
        file.seekp(0, ios::beg);
        file.write(block, BLOCK_SIZE);
        file.flush();

        delete[] block;

        bitmap->setBit(0);
        saveBitmap();
    }

    void loadBitmap() {
        char* block = new char[BLOCK_SIZE];

        file.clear();
        file.seekg(0, ios::beg);
        file.read(block, BLOCK_SIZE);

        if (!file.fail()) {
            bitmap->deserialize(block);
        }

        delete[] block;
    }

    void saveBitmap() {
        char* block = new char[BLOCK_SIZE];
        memset(block, 0, BLOCK_SIZE);
        bitmap->serialize(block);

        file.clear();
        file.seekp(0, ios::beg);
        file.write(block, BLOCK_SIZE);
        file.flush();

        delete[] block;
    }

public:
    StorageManager(const  string& filename, size_t blocks = 10000)
        : dataFile(filename), totalBlocks(blocks) {

        bitmap = new Bitmap(totalBlocks);

        file.open(dataFile, ios::in | ios::out | ios::binary);

        if (!file.is_open()) {
            file.open(dataFile, ios::out | ios::binary);
            file.close();
            file.open(dataFile, ios::in | ios::out | ios::binary);
            initializeStorage();
        }
        else {
            loadBitmap();
        }
    }

    ~StorageManager() {
        if (file.is_open()) {
            saveBitmap();
            file.close();
        }
        delete bitmap;
    }


    uint64_t allocateBlock() {
        size_t blockNum = bitmap->findFreeBlock();
        if (blockNum >= totalBlocks) {
            throw  runtime_error("No free blocks available");
        }
        bitmap->setBit(blockNum);
        saveBitmap();
        return blockNum * BLOCK_SIZE;
    }

    void freeBlock(uint64_t offset) {
        size_t blockNum = offset / BLOCK_SIZE;
        if (blockNum > 0 && blockNum < totalBlocks) {
            bitmap->clearBit(blockNum);
            saveBitmap();
        }
    }

    void writeBlock(uint64_t offset, const char* data, size_t size) {
        if (size > BLOCK_SIZE) {
            throw  runtime_error("Data exceeds block size");
        }

        file.clear();
        file.seekp(offset, ios::beg);
        file.write(data, size);
        file.flush();

        if (file.fail()) {
            throw  runtime_error("Disk write failed at offset " + to_string(offset));
        }
    }

    void readBlock(uint64_t offset, char* buffer, size_t size) {
        file.clear();
        file.seekg(offset, ios::beg);
        file.read(buffer, size);

        if (file.gcount() != static_cast<streamsize>(size)) {
            throw runtime_error("Disk read failed at offset " + to_string(offset) +
                ". Expected " + to_string(size) +
                ", got " + to_string(file.gcount()));
        }
    }

    uint64_t storeEdges(const  vector<Edge>& edges) {
        if (edges.empty()) return 0;

        uint64_t firstOffset = 0;
        uint64_t prevOffset = 0;

        for (size_t i = 0; i < edges.size(); i++) {
            Edge edge = edges[i];
            uint64_t currentOffset = allocateBlock();

            if (i == 0) {
                firstOffset = currentOffset;
            }
            else {
                updateNextEdgeOffset(prevOffset, currentOffset);
            }

            char buffer[BLOCK_SIZE];
            memset(buffer, 0, BLOCK_SIZE);
            edge.serialize(buffer);
            writeBlock(currentOffset, buffer, BLOCK_SIZE);

            prevOffset = currentOffset;
        }

        return firstOffset;
    }

    vector<Edge> loadEdges(uint64_t offset) {
        vector<Edge> edges;

        int loopSafety = 0;

        while (offset != 0 && loopSafety < 10000) {
            char buffer[BLOCK_SIZE];
            memset(buffer, 0, BLOCK_SIZE);
            readBlock(offset, buffer, BLOCK_SIZE);

            Edge edge = Edge::deserialize(buffer);
            edges.push_back(edge);

            offset = edge.nextEdgeOffset;
            loopSafety++;
        }

        return edges;
    }

    void freeEdgeChain(uint64_t offset) {
        int loopSafety = 0;
        while (offset != 0 && loopSafety < 10000) {
            char buffer[BLOCK_SIZE];
            memset(buffer, 0, BLOCK_SIZE);
            readBlock(offset, buffer, BLOCK_SIZE);

            Edge edge = Edge::deserialize(buffer);
            uint64_t nextOffset = edge.nextEdgeOffset;

            freeBlock(offset);
            offset = nextOffset;
            loopSafety++;
        }
    }

    uint64_t storeUser(User& user) {
        if (!user.ratedMovies.empty()) {
            user.ratedMoviesOffset = storeEdges(user.ratedMovies);
        }

        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);
        size_t size;
        user.serialize(buffer, size);

        uint64_t offset = allocateBlock();
        writeBlock(offset, buffer, BLOCK_SIZE);
        return offset;
    }

    User loadUser(uint64_t offset) {
        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);

        readBlock(offset, buffer, BLOCK_SIZE);

        size_t bytesRead;
        User user = User::deserialize(buffer, bytesRead);

        if (user.ratedMoviesOffset != 0) {
            user.ratedMovies = loadEdges(user.ratedMoviesOffset);
        }

        return user;
    }

    void updateUser(uint64_t offset, User& user) {
        User oldUser = loadUser(offset);
        if (oldUser.ratedMoviesOffset != 0) {
            freeEdgeChain(oldUser.ratedMoviesOffset);
        }

        if (!user.ratedMovies.empty()) {
            user.ratedMoviesOffset = storeEdges(user.ratedMovies);
        }

        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);
        size_t size;
        user.serialize(buffer, size);

        writeBlock(offset, buffer, BLOCK_SIZE);
    }


    uint64_t storeMovie(Movie& movie) {
        if (!movie.ratedByUsers.empty()) {
            movie.ratedByUsersOffset = storeEdges(movie.ratedByUsers);
        }
        if (!movie.similarMovies.empty()) {
            movie.similarMoviesOffset = storeEdges(movie.similarMovies);
        }

        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);
        size_t size;
        movie.serialize(buffer, size);

        uint64_t offset = allocateBlock();
        writeBlock(offset, buffer, BLOCK_SIZE);
        return offset;
    }

    Movie loadMovie(uint64_t offset) {
        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);

        readBlock(offset, buffer, BLOCK_SIZE);

        size_t bytesRead;
        Movie movie = Movie::deserialize(buffer, bytesRead);

        if (movie.ratedByUsersOffset != 0) {
            movie.ratedByUsers = loadEdges(movie.ratedByUsersOffset);
        }
        if (movie.similarMoviesOffset != 0) {
            movie.similarMovies = loadEdges(movie.similarMoviesOffset);
        }

        return movie;
    }

    void updateMovie(uint64_t offset, Movie& movie) {
        Movie oldMovie = loadMovie(offset);
        if (oldMovie.ratedByUsersOffset != 0) {
            freeEdgeChain(oldMovie.ratedByUsersOffset);
        }
        if (oldMovie.similarMoviesOffset != 0) {
            freeEdgeChain(oldMovie.similarMoviesOffset);
        }

        if (!movie.ratedByUsers.empty()) {
            movie.ratedByUsersOffset = storeEdges(movie.ratedByUsers);
        }
        if (!movie.similarMovies.empty()) {
            movie.similarMoviesOffset = storeEdges(movie.similarMovies);
        }

        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);
        size_t size;
        movie.serialize(buffer, size);

        writeBlock(offset, buffer, BLOCK_SIZE);
    }

private:
    void updateNextEdgeOffset(uint64_t edgeOffset, uint64_t nextOffset) {
        char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE);

        readBlock(edgeOffset, buffer, BLOCK_SIZE);

        Edge edge = Edge::deserialize(buffer);
        edge.nextEdgeOffset = nextOffset;

        edge.serialize(buffer);
        writeBlock(edgeOffset, buffer, BLOCK_SIZE);
    }
};

#endif