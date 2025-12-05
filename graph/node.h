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


    void serialize(char* buffer, size_t& size) const;
    static User deserialize(const char* buffer, size_t& bytesRead);
};

#endif