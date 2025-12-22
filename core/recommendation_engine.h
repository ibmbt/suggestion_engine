#ifndef RECOMMENDATION_ENGINE_H
#define RECOMMENDATION_ENGINE_H

#include "../graph/graph_database.h"
#include "storage_manager.h"
#include "hash_table.h"
#include <queue> 
#include <cmath>
#include <algorithm>
#include <set>

using namespace std;

class MovieLockManager {
private:
    HashTable<uint32_t, mutex*>* locks;
    mutex managerMutex;

public:
    MovieLockManager() {
        locks = new HashTable<uint32_t, mutex*>(2003);
    }

    ~MovieLockManager() {
        vector<pair<uint32_t, mutex*>> allLocks = locks->getAllPairs();
        for (const auto& p : allLocks) {
            delete p.second;
        }
        delete locks;
    }

    mutex& getLock(uint32_t movieID) {
        lock_guard<mutex> guard(managerMutex);

        mutex* m;
        if (!locks->find(movieID, m)) {
            m = new mutex();
            locks->insert(movieID, m);
        }
        return *m;
    }
};

struct MovieScore {
    uint32_t movieID;
    float score;

    bool operator<(const MovieScore& other) const {
        return score < other.score;
    }

    bool operator>(const MovieScore& other) const {
        return score > other.score;
    }
};

struct RecommendationResult {
    uint32_t movieID;
    string title;
    vector<string> genres;
    float score;
    float avgRating;
    uint32_t ratingCount;
};

struct UserProfile {
    HashTable<string, float>* genreScores;
    HashTable<uint32_t, bool>* ratedMovies;
    float avgUserRating;
    uint32_t totalRatings;

    UserProfile() {
        genreScores = new HashTable<string, float>(211);
        ratedMovies = new HashTable<uint32_t, bool>(1009);
        avgUserRating = 0.0f;
        totalRatings = 0;
    }

    ~UserProfile() {
        delete genreScores;
        delete ratedMovies;
    }
};

class RecommendationEngine {
private:
    GraphDatabase* graphDB;
    EdgeFileManager* edgeManager;
    MovieLockManager* movieLocks;

    friend class MovieLensParser;


    float calculateMovieScore(const Movie& movie, const UserProfile& profile) {
        bool alreadyRated = false;
        if (profile.ratedMovies->find(movie.movieID, alreadyRated)) {
            return -1.0f;
        }

        if (movie.ratingCount < 2) {
            return -1.0f;
        }

        vector<string> movieGenres = movie.getGenres();
        if (movieGenres.empty()) {
            return -1.0f;
        }

        float genreMatchScore = 0.0f;
        for (size_t i = 0; i < movieGenres.size(); i++) {
            float userGenreScore = 0.0f;
            profile.genreScores->find(movieGenres[i], userGenreScore);

            float genreWeight = 1.0f;
            if (i == 0) {
                genreWeight = 3.0f;
            }

            genreMatchScore = genreMatchScore + (userGenreScore * genreWeight);
        }

        if (genreMatchScore <= 0.0f) {
            return 0.1f;
        }

        float movieAvgRating = movie.getAvgRating();
        float popularityBoost = log(1.0f + movie.ratingCount) / 10.0f;
        float finalScore = genreMatchScore * movieAvgRating * (1.0f + popularityBoost);

        return finalScore;
    }

