#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <iomanip>
#include <cmath>

#include "./core/types.h"
#include "./graph/node.h"
#include "./core/bitmap.h"
#include "./core/storage_manager.h" 
#include "./core/btree.h"

using namespace std;

Bitmap* userBitmap = nullptr;
Bitmap* movieBitmap = nullptr;

FixedStorage<User>* userStorage = nullptr;
FixedStorage<Movie>* movieStorage = nullptr;

BTree<uint32_t, uint64_t>* userIndex = nullptr;
BTree<uint32_t, uint64_t>* movieIndex = nullptr;

EdgeFileManager* edgeManager = nullptr;


void printSeparator() {
    cout << "----------------------------------------------------------------\n";
}

void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Initialize all components
void initDatabase() {
    // Create ratings directory if it doesn't exist
#ifdef _WIN32
    system("mkdir ratings 2>NUL");
#else
    system("mkdir -p ratings 2>/dev/null");
#endif

    // 100 blocks is enough for testing manually
    userBitmap = new Bitmap(100);
    movieBitmap = new Bitmap(100);

    // Initialize Storage
    userStorage = new FixedStorage<User>("users.dat", User::getSize(), USERS_PER_BLOCK);
    movieStorage = new FixedStorage<Movie>("movies.dat", Movie::getSize(), MOVIES_PER_BLOCK);

    // Initialize Indices
    userIndex = new BTree<uint32_t, uint64_t>("users.idx");
    movieIndex = new BTree<uint32_t, uint64_t>("movies.idx");

    // Initialize Edge Manager
    edgeManager = new EdgeFileManager("ratings/");

    // Check if we need to reload existing bitmaps from file?
    // For this manual test, we assume a fresh start or simple persistence.
    // Ideally, you'd save/load the Bitmap state to a file like "metadata.dat".
    // For now, we will rely on the BTree to recover "used" status if we restart.

    // Quick Recovery Logic: Sync Bitmap with BTree
    vector<pair<uint32_t, uint64_t>> existingUsers = userIndex->getAllPairs();
    for (auto& p : existingUsers) userBitmap->setBit(p.first);

    vector<pair<uint32_t, uint64_t>> existingMovies = movieIndex->getAllPairs();
    for (auto& p : existingMovies) movieBitmap->setBit(p.first);
}

// Cleanup to prevent memory leaks
void shutdownDatabase() {
    delete userBitmap;
    delete movieBitmap;
    delete userStorage;
    delete movieStorage;
    delete userIndex;
    delete movieIndex;
    delete edgeManager;
}

// ============================================================================
// MENU ACTIONS
// ============================================================================

void addUser() {
    uint32_t id;
    string name;

    cout << ">> Enter User ID: ";
    cin >> id;
    if (cin.fail()) { clearInput(); return; }

    // Check 1: Bitmap (Fast check)
    if (!userBitmap->isFree(id)) {
        cout << "[Error] User ID " << id << " is already taken (checked Bitmap).\n";
        return;
    }

    cout << ">> Enter Username: ";
    cin.ignore();
    getline(cin, name);

    // Create Node
    User newUser(id, name);

    try {
        // Step 1: Write to Disk
        userStorage->writeNode(id, newUser);

        // Step 2: Update Index (Map ID -> 1 for existence)
        userIndex->insert(id, 1);

        // Step 3: Update Bitmap
        userBitmap->setBit(id);

        cout << "[Success] User added to Disk, Index, and Bitmap.\n";
    }
    catch (const exception& e) {
        cout << "[Error] Storage failure: " << e.what() << "\n";
    }
}

void addMovie() {
    uint32_t id;
    string title;

    cout << ">> Enter Movie ID: ";
    cin >> id;
    if (cin.fail()) { clearInput(); return; }

    if (!movieBitmap->isFree(id)) {
        cout << "[Error] Movie ID " << id << " is already taken.\n";
        return;
    }

    cout << ">> Enter Movie Title: ";
    cin.ignore();
    getline(cin, title);

    // Ask for genres
    vector<string> genres;
    cout << ">> Enter number of genres (max " << MAX_GENRES << "): ";
    int numGenres;
    cin >> numGenres;
    if (cin.fail()) { clearInput(); numGenres = 0; }

    cin.ignore();
    for (int i = 0; i < numGenres && i < MAX_GENRES; i++) {
        string genre;
        cout << "   Genre " << (i + 1) << ": ";
        getline(cin, genre);
        if (!genre.empty()) {
            genres.push_back(genre);
        }
    }

    if (genres.empty()) {
        genres.push_back("General");
    }

    Movie newMovie(id, title, genres);

    try {
        movieStorage->writeNode(id, newMovie);
        movieIndex->insert(id, 1);
        movieBitmap->setBit(id);
        cout << "[Success] Movie added.\n";
    }
    catch (const exception& e) {
        cout << "[Error] " << e.what() << "\n";
    }
}

