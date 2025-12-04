#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <cstdint>
#include "edge.h"
using namespace std;

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

    Movie();
    Movie(uint32_t id, const string& t, const vector<string>& g);

    void addRating(uint32_t userID, float rating);
    void updateAvgRating();

    void serialize(char* buffer, size_t& size) const;
    static Movie deserialize(const char* buffer, size_t& bytesRead);
};

class User {
public:
    uint32_t userID;
    string username;

    vector<RatingEntry> ratings;

    vector<Edge> ratedMovies;
    uint64_t ratedMoviesOffset;

    User();
    User(uint32_t id, const string& name);
    void rateMovie(uint32_t movieID, float rating);
    bool hasRated(uint32_t movieID) const;
    float getRating(uint32_t movieID) const;
    vector<uint32_t> getRatedMovieIDs() const;
    void serialize(char* buffer, size_t& size) const;
    static User deserialize(const char* buffer, size_t& bytesRead);
};

#endif 