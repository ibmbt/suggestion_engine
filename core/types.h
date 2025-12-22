#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cstddef>

enum class ErrorCode {
    SUCCESS = 0,

    // Auth Errors (100-199)
    AUTH_INVALID_CREDENTIALS = 100,
    AUTH_USERNAME_EXISTS = 101,
    AUTH_WEAK_PASSWORD = 102,
    AUTH_SESSION_EXPIRED = 103,

    // User Errors (200-299)
    USER_NOT_FOUND = 200,
    USER_INVALID_ID = 201,

    // Movie Errors (300-399)
    MOVIE_NOT_FOUND = 300,
    MOVIE_INVALID_TITLE = 301,
    MOVIE_INVALID_GENRE = 302,

    // Rating Errors (400-499)
    RATING_OUT_OF_RANGE = 400,
    RATING_DUPLICATE = 401,

    // Storage Errors (500-599)
    STORAGE_READ_FAILED = 500,
    STORAGE_WRITE_FAILED = 501,
    STORAGE_DISK_FULL = 502,
    STORAGE_CORRUPTED = 503,

    // System Errors (900-999)
    SYSTEM_MUTEX_TIMEOUT = 900,
    SYSTEM_OUT_OF_MEMORY = 901,
    SYSTEM_UNKNOWN = 999
};

const size_t BLOCK_SIZE = 4096;
const size_t BTREE_DEGREE = 64;
const size_t HASH_TABLE_SIZE = 1009;
const float MIN_RATING = 1.0f;
const float MAX_RATING = 5.0f;
const uint64_t METADATA_SIZE = 16;

const size_t MAX_USERNAME_LENGTH = 64;
const size_t MAX_TITLE_LENGTH = 128;
const size_t MAX_GENRES = 5;
const size_t MAX_GENRE_LENGTH = 32;

const size_t USER_NODE_SIZE = 4 + MAX_USERNAME_LENGTH + 4 + 4 + 8;
const size_t MOVIE_NODE_SIZE = 4 + MAX_TITLE_LENGTH + (MAX_GENRES * MAX_GENRE_LENGTH) + 4 + 4 + 4 + 8;

const size_t USERS_PER_BLOCK = BLOCK_SIZE / USER_NODE_SIZE;
const size_t MOVIES_PER_BLOCK = BLOCK_SIZE / MOVIE_NODE_SIZE;

static const uint32_t MAX_SLOTS = 1000000;

const int BATCH_SIZE = 20000;


#endif