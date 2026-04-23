#include "auth.h"
#include "logger.h"

#include <sodium.h>

#include <fstream>

bool SessionManager::loadUsers(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::error("Cannot open users file: " + filepath);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (line.empty() || line[0] == '#') continue;
        allowedUsers.insert(line);
    }

    Logger::info("Loaded " + std::to_string(allowedUsers.size()) + " allowed users");
    return !allowedUsers.empty();
}

bool SessionManager::isAllowedUser(const std::string& username) const {
    return allowedUsers.count(username) > 0;
}

std::string SessionManager::generateToken() {
    unsigned char buf[32];
    randombytes_buf(buf, sizeof(buf));

    char hex[sizeof(buf) * 2 + 1];
    sodium_bin2hex(hex, sizeof(hex), buf, sizeof(buf));

    return std::string(hex);
}

std::string SessionManager::createSession(const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);

    if (allowedUsers.count(username) == 0) return "";
    if (activeUsers.count(username) > 0) return "";

    auto token = generateToken();
    sessions[token] = {username, token, 1, std::chrono::steady_clock::now()};
    activeUsers[username] = token;

    return token;
}

bool SessionManager::validateToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = sessions.find(token);
    if (it == sessions.end()) return false;

    auto elapsed = std::chrono::steady_clock::now() - it->second.lastActivity;
    if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > TIMEOUT_SECONDS) {
        activeUsers.erase(it->second.username);
        sessions.erase(it);
        return false;
    }

    it->second.lastActivity = std::chrono::steady_clock::now();
    return true;
}

bool SessionManager::validateSeq(const std::string& token, uint32_t seq) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = sessions.find(token);
    if (it == sessions.end()) return false;
    if (seq != it->second.nextSeq) return false;

    it->second.nextSeq++;
    return true;
}

std::string SessionManager::getUsernameForToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = sessions.find(token);
    return (it != sessions.end()) ? it->second.username : "";
}

void SessionManager::destroySession(const std::string& token) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = sessions.find(token);
    if (it != sessions.end()) {
        activeUsers.erase(it->second.username);
        sessions.erase(it);
    }
}

void SessionManager::cleanExpired() {
    std::lock_guard<std::mutex> lock(mtx);
    auto now = std::chrono::steady_clock::now();

    for (auto it = sessions.begin(); it != sessions.end();) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastActivity).count();
        if (age > TIMEOUT_SECONDS) {
            activeUsers.erase(it->second.username);
            it = sessions.erase(it);
        } else {
            ++it;
        }
    }
}
