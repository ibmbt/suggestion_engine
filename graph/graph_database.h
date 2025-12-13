#ifndef GRAPH_DATABASE_H
#define GRAPH_DATABASE_H

#include "../core/btree.h"
#include "../core/bitmap.h"
#include "../core/hash_table.h"
#include "node.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <cstring>

using namespace std;

class GraphDatabase {
private:
    BTree<uint32_t, uint64_t>* userIndex;
    BTree<uint32_t, uint64_t>* movieIndex;

    // Data files
    fstream userDataFile;
    fstream movieDataFile;

    Bitmap* userSlotBitmap;
    Bitmap* movieSlotBitmap;

    uint32_t maxUserSlot;
    uint32_t maxMovieSlot;

    uint32_t totalUsers;
    uint32_t totalMovies;


    const string USER_DATA_FILE = "users.dat";
    const string MOVIE_DATA_FILE = "movies.dat";
    const string USER_INDEX_FILE = "user_index.dat";
    const string MOVIE_INDEX_FILE = "movie_index.dat";
    const string METADATA_FILE = "metadata.dat";

    uint64_t slotToOffset(uint32_t slot, size_t nodeSize) const {
        return METADATA_SIZE + (static_cast<uint64_t>(slot) * nodeSize);
    }

    uint32_t offsetToSlot(uint64_t offset, size_t nodeSize) const {
        return static_cast<uint32_t>((offset - METADATA_SIZE) / nodeSize);
    }

    static const size_t METADATA_HEADER_SIZE = 32;

    void openDataFiles() {
        userDataFile.open(USER_DATA_FILE, ios::in | ios::out | ios::binary);
        if (!userDataFile.is_open()) {
            userDataFile.open(USER_DATA_FILE, ios::out | ios::binary);
            userDataFile.close();
            userDataFile.open(USER_DATA_FILE, ios::in | ios::out | ios::binary);
        }

        movieDataFile.open(MOVIE_DATA_FILE, ios::in | ios::out | ios::binary);
        if (!movieDataFile.is_open()) {
            movieDataFile.open(MOVIE_DATA_FILE, ios::out | ios::binary);
            movieDataFile.close();
            movieDataFile.open(MOVIE_DATA_FILE, ios::in | ios::out | ios::binary);
        }
    }

    void writeUser(uint64_t offset, const User& node) {
        char buffer[User::getSize()];
        node.serialize(buffer);

        userDataFile.seekp(offset, ios::beg);
        userDataFile.write(buffer, User::getSize());
        userDataFile.flush();

        if (userDataFile.fail()) {
            throw runtime_error("Failed to write user node");
        }
    }

    User readUser(uint64_t offset) {
        char buffer[User::getSize()];

        userDataFile.seekg(offset, ios::beg);
        userDataFile.read(buffer, User::getSize());

        if (userDataFile.fail() || userDataFile.gcount() != static_cast<streamsize>(User::getSize())) {
            throw runtime_error("Failed to read user node");
        }

        return User::deserialize(buffer);
    }

    void writeMovie(uint64_t offset, const Movie& node) {
        char buffer[Movie::getSize()];
        node.serialize(buffer);

        movieDataFile.seekp(offset, ios::beg);
        movieDataFile.write(buffer, Movie::getSize());
        movieDataFile.flush();

        if (movieDataFile.fail()) {
            throw runtime_error("Failed to write movie node");
        }
    }

    Movie readMovie(uint64_t offset) {
        char buffer[Movie::getSize()];

        movieDataFile.seekg(offset, ios::beg);
        movieDataFile.read(buffer, Movie::getSize());

        if (movieDataFile.fail() || movieDataFile.gcount() != static_cast<streamsize>(Movie::getSize())) {
            throw runtime_error("Failed to read movie node");
        }

        return Movie::deserialize(buffer);
    }

