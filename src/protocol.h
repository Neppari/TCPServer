#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

using json = nlohmann::json;

constexpr uint32_t MAX_PAYLOAD_SIZE = 4096;
constexpr int MAX_USERNAME_LENGTH = 32;

// Wire format: [4 bytes length (network order)][JSON payload]
bool sendMessage(int fd, const json& msg);
json readMessage(int fd); // returns nullptr on error/disconnect

// --- Client -> Server builders ---
json buildLogin(const std::string& username);
json buildGameEvent(const std::string& token, uint32_t seq, int gold, int xp, int items);
json buildGetState(const std::string& token);
json buildLeaderboard(const std::string& token);
json buildLogout(const std::string& token);

// --- Server -> Client builders ---
json buildLoginOk(const std::string& token, int score);
json buildEventResult(bool success, const std::string& desc, int score);
json buildState(int score, int totalGold, int totalXp, int totalItems);
json buildLeaderboardResult(const json& scores);
json buildError(int code);

// --- Validators (return error string, empty = valid) ---
std::string validateLogin(const json& msg);
std::string validateGameEvent(const json& msg);
std::string validateTokenMessage(const json& msg);
