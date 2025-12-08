#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <iomanip>

#include "graph/node.h"
#include "core/storage_manager.h"
#include "core/btree.h"

using namespace std;

StorageManager* storage = nullptr;

BTree<uint32_t, uint64_t>* userIndex = nullptr;
BTree<uint32_t, uint64_t>* movieIndex = nullptr;


void printSeparator() {
    cout << "------------------------------------------------\n";
}

// Helper to clear input buffer
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void displayUser(const User& u) {
    cout << " [User ID]: " << u.userID << " | [Name]: " << u.username << "\n";
    cout << "   Rated Movies: " << u.ratings.size() << "\n";
    for (const auto& r : u.ratings) {
        cout << "     -> MovieID: " << r.movieID << " (Rating: " << r.rating << ")\n";
    }
}

void displayMovie(const Movie& m) {
    cout << " [Movie ID]: " << m.movieID << " | [Title]: " << m.title << "\n";
    cout << "   Avg Rating: " << fixed << setprecision(1) << m.avgRating
        << " (" << m.ratingCount << " votes)\n";
    cout << "   Genres: ";
    for (const auto& g : m.genres) cout << g << " ";
    cout << "\n";
}


void addUser() {
    uint32_t id;
    string name;

    cout << "Enter User ID: ";
    cin >> id;

    // Check if ID exists
    uint64_t dummyOffset;
    if (userIndex->search(id, dummyOffset)) {
        cout << "Error: User ID already exists!\n";
        return;
    }

    cout << "Enter Username: ";
    cin.ignore();
    getline(cin, name);

    // Create and Store
    User newUser(id, name);
    try {
        // 1. Store in Data File -> Get Offset
        uint64_t offset = storage->storeUser(newUser);

        // 2. Map ID to Offset in BTree
        userIndex->insert(id, offset);

        cout << "User added successfully (Stored at offset: " << offset << ")\n";
    }
    catch (const exception& e) {
        cout << "Storage Error: " << e.what() << "\n";
    }
}

void addMovie() {
    uint32_t id;
    string title, genreLine;
    vector<string> genres;

    cout << "Enter Movie ID: ";
    cin >> id;

    uint64_t dummyOffset;
    if (movieIndex->search(id, dummyOffset)) {
        cout << "Error: Movie ID already exists!\n";
        return;
    }

    cout << "Enter Title: ";
    cin.ignore();
    getline(cin, title);

    cout << "Enter Genres (space separated, e.g., Action SciFi): ";
    getline(cin, genreLine);

    // Simple split for genres
    size_t pos = 0;
    string token;
    while ((pos = genreLine.find(" ")) != string::npos) {
        token = genreLine.substr(0, pos);
        if (!token.empty()) genres.push_back(token);
        genreLine.erase(0, pos + 1);
    }
    if (!genreLine.empty()) genres.push_back(genreLine);

    // Create and Store
    Movie newMovie(id, title, genres);
    try {
        uint64_t offset = storage->storeMovie(newMovie);
        movieIndex->insert(id, offset);
        cout << "Movie added successfully (Stored at offset: " << offset << ")\n";
    }
    catch (const exception& e) {
        cout << "Storage Error: " << e.what() << "\n";
    }
}

void findUser() {
    uint32_t id;
    cout << "Enter User ID to find: ";
    cin >> id;

    uint64_t offset;
    if (userIndex->search(id, offset)) {
        try {
            User u = storage->loadUser(offset);
            printSeparator();
            displayUser(u);
        }
        catch (const exception& e) {
            cout << "Error loading user data: " << e.what() << "\n";
        }
    }
    else {
        cout << "User not found.\n";
    }
}

void findMovie() {
    uint32_t id;
    cout << "Enter Movie ID to find: ";
    cin >> id;

    uint64_t offset;
    if (movieIndex->search(id, offset)) {
        try {
            Movie m = storage->loadMovie(offset);
            printSeparator();
            displayMovie(m);
        }
        catch (const exception& e) {
            cout << "Error loading movie data: " << e.what() << "\n";
        }
    }
    else {
        cout << "Movie not found.\n";
    }
}

void rateMovie() {
    uint32_t uid, mid;
    float rating;

    cout << "Enter User ID: "; cin >> uid;
    cout << "Enter Movie ID: "; cin >> mid;
    cout << "Enter Rating (1.0 - 5.0): "; cin >> rating;

    uint64_t uOff, mOff;
    bool userExists = userIndex->search(uid, uOff);
    bool movieExists = movieIndex->search(mid, mOff);

    if (!userExists || !movieExists) {
        cout << "Error: User or Movie not found.\n";
        return;
    }

    try {
        // 1. Load both objects from disk
        User u = storage->loadUser(uOff);
        Movie m = storage->loadMovie(mOff);

        // 2. Update In-Memory Objects
        u.rateMovie(mid, rating); // Adds to User's list and creates Edge
        m.addRating(uid, rating); // Adds to Movie's list and creates Edge
        m.updateAvgRating();

        storage->updateUser(uOff, u);
        storage->updateMovie(mOff, m);

        cout << "Rating applied and saved to disk.\n";

    }
    catch (const exception& e) {
        cout << "Error during rating transaction: " << e.what() << "\n";
    }
}

void listAllUsers() {
    cout << "--- Listing All Users from BTree ---\n";
    vector<pair<uint32_t, uint64_t>> all = userIndex->getAllPairs();

    if (all.empty()) cout << "No users found.\n";

    for (auto& pair : all) {
        // pair.first is ID, pair.second is Offset
        try {
            User u = storage->loadUser(pair.second);
            displayUser(u);
        }
        catch (exception& e) {
            cout << "Error loading ID " << pair.first << "\n";
        }
    }
}

void listAllMovies() {
    cout << "--- Listing All Movies from BTree ---\n";
    vector<pair<uint32_t, uint64_t>> all = movieIndex->getAllPairs();

    if (all.empty()) cout << "No movies found.\n";

    for (auto& pair : all) {
        try {
            Movie m = storage->loadMovie(pair.second);
            displayMovie(m);
        }
        catch (exception& e) {
            cout << "Error loading ID " << pair.first << "\n";
        }
    }
}


int main() {
    try {
        storage = new StorageManager("data_storage.dat");
        userIndex = new BTree<uint32_t, uint64_t>("users.idx");
        movieIndex = new BTree<uint32_t, uint64_t>("movies.idx");
    }
    catch (const exception& e) {
        cerr << "Fatal Initialization Error: " << e.what() << endl;
        return 1;
    }

    int choice = 0;
    while (true) {
        printSeparator();
        cout << "1. Add User\n";
        cout << "2. Add Movie\n";
        cout << "3. Find User (by ID)\n";
        cout << "4. Find Movie (by ID)\n";
        cout << "5. Rate a Movie (Connect User & Movie)\n";
        cout << "6. List All Users\n";
        cout << "7. List All Movies\n";
        cout << "0. Exit\n";
        printSeparator();
        cout << "Enter choice: ";
        cin >> choice;

        if (cin.fail()) {
            clearInput();
            continue;
        }

        switch (choice) {
        case 1: addUser(); break;
        case 2: addMovie(); break;
        case 3: findUser(); break;
        case 4: findMovie(); break;
        case 5: rateMovie(); break;
        case 6: listAllUsers(); break;
        case 7: listAllMovies(); break;
        case 0:
            cout << "Saving and Exiting...\n";
            delete storage;
            delete userIndex;
            delete movieIndex;
            return 0;
        default:
            cout << "Invalid choice.\n";
        }
    }
}