    UserProfile* buildUserProfile(uint32_t userID) {
        UserProfile* profile = new UserProfile();

        vector<RatingEdge> userRatings = edgeManager->readRatings(userID);

        if (userRatings.empty()) {
            return profile;
        }

        profile->totalRatings = userRatings.size();

        float ratingSum = 0.0f;
        for (size_t i = 0; i < userRatings.size(); i++) {
            ratingSum = ratingSum + userRatings[i].getRating();
            profile->ratedMovies->insert(userRatings[i].movieID, true);
        }
        profile->avgUserRating = ratingSum / profile->totalRatings;

        for (size_t i = 0; i < userRatings.size(); i++) {
            RatingEdge rating = userRatings[i];

            try {
                Movie movie = graphDB->getMovie(rating.movieID);
                float userRating = rating.getRating();
                vector<string> genres = movie.getGenres();

                float ratingWeight = userRating - profile->avgUserRating;

                for (size_t j = 0; j < genres.size(); j++) {
                    string genre = genres[j];
                    float existingScore = 0.0f;
                    profile->genreScores->find(genre, existingScore);

                    float genreWeight = 1.0f;
                    if (j == 0) {
                        genreWeight = 2.0f;
                    }

                    float newScore = existingScore + (userRating * ratingWeight * genreWeight);
                    profile->genreScores->insert(genre, newScore);
                }
            }
            catch (...) {
                continue;
            }
        }

        return profile;
    }

    vector<RecommendationResult> extractResults(priority_queue<MovieScore, vector<MovieScore>, greater<MovieScore>>& minHeap) {
        vector<MovieScore> topMovies;
        while (!minHeap.empty()) {
            topMovies.push_back(minHeap.top());
            minHeap.pop();
        }
        reverse(topMovies.begin(), topMovies.end());

        vector<RecommendationResult> results;
        for (size_t i = 0; i < topMovies.size(); i++) {
            try {
                Movie movie = graphDB->getMovie(topMovies[i].movieID);

                RecommendationResult result;
                result.movieID = topMovies[i].movieID;
                result.title = movie.getTitle();
                result.genres = movie.getGenres();
                result.score = topMovies[i].score;
                result.avgRating = movie.getAvgRating();
                result.ratingCount = movie.ratingCount;

                results.push_back(result);
            }
            catch (...) {
                continue;
            }
        }

        return results;
    }

    vector<RecommendationResult> recommendFromDisk(uint32_t userID, int topN) {
        UserProfile* profile = buildUserProfile(userID);

        if (profile->totalRatings == 0) {
            delete profile;
            return vector<RecommendationResult>();
        }

        vector<pair<string, float>> topGenres;
        vector<pair<string, float>> allGenreScores = profile->genreScores->getAllPairs();

        sort(allGenreScores.begin(), allGenreScores.end(),
            [](const pair<string, float>& a, const pair<string, float>& b) {
                return a.second > b.second;
            });

        for (size_t i = 0; i < min((size_t)5, allGenreScores.size()); i++) {
            if (allGenreScores[i].second > 0) {
                topGenres.push_back(allGenreScores[i]);
            }
        }

        set<uint32_t> candidates;
        for (const auto& genrePair : topGenres) {
            vector<uint32_t> genreMovies = graphDB->getMoviesByGenre(genrePair.first);

            for (size_t i = 0; i < min((size_t)100, genreMovies.size()); i++) {
                candidates.insert(genreMovies[i]);
            }
        }

        if (candidates.empty()) {
            vector<uint32_t> allMovies = graphDB->getAllMovieIDs();
            for (size_t i = 0; i < min((size_t)200, allMovies.size()); i++) {
                candidates.insert(allMovies[i]);
            }
        }

        priority_queue<MovieScore, vector<MovieScore>, greater<MovieScore>> minHeap;

        for (uint32_t movieID : candidates) {
            try {
                Movie movie = graphDB->getMovie(movieID);
                float score = calculateMovieScore(movie, *profile);

                if (score < 0) continue;

                if (minHeap.size() < (size_t)topN) {
                    MovieScore ms;
                    ms.movieID = movieID;
                    ms.score = score;
                    minHeap.push(ms);
                }
                else if (score > minHeap.top().score) {
                    minHeap.pop();
                    MovieScore ms;
                    ms.movieID = movieID;
                    ms.score = score;
                    minHeap.push(ms);
                }
            }
            catch (...) {
                continue;
            }
        }

        delete profile;
        return extractResults(minHeap);
    }

public:
    RecommendationEngine() {
        graphDB = new GraphDatabase();
        edgeManager = new EdgeFileManager("ratings");
        movieLocks = new MovieLockManager();
    }