    void saveMetadata() {
        fstream metaFile(METADATA_FILE, ios::out | ios::binary | ios::trunc);
        if (!metaFile.is_open()) {
            return;
        }

        metaFile.write(reinterpret_cast<const char*>(&totalUsers), sizeof(uint32_t));
        metaFile.write(reinterpret_cast<const char*>(&totalMovies), sizeof(uint32_t));
        metaFile.write(reinterpret_cast<const char*>(&maxUserSlot), sizeof(uint32_t));
        metaFile.write(reinterpret_cast<const char*>(&maxMovieSlot), sizeof(uint32_t));

        uint32_t userBitmapSize = static_cast<uint32_t>(userSlotBitmap->getByteSize());
        uint32_t movieBitmapSize = static_cast<uint32_t>(movieSlotBitmap->getByteSize());
        metaFile.write(reinterpret_cast<const char*>(&userBitmapSize), sizeof(uint32_t));
        metaFile.write(reinterpret_cast<const char*>(&movieBitmapSize), sizeof(uint32_t));

        char* userBitmapData = new char[userBitmapSize];
        userSlotBitmap->serialize(userBitmapData);
        metaFile.write(userBitmapData, userBitmapSize);
        delete[] userBitmapData;

        char* movieBitmapData = new char[movieBitmapSize];
        movieSlotBitmap->serialize(movieBitmapData);
        metaFile.write(movieBitmapData, movieBitmapSize);
        delete[] movieBitmapData;

        metaFile.close();
    }

    void loadMetadata() {
        fstream metaFile(METADATA_FILE, ios::in | ios::binary);
        if (!metaFile.is_open()) {
            return;
        }

        try {
            metaFile.read(reinterpret_cast<char*>(&totalUsers), sizeof(uint32_t));
            metaFile.read(reinterpret_cast<char*>(&totalMovies), sizeof(uint32_t));
            metaFile.read(reinterpret_cast<char*>(&maxUserSlot), sizeof(uint32_t));
            metaFile.read(reinterpret_cast<char*>(&maxMovieSlot), sizeof(uint32_t));

            if (metaFile.fail()) {
                throw runtime_error("Failed to read metadata header");
            }

            uint32_t userBitmapSize = 0;
            uint32_t movieBitmapSize = 0;
            metaFile.read(reinterpret_cast<char*>(&userBitmapSize), sizeof(uint32_t));
            metaFile.read(reinterpret_cast<char*>(&movieBitmapSize), sizeof(uint32_t));

            if (metaFile.fail()) {
                throw runtime_error("Failed to read bitmap sizes");
            }

            char* userBitmapData = new char[userBitmapSize];
            metaFile.read(userBitmapData, userBitmapSize);
            if (!metaFile.fail()) {
                userSlotBitmap->deserialize(userBitmapData);
            }
            delete[] userBitmapData;

            char* movieBitmapData = new char[movieBitmapSize];
            metaFile.read(movieBitmapData, movieBitmapSize);
            if (!metaFile.fail()) {
                movieSlotBitmap->deserialize(movieBitmapData);
            }
            delete[] movieBitmapData;

        }
        catch (...) {
            totalUsers = 0;
            totalMovies = 0;
            maxUserSlot = 0;
            maxMovieSlot = 0;
        }

        metaFile.close();
    }

public:
    GraphDatabase() {
        userIndex = new BTree<uint32_t, uint64_t>(USER_INDEX_FILE);
        movieIndex = new BTree<uint32_t, uint64_t>(MOVIE_INDEX_FILE);

        userSlotBitmap = new Bitmap(MAX_SLOTS);
        movieSlotBitmap = new Bitmap(MAX_SLOTS);

        openDataFiles();

        maxUserSlot = 0;
        maxMovieSlot = 0;
        totalUsers = 0;
        totalMovies = 0;

        loadMetadata();
    }

    ~GraphDatabase() {
        saveMetadata();

        delete userIndex;
        delete movieIndex;
        delete userSlotBitmap;
        delete movieSlotBitmap;

        if (userDataFile.is_open()) userDataFile.close();
        if (movieDataFile.is_open()) movieDataFile.close();
    }


