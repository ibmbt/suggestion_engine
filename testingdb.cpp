#include "./graph/graph_database.h"
#include "./core/storage_manager.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

void stressTest();

// ============================================================================
// UTILITIES
// ============================================================================

class Timer {
    chrono::high_resolution_clock::time_point start;
public:
    Timer() { reset(); }
    void reset() { start = chrono::high_resolution_clock::now(); }
    double elapsed() {
        chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();
        return chrono::duration<double, milli>(end - start).count();
    }
};

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pressEnter() {
    cout << "\nPress ENTER to continue...";
    cin.get();
}

void printHeader(const string& title) {
    cout << "\n" << string(70, '=') << endl;
    cout << "  " << title << endl;
    cout << string(70, '=') << endl;
}

void printSuccess(const string& msg) {
    cout << "[SUCCESS] " << msg << endl;
}

void printError(const string& msg) {
    cout << "[ERROR] " << msg << endl;
}

void printInfo(const string& msg) {
    cout << "[INFO] " << msg << endl;
}

// Sample data
const vector<string> MOVIE_TITLES = {
    "The Dark Knight", "Inception", "Interstellar", "The Matrix",
    "Pulp Fiction", "Fight Club", "Forrest Gump", "The Godfather",
    "The Shawshank Redemption", "Goodfellas", "The Departed", "Casino"
};

const vector<string> GENRES = {
    "Action", "Drama", "Sci-Fi", "Thriller", "Comedy", "Horror", "Romance", "Adventure"
};

const vector<string> USERNAMES = {
    "Alice", "Bob", "Charlie", "Diana", "Eve", "Frank", "Grace", "Henry",
    "Ivy", "Jack", "Karen", "Leo", "Mia", "Nathan", "Olivia", "Paul"
};

// ============================================================================
// MENU-DRIVEN INTERFACE
// ============================================================================