    ~RecommendationEngine() {
        delete graphDB;
        delete edgeManager;
        delete movieLocks;
    }

    vector<RecommendationResult> getColdStartRecommendations(int limitPerGenre = 1) {
        vector<RecommendationResult> results;
        vector<string> majorGenres;
        majorGenres.push_back("Action");
        majorGenres.push_back("Comedy");
        majorGenres.push_back("Drama");
        majorGenres.push_back("Romance");
        majorGenres.push_back("Sci-Fi");
        majorGenres.push_back("Horror");
        majorGenres.push_back("Thriller");

        set<uint32_t> addedMovies;

        for (const string& genre : majorGenres) {
            vector<uint32_t> ids = graphDB->getMoviesByGenre(genre);

            priority_queue<MovieScore, vector<MovieScore>, greater<MovieScore>> topMovies;

            size_t examineCount = min(ids.size(), (size_t)50);

            for (size_t i = 0; i < examineCount; i++) {
                try {
                    Movie m = getMovie(ids[i]);

                    if (m.ratingCount < 10) continue;

                    if (addedMovies.find(ids[i]) != addedMovies.end()) continue;

                    float score = m.getAvgRating();

                    if (topMovies.size() < (size_t)limitPerGenre) {
                        MovieScore ms;
                        ms.movieID = ids[i];
                        ms.score = score;
                        topMovies.push(ms);
                    }
                    else if (score > topMovies.top().score) {
                        topMovies.pop();
                        MovieScore ms;
                        ms.movieID = ids[i];
                        ms.score = score;
                        topMovies.push(ms);
                    }
                }
                catch (...) {}
            }

            if (!topMovies.empty()) {
                MovieScore best = topMovies.top();
                try {
                    Movie m = getMovie(best.movieID);
                    RecommendationResult res;
                    res.movieID = best.movieID;
                    res.title = m.getTitle();
                    res.genres = m.getGenres();
                    res.avgRating = m.getAvgRating();
                    res.ratingCount = m.ratingCount;
                    res.score = best.score;
                    results.push_back(res);
                    addedMovies.insert(best.movieID);
                }
                catch (...) {}
            }
        }

        return results;
    }

    bool containsIgnoreCase(const string& text, const string& query) {
        string textClean, queryClean;

        for (unsigned char c : text) {
            if (isalpha(c)) {
                textClean += tolower(c);
            }
        }

        for (unsigned char c : query) {
            if (isalpha(c)) {
                queryClean += tolower(c);
            }
        }

        return textClean.find(queryClean) != string::npos;
    }


    vector<string> getAllGenres() {
        return graphDB->getAllGenresFromIndex();
    }

    vector<Movie> searchMoviesByTitle(const string& query) {
        vector<Movie> matches;
        vector<uint32_t> movieIDs = graphDB->searchMoviesByTitle(query);

        for (uint32_t id : movieIDs) {
            try {
                matches.push_back(graphDB->getMovie(id));
            }
            catch (...) {}
        }

        return matches;
    }

    void createUser(uint32_t userID, const string& username) {
        if (username.empty() || username.length() >= MAX_USERNAME_LENGTH) {
            throw runtime_error("Invalid username");
        }

        edgeManager->deleteUserEdges(userID);
        graphDB->addUser(userID, username);
    }

    User getUserProfile(uint32_t userID) {
        return graphDB->getUser(userID);
    }

    bool userExists(uint32_t userID) {
        return graphDB->userExists(userID);
    }

    void deleteUser(uint32_t userID) {
        graphDB->deleteUser(userID);
        edgeManager->deleteUserEdges(userID);
    }

    vector<uint32_t> getAllUserIDs() {
        return graphDB->getAllUserIDs();
    }