    void addUser(uint32_t userID, const string& username) {
        uint64_t existingOffset;
        if (userIndex->search(userID, existingOffset)) {
            throw runtime_error("User already exists");
        }

        User node(userID, username);

        uint32_t slot = userSlotBitmap->findFreeBlock();
        if (slot >= MAX_SLOTS) {
            throw runtime_error("User storage capacity exceeded");
        }

        userSlotBitmap->setBit(slot);

        if (slot > maxUserSlot) {
            maxUserSlot = slot;
        }

        uint64_t offset = slotToOffset(slot, User::getSize());

        writeUser(offset, node);

        userIndex->insert(userID, offset);

        totalUsers++;

    }

    User getUser(uint32_t userID) {

        uint64_t offset;
        if (!userIndex->search(userID, offset)) {
            throw runtime_error("User does not exist");
        }

        User node = readUser(offset);

        if (node.userID != userID) {
            throw runtime_error("Data corruption: ID mismatch");
        }
        return node;
    }

    void updateUser(uint32_t userID, const User& node) {
        uint64_t offset;
        if (!userIndex->search(userID, offset)) {
            throw runtime_error("User does not exist");
        }

        writeUser(offset, node);
    }

    bool userExists(uint32_t userID) {
        uint64_t offset;
        return userIndex->search(userID, offset);
    }

    void deleteUser(uint32_t userID) {
        uint64_t offset;
        if (!userIndex->search(userID, offset)) {
            throw runtime_error("User does not exist");
        }

        uint32_t slot = offsetToSlot(offset, User::getSize());
        userSlotBitmap->clearBit(slot);

        totalUsers--;
    }

    void addMovie(uint32_t movieID, const string& title, const vector<string>& genres) {
        uint64_t existingOffset;
        if (movieIndex->search(movieID, existingOffset)) {
            throw runtime_error("Movie already exists");
        }

        Movie node(movieID, title, genres);

        uint32_t slot = movieSlotBitmap->findFreeBlock();
        if (slot >= MAX_SLOTS) {
            throw runtime_error("Movie storage capacity exceeded");
        }

        movieSlotBitmap->setBit(slot);

        if (slot > maxMovieSlot) {
            maxMovieSlot = slot;
        }

        uint64_t offset = slotToOffset(slot, Movie::getSize());

        writeMovie(offset, node);

        movieIndex->insert(movieID, offset);

        totalMovies++;

    }

    Movie getMovie(uint32_t movieID) {


        uint64_t offset;
        if (!movieIndex->search(movieID, offset)) {
            throw runtime_error("Movie does not exist");
        }

        Movie node = readMovie(offset);

        if (node.movieID != movieID) {
            throw runtime_error("Data corruption: ID mismatch");
        }


        return node;
    }

    void updateMovie(uint32_t movieID, const Movie& node) {
        uint64_t offset;
        if (!movieIndex->search(movieID, offset)) {
            throw runtime_error("Movie does not exist");
        }

        writeMovie(offset, node);
    }

    bool movieExists(uint32_t movieID) {
        uint64_t offset;
        return movieIndex->search(movieID, offset);
    }

    void deleteMovie(uint32_t movieID) {
        uint64_t offset;
        if (!movieIndex->search(movieID, offset)) {
            throw runtime_error("Movie does not exist");
        }

        uint32_t slot = offsetToSlot(offset, Movie::getSize());
        movieSlotBitmap->clearBit(slot);

        totalMovies--;
    }

    vector<uint32_t> getAllUserIDs() {
        vector<uint32_t> ids;
        ids.reserve(totalUsers);

        vector<pair<uint32_t, uint64_t>> pairs = userIndex->getAllPairs();

        for (size_t i = 0; i < pairs.size(); i++) {
            ids.push_back(pairs[i].first);
        }

        return ids;
    }

    vector<uint32_t> getAllMovieIDs() {
        vector<uint32_t> ids;
        ids.reserve(totalMovies);

        vector<pair<uint32_t, uint64_t>> pairs = movieIndex->getAllPairs();

        for (size_t i = 0; i < pairs.size(); i++) {
            ids.push_back(pairs[i].first);
        }

        return ids;
    }

    uint32_t getUserCount() const {
        return totalUsers;
    }

    uint32_t getMovieCount() const {
        return totalMovies;
    }

};

#endif 