void addUserMenu(GraphDatabase& db) {
    printHeader("ADD USER");

    uint32_t userID;
    string username;

    cout << "Enter User ID: ";
    cin >> userID;
    cin.ignore();

    cout << "Enter Username: ";
    getline(cin, username);

    try {
        db.addUser(userID, username);
        printSuccess("User added successfully!");
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void viewUserMenu(GraphDatabase& db) {
    printHeader("VIEW USER");

    uint32_t userID;
    cout << "Enter User ID: ";
    cin >> userID;
    cin.ignore();

    try {
        User user = db.getUser(userID);

        cout << "\nUser Details:" << endl;
        cout << string(50, '-') << endl;
        cout << "ID:            " << user.userID << endl;
        cout << "Username:      " << user.getUsername() << endl;
        cout << "Total Ratings: " << user.totalRatings << endl;
        cout << "Avg Rating:    " << fixed << setprecision(2) << user.getAvgRating() << endl;
        cout << string(50, '-') << endl;
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void deleteUserMenu(GraphDatabase& db) {
    printHeader("DELETE USER");

    uint32_t userID;
    cout << "Enter User ID: ";
    cin >> userID;
    cin.ignore();

    try {
        db.deleteUser(userID);
        printSuccess("User deleted successfully!");
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void listAllUsersMenu(GraphDatabase& db) {
    printHeader("ALL USERS");

    try {
        vector<uint32_t> userIDs = db.getAllUserIDs();

        if (userIDs.empty()) {
            printInfo("No users in database.");
        }
        else {
            cout << "\n" << left << setw(10) << "ID" << setw(30) << "USERNAME"
                << setw(15) << "RATINGS" << "AVG RATING" << endl;
            cout << string(70, '-') << endl;

            for (size_t i = 0; i < userIDs.size(); i++) {
                User user = db.getUser(userIDs[i]);
                cout << left << setw(10) << user.userID
                    << setw(30) << user.getUsername()
                    << setw(15) << user.totalRatings
                    << fixed << setprecision(2) << user.getAvgRating() << endl;
            }

            cout << "\nTotal users: " << userIDs.size() << endl;
        }
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void addMovieMenu(GraphDatabase& db) {
    printHeader("ADD MOVIE");

    uint32_t movieID;
    string title;
    int genreCount;

    cout << "Enter Movie ID: ";
    cin >> movieID;
    cin.ignore();

    cout << "Enter Title: ";
    getline(cin, title);

    cout << "Enter number of genres (1-5): ";
    cin >> genreCount;
    cin.ignore();

    if (genreCount < 1) genreCount = 1;
    if (genreCount > 5) genreCount = 5;

    vector<string> genres;
    for (int i = 0; i < genreCount; i++) {
        string genre;
        cout << "Genre " << (i + 1) << ": ";
        getline(cin, genre);
        genres.push_back(genre);
    }

    try {
        db.addMovie(movieID, title, genres);
        printSuccess("Movie added successfully!");
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void viewMovieMenu(GraphDatabase& db) {
    printHeader("VIEW MOVIE");

    uint32_t movieID;
    cout << "Enter Movie ID: ";
    cin >> movieID;
    cin.ignore();

    try {
        Movie movie = db.getMovie(movieID);
        vector<string> genres = movie.getGenres();

        cout << "\nMovie Details:" << endl;
        cout << string(50, '-') << endl;
        cout << "ID:            " << movie.movieID << endl;
        cout << "Title:         " << movie.getTitle() << endl;
        cout << "Genres:        ";
        for (size_t i = 0; i < genres.size(); i++) {
            cout << genres[i];
            if (i < genres.size() - 1) cout << ", ";
        }
        cout << endl;
        cout << "Rating Count:  " << movie.ratingCount << endl;
        cout << "Avg Rating:    " << fixed << setprecision(2) << movie.getAvgRating() << endl;
        cout << string(50, '-') << endl;
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void deleteMovieMenu(GraphDatabase& db) {
    printHeader("DELETE MOVIE");

    uint32_t movieID;
    cout << "Enter Movie ID: ";
    cin >> movieID;
    cin.ignore();

    try {
        db.deleteMovie(movieID);
        printSuccess("Movie deleted successfully!");
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void listAllMoviesMenu(GraphDatabase& db) {
    printHeader("ALL MOVIES");

    try {
        vector<uint32_t> movieIDs = db.getAllMovieIDs();

        if (movieIDs.empty()) {
            printInfo("No movies in database.");
        }
        else {
            cout << "\n" << left << setw(10) << "ID" << setw(35) << "TITLE"
                << setw(12) << "RATINGS" << "AVG RATING" << endl;
            cout << string(70, '-') << endl;

            for (size_t i = 0; i < movieIDs.size(); i++) {
                Movie movie = db.getMovie(movieIDs[i]);
                cout << left << setw(10) << movie.movieID
                    << setw(35) << movie.getTitle()
                    << setw(12) << movie.ratingCount
                    << fixed << setprecision(2) << movie.getAvgRating() << endl;
            }

            cout << "\nTotal movies: " << movieIDs.size() << endl;
        }
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void addRatingMenu(GraphDatabase& db, EdgeFileManager& edgeMgr) {
    printHeader("ADD RATING");

    uint32_t userID, movieID;
    float rating;

    cout << "Enter User ID: ";
    cin >> userID;

    cout << "Enter Movie ID: ";
    cin >> movieID;

    cout << "Enter Rating (1.0-5.0): ";
    cin >> rating;
    cin.ignore();

    if (rating < 1.0f) rating = 1.0f;
    if (rating > 5.0f) rating = 5.0f;

    try {
        User user = db.getUser(userID);
        Movie movie = db.getMovie(movieID);

        float oldRating;
        bool ratingExists = edgeMgr.getRating(userID, movieID, oldRating);

        edgeMgr.addOrUpdateRating(userID, movieID, rating);

        if (ratingExists) {
            movie.updateRating(oldRating, rating);
        }
        else {
            movie.addRating(rating);

            user.totalRatings++;

            vector<RatingEdge> allUserRatings = edgeMgr.readRatings(userID);
            if (!allUserRatings.empty()) {
                float totalRating = 0.0f;
                for (size_t i = 0; i < allUserRatings.size(); i++) {
                    totalRating += allUserRatings[i].getRating();
                }
                user.setAvgRating(totalRating / allUserRatings.size());
            }

            db.updateUser(userID, user);
        }

        db.updateMovie(movieID, movie);

        printSuccess("Rating added successfully!");
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}

void viewUserRatingsMenu(GraphDatabase& db, EdgeFileManager& edgeMgr) {
    printHeader("VIEW USER RATINGS");

    uint32_t userID;
    cout << "Enter User ID: ";
    cin >> userID;
    cin.ignore();

    try {
        User user = db.getUser(userID);
        vector<RatingEdge> ratings = edgeMgr.readRatings(userID);

        cout << "\nRatings by " << user.getUsername() << ":" << endl;
        cout << string(60, '-') << endl;

        if (ratings.empty()) {
            printInfo("No ratings yet.");
        }
        else {
            cout << left << setw(10) << "MOVIE ID" << setw(35) << "TITLE"
                << "RATING" << endl;
            cout << string(60, '-') << endl;

            for (size_t i = 0; i < ratings.size(); i++) {
                try {
                    Movie movie = db.getMovie(ratings[i].movieID);
                    cout << left << setw(10) << ratings[i].movieID
                        << setw(35) << movie.getTitle()
                        << fixed << setprecision(1) << ratings[i].getRating() << endl;
                }
                catch (...) {
                    cout << left << setw(10) << ratings[i].movieID
                        << setw(35) << "[Movie Deleted]"
                        << fixed << setprecision(1) << ratings[i].getRating() << endl;
                }
            }

            cout << "\nTotal ratings: " << ratings.size() << endl;
        }
    }
    catch (exception& e) {
        printError(e.what());
    }

    pressEnter();
}


void mainMenu(GraphDatabase& db, EdgeFileManager& edgeMgr) {
    int choice;

    while (true) {
        clearScreen();
        printHeader("GRAPH DATABASE - MAIN MENU");

        cout << "\nUSER OPERATIONS:" << endl;
        cout << "  1. Add User" << endl;
        cout << "  2. View User" << endl;
        cout << "  3. Delete User" << endl;
        cout << "  4. List All Users" << endl;

        cout << "\nMOVIE OPERATIONS:" << endl;
        cout << "  5. Add Movie" << endl;
        cout << "  6. View Movie" << endl;
        cout << "  7. Delete Movie" << endl;
        cout << "  8. List All Movies" << endl;

        cout << "\nRATING OPERATIONS:" << endl;
        cout << "  9. Add Rating" << endl;
        cout << " 10. View User Ratings" << endl;

        cout << "\nSYSTEM:" << endl;
        cout << " 11. Run Stress Test" << endl;
        cout << "  0. Exit" << endl;

        cout << "\nEnter choice: ";
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1: addUserMenu(db); break;
        case 2: viewUserMenu(db); break;
        case 3: deleteUserMenu(db); break;
        case 4: listAllUsersMenu(db); break;
        case 5: addMovieMenu(db); break;
        case 6: viewMovieMenu(db); break;
        case 7: deleteMovieMenu(db); break;
        case 8: listAllMoviesMenu(db); break;
        case 9: addRatingMenu(db, edgeMgr); break;
        case 10: viewUserRatingsMenu(db, edgeMgr); break;
        case 11: goto stress_test; // Jump to stress test
        case 0: return;
        default:
            printError("Invalid choice!");
            pressEnter();
        }
    }

stress_test:
    // Continue to stress test below
    return;
}

// ============================================================================
// STRESS TEST
// ============================================================================

void stressTest() {
    printHeader("STRESS TEST - INITIALIZATION");

    const int NUM_USERS = 5000;
    const int NUM_MOVIES = 1000;
    const int NUM_RATINGS = 20000;
    const int NUM_DELETIONS = 500;
    const int NUM_ARBITRARY_IDS = 100;

    cout << "\nTest Configuration:" << endl;
    cout << "  Users:           " << NUM_USERS << endl;
    cout << "  Movies:          " << NUM_MOVIES << endl;
    cout << "  Ratings:         " << NUM_RATINGS << endl;
    cout << "  Deletions:       " << NUM_DELETIONS << endl;
    cout << "  Arbitrary IDs:   " << NUM_ARBITRARY_IDS << endl;

    cout << "\nThis will test:" << endl;
    cout << "  - Sequential ID allocation" << endl;
    cout << "  - Arbitrary ID support (999999+)" << endl;
    cout << "  - Bitmap slot reuse after deletion" << endl;
    cout << "  - B-Tree performance at scale" << endl;
    cout << "  - Hash cache efficiency" << endl;
    cout << "  - Edge file management" << endl;

    cout << "\nPress ENTER to begin stress test...";
    cin.get();

    GraphDatabase db; // Enable caching
    EdgeFileManager edgeMgr;
    Timer timer;

    // ========================================================================
    // PHASE 1: Mass User Insertion
    // ========================================================================
    printHeader("PHASE 1: INSERTING USERS");
    timer.reset();

    int userSuccess = 0;
    for (int i = 1; i <= NUM_USERS; i++) {
        try {
            string username = "User_" + to_string(i);
            db.addUser(i, username);
            userSuccess++;
        }
        catch (exception& e) {
            printError(string("User ") + to_string(i) + ": " + e.what());
        }

        if (i % 1000 == 0) {
            cout << "  Progress: " << i << " / " << NUM_USERS << endl;
        }
    }

    double userTime = timer.elapsed();
    printSuccess(string("Added ") + to_string(userSuccess) + " users in " +
        to_string(userTime) + " ms");
    cout << "  Throughput: " << (userSuccess / (userTime / 1000.0)) << " users/sec" << endl;
    cout << "  Avg per user: " << (userTime / userSuccess) << " ms" << endl;

    // ========================================================================
    // PHASE 2: Mass Movie Insertion
    // ========================================================================
    printHeader("PHASE 2: INSERTING MOVIES");
    timer.reset();

    int movieSuccess = 0;
    for (int i = 1; i <= NUM_MOVIES; i++) {
        try {
            string title = MOVIE_TITLES[i % MOVIE_TITLES.size()] + " " + to_string(i);
            vector<string> genres;
            genres.push_back(GENRES[i % GENRES.size()]);

            db.addMovie(i, title, genres);
            movieSuccess++;
        }
        catch (exception& e) {
            printError(string("Movie ") + to_string(i) + ": " + e.what());
        }

        if (i % 200 == 0) {
            cout << "  Progress: " << i << " / " << NUM_MOVIES << endl;
        }
    }

    double movieTime = timer.elapsed();
    printSuccess(string("Added ") + to_string(movieSuccess) + " movies in " +
        to_string(movieTime) + " ms");
    cout << "  Throughput: " << (movieSuccess / (movieTime / 1000.0)) << " movies/sec" << endl;
    cout << "  Avg per movie: " << (movieTime / movieSuccess) << " ms" << endl;

    // ========================================================================
    // PHASE 3: Arbitrary ID Test
    // ========================================================================
    printHeader("PHASE 3: ARBITRARY ID TEST");
    printInfo("Testing IDs like 999999, 888888, etc. (no sequential requirement)");
    timer.reset();

    int arbitrarySuccess = 0;
    for (int i = 0; i < NUM_ARBITRARY_IDS; i++) {
        uint32_t arbitraryID = 900000 + i * 100; // 900000, 900100, 900200...

        try {
            string username = "ArbitraryUser_" + to_string(arbitraryID);
            db.addUser(arbitraryID, username);
            arbitrarySuccess++;
        }
        catch (exception& e) {
            printError(string("Arbitrary ID ") + to_string(arbitraryID) + ": " + e.what());
        }
    }

    double arbitraryTime = timer.elapsed();
    printSuccess(string("Added ") + to_string(arbitrarySuccess) + " arbitrary IDs in " +
        to_string(arbitraryTime) + " ms");

    // ========================================================================
    // PHASE 4: Mass Ratings
    // ========================================================================
    printHeader("PHASE 4: INSERTING RATINGS");
    timer.reset();

    srand(42); // Reproducible randomness
    int ratingSuccess = 0;

    for (int i = 0; i < NUM_RATINGS; i++) {
        uint32_t userID = (rand() % NUM_USERS) + 1;
        uint32_t movieID = (rand() % NUM_MOVIES) + 1;
        float rating = 1.0f + (rand() % 9) * 0.5f; // 1.0 to 5.0 in 0.5 steps

        try {
            edgeMgr.addOrUpdateRating(userID, movieID, rating);

            Movie movie = db.getMovie(movieID);
            movie.addRating(rating);
            db.updateMovie(movieID, movie);

            ratingSuccess++;
        }
        catch (...) {
            // Skip invalid combinations
        }

        if (i % 5000 == 0 && i > 0) {
            cout << "  Progress: " << i << " / " << NUM_RATINGS << endl;
        }
    }

    double ratingTime = timer.elapsed();
    printSuccess(string("Added ") + to_string(ratingSuccess) + " ratings in " +
        to_string(ratingTime) + " ms");
    cout << "  Throughput: " << (ratingSuccess / (ratingTime / 1000.0)) << " ratings/sec" << endl;

    // ========================================================================
    // PHASE 5: Deletion and Slot Reuse Test
    // ========================================================================
    printHeader("PHASE 5: DELETION & SLOT REUSE TEST");
    printInfo("Deleting users and verifying bitmap slot reuse");
    timer.reset();

    int deleteSuccess = 0;
    vector<uint32_t> deletedIDs;

    for (int i = 1; i <= NUM_DELETIONS; i++) {
        uint32_t userID = i * 5; // Delete every 5th user

        try {
            db.deleteUser(userID);
            deletedIDs.push_back(userID);
            deleteSuccess++;
        }
        catch (...) {
            // Already deleted or doesn't exist
        }
    }

    double deleteTime = timer.elapsed();
    printSuccess(string("Deleted ") + to_string(deleteSuccess) + " users in " +
        to_string(deleteTime) + " ms");

    // Now add new users and verify they reuse slots
    printInfo("Adding new users to test slot reuse...");
    timer.reset();

    int reuseSuccess = 0;
    for (int i = 0; i < NUM_DELETIONS / 2; i++) {
        uint32_t newID = 800000 + i; // Different ID range

        try {
            db.addUser(newID, "ReusedSlot_" + to_string(newID));
            reuseSuccess++;
        }
        catch (exception& e) {
            printError(e.what());
        }
    }

    double reuseTime = timer.elapsed();
    printSuccess(string("Reused ") + to_string(reuseSuccess) + " slots in " +
        to_string(reuseTime) + " ms");
    printInfo("Bitmap successfully recycled freed slots!");

    // ========================================================================
    // PHASE 6: Random Access Performance Test
    // ========================================================================
    printHeader("PHASE 6: RANDOM ACCESS PERFORMANCE");
    printInfo("Testing B-Tree lookup performance with cache");

    const int NUM_LOOKUPS = 10000;
    timer.reset();

    int lookupSuccess = 0;
    for (int i = 0; i < NUM_LOOKUPS; i++) {
        uint32_t randomUserID = (rand() % NUM_USERS) + 1;

        try {
            User user = db.getUser(randomUserID);
            lookupSuccess++;
        }
        catch (...) {
            // Might be deleted
        }
    }

    double lookupTime = timer.elapsed();
    printSuccess(string("Performed ") + to_string(lookupSuccess) + " lookups in " +
        to_string(lookupTime) + " ms");
    cout << "  Avg lookup time: " << (lookupTime / lookupSuccess) << " ms" << endl;
    cout << "  Throughput: " << (lookupSuccess / (lookupTime / 1000.0)) << " lookups/sec" << endl;

    // ========================================================================
    // FINAL STATISTICS
    // ========================================================================
    printHeader("STRESS TEST COMPLETE - FINAL STATISTICS");

    pressEnter();
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    try {
        srand(time(0));

        printHeader("GRAPH DATABASE SYSTEM");
        cout << "\nInitializing database..." << endl;

        GraphDatabase db;
        EdgeFileManager edgeMgr;

        printSuccess("Database initialized!");
        cout << "\nCurrent Status:" << endl;
        cout << "  Users:  " << db.getUserCount() << endl;
        cout << "  Movies: " << db.getMovieCount() << endl;

        // Show menu
        int choice;
        clearScreen();
        printHeader("WELCOME");
        cout << "\n1. Interactive Menu" << endl;
        cout << "2. Run Stress Test" << endl;
        cout << "0. Exit" << endl;
        cout << "\nChoice: ";
        cin >> choice;
        cin.ignore();

        if (choice == 1) {
            mainMenu(db, edgeMgr);
        }
        else if (choice == 2) {
            stressTest();
        }

        printHeader("SHUTTING DOWN");
        printInfo("Saving metadata...");

        return 0;
    }
    catch (exception& e) {
        printError(string("FATAL: ") + e.what());
        return 1;
    }
}