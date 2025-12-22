#ifndef PARSER_H
#define PARSER_H

#include "recommendation_engine.h"
#include "../authentication/auth_manager.h"
#include "types.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

class MovieLensParser {
private:
    RecommendationEngine* engine;
    AuthManager* auth;
    string datasetPath;

    vector<string> split(const string& str, char delim) {
        vector<string> tokens;
        stringstream ss(str);
        string token;
        while (getline(ss, token, delim)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    string trim(const string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    vector<string> extractGenres(const vector<string>& parts) {
        vector<string> genreNames = {
            "unknown", "Action", "Adventure", "Animation", "Children's",
            "Comedy", "Crime", "Documentary", "Drama", "Fantasy",
            "Film-Noir", "Horror", "Musical", "Mystery", "Romance",
            "Sci-Fi", "Thriller", "War", "Western"
        };

        vector<string> movieGenres;
        for (size_t i = 0; i < 19 && (5 + i) < parts.size(); i++) {
            if (trim(parts[5 + i]) == "1") {
                if (genreNames[i] != "unknown") {
                    movieGenres.push_back(genreNames[i]);
                }
            }
        }

        if (movieGenres.empty()) {
            movieGenres.push_back("Unknown");
        }

        return movieGenres;
    }

    void flushUserBuffers(HashTable<uint32_t, vector<RatingEdge>*>& buffers) {
        vector<pair<uint32_t, vector<RatingEdge>*>> users = buffers.getAllPairs();

        for (const auto& p : users) {
            uint32_t uid = p.first;
            vector<RatingEdge>* newRatings = p.second;

            vector<RatingEdge> existingRatings = engine->edgeManager->readRatings(uid);

            existingRatings.insert(existingRatings.end(), newRatings->begin(), newRatings->end());

            engine->edgeManager->writeRatings(uid, existingRatings);

            try {
                User u = engine->graphDB->getUser(uid);
                u.totalRatings = existingRatings.size();
                engine->graphDB->updateUser(uid, u);
            }
            catch (...) {}

            delete newRatings;
        }

        buffers.clear();
    }

    void flushMovieStats(HashTable<uint32_t, pair<uint32_t, uint32_t>*>& stats) {
        vector<pair<uint32_t, pair<uint32_t, uint32_t>*>> movies = stats.getAllPairs();

        for (const auto& p : movies) {
            uint32_t mid = p.first;
            pair<uint32_t, uint32_t>* stat = p.second;

            try {
                Movie m = engine->graphDB->getMovie(mid);
                m.ratingCount += stat->first;
                m.sumRating += stat->second;
                engine->graphDB->updateMovie(mid, m);
            }
            catch (...) {}

            delete stat;
        }
        stats.clear();
    }

public:
    MovieLensParser(RecommendationEngine* eng, AuthManager* authMgr, const string& path = "../ml-100k")
        : engine(eng), auth(authMgr), datasetPath(path) {
    }

    bool parseMovies() {
        cout << "\n[Parser] Loading movies from u.item..." << endl;

        string filename = datasetPath + "/u.item";
        ifstream file(filename);

        if (!file.is_open()) {
            cerr << "[ERROR] Could not open " << filename << endl;
            return false;
        }

        int count = 0;
        int errors = 0;
        string line;

        while (getline(file, line)) {
            try {
                vector<string> parts = split(line, '|');

                if (parts.size() < 24) {
                    errors++;
                    continue;
                }

                uint32_t movieID = stoi(trim(parts[0]));
                string title = trim(parts[1]);
                vector<string> genres = extractGenres(parts);

                engine->addMovie(movieID, title, genres);
                count++;

                if (count % 500 == 0) {
                    cout << "  Loaded " << count << " movies..." << endl;
                }
            }
            catch (const exception& e) {
                errors++;
            }
        }

        file.close();
        cout << "[OK] Loaded " << count << " movies (" << errors << " errors)" << endl;
        return true;
    }

    bool parseUsers() {
        cout << "\n[Parser] Loading users from u.user..." << endl;

        string filename = datasetPath + "/u.user";
        ifstream file(filename);

        if (!file.is_open()) {
            cerr << "[ERROR] Could not open " << filename << endl;
            return false;
        }

        int count = 0;
        int errors = 0;
        string line;

        while (getline(file, line)) {
            try {
                vector<string> parts = split(line, '|');

                if (parts.size() < 5) {
                    errors++;
                    continue;
                }

                uint32_t userID = stoi(trim(parts[0]));
                string username = "user" + to_string(userID);
                string defaultPassword = "movielens123";

                try {
                    auth->registerUser(username, defaultPassword);
                }
                catch (const exception& e) {
                }

                engine->createUser(userID, username);
                count++;

                if (count % 200 == 0) {
                    cout << "  Loaded " << count << " users..." << endl;
                }
            }
            catch (const exception& e) {
                errors++;
            }
        }

        file.close();
        cout << "[OK] Loaded " << count << " users (" << errors << " errors)" << endl;
        cout << "[INFO] Default password for all users: movielens123" << endl;
        return true;
    }



    bool parseRatings() {
        cout << "\n[Parser] Loading ratings from u.data..." << endl;
        cout << "[INFO] Using Phased Batch Loading (Safe for Low RAM)..." << endl;

        string filename = datasetPath + "/u.data";
        ifstream file(filename);

        if (!file.is_open()) {
            cerr << "[ERROR] Could not open " << filename << endl;
            return false;
        }


        HashTable<uint32_t, vector<RatingEdge>*> userBuffers(1009);

        HashTable<uint32_t, pair<uint32_t, uint32_t>*> movieStats(2000);

        int count = 0;
        string line;

        while (getline(file, line)) {
            vector<string> parts = split(line, '\t');
            if (parts.size() < 3) continue;

            try {
                uint32_t userID = stoi(trim(parts[0]));
                uint32_t movieID = stoi(trim(parts[1]));
                float rating = stof(trim(parts[2]));

                vector<RatingEdge>* userRatings;
                if (!userBuffers.find(userID, userRatings)) {
                    userRatings = new vector<RatingEdge>();
                    userBuffers.insert(userID, userRatings);
                }
                userRatings->push_back(RatingEdge(movieID, rating));

                pair<uint32_t, uint32_t>* stats;
                if (!movieStats.find(movieID, stats)) {
                    stats = new pair<uint32_t, uint32_t>(0, 0);
                    movieStats.insert(movieID, stats);
                }
                stats->first++;
                stats->second += static_cast<uint32_t>(rating * 100);

                count++;

                if (count % BATCH_SIZE == 0) {
                    cout << "  [Batch] Processed " << count << " lines. Flushing to disk..." << endl;
                    flushUserBuffers(userBuffers);
                }
            }
            catch (...) {
                continue;
            }
        }

        if (userBuffers.getSize() > 0) {
            cout << "  [Batch] Flushing final user data..." << endl;
            flushUserBuffers(userBuffers);
        }

        cout << "  [Final] Updating movie statistics..." << endl;
        flushMovieStats(movieStats);

        file.close();
        cout << "[OK] Successfully loaded " << count << " ratings." << endl;
        return true;
    }

    bool parseAll() {
        cout << "\n========================================" << endl;
        cout << " MovieLens 100k Dataset Parser" << endl;
        cout << "========================================" << endl;

        if (!parseMovies()) return false;
        if (!parseUsers()) return false;
        if (!parseRatings()) return false;

        cout << "\n========================================" << endl;
        cout << " Parsing Complete!" << endl;
        cout << "========================================" << endl;

        engine->printStats();
        return true;
    }
};


#endif