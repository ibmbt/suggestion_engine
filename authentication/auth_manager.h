#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include "../core/btree.h"
#include "../core/hash_table.h"
#include <string>
#include <cstring>
#include <stdexcept>
#include <ctime>
#include <random>

using namespace std;

// Simple hash function for passwords using 5 shifts and addition
uint64_t simpleHash(const string& password) {
    uint64_t hash = 5381;
    for (char c : password) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

struct AuthRecord {
    uint32_t userID;
    char username[MAX_USERNAME_LENGTH];
    uint64_t passwordHash;
    uint64_t createdAt;
    uint64_t lastLogin;

    AuthRecord() : userID(0), passwordHash(0), createdAt(0), lastLogin(0) {
        memset(username, 0, MAX_USERNAME_LENGTH);
    }

    void serialize(char* buffer) const {
        size_t offset = 0;
        memcpy(buffer + offset, &userID, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(buffer + offset, username, MAX_USERNAME_LENGTH);
        offset += MAX_USERNAME_LENGTH;
        memcpy(buffer + offset, &passwordHash, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(buffer + offset, &createdAt, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(buffer + offset, &lastLogin, sizeof(uint64_t));
    }

    static AuthRecord deserialize(const char* buffer) {
        AuthRecord record;
        size_t offset = 0;
        memcpy(&record.userID, buffer + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(record.username, buffer + offset, MAX_USERNAME_LENGTH);
        offset += MAX_USERNAME_LENGTH;
        memcpy(&record.passwordHash, buffer + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&record.createdAt, buffer + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        memcpy(&record.lastLogin, buffer + offset, sizeof(uint64_t));
        return record;
    }

    static constexpr size_t getSize() {
        return sizeof(uint32_t) + MAX_USERNAME_LENGTH + sizeof(uint64_t) * 3;
    }
};

struct Session {
    uint32_t userID;
    string username;
    uint64_t loginTime;
    bool isActive;

    Session() : userID(0), loginTime(0), isActive(false) {}
};

class AuthManager {
private:
    BTree<uint32_t, uint64_t>* authIndex;

    HashTable<string, uint32_t>* usernameLookup;

    fstream authFile;
    const string AUTH_FILE = "auth.dat";
    const string AUTH_INDEX_FILE = "auth_index.dat";

    HashTable<uint32_t, Session>* activeSessions;

    uint32_t nextUserID;

    void saveAuthRecord(uint32_t userID, const AuthRecord& record) {
        uint64_t offset;
        if (!authIndex->search(userID, offset)) {
            authFile.seekp(0, ios::end);
            offset = authFile.tellp();
        }

        char buffer[AuthRecord::getSize()];
        record.serialize(buffer);

        authFile.seekp(offset, ios::beg);
        authFile.write(buffer, AuthRecord::getSize());
        authFile.flush();

        if (authFile.fail()) {
            throw runtime_error("Failed to write auth record");
        }

        authIndex->insert(userID, offset);
    }

    AuthRecord loadAuthRecord(uint32_t userID) {
        uint64_t offset;
        if (!authIndex->search(userID, offset)) {
            throw runtime_error("User not found");
        }

        char buffer[AuthRecord::getSize()];
        authFile.seekg(offset, ios::beg);
        authFile.read(buffer, AuthRecord::getSize());

        if (authFile.fail()) {
            throw runtime_error("Failed to read auth record");
        }

        return AuthRecord::deserialize(buffer);
    }

    uint32_t generateUserID() {
        return nextUserID++;
    }

public:
    AuthManager() : nextUserID(1) {
        authIndex = new BTree<uint32_t, uint64_t>(AUTH_INDEX_FILE);
        usernameLookup = new HashTable<string, uint32_t>(1009);
        activeSessions = new HashTable<uint32_t, Session>(1009);

        authFile.open(AUTH_FILE, ios::in | ios::out | ios::binary);
        if (!authFile.is_open()) {
            authFile.open(AUTH_FILE, ios::out | ios::binary);
            authFile.close();
            authFile.open(AUTH_FILE, ios::in | ios::out | ios::binary);
        }

        vector<pair<uint32_t, uint64_t>> allRecords = authIndex->getAllPairs();
        for (const auto& pair : allRecords) {
            try {
                AuthRecord record = loadAuthRecord(pair.first);
                usernameLookup->insert(string(record.username), record.userID);

                if (record.userID >= nextUserID) {
                    nextUserID = record.userID + 1;
                }
            }
            catch (...) {
                continue;
            }
        }
    }

    ~AuthManager() {
        delete authIndex;
        delete usernameLookup;
        delete activeSessions;

        if (authFile.is_open()) {
            authFile.close();
        }
    }

    uint32_t registerUser(const string& username, const string& password) {
        if (username.empty() || username.length() >= MAX_USERNAME_LENGTH) {
            throw runtime_error("Invalid username length");
        }

        if (password.length() < 6) {
            throw runtime_error("Password must be at least 6 characters");
        }

        uint32_t existingID;
        if (usernameLookup->find(username, existingID)) {
            throw runtime_error("Username already exists");
        }

        AuthRecord record;
        record.userID = generateUserID();
        strncpy(record.username, username.c_str(), MAX_USERNAME_LENGTH - 1);
        record.passwordHash = simpleHash(password);
        record.createdAt = static_cast<uint64_t>(time(nullptr));
        record.lastLogin = 0;

        saveAuthRecord(record.userID, record);
        usernameLookup->insert(username, record.userID);

        return record.userID;
    }

    uint32_t login(const string& username, const string& password) {
        uint32_t userID;
        if (!usernameLookup->find(username, userID)) {
            throw runtime_error("Invalid username or password");
        }

        AuthRecord record = loadAuthRecord(userID);

        uint64_t providedHash = simpleHash(password);
        if (record.passwordHash != providedHash) {
            throw runtime_error("Invalid username or password");
        }

        record.lastLogin = static_cast<uint64_t>(time(nullptr));
        saveAuthRecord(userID, record);

        Session session;
        session.userID = userID;
        session.username = username;
        session.loginTime = record.lastLogin;
        session.isActive = true;

        activeSessions->insert(userID, session);

        return userID;
    }

    void logout(uint32_t userID) {
        Session session;
        if (activeSessions->find(userID, session)) {
            session.isActive = false;
            activeSessions->insert(userID, session);
        }
    }

    bool isLoggedIn(uint32_t userID) {
        Session session;
        if (activeSessions->find(userID, session)) {
            return session.isActive;
        }
        return false;
    }

    Session getSession(uint32_t userID) {
        Session session;
        if (!activeSessions->find(userID, session)) {
            throw runtime_error("No active session");
        }
        return session;
    }

    void changePassword(uint32_t userID, const string& oldPassword,
        const string& newPassword) {
        AuthRecord record = loadAuthRecord(userID);

        uint64_t oldHash = simpleHash(oldPassword);
        if (record.passwordHash != oldHash) {
            throw runtime_error("Incorrect current password");
        }

        if (newPassword.length() < 6) {
            throw runtime_error("New password must be at least 6 characters");
        }

        record.passwordHash = simpleHash(newPassword);
        saveAuthRecord(userID, record);
    }

    string getUsername(uint32_t userID) {
        AuthRecord record = loadAuthRecord(userID);
        return string(record.username);
    }

    bool usernameExists(const string& username) {
        uint32_t userID;
        return usernameLookup->find(username, userID);
    }

    uint32_t getUserID(const string& username) {
        uint32_t userID;
        if (!usernameLookup->find(username, userID)) {
            throw runtime_error("Username not found");
        }
        return userID;
    }

    void deleteAccount(uint32_t userID, const string& password) {
        AuthRecord record = loadAuthRecord(userID);

        uint64_t providedHash = simpleHash(password);
        if (record.passwordHash != providedHash) {
            throw runtime_error("Incorrect password");
        }

        usernameLookup->remove(string(record.username));

        logout(userID);

    }
};

#endif