#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cstddef>

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



#endif