    void addMovie(uint32_t movieID, const string& title, const vector<string>& genres) {
        if (title.empty() || title.length() >= MAX_TITLE_LENGTH) {
            throw runtime_error("Invalid movie title");
        }
        if (genres.empty() || genres.size() > MAX_GENRES) {
            throw runtime_error("Invalid genres");
        }

        graphDB->addMovie(movieID, title, genres);
    }

    Movie getMovie(uint32_t movieID) {
        return graphDB->getMovie(movieID);
    }

    bool movieExists(uint32_t movieID) {
        return graphDB->movieExists(movieID);
    }

    void deleteMovie(uint32_t movieID) {
        graphDB->deleteMovie(movieID);
    }

    vector<uint32_t> getAllMovieIDs() {
        return graphDB->getAllMovieIDs();
    }

    vector<uint32_t> getMoviesByGenre(const string& genre) {
        return graphDB->getMoviesByGenre(genre);
    }

    void addRating(uint32_t userID, uint32_t movieID, float rating) {
        if (rating < MIN_RATING || rating > MAX_RATING) {
            throw runtime_error("Rating must be between 1.0 and 5.0");
        }

        if (!userExists(userID)) {
            throw runtime_error("User does not exist");
        }

        if (!movieExists(movieID)) {
            throw runtime_error("Movie does not exist");
        }

        float oldRating;
        bool hadRating = edgeManager->getRating(userID, movieID, oldRating);

        edgeManager->addOrUpdateRating(userID, movieID, rating);
        {
            lock_guard<mutex> movieLock(movieLocks->getLock(movieID));

            Movie movie = getMovie(movieID);

            if (hadRating) {
                movie.updateRating(oldRating, rating);
            }
            else {
                movie.addRating(rating);
            }

            graphDB->updateMovie(movieID, movie);
        }

        if (!hadRating) {
            User user = getUserProfile(userID);
            user.totalRatings = user.totalRatings + 1;
            graphDB->updateUser(userID, user);
        }
    }

    bool getRating(uint32_t userID, uint32_t movieID, float& rating) {
        return edgeManager->getRating(userID, movieID, rating);
    }

    vector<RatingEdge> getUserRatings(uint32_t userID) {
        return edgeManager->readRatings(userID);
    }

    bool hasRated(uint32_t userID, uint32_t movieID) {
        return edgeManager->hasRating(userID, movieID);
    }

    vector<RecommendationResult> getRecommendations(uint32_t userID, int topN = 10) {
        if (!userExists(userID)) {
            throw runtime_error("User does not exist");
        }

        return recommendFromDisk(userID, topN);
    }

    vector<RecommendationResult> recommendPopular(int topN = 10) {
        priority_queue<MovieScore, vector<MovieScore>, greater<MovieScore>> minHeap;

        vector<uint32_t> allMovies = graphDB->getAllMovieIDs();

        for (uint32_t movieID : allMovies) {
            try {
                Movie movie = graphDB->getMovie(movieID);

                if (movie.ratingCount < 5) {
                    continue;
                }

                float score = movie.getAvgRating() * log(1.0f + movie.ratingCount);

                if (minHeap.size() < (size_t)topN) {
                    MovieScore ms;
                    ms.movieID = movieID;
                    ms.score = score;
                    minHeap.push(ms);
                }
                else if (score > minHeap.top().score) {
                    minHeap.pop();
                    MovieScore ms;
                    ms.movieID = movieID;
                    ms.score = score;
                    minHeap.push(ms);
                }
            }
            catch (...) {
                continue;
            }
        }

        return extractResults(minHeap);
    }

    void printStats() {
        cout << "\n\nDatabase:" << endl;
        cout << "  Total Users:  " << graphDB->getUserCount() << endl;
        cout << "  Total Movies: " << graphDB->getMovieCount() << endl;
        cout << "\n----------------------------------------\n" << endl;
    }
};

#endif