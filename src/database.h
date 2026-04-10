#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <sqlite3.h>

struct ScoreEntry {
    std::string username;
    int score;
    std::string timestamp;
};

class Database {
public:
    ~Database();

    bool init(const std::string& path);
    void close();

    bool updateScore(const std::string& username, int score);
    int getScore(const std::string& username);
    std::vector<ScoreEntry> getLeaderboard(int limit = 10);

private:
    sqlite3* db = nullptr;
    std::mutex mtx;
};
