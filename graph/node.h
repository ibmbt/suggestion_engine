#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include "edge.h"
using namespace std;

inline void writeString(char*& buffer, const string& s) {
    uint32_t len = static_cast<uint32_t>(s.length());
    memcpy(buffer, &len, sizeof(uint32_t));
    buffer += sizeof(uint32_t);

    if (len > 0) {
        memcpy(buffer, s.c_str(), len);
        buffer += len;
    }
}

inline string readString(const char*& buffer) {
    uint32_t len;
    memcpy(&len, buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);

    string s = "";
    if (len > 0) {
        s.resize(len);
        memcpy(&s[0], buffer, len);
        buffer += len;
    }
    return s;
}
class Movie {
public:
    uint32_t movieID;
    string title;
    vector<string> genres;

    float avgRating;
    uint32_t ratingCount;

    vector<Edge> ratedByUsers;
    vector<Edge> similarMovies;

    uint64_t ratedByUsersOffset;
    uint64_t similarMoviesOffset;

    Movie()
        : movieID(0), avgRating(0.0f), ratingCount(0),
        ratedByUsersOffset(0), similarMoviesOffset(0) {
    }

    Movie(uint32_t id, const string& t, const vector<string>& g)
        : movieID(id), title(t), genres(g), avgRating(0.0f),
        ratingCount(0), ratedByUsersOffset(0), similarMoviesOffset(0) {
    }

    void addRating(uint32_t userID, float rating) {
        float total = avgRating * ratingCount;
        ratingCount++;
        avgRating = (total + rating) / ratingCount;

        Edge edge(userID, movieID, EdgeType::RATED, rating);
        ratedByUsers.push_back(edge);
    }

    void updateAvgRating() {
        if (ratingCount == 0) {
            avgRating = 0.0f;
            return;
        }

        float total = 0.0f;

        for (size_t i = 0; i < ratedByUsers.size(); ++i) {
            Edge& edge = ratedByUsers[i];
            total += edge.weight;
        }

        avgRating = total / ratingCount;
    }


    void serialize(char* buffer, size_t& size) const {
        char* originalStart = buffer;

        memcpy(buffer, &movieID, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        writeString(buffer, title);

        uint32_t genreCount = static_cast<uint32_t>(genres.size());
        memcpy(buffer, &genreCount, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        for (size_t i = 0; i < genres.size(); i++) {
            writeString(buffer, genres[i]);
        }

        memcpy(buffer, &avgRating, sizeof(float));
        buffer = buffer + sizeof(float);

        memcpy(buffer, &ratingCount, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        memcpy(buffer, &ratedByUsersOffset, sizeof(uint64_t));
        buffer = buffer + sizeof(uint64_t);

        memcpy(buffer, &similarMoviesOffset, sizeof(uint64_t));
        buffer = buffer + sizeof(uint64_t);

        size = static_cast<size_t>(buffer - originalStart);
    }

    static Movie deserialize(const char* buffer, size_t& bytesRead) {
        const char* originalStart = buffer;
        Movie m;

        memcpy(&m.movieID, buffer, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        m.title = readString(buffer);

        uint32_t genreCount;
        memcpy(&genreCount, buffer, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        for (size_t i = 0; i < genreCount; i++) {
            string g = readString(buffer);
            m.genres.push_back(g);
        }

        memcpy(&m.avgRating, buffer, sizeof(float));
        buffer = buffer + sizeof(float);

        memcpy(&m.ratingCount, buffer, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        memcpy(&m.ratedByUsersOffset, buffer, sizeof(uint64_t));
        buffer = buffer + sizeof(uint64_t);

        memcpy(&m.similarMoviesOffset, buffer, sizeof(uint64_t));
        buffer = buffer + sizeof(uint64_t);

        bytesRead = static_cast<size_t>(buffer - originalStart);
        return m;
    }
};

class User {
public:
    uint32_t userID;
    string username;

    vector<RatingEntry> ratings;

    vector<Edge> ratedMovies;
    uint64_t ratedMoviesOffset;

    User() : userID(0), ratedMoviesOffset(0) {}

    User(uint32_t id, const std::string& name)
        : userID(id), username(name), ratedMoviesOffset(0) {
    }

    void rateMovie(uint32_t movieID, float rating) {
        for (int i = 0; i < ratings.size(); ++i) {
            if (ratings[i].movieID == movieID) {
                ratings[i].rating = rating;
                ratings[i].timestamp = static_cast<uint64_t>(time(nullptr));
                return;
            }
        }

        ratings.push_back(RatingEntry(movieID, rating));

        Edge edge(userID, movieID, EdgeType::RATED, rating);
        ratedMovies.push_back(edge);
    }

    bool hasRated(uint32_t movieID) const {
        for (int i = 0; i < ratings.size(); ++i) {
            if (ratings[i].movieID == movieID) {
                return true;
            }
        }
        return false;
    }


    float getRating(uint32_t movieID) const {
        for (int i = 0; i < ratings.size(); ++i) {
            if (ratings[i].movieID == movieID) {
                return ratings[i].rating;
            }
        }
        return 0.0f;
    }


    vector<uint32_t> getRatedMovieIDs() const {
        vector<uint32_t> ids;

        for (int i = 0; i < ratings.size(); ++i) {
            ids.push_back(ratings[i].movieID);
        }
        return ids;
    }


    void serialize(char* buffer, size_t& size) const {
        char* originalStart = buffer;

        memcpy(buffer, &userID, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        writeString(buffer, username);

        uint32_t ratingsCount = static_cast<uint32_t>(ratings.size());
        memcpy(buffer, &ratingsCount, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        for (size_t i = 0; i < ratings.size(); i++) {
            memcpy(buffer, &ratings[i], sizeof(RatingEntry));
            buffer = buffer + sizeof(RatingEntry);
        }

        memcpy(buffer, &ratedMoviesOffset, sizeof(uint64_t));
        buffer = buffer + sizeof(uint64_t);

        size = static_cast<size_t>(buffer - originalStart);
    }

    static User deserialize(const char* buffer, size_t& bytesRead) {
        const char* originalStart = buffer;
        User u;

        memcpy(&u.userID, buffer, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        u.username = readString(buffer);

        uint32_t ratingsCount;
        memcpy(&ratingsCount, buffer, sizeof(uint32_t));
        buffer = buffer + sizeof(uint32_t);

        u.ratings.resize(ratingsCount);

        for (size_t i = 0; i < ratingsCount; i++) {
            memcpy(&u.ratings[i], buffer, sizeof(RatingEntry));
            buffer = buffer + sizeof(RatingEntry);
        }

        memcpy(&u.ratedMoviesOffset, buffer, sizeof(uint64_t));
        buffer = buffer + sizeof(uint64_t);

        bytesRead = static_cast<size_t>(buffer - originalStart);
        return u;
    }
};

#endif