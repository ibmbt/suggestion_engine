#ifndef GRAPH_DATABASE_H
#define GRAPH_DATABASE_H

#include "../core/btree.h"
#include "../core/storage_manager.h"
#include "../core/hash_table.h"
#include "node.h"
#include <vector>
#include <algorithm>
#include <set>

const size_t MAX_MOVIES_PER_GENRE = 5000;

using namespace std;

class GraphDatabase {
private:
    BTree<uint32_t, uint64_t>* userIndex;
    BTree<uint32_t, uint64_t>* movieIndex;

    FixedStorage<User>* userStorage;
    FixedStorage<Movie>* movieStorage;

    HashTable<string, vector<uint32_t>*>* genreIndex;
    HashTable<string, uint32_t>* titleIndex;

    string normalizeTitle(const string& title) const {
        string normalized;
        for (size_t i = 0; i < title.length(); i++) {
            char c = title[i];
            if (c >= 0 && c <= 127 && isalnum(static_cast<unsigned char>(c))) {
                normalized += tolower(static_cast<unsigned char>(c));
            }
        }
        return normalized;
    }

    void indexMovieGenres(uint32_t movieID, const vector<string>& genres) {
        for (const string& genre : genres) {
            vector<uint32_t>* movieList;
            if (!genreIndex->find(genre, movieList)) {
                movieList = new vector<uint32_t>();
                movieList->reserve(100);
                genreIndex->insert(genre, movieList);
            }

            if (find(movieList->begin(), movieList->end(), movieID) == movieList->end()) {
                if (movieList->size() < MAX_MOVIES_PER_GENRE) {
                    movieList->push_back(movieID);
                }
            }
        }
    }

    void removeMovieFromGenreIndex(uint32_t movieID, const vector<string>& genres) {
        for (const string& genre : genres) {
            vector<uint32_t>* movieList;
            if (genreIndex->find(genre, movieList)) {
                movieList->erase(
                    remove(movieList->begin(), movieList->end(), movieID),
                    movieList->end()
                );
            }
        }
    }

public:
    GraphDatabase() {
        userIndex = new BTree<uint32_t, uint64_t>("user_index.dat");
        movieIndex = new BTree<uint32_t, uint64_t>("movie_index.dat");

        userStorage = new FixedStorage<User>(
            "users.dat",
            User::getSize(),
            USERS_PER_BLOCK
        );
        movieStorage = new FixedStorage<Movie>(
            "movies.dat",
            Movie::getSize(),
            MOVIES_PER_BLOCK
        );

        genreIndex = new HashTable<string, vector<uint32_t>*>(211);
        titleIndex = new HashTable<string, uint32_t>(10007);

        rebuildIndices();
    }

    ~GraphDatabase() {
        delete userIndex;
        delete movieIndex;
        delete userStorage;
        delete movieStorage;

        vector<string> genres = getAllGenresFromIndex();
        for (const string& genre : genres) {
            vector<uint32_t>* list;
            if (genreIndex->find(genre, list)) {
                delete list;
            }
        }
        delete genreIndex;
        delete titleIndex;
    }

    void rebuildIndices() {
        vector<pair<uint32_t, uint64_t>> allMovies = movieIndex->getAllPairs();

        for (const auto& pair : allMovies) {
            try {
                Movie movie = movieStorage->readNode(pair.first);

                vector<string> genres = movie.getGenres();
                indexMovieGenres(movie.movieID, genres);

                string normTitle = normalizeTitle(movie.getTitle());
                titleIndex->insert(normTitle, movie.movieID);
            }
            catch (...) {
                continue;
            }
        }
    }

    vector<uint32_t> getMoviesByGenre(const string& genre) {
        vector<uint32_t>* movieList;
        if (genreIndex->find(genre, movieList)) {
            return *movieList;
        }
        return vector<uint32_t>();
    }

