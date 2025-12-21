# Custom C++ Movie Recommendation Engine

A high-performance, file-based movie recommendation system built from scratch in C++. This project demonstrates advanced usage of **custom data structures** (B-Trees, Hash Tables) and **disk-based storage management** to create a fully functional database and recommendation engine without relying on external SQL/NoSQL libraries.

## 🚀 Key Features

* **Custom Database Engine**: Implements a disk-based **B-Tree (Order 64)** for efficient indexing and retrieval of records.
* **Hybrid Recommendation Algorithm**: Uses a weighted scoring system combining **Genre Affinity**, **Movie Quality (Avg Rating)**, and **Global Popularity** to generate personalized suggestions.
* **Optimized Storage**:
    * **Fixed-Block Storage**: Manages binary files (`.dat`) with custom serialization for Users and Movies.
    * **Edge File Manager**: efficient handling of edge lists (user ratings) stored as binary edge files.
* **Memory-Safe Parser**: Includes a **Phased Batch Loader** capable of processing massive datasets (MovieLens) by flushing data to disk in controlled chunks to prevent RAM exhaustion.
* **Custom Data Structures**:
    * **B-Tree**: Handles primary keys and disk offsets.
    * **Hash Table**: Custom collision-handling implementation for fast in-memory lookups.
    * **Priority Queue (Min-Heap)**: Used for ranking top-N recommendations efficiently.
* **Authentication System**: Complete user registration, login, and session management system with password hashing.

## 🛠️ Architecture

The system is modularized into several core components:

* **`GraphDatabase`**: The central hub that manages `BTree` indices and `FixedStorage` for node data.
* **`RecommendationEngine`**: The logic core. It builds `UserProfiles` dynamically based on rating history and calculates similarity scores.
* **`AuthManager`**: Handles user security, maintaining a separate B-Tree index for credential management.
* **`MovieLensParser`**: A robust tool that parses raw dataset files and populates the custom database.

## 📦 Installation & Setup

### Prerequisites
* C++ Compiler (GCC, Clang, or MSVC) supporting C++11 or higher.
* **MovieLens 100k Dataset**: You can download it [here](https://grouplens.org/datasets/movielens/100k/).

### Build Instructions

## 📂 File Structure

* **`btree.h`**: Template implementation of the B-Tree index.
* **`graph_database.h`**: Facade for managing indices and storage.
* **`recommendation_engine.h`**: Core algorithms for scoring and ranking.
* **`parser.h`**: High-performance parser with batch processing.
* **`storage_manager.h`**: Low-level binary file I/O operations.
* **`hash_table.h`**: In-memory dictionary implementation.
* **`auth_manager.h`**: User security and session handling.
* **`types.h`**: Global constants and configuration.

## ⚡ Performance Optimizations

1.  **Phased Batch Loading**: The parser processes ratings in chunks (e.g., 20k records) and performs "Append-Update" operations. This ensures memory usage stays flat ($O(1)$) regardless of dataset size.
2.  **Inverted Indices**: The system maintains in-memory Hash Tables for `Genre -> [MovieIDs]` mapping, allowing $O(1)$ lookup for candidate generation.
3.  **Binary Search**: Used within B-Tree nodes to quickly locate keys during traversal.
---
