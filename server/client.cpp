#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <limits>

using namespace std;

struct Message {
    int type;
    int userID;
    char data[8192];
};

// Message types
const int LOGIN = 1;
const int REGISTER = 2;
const int GET_RECOMMENDATIONS = 3;
const int SEARCH_MOVIES = 4;
const int ADD_RATING = 5;
const int GET_POPULAR = 6;
const int GET_USER_RATINGS = 7;
const int GET_MOVIE_DETAILS = 8;
const int SEARCH_BY_GENRE = 9;
const int GET_ALL_GENRES = 10;
const int GET_MOVIES_BY_GENRE = 11;
const int CHANGE_PASSWORD = 12;
const int GET_COLD_START = 13;
const int LOGOUT = 14;
const int SUCCESS = 100;
const int ERROR = 101;

int clientSocket = -1;
int currentUserID = 0;
bool loggedIn = false;

void copyToBuffer(char* buffer, const string& source, size_t size) {
    size_t len = source.length();
    if (len >= size) len = size - 1;

    strncpy(buffer, source.c_str(), len);
    buffer[len] = '\0';
}

vector<string> splitString(const string& str, char delimiter) {
    vector<string> tokens;
    string token;
    stringstream ss(str);
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printHeader(const string& title) {
    cout << "\n========================================" << endl;
    cout << " " << title << endl;
    cout << "========================================\n" << endl;
}

void pausing() {
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

bool sendMsg(Message& msg) {
    int sent = send(clientSocket, &msg, sizeof(Message), 0);
    return sent > 0;
}

bool recvMsg(Message& msg) {
    int received = recv(clientSocket, &msg, sizeof(Message), 0);
    return received > 0;
}

bool connectToServer(string ip, int port) {
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Failed to create socket" << endl;
        return false;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid IP address" << endl;
        close(clientSocket);
        return false;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to connect to server" << endl;
        close(clientSocket);
        return false;
    }

    cout << "Connected to server at " << ip << ":" << port << endl;
    return true;
}

bool login() {
    clearScreen();
    printHeader("LOGIN");

    string username, password;
    cout << "Username: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;

    Message msg, response;
    memset(&msg, 0, sizeof(Message));
    msg.type = LOGIN;

    string payload = username + "|" + password;
    copyToBuffer(msg.data, payload, sizeof(msg.data));

    if (!sendMsg(msg) || !recvMsg(response)) {
        cout << "\n[ERROR] Connection error" << endl;
        pausing();
        return false;
    }

    if (response.type == SUCCESS) {
        currentUserID = response.userID;
        loggedIn = true;
        cout << "\n[OK] Login successful! Welcome, " << username << "!" << endl;
        pausing();
        return true;
    }
    else {
        cout << "\n[ERROR] " << response.data << endl;
        pausing();
        return false;
    }
}

bool registerUser() {
    clearScreen();
    printHeader("REGISTER");

    string username, password, confirm;
    cout << "Username (3-63 chars): ";
    cin >> username;
    cout << "Password (min 6 chars): ";
    cin >> password;
    cout << "Confirm Password: ";
    cin >> confirm;

    if (password != confirm) {
        cout << "\n[ERROR] Passwords do not match!" << endl;
        pausing();
        return false;
    }

    Message msg, response;
    memset(&msg, 0, sizeof(Message));
    msg.type = REGISTER;

    string payload = username + "|" + password;
    copyToBuffer(msg.data, payload, sizeof(msg.data));

    if (!sendMsg(msg) || !recvMsg(response)) {
        cout << "\n[ERROR] Connection error" << endl;
        pausing();
        return false;
    }

    if (response.type == SUCCESS) {
        currentUserID = response.userID;
        cout << "\n[OK] Registration successful!" << endl;
        cout << "Your User ID: " << currentUserID << endl;
        cout << "You can now login." << endl;
        pausing();
        return true;
    }
    else {
        cout << "\n[ERROR] " << response.data << endl;
        pausing();
        return false;
    }
}

void getRecommendations() {
    clearScreen();
    printHeader("MOVIE RECOMMENDATIONS");

    int topN = 10;
    cout << "How many recommendations? (1-20): ";
    cin >> topN;
    if (topN < 1) topN = 1;
    if (topN > 20) topN = 20;

    cout << "\n... Analyzing your preferences ..." << endl;

    Message msg, response;
    memset(&msg, 0, sizeof(Message));
    msg.type = GET_RECOMMENDATIONS;
    msg.userID = currentUserID;

    string payload = to_string(topN);
    copyToBuffer(msg.data, payload, sizeof(msg.data));

    if (!sendMsg(msg) || !recvMsg(response)) {
        cout << "\n[ERROR] Connection error" << endl;
        pausing();
        return;
    }

    if (response.type == SUCCESS) {
        int count = 1;
        bool hasPersonalized = strlen(response.data) > 0;

        if (hasPersonalized) {
            cout << "\n----------------------------------------" << endl;
            cout << "   RECOMMENDED MOVIES FOR YOU         " << endl;
            cout << "----------------------------------------\n" << endl;

            stringstream ss(response.data);
            string line;

            while (getline(ss, line) && count <= topN) {
                if (line.empty()) continue;

                vector<string> parts = splitString(line, '|');
                if (parts.size() >= 5) {
                    int movieID = stoi(parts[0]);
                    string title = parts[1];
                    float score = stof(parts[2]);
                    string genres = parts[4];

                    cout << count++ << ". " << title << endl;
                    cout << "   ID: " << movieID << " | Match Score: " << fixed << setprecision(2) << score << endl;
                    cout << "   Genres: " << genres << endl << endl;
                }
            }
        }

        if (count == 1 || !hasPersonalized) {
            if (!hasPersonalized) cout << "[INFO] No rating history found." << endl;
            cout << "... Fetching top rated movies across genres ...\n" << endl;

            memset(&msg, 0, sizeof(Message));
            msg.type = GET_COLD_START;
            msg.userID = currentUserID;

            if (sendMsg(msg) && recvMsg(response) && response.type == SUCCESS) {
                cout << "----------------------------------------" << endl;
                cout << "   TOP PICKS TO GET YOU STARTED       " << endl;
                cout << "----------------------------------------\n" << endl;

                stringstream ss2(response.data);
                string line;
                count = 1;

                while (getline(ss2, line)) {
                    if (line.empty()) continue;

                    vector<string> parts = splitString(line, '|');
                    if (parts.size() >= 3) {
                        int movieID = stoi(parts[0]);
                        string title = parts[1];
                        float rating = stof(parts[2]);
                        string genres = (parts.size() > 3) ? parts[3] : "";

                        cout << count++ << ". " << title << endl;
                        cout << "   ID: " << movieID << " | Avg Rating: " << fixed << setprecision(1) << rating << "/5.0" << endl;
                        cout << "   Genres: " << genres << endl << endl;
                    }
                }
            }
        }
    }
    else {
        cout << "\n[ERROR] " << response.data << endl;
    }

    pausing();
}

void rateMovie() {
    clearScreen();
    printHeader("RATE A MOVIE");

    cout << "1. Search by Name" << endl;
    cout << "2. Enter Movie ID manually" << endl;
    cout << "Choice: ";
    int method;
    if (!(cin >> method)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "\n[ERROR] Invalid input." << endl;
        pausing();
        return;
    }

    int targetID = 0;

    if (method == 1) {
        cin.ignore();
        string query;
        cout << "\nEnter partial movie name: ";
        getline(cin, query);

        Message msg, response;
        memset(&msg, 0, sizeof(Message));
        msg.type = SEARCH_MOVIES;
        msg.userID = currentUserID;
        copyToBuffer(msg.data, query, sizeof(msg.data));

        if (!sendMsg(msg) || !recvMsg(response)) {
            cout << "\n[ERROR] Connection error" << endl;
            pausing();
            return;
        }

        if (response.type == SUCCESS) {
            vector<int> movieIDs;
            vector<string> titles;

            stringstream ss(response.data);
            string line;

            while (getline(ss, line)) {
                if (line.empty()) continue;

                vector<string> parts = splitString(line, '|');
                if (parts.size() >= 2) {
                    int id = stoi(parts[0]);
                    string title = parts[1];
                    movieIDs.push_back(id);
                    titles.push_back(title);
                }
            }

            if (movieIDs.empty()) {
                cout << "No movies found matching '" << query << "'" << endl;
                pausing();
                return;
            }

            cout << "\nFound " << movieIDs.size() << " movies:" << endl;
            for (size_t i = 0; i < movieIDs.size() && i < 10; i++) {
                cout << (i + 1) << ". " << titles[i] << " (ID: " << movieIDs[i] << ")" << endl;
            }

            int selection;
            cout << "\nSelect number (0 to cancel): ";
            cin >> selection;

            if (selection > 0 && selection <= (int)movieIDs.size()) {
                targetID = movieIDs[selection - 1];
            }
            else {
                return;
            }
        }
        else {
            cout << "\n[ERROR] " << response.data << endl;
            pausing();
            return;
        }
    }
    else {
        cout << "Movie ID: ";
        cin >> targetID;
    }

    Message msg, response;
    memset(&msg, 0, sizeof(Message));
    msg.type = GET_MOVIE_DETAILS;
    msg.userID = currentUserID;

    string payload = to_string(targetID);
    copyToBuffer(msg.data, payload, sizeof(msg.data));

    if (!sendMsg(msg) || !recvMsg(response)) {
        cout << "\n[ERROR] Connection error" << endl;
        pausing();
        return;
    }

    if (response.type != SUCCESS) {
        cout << "\n[ERROR] Movie not found!" << endl;
        pausing();
        return;
    }

    string respStr(response.data);
    vector<string> parts = splitString(respStr, '|');

    if (parts.size() >= 2) {
        string title = parts[1];
        cout << "\nSelected: " << title << endl;

        float rating;
        cout << "\nYour rating (1.0 - 5.0): ";
        cin >> rating;

        if (rating < 1.0 || rating > 5.0) {
            cout << "\n[ERROR] Invalid rating!" << endl;
            pausing();
            return;
        }

        memset(&msg, 0, sizeof(Message));
        msg.type = ADD_RATING;
        msg.userID = currentUserID;

        string ratePayload = to_string(targetID) + "|" + to_string(rating);
        copyToBuffer(msg.data, ratePayload, sizeof(msg.data));

        if (!sendMsg(msg) || !recvMsg(response)) {
            cout << "\n[ERROR] Connection error" << endl;
            pausing();
            return;
        }

        if (response.type == SUCCESS) {
            cout << "\n[OK] Rating saved successfully!" << endl;
        }
        else {
            cout << "\n[ERROR] " << response.data << endl;
        }
    }
    else {
        cout << "\n[ERROR] Invalid data received." << endl;
    }

    pausing();
}

void viewRatings() {
    clearScreen();
    printHeader("MY RATINGS");

    Message msg, response;
    memset(&msg, 0, sizeof(Message));
    msg.type = GET_USER_RATINGS;
    msg.userID = currentUserID;

    if (!sendMsg(msg) || !recvMsg(response)) {
        cout << "\n[ERROR] Connection error" << endl;
        pausing();
        return;
    }

    if (response.type == SUCCESS) {
        vector<pair<int, float>> ratings;

        stringstream ss(response.data);
        string line;

        while (getline(ss, line)) {
            if (line.empty()) continue;

            vector<string> parts = splitString(line, '|');
            if (parts.size() >= 2) {
                int movieID = stoi(parts[0]);
                float rating = stof(parts[1]);
                ratings.push_back({ movieID, rating });
            }
        }

        if (ratings.empty()) {
            cout << "You haven't rated any movies yet." << endl;
        }
        else {
            cout << "Total movies rated: " << ratings.size() << "\n" << endl;

            for (size_t i = 0; i < ratings.size(); i++) {
                memset(&msg, 0, sizeof(Message));
                msg.type = GET_MOVIE_DETAILS;
                msg.userID = currentUserID;

                string payload = to_string(ratings[i].first);
                copyToBuffer(msg.data, payload, sizeof(msg.data));

                if (sendMsg(msg) && recvMsg(response) && response.type == SUCCESS) {
                    string respStr(response.data);
                    vector<string> parts = splitString(respStr, '|');

                    if (parts.size() >= 2) {
                        string title = parts[1];
                        cout << "* " << title << endl;
                        cout << "  Your rating: " << fixed << setprecision(1) << ratings[i].second << "/5.0" << endl;
                        cout << "  Movie ID: " << ratings[i].first << endl << endl;
                    }
                }
            }
        }
    }
    else {
        cout << "\n[ERROR] " << response.data << endl;
    }

    pausing();
}

void searchMovies() {
    clearScreen();
    printHeader("SEARCH MOVIES");

    cout << "Search by:" << endl;
    cout << "1. Movie Name" << endl;
    cout << "2. Genre (Select from list)" << endl;
    cout << "\nChoice: ";

    int choice;
    if (!(cin >> choice)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }

    if (choice == 1) {
        cin.ignore();
        string query;
        cout << "\nEnter partial movie name: ";
        getline(cin, query);

        Message msg, response;
        memset(&msg, 0, sizeof(Message));
        msg.type = SEARCH_MOVIES;
        msg.userID = currentUserID;
        copyToBuffer(msg.data, query, sizeof(msg.data));

        if (!sendMsg(msg) || !recvMsg(response)) {
            cout << "\n[ERROR] Connection error" << endl;
            pausing();
            return;
        }

        if (response.type == SUCCESS) {
            stringstream ss(response.data);
            string line;
            int count = 1;

            cout << "\n";
            while (getline(ss, line) && count <= 10) {
                if (line.empty()) continue;

                vector<string> parts = splitString(line, '|');
                if (parts.size() >= 2) {
                    int id = stoi(parts[0]);
                    string title = parts[1];
                    cout << count++ << ". " << title << " (ID: " << id << ")" << endl;
                }
            }

            if (count == 1) {
                cout << "No movies found matching '" << query << "'" << endl;
            }
        }
        else {
            cout << "\n[ERROR] " << response.data << endl;
        }
    }
    else if (choice == 2) {
        Message msg, response;
        memset(&msg, 0, sizeof(Message));
        msg.type = GET_ALL_GENRES;
        msg.userID = currentUserID;

        if (!sendMsg(msg) || !recvMsg(response)) {
            cout << "\n[ERROR] Connection error" << endl;
            pausing();
            return;
        }

        if (response.type == SUCCESS) {
            vector<string> genres;
            stringstream ss(response.data);
            string genre;

            while (getline(ss, genre)) {
                if (!genre.empty()) {
                    genres.push_back(genre);
                }
            }

            if (genres.empty()) {
                cout << "\nNo genres found." << endl;
                pausing();
                return;
            }

            cout << "\nAvailable Genres:" << endl;
            for (size_t i = 0; i < genres.size(); i++) {
                cout << (i + 1) << ". " << genres[i] << endl;
            }

            cout << "\nSelect Genre Number: ";
            int gIdx;
            cin >> gIdx;

            if (gIdx > 0 && gIdx <= (int)genres.size()) {
                string selectedGenre = genres[gIdx - 1];

                memset(&msg, 0, sizeof(Message));
                msg.type = GET_MOVIES_BY_GENRE;
                msg.userID = currentUserID;
                copyToBuffer(msg.data, selectedGenre, sizeof(msg.data));

                if (sendMsg(msg) && recvMsg(response) && response.type == SUCCESS) {
                    vector<int> movieIDs;
                    stringstream ss2(response.data);
                    string line;

                    while (getline(ss2, line)) {
                        if (!line.empty()) {
                            movieIDs.push_back(atoi(line.c_str()));
                        }
                    }

                    cout << "\nFound " << movieIDs.size() << " movies in '" << selectedGenre << "':" << endl;
                    int count = 1;
                    for (size_t i = 0; i < movieIDs.size() && count <= 20; i++) {
                        memset(&msg, 0, sizeof(Message));
                        msg.type = GET_MOVIE_DETAILS;
                        msg.userID = currentUserID;

                        string payload = to_string(movieIDs[i]);
                        copyToBuffer(msg.data, payload, sizeof(msg.data));

                        if (sendMsg(msg) && recvMsg(response) && response.type == SUCCESS) {
                            string respStr(response.data);
                            vector<string> parts = splitString(respStr, '|');
                            if (parts.size() >= 3) {
                                string title = parts[1];
                                float avgRating = stof(parts[2]);
                                cout << count++ << ". " << title << " (" << fixed << setprecision(1) << avgRating << ")" << endl;
                            }
                        }
                    }
                    if (count > 20) {
                        cout << "\n(Showing first 20 results)" << endl;
                    }
                }
            }
        }
    }

    pausing();
}

void viewPopular() {
    clearScreen();
    printHeader("POPULAR MOVIES");

    Message msg, response;
    memset(&msg, 0, sizeof(Message));
    msg.type = GET_POPULAR;
    msg.userID = currentUserID;
    strcpy(msg.data, "15");

    if (!sendMsg(msg) || !recvMsg(response)) {
        cout << "\n[ERROR] Connection error" << endl;
        pausing();
        return;
    }

    if (response.type == SUCCESS) {
        stringstream ss(response.data);
        string line;
        int count = 1;

        while (getline(ss, line)) {
            if (line.empty()) continue;

            vector<string> parts = splitString(line, '|');

            if (parts.size() >= 4) {
                int id = stoi(parts[0]);
                string title = parts[1];
                float rating = stof(parts[2]);
                int ratingCount = stoi(parts[3]);

                cout << count++ << ". " << title << endl;
                cout << "   Rating: " << fixed << setprecision(1) << rating << "/5.0";
                cout << " (" << ratingCount << " ratings)" << endl;
                cout << "   ID: " << id << endl << endl;
            }
        }
    }
    else {
        cout << "\n[ERROR] " << response.data << endl;
    }

    pausing();
}

void changePassword() {
    clearScreen();
    printHeader("CHANGE PASSWORD");

    string oldPassword, newPassword, confirm;
    cout << "Current Password: ";
    cin >> oldPassword;
    cout << "New Password (min 6 chars): ";
    cin >> newPassword;
    cout << "Confirm New Password: ";
    cin >> confirm;

    if (newPassword != confirm) {
        cout << "\n[ERROR] Passwords do not match!" << endl;
        pausing();
        return;
    }

    Message msg, response;
    memset(&msg, 0, sizeof(Message));
    msg.type = CHANGE_PASSWORD;
    msg.userID = currentUserID;

    string payload = oldPassword + "|" + newPassword;
    copyToBuffer(msg.data, payload, sizeof(msg.data));

    if (!sendMsg(msg) || !recvMsg(response)) {
        cout << "\n[ERROR] Connection error" << endl;
        pausing();
        return;
    }

    if (response.type == SUCCESS) {
        cout << "\n[OK] Password changed successfully!" << endl;
    }
    else {
        cout << "\n[ERROR] " << response.data << endl;
    }

    pausing();
}

void showMainMenu() {
    cout << "\n1. Login" << endl;
    cout << "2. Register" << endl;
    cout << "3. Exit" << endl;
    cout << "\nChoice: ";
}

void showUserMenu() {
    printHeader("USER MENU");
    cout << "1. Get Recommendations" << endl;
    cout << "2. Rate a Movie" << endl;
    cout << "3. View My Ratings" << endl;
    cout << "4. Search Movies" << endl;
    cout << "5. View Popular Movies" << endl;
    cout << "6. Change Password" << endl;
    cout << "7. Logout" << endl;
    cout << "\nChoice: ";
}

int main(int argc, char* argv[]) {
    string serverIP = "127.0.0.1";
    int serverPort = 8080;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 && i + 1 < argc) {
            serverIP = argv[++i];
        }
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            serverPort = atoi(argv[++i]);
        }
    }

    clearScreen();
    cout << "+--------------------------------------+" << endl;
    cout << "|    MOVIE RECOMMENDATION SYSTEM v2.0  |" << endl;
    cout << "+--------------------------------------+" << endl;

    if (!connectToServer(serverIP, serverPort)) {
        return 1;
    }

    pausing();

    while (!loggedIn) {
        clearScreen();
        printHeader("MAIN MENU");
        showMainMenu();

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (choice == 1) {
            login();
        }
        else if (choice == 2) {
            registerUser();
        }
        else if (choice == 3) {
            close(clientSocket);
            cout << "\nThank you for using Movie Recommendation System!" << endl;
            return 0;
        }
    }

    while (loggedIn) {
        clearScreen();
        showUserMenu();

        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
        case 1:
            getRecommendations();
            break;
        case 2:
            rateMovie();
            break;
        case 3:
            viewRatings();
            break;
        case 4:
            searchMovies();
            break;
        case 5:
            viewPopular();
            break;
        case 6:
            changePassword();
            break;
        case 7:
        {
            Message msg, response;
            memset(&msg, 0, sizeof(Message));
            msg.type = LOGOUT;
            msg.userID = currentUserID;

            if (sendMsg(msg) && recvMsg(response)) {
                loggedIn = false;
                cout << "\n[OK] Logged out successfully!" << endl;
                pausing();
            }
        }
        break;
        default:
            cout << "\nInvalid choice!" << endl;
            pausing();
        }
    }

    close(clientSocket);
    return 0;
}