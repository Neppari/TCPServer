#include "game.h"
#include "database.h"
#include "logger.h"

GameEngine::GameEngine(Database& db) : db(db) {}

void GameEngine::addPlayer(const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);

    PlayerState state;
    state.score = db.getScore(username);
    players[username] = state;

    Logger::info("Player joined: " + username + " (score: " + std::to_string(state.score) + ")");
}

void GameEngine::removePlayer(const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);
    players.erase(username);
    Logger::info("Player left: " + username);
}

int GameEngine::computeScore(const EventData& e) {
    return e.gold * 1 + e.experience * 2 + e.itemsFound * 50;
}

EventResult GameEngine::processEvent(const std::string& username, const EventData& event) {
    if (event.gold < 0 || event.gold > MAX_GOLD)
        return {false, "gold out of range (0-" + std::to_string(MAX_GOLD) + ")", 0};
    if (event.experience < 0 || event.experience > MAX_XP)
        return {false, "experience out of range (0-" + std::to_string(MAX_XP) + ")", 0};
    if (event.itemsFound < 0 || event.itemsFound > MAX_ITEMS)
        return {false, "items out of range (0-" + std::to_string(MAX_ITEMS) + ")", 0};

    int gained = computeScore(event);

    std::lock_guard<std::mutex> lock(mtx);
    auto it = players.find(username);
    if (it == players.end())
        return {false, "player not in game", 0};

    it->second.totalGold  += event.gold;
    it->second.totalXp    += event.experience;
    it->second.totalItems += event.itemsFound;
    it->second.score      += gained;

    db.updateScore(username, it->second.score);

    return {true, "accepted (+" + std::to_string(gained) + " pts)", it->second.score};
}

PlayerState GameEngine::getPlayerState(const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = players.find(username);
    return (it != players.end()) ? it->second : PlayerState{};
}