void listUsers() {
    cout << "\n--- User List (Retrieved from BTree -> Disk) ---\n";

    // 1. Get all IDs from BTree
    vector<pair<uint32_t, uint64_t>> all = userIndex->getAllPairs();

    if (all.empty()) {
        cout << "Database is empty.\n";
        return;
    }

    // 2. Iterate and Load from Disk
    cout << left << setw(10) << "ID" << setw(30) << "Username" << setw(15) << "Ratings Made" << endl;
    printSeparator();

    for (auto& p : all) {
        uint32_t uid = p.first;
        try {
            User u = userStorage->readNode(uid);
            cout << left << setw(10) << u.userID
                << setw(30) << u.getUsername()
                << setw(15) << u.totalRatings << endl;
        }
        catch (...) {
            cout << "Error reading ID " << uid << endl;
        }
    }
}

void listMovies() {
    cout << "\n--- Movie List (Retrieved from BTree -> Disk) ---\n";

    vector<pair<uint32_t, uint64_t>> all = movieIndex->getAllPairs();

    if (all.empty()) {
        cout << "Database is empty.\n";
        return;
    }

    cout << left << setw(10) << "ID" << setw(30) << "Title" << setw(12) << "Avg Rating"
        << setw(8) << "Count" << "Genres" << endl;
    printSeparator();

    for (auto& p : all) {
        uint32_t mid = p.first;
        try {
            Movie m = movieStorage->readNode(mid);

            // Get genres as a comma-separated string
            vector<string> genres = m.getGenres();
            string genreStr = "";
            for (size_t i = 0; i < genres.size(); i++) {
                genreStr += genres[i];
                if (i < genres.size() - 1) genreStr += ", ";
            }

            cout << left << setw(10) << m.movieID
                << setw(30) << m.getTitle()
                << setw(12) << fixed << setprecision(1) << m.getAvgRating()
                << setw(8) << m.ratingCount
                << genreStr << endl;
        }
        catch (...) {
            cout << "Error reading ID " << mid << endl;
        }
    }
}

void addRating() {
    uint32_t uid, mid;
    float rating;

    cout << ">> Enter User ID: "; cin >> uid;
    cout << ">> Enter Movie ID: "; cin >> mid;
    cout << ">> Enter Rating (1.0-5.0): "; cin >> rating;

    // Check existence via Bitmaps (O(1) check)
    if (userBitmap->isFree(uid)) { cout << "[Error] User not found.\n"; return; }
    if (movieBitmap->isFree(mid)) { cout << "[Error] Movie not found.\n"; return; }

    try {
        // 1. Update Edge File (The raw rating data)
        edgeManager->addOrUpdateRating(uid, mid, rating);

        // 2. Update User Node (Stats)
        User u = userStorage->readNode(uid);
        u.totalRatings++;
        userStorage->writeNode(uid, u);

        // 3. Update Movie Node (Stats)
        Movie m = movieStorage->readNode(mid);
        m.addRating(rating); // Updates sum and count
        movieStorage->writeNode(mid, m);

        cout << "[Success] Rating saved. User and Movie stats updated.\n";

    }
    catch (const exception& e) {
        cout << "[Error] Rating transaction failed: " << e.what() << "\n";
    }
}

void viewUserRatings() {
    uint32_t uid;
    cout << ">> Enter User ID: "; cin >> uid;

    if (userBitmap->isFree(uid)) { cout << "[Error] User not found.\n"; return; }

    vector<RatingEdge> ratings = edgeManager->readRatings(uid);

    if (ratings.empty()) {
        cout << "No ratings found for this user.\n";
        return;
    }

    cout << "\nRatings for User " << uid << ":\n";
    for (auto& r : ratings) {
        // Optional: Load movie title for better display
        string title = "Unknown";
        if (!movieBitmap->isFree(r.movieID)) {
            Movie m = movieStorage->readNode(r.movieID);
            title = m.getTitle();
        }
        cout << " - Movie: " << title << " (ID: " << r.movieID << ") -> " << r.getRating() << "/5.0\n";
    }
}

// ============================================================================
// MAIN LOOP
// ============================================================================
int main() {
    initDatabase();

    int choice = 0;
    while (true) {
        cout << "\n=== MANUAL DATABASE TESTER ===\n";
        cout << "1. Add User\n";
        cout << "2. Add Movie\n";
        cout << "3. List All Users\n";
        cout << "4. List All Movies\n";
        cout << "5. Add Rating (Connect User -> Movie)\n";
        cout << "6. View User Ratings\n";
        cout << "0. Exit\n";
        printSeparator();
        cout << "Choice: ";
        cin >> choice;

        if (cin.fail()) { clearInput(); continue; }

        switch (choice) {
        case 1: addUser(); break;
        case 2: addMovie(); break;
        case 3: listUsers(); break;
        case 4: listMovies(); break;
        case 5: addRating(); break;
        case 6: viewUserRatings(); break;
        case 0:
            cout << "Exiting...\n";
            shutdownDatabase();
            return 0;
        default: cout << "Invalid choice.\n";
        }
    }
}