    vector<uint32_t> searchMoviesByTitle(const string& query) {
        vector<uint32_t> results;
        string normQuery = normalizeTitle(query);

        if (normQuery.empty()) return results;

        vector<pair<string, uint32_t>> allTitles = titleIndex->getAllPairs();

        for (const auto& pair : allTitles) {
            if (pair.first.find(normQuery) != string::npos) {
                results.push_back(pair.second);
            }
        }

        return results;
    }

    vector<string> getAllGenresFromIndex() {
        return genreIndex->getAllKeys();
    }

    void addUser(uint32_t userID, const string& username) {
        User user(userID, username);
        userStorage->writeNode(userID, user);
        userIndex->insert(userID, userID);
    }

    User getUser(uint32_t userID) {
        uint64_t offset;
        if (!userIndex->search(userID, offset)) {
            throw runtime_error("User not found");
        }
        return userStorage->readNode(userID);
    }

    void updateUser(uint32_t userID, const User& user) {
        uint64_t offset;
        if (!userIndex->search(userID, offset)) {
            throw runtime_error("User not found");
        }
        userStorage->writeNode(userID, user);
    }

    bool userExists(uint32_t userID) {
        uint64_t offset;
        return userIndex->search(userID, offset);
    }

    void deleteUser(uint32_t userID) {
        userIndex->remove(userID);
    }

    vector<uint32_t> getAllUserIDs() {
        vector<uint32_t> ids;
        vector<pair<uint32_t, uint64_t>> pairs = userIndex->getAllPairs();
        for (const auto& p : pairs) {
            ids.push_back(p.first);
        }
        return ids;
    }

    size_t getUserCount() {
        return userIndex->size();
    }

    void addMovie(uint32_t movieID, const string& title, const vector<string>& genres) {
        Movie movie(movieID, title, genres);
        movieStorage->writeNode(movieID, movie);
        movieIndex->insert(movieID, movieID);

        indexMovieGenres(movieID, genres);
        string normTitle = normalizeTitle(title);
        titleIndex->insert(normTitle, movieID);
    }

    Movie getMovie(uint32_t movieID) {
        uint64_t offset;
        if (!movieIndex->search(movieID, offset)) {
            throw runtime_error("Movie not found");
        }
        return movieStorage->readNode(movieID);
    }

    void updateMovie(uint32_t movieID, const Movie& movie) {
        uint64_t offset;
        if (!movieIndex->search(movieID, offset)) {
            throw runtime_error("Movie not found");
        }

        try {
            Movie oldMovie = movieStorage->readNode(movieID);
            removeMovieFromGenreIndex(movieID, oldMovie.getGenres());
            string oldTitle = normalizeTitle(oldMovie.getTitle());
            titleIndex->remove(oldTitle);
        }
        catch (...) {}

        movieStorage->writeNode(movieID, movie);

        indexMovieGenres(movieID, movie.getGenres());
        string normTitle = normalizeTitle(movie.getTitle());
        titleIndex->insert(normTitle, movieID);
    }

    bool movieExists(uint32_t movieID) {
        uint64_t offset;
        return movieIndex->search(movieID, offset);
    }

    void deleteMovie(uint32_t movieID) {
        try {
            Movie movie = movieStorage->readNode(movieID);
            removeMovieFromGenreIndex(movieID, movie.getGenres());
            string normTitle = normalizeTitle(movie.getTitle());
            titleIndex->remove(normTitle);
        }
        catch (...) {}

        movieIndex->remove(movieID);
    }

    vector<uint32_t> getAllMovieIDs() {
        vector<uint32_t> ids;
        vector<pair<uint32_t, uint64_t>> pairs = movieIndex->getAllPairs();
        for (const auto& p : pairs) {
            ids.push_back(p.first);
        }
        return ids;
    }

    vector<pair<uint32_t, uint64_t>> getMovieIndex() {
        return movieIndex->getAllPairs();
    }

    size_t getMovieCount() {
        return movieIndex->size();
    }
};

#endif