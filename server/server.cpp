#include "../core/recommendation_engine.h"
#include "../authentication/auth_manager.h"
#include "../core/parser.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <csignal>
#include <thread>
#include <mutex>
#include <vector>
#include <shared_mutex>
#include <sstream>
#include <algorithm>

using namespace std;

// Global variables
RecommendationEngine* engine = nullptr;
AuthManager* auth = nullptr;
int serverSocket = -1;
bool running = true;
shared_mutex storageMutex;

// Mutexes for thread safety
mutex engineMutex;
mutex authMutex;

struct Message {
    int type;
    int userID;
    char data[8192];
};

// Message Types
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


void cleanup() {
    running = false;
    if (serverSocket >= 0) {
        close(serverSocket);
    }
    if (engine) delete engine;
    if (auth) delete auth;
    cout << "\n[Server] Shutdown complete" << endl;
}

void handleSignal(int sig) {
    cout << "\n[Server] Shutting down..." << endl;
    cleanup();
    exit(0);
}

void copyToBuffer(char* buffer, const string& source, size_t size) {
    memset(buffer, 0, size);
    size_t len = source.length();
    if (len >= size) {
        len = size - 1;
    }
    if (len > 0) {
        memcpy(buffer, source.c_str(), len);
    }
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

bool sendMsg(int socket, Message& msg) {
    int sent = send(socket, &msg, sizeof(Message), 0);
    return sent > 0;
}

bool recvMsg(int socket, Message& msg) {
    int received = recv(socket, &msg, sizeof(Message), 0);
    return received > 0;
}


void handleLogin(Message& request, Message& response) {
    lock_guard<mutex> lock(authMutex);

    string reqData(request.data);
    vector<string> parts = splitString(reqData, '|');

    if (parts.size() < 2) {
        response.type = ERROR;
        copyToBuffer(response.data, "Invalid credentials format", sizeof(response.data));
        return;
    }

    try {
        string username = parts[0];
        string password = parts[1];
        int userID = auth->login(username, password);

        {
            lock_guard<mutex> engineLock(engineMutex);
            if (!engine->userExists(userID)) {
                engine->createUser(userID, username);
            }
        }

        response.type = SUCCESS;
        response.userID = userID;
        copyToBuffer(response.data, "Login successful", sizeof(response.data));
        cout << "[Server] User logged in: " << username << " (ID: " << userID << ")" << endl;
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleRegister(Message& request, Message& response) {
    lock_guard<mutex> lock(authMutex);

    string reqData(request.data);
    vector<string> parts = splitString(reqData, '|');

    if (parts.size() < 2) {
        response.type = ERROR;
        copyToBuffer(response.data, "Invalid format", sizeof(response.data));
        return;
    }

    try {
        string username = parts[0];
        string password = parts[1];
        int userID = auth->registerUser(username, password);

        {
            lock_guard<mutex> engineLock(engineMutex);
            if (!engine->userExists(userID)) {
                engine->createUser(userID, username);
            }
        }

        response.type = SUCCESS;
        response.userID = userID;
        copyToBuffer(response.data, "Registration successful", sizeof(response.data));
        cout << "[Server] New user registered: " << username << " (ID: " << userID << ")" << endl;
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleRecommendations(Message& request, Message& response) {
    shared_lock<shared_mutex> lock(storageMutex);
    lock_guard<mutex> engineLock(engineMutex);

    try {
        int topN = 10;
        string reqData(request.data);
        if (!reqData.empty()) {
            topN = stoi(reqData);
        }

        vector<RecommendationResult> recs = engine->getRecommendations(request.userID, topN);

        string result;
        for (const auto& rec : recs) {
            result += to_string(rec.movieID) + "|";
            result += rec.title + "|";
            result += to_string(rec.score) + "|";
            result += to_string(rec.avgRating) + "|";

            for (size_t j = 0; j < rec.genres.size(); j++) {
                result += rec.genres[j];
                if (j < rec.genres.size() - 1) result += ",";
            }
            result += "\n";
        }

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleColdStart(Message& request, Message& response) {
    lock_guard<mutex> lock(engineMutex);

    try {
        vector<RecommendationResult> recs = engine->getColdStartRecommendations();

        string result;
        for (const auto& rec : recs) {
            result += to_string(rec.movieID) + "|";
            result += rec.title + "|";
            result += to_string(rec.avgRating) + "|";

            for (size_t j = 0; j < rec.genres.size(); j++) {
                result += rec.genres[j];
                if (j < rec.genres.size() - 1) result += ",";
            }
            result += "\n";
        }

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleSearch(Message& request, Message& response) {
    lock_guard<mutex> lock(engineMutex);

    try {
        string query(request.data);
        vector<Movie> movies = engine->searchMoviesByTitle(query);

        string result;
        for (const auto& movie : movies) {
            result += to_string(movie.movieID) + "|";
            result += movie.getTitle() + "|";
            result += to_string(movie.getAvgRating()) + "\n";
        }

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleGetMovieDetails(Message& request, Message& response) {
    lock_guard<mutex> lock(engineMutex);

    try {
        string reqData(request.data);
        int movieID = stoi(reqData);
        Movie movie = engine->getMovie(movieID);

        string result = to_string(movie.movieID) + "|";
        result += movie.getTitle() + "|";
        result += to_string(movie.getAvgRating()) + "|";
        result += to_string(movie.ratingCount);

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleAddRating(Message& request, Message& response) {
    unique_lock<shared_mutex> lock(storageMutex);
    lock_guard<mutex> engineLock(engineMutex);

    try {
        string reqData(request.data);
        vector<string> parts = splitString(reqData, '|');

        if (parts.size() >= 2) {
            int movieID = stoi(parts[0]);
            float rating = stof(parts[1]);

            engine->addRating(request.userID, movieID, rating);
            response.type = SUCCESS;
            copyToBuffer(response.data, "Rating added", sizeof(response.data));
        }
        else {
            throw runtime_error("Invalid data format");
        }
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleGetUserRatings(Message& request, Message& response) {
    lock_guard<mutex> lock(engineMutex);

    try {
        vector<RatingEdge> ratings = engine->getUserRatings(request.userID);

        string result;
        for (const auto& rating : ratings) {
            result += to_string(rating.movieID) + "|";
            result += to_string(rating.getRating()) + "\n";
        }

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleGetAllGenres(Message& request, Message& response) {
    lock_guard<mutex> lock(engineMutex);

    try {
        vector<string> genres = engine->getAllGenres();

        string result;
        for (const auto& genre : genres) {
            result += genre + "\n";
        }

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleGetMoviesByGenre(Message& request, Message& response) {
    lock_guard<mutex> lock(engineMutex);

    try {
        string genre(request.data);
        vector<uint32_t> movieIDs = engine->getMoviesByGenre(genre);

        string result;
        for (size_t i = 0; i < movieIDs.size() && i < 100; i++) {
            result += to_string(movieIDs[i]) + "\n";
        }

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handlePopular(Message& request, Message& response) {
    lock_guard<mutex> lock(engineMutex);

    try {
        int topN = 15;
        string reqData(request.data);
        if (!reqData.empty()) {
            topN = stoi(reqData);
        }

        vector<RecommendationResult> popular = engine->recommendPopular(topN);

        string result;
        for (const auto& movie : popular) {
            result += to_string(movie.movieID) + "|";
            result += movie.title + "|";
            result += to_string(movie.avgRating) + "|";
            result += to_string(movie.ratingCount) + "\n";
        }

        response.type = SUCCESS;
        copyToBuffer(response.data, result, sizeof(response.data));
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleChangePassword(Message& request, Message& response) {
    lock_guard<mutex> lock(authMutex);

    try {
        string reqData(request.data);
        vector<string> parts = splitString(reqData, '|');

        if (parts.size() >= 2) {
            string oldPassword = parts[0];
            string newPassword = parts[1];

            auth->changePassword(request.userID, oldPassword, newPassword);
            response.type = SUCCESS;
            copyToBuffer(response.data, "Password changed", sizeof(response.data));
        }
        else {
            throw runtime_error("Invalid data format");
        }
    }
    catch (exception& e) {
        response.type = ERROR;
        copyToBuffer(response.data, e.what(), sizeof(response.data));
    }
}

void handleLogout(Message& request, Message& response) {
    lock_guard<mutex> lock(authMutex);
    auth->logout(request.userID);
    response.type = SUCCESS;
    copyToBuffer(response.data, "Logged out", sizeof(response.data));
}
void handleClient(int clientSocket) {
    cout << "[Server] Client connected (thread: " << this_thread::get_id() << ")" << endl;

    while (running) {
        Message request, response;
        memset(&response, 0, sizeof(Message));

        if (!recvMsg(clientSocket, request)) {
            break;
        }

        switch (request.type) {
        case LOGIN:
            handleLogin(request, response);
            break;
        case REGISTER:
            handleRegister(request, response);
            break;
        case GET_RECOMMENDATIONS:
            handleRecommendations(request, response);
            break;
        case GET_COLD_START:
            handleColdStart(request, response);
            break;
        case SEARCH_MOVIES:
            handleSearch(request, response);
            break;
        case GET_MOVIE_DETAILS:
            handleGetMovieDetails(request, response);
            break;
        case ADD_RATING:
            handleAddRating(request, response);
            break;
        case GET_USER_RATINGS:
            handleGetUserRatings(request, response);
            break;
        case GET_ALL_GENRES:
            handleGetAllGenres(request, response);
            break;
        case GET_MOVIES_BY_GENRE:
            handleGetMoviesByGenre(request, response);
            break;
        case GET_POPULAR:
            handlePopular(request, response);
            break;
        case CHANGE_PASSWORD:
            handleChangePassword(request, response);
            break;
        case LOGOUT:
            handleLogout(request, response);
            break;
        default:
            response.type = ERROR;
            copyToBuffer(response.data, "Unknown request", sizeof(response.data));
        }

        if (!sendMsg(clientSocket, response)) {
            break;
        }
    }

    close(clientSocket);
    cout << "[Server] Client disconnected (thread: " << this_thread::get_id() << ")" << endl;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    int port = 8080;
    bool loadData = false;
    string dataPath = "../ml-100k";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = stoi(argv[++i]);
        }
        else if (arg == "--load" && i + 1 < argc) {
            loadData = true;
            dataPath = argv[++i];
        }
    }

    cout << "========================================" << endl;
    cout << " Movie Recommendation Server" << endl;
    cout << "========================================" << endl;

    cout << "[Server] Initializing..." << endl;
    engine = new RecommendationEngine();
    auth = new AuthManager();

    if (loadData) {
        cout << "[Server] Loading dataset from: " << dataPath << endl;
        MovieLensParser parser(engine, auth, dataPath);
        if (!parser.parseAll()) {
            cerr << "[Server] Failed to load dataset" << endl;
            cleanup();
            return 1;
        }
    }
    else {
        engine->printStats();
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "[Server] Failed to create socket" << endl;
        cleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "[Server] Failed to bind to port " << port << endl;
        cleanup();
        return 1;
    }

    if (listen(serverSocket, 50) < 0) {
        cerr << "[Server] Failed to listen" << endl;
        cleanup();
        return 1;
    }

    cout << "[Server] Listening on port " << port << endl;
    cout << "[Server] Multi-threaded mode - supports concurrent clients" << endl;
    cout << "[Server] Press Ctrl+C to stop" << endl;

    vector<thread*> threads;

    while (running) {
        struct sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            if (running) {
                cerr << "[Server] Accept failed" << endl;
            }
            continue;
        }

        thread(handleClient, clientSocket).detach();
    }

    cleanup();
    return 0;

}