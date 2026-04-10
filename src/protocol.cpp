#include "protocol.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <vector>

// ── Framing helpers ──────────────────────────────────────────────

static bool sendAll(int fd, const void* data, size_t len) {
    auto ptr = static_cast<const char*>(data);
    while (len > 0) {
        ssize_t n = send(fd, ptr, len, MSG_NOSIGNAL);
        if (n <= 0) return false;
        ptr += n;
        len -= n;
    }
    return true;
}

static bool recvAll(int fd, void* data, size_t len) {
    auto ptr = static_cast<char*>(data);
    while (len > 0) {
        ssize_t n = recv(fd, ptr, len, 0);
        if (n <= 0) return false;
        ptr += n;
        len -= n;
    }
    return true;
}

bool sendMessage(int fd, const json& msg) {
    std::string payload = msg.dump();
    if (payload.size() > MAX_PAYLOAD_SIZE) return false;

    uint32_t netLen = htonl(static_cast<uint32_t>(payload.size()));
    if (!sendAll(fd, &netLen, sizeof(netLen))) return false;
    return sendAll(fd, payload.data(), payload.size());
}

json readMessage(int fd) {
    uint32_t netLen = 0;
    if (!recvAll(fd, &netLen, sizeof(netLen))) return nullptr;

    uint32_t len = ntohl(netLen);
    if (len == 0 || len > MAX_PAYLOAD_SIZE) return nullptr;

    std::vector<char> buf(len);
    if (!recvAll(fd, buf.data(), len)) return nullptr;

    try {
        return json::parse(buf.begin(), buf.end());
    } catch (...) {
        return nullptr;
    }
}

// ── Client -> Server builders ────────────────────────────────────

json buildLogin(const std::string& username) {
    return {{"type", "LOGIN"}, {"username", username}};
}

json buildGameEvent(const std::string& token, uint32_t seq, int gold, int xp, int items) {
    return {
        {"type", "GAME_EVENT"}, {"token", token}, {"seq", seq},
        {"gold", gold}, {"experience", xp}, {"items_found", items}
    };
}

json buildGetState(const std::string& token) {
    return {{"type", "GET_STATE"}, {"token", token}};
}

json buildLeaderboard(const std::string& token) {
    return {{"type", "LEADERBOARD"}, {"token", token}};
}

json buildLogout(const std::string& token) {
    return {{"type", "LOGOUT"}, {"token", token}};
}

// ── Server -> Client builders ────────────────────────────────────

json buildLoginOk(const std::string& token, int score) {
    return {{"type", "LOGIN_OK"}, {"token", token}, {"state", {{"score", score}}}};
}

json buildEventResult(bool success, const std::string& desc, int score) {
    return {{"type", "EVENT_RESULT"}, {"success", success}, {"description", desc}, {"score", score}};
}

json buildState(int score, int totalGold, int totalXp, int totalItems) {
    return {
        {"type", "STATE"}, {"score", score},
        {"total_gold", totalGold}, {"total_xp", totalXp}, {"total_items", totalItems}
    };
}

json buildLeaderboardResult(const json& scores) {
    return {{"type", "LEADERBOARD_RESULT"}, {"scores", scores}};
}

json buildError(int code) {
    return {{"type", "ERROR"}, {"code", code}};
}

// ── Validators ───────────────────────────────────────────────────

std::string validateLogin(const json& msg) {
    if (!msg.contains("username") || !msg["username"].is_string())
        return "missing/invalid 'username'";

    std::string name = msg["username"];
    if (name.empty() || static_cast<int>(name.size()) > MAX_USERNAME_LENGTH)
        return "username length must be 1-" + std::to_string(MAX_USERNAME_LENGTH);

    for (char c : name) {
        if (!std::isalnum(c) && c != '_')
            return "username has invalid characters (alphanumeric and _ only)";
    }
    return "";
}

std::string validateGameEvent(const json& msg) {
    if (!msg.contains("token") || !msg["token"].is_string()) return "missing 'token'";
    if (!msg.contains("seq") || !msg["seq"].is_number_unsigned()) return "missing 'seq'";
    if (!msg.contains("gold") || !msg["gold"].is_number_integer()) return "missing 'gold'";
    if (!msg.contains("experience") || !msg["experience"].is_number_integer()) return "missing 'experience'";
    if (!msg.contains("items_found") || !msg["items_found"].is_number_integer()) return "missing 'items_found'";
    return "";
}

std::string validateTokenMessage(const json& msg) {
    if (!msg.contains("token") || !msg["token"].is_string()) return "missing 'token'";
    return "";
}
