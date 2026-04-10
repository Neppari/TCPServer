#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

class Database;

struct PlayerState {
    int score = 0;
    int totalGold = 0;
    int totalXp = 0;
    int totalItems = 0;
};

struct EventData {
    int gold = 0;
    int experience = 0;
    int itemsFound = 0;
};

struct EventResult {
    bool success;
    std::string description;
    int score;
};

class GameEngine {
public:
    explicit GameEngine(Database& db);

    void addPlayer(const std::string& username);
    void removePlayer(const std::string& username);
    EventResult processEvent(const std::string& username, const EventData& event);
    PlayerState getPlayerState(const std::string& username);

private:
    static constexpr int MAX_GOLD  = 500;
    static constexpr int MAX_XP    = 1000;
    static constexpr int MAX_ITEMS = 10;

    int computeScore(const EventData& e);

    Database& db;
    std::unordered_map<std::string, PlayerState> players;
    std::mutex mtx;
};
