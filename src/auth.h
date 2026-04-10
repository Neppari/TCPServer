#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <cstdint>

struct Session {
    std::string username;
    std::string token;
    uint32_t nextSeq = 1;
    std::chrono::steady_clock::time_point lastActivity;
};

class SessionManager {
public:
    bool loadUsers(const std::string& filepath);
    bool isAllowedUser(const std::string& username) const;

    std::string createSession(const std::string& username); // empty = failure
    bool validateToken(const std::string& token);
    bool validateSeq(const std::string& token, uint32_t seq);
    std::string getUsernameForToken(const std::string& token);
    void destroySession(const std::string& token);
    void cleanExpired();

private:
    static constexpr int TIMEOUT_SECONDS = 120;

    std::unordered_set<std::string> allowedUsers;           // loaded once at startup
    std::unordered_map<std::string, Session> sessions;      // token -> session
    std::unordered_map<std::string, std::string> activeUsers; // username -> token
    std::mutex mtx;

    std::string generateToken();
};
