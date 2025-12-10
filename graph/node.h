#ifndef NODE_H
#define NODE_H

#include <string>
#include <cstdint>
#include <cstring>
#include <vector>
#include <ctime>
#include "../core/types.h"

using namespace std;

struct User {
    uint32_t userID;
    char username[MAX_USERNAME_LENGTH];
    uint32_t totalRatings;
    uint32_t avgRating;
    uint64_t edgeFileOffset;      // if one field for all users

    User() : userID(0), totalRatings(0), avgRating(0), edgeFileOffset(0) {
        memset(username, 0, MAX_USERNAME_LENGTH);
    }

    User(uint32_t id, const string& name)
        : userID(id), totalRatings(0), avgRating(0), edgeFileOffset(0) {
        memset(username, 0, MAX_USERNAME_LENGTH);
        size_t len = name.length();
        if (len >= MAX_USERNAME_LENGTH) len = MAX_USERNAME_LENGTH - 1;
        memcpy(username, name.c_str(), len);
    }

    string getUsername() const {
        return string(username);
    }

    void setUsername(const string& name) {
        memset(username, 0, MAX_USERNAME_LENGTH);
        size_t len = name.length();
        if (len >= MAX_USERNAME_LENGTH) len = MAX_USERNAME_LENGTH - 1;
        memcpy(username, name.c_str(), len);
    }

    float getAvgRating() const {
        return avgRating / 100.0f;
    }

    void setAvgRating(float avg) {
        avgRating = static_cast<uint32_t>(avg * 100);
    }

    void serialize(char* buffer) const {
        size_t offset = 0;
        memcpy(buffer + offset, &userID, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, username, MAX_USERNAME_LENGTH);
        offset += MAX_USERNAME_LENGTH;
        memcpy(buffer + offset, &totalRatings, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &avgRating, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &edgeFileOffset, sizeof(uint64_t));
    }

    static User deserialize(const char* buffer) {
        User node;
        size_t offset = 0;
        memcpy(&node.userID, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(node.username, buffer + offset, MAX_USERNAME_LENGTH);
        offset += MAX_USERNAME_LENGTH;
        memcpy(&node.totalRatings, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(&node.avgRating, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(&node.edgeFileOffset, buffer + offset, sizeof(uint64_t));
        return node;
    }

    static constexpr size_t getSize() {
        return USER_NODE_SIZE;
    }
};

struct Movie {
    uint32_t movieID;
    char title[MAX_TITLE_LENGTH];
    char genres[MAX_GENRES][MAX_GENRE_LENGTH];
    uint32_t genreCount;
    uint32_t ratingCount;
    uint32_t sumRating;
    uint64_t reserved;

    Movie() : movieID(0), genreCount(0), ratingCount(0), sumRating(0), reserved(0) {
        memset(title, 0, MAX_TITLE_LENGTH);
        memset(genres, 0, MAX_GENRES * MAX_GENRE_LENGTH);
    }

    Movie(uint32_t id, const string& t, const vector<string>& g)
        : movieID(id), genreCount(0), ratingCount(0), sumRating(0), reserved(0) {
        memset(title, 0, MAX_TITLE_LENGTH);
        memset(genres, 0, MAX_GENRES * MAX_GENRE_LENGTH);

        setTitle(t);
        setGenres(g);
    }

    string getTitle() const {
        return string(title);
    }

    void setTitle(const string& t) {
        memset(title, 0, MAX_TITLE_LENGTH);
        size_t len = t.length();
        if (len >= MAX_TITLE_LENGTH) len = MAX_TITLE_LENGTH - 1;
        memcpy(title, t.c_str(), len);
    }

    vector<string> getGenres() const {
        vector<string> result;
        for (uint32_t i = 0; i < genreCount && i < MAX_GENRES; i++) {
            result.push_back(string(genres[i]));
        }
        return result;
    }

    void setGenres(const vector<string>& g) {
        genreCount = 0;
        for (size_t i = 0; i < g.size() && i < MAX_GENRES; i++) {
            size_t len = g[i].length();
            if (len >= MAX_GENRE_LENGTH) len = MAX_GENRE_LENGTH - 1;
            memcpy(genres[i], g[i].c_str(), len);
            genreCount++;
        }
    }

    float getAvgRating() const {
        if (ratingCount == 0) return 0.0f;
        return (sumRating / 100.0f) / ratingCount;
    }

    void addRating(float rating) {
        sumRating += static_cast<uint32_t>(rating * 100);
        ratingCount++;
    }

    void updateRating(float oldRating, float newRating) {
        sumRating -= static_cast<uint32_t>(oldRating * 100);
        sumRating += static_cast<uint32_t>(newRating * 100);
    }

    void serialize(char* buffer) const {
        size_t offset = 0;
        memcpy(buffer + offset, &movieID, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, title, MAX_TITLE_LENGTH);
        offset += MAX_TITLE_LENGTH;
        memcpy(buffer + offset, genres, MAX_GENRES * MAX_GENRE_LENGTH);
        offset += MAX_GENRES * MAX_GENRE_LENGTH;
        memcpy(buffer + offset, &genreCount, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &ratingCount, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &sumRating, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &reserved, sizeof(uint64_t));
    }

    static Movie deserialize(const char* buffer) {
        Movie node;
        size_t offset = 0;
        memcpy(&node.movieID, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(node.title, buffer + offset, MAX_TITLE_LENGTH);
        offset += MAX_TITLE_LENGTH;
        memcpy(node.genres, buffer + offset, MAX_GENRES * MAX_GENRE_LENGTH);
        offset += MAX_GENRES * MAX_GENRE_LENGTH;
        memcpy(&node.genreCount, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(&node.ratingCount, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(&node.sumRating, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(&node.reserved, buffer + offset, sizeof(uint64_t));
        return node;
    }

    static constexpr size_t getSize() {
        return MOVIE_NODE_SIZE;
    }
};


struct RatingEdge {
    uint32_t movieID;
    uint32_t ratingValue;
    uint64_t timestamp;

    RatingEdge() : movieID(0), ratingValue(0), timestamp(0) {}

    RatingEdge(uint32_t mID, float rating)
        : movieID(mID), ratingValue(static_cast<uint32_t>(rating * 100)) {
        timestamp = static_cast<uint64_t>(time(nullptr));
    }

    float getRating() const {
        return ratingValue / 100.0f;
    }

    void setRating(float rating) {
        ratingValue = static_cast<uint32_t>(rating * 100);
        timestamp = static_cast<uint64_t>(time(nullptr));
    }

    void serialize(char* buffer) const {
        size_t offset = 0;
        memcpy(buffer + offset, &movieID, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &ratingValue, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, &timestamp, sizeof(uint64_t));
    }

    static RatingEdge deserialize(const char* buffer) {
        RatingEdge edge;
        size_t offset = 0;
        memcpy(&edge.movieID, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(&edge.ratingValue, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(&edge.timestamp, buffer + offset, sizeof(uint64_t));
        return edge;
    }

    static constexpr size_t getSize() {
        return sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t);
    }
};

#endif