#include "network.h"
#include "protocol.h"
#include "auth.h"
#include "game.h"
#include "database.h"
#include "logger.h"

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <cstring>
#include <algorithm>

Network::Network(int port, SessionManager& auth, GameEngine& game, Database& db)
    : port(port), auth(auth), game(game), db(db) {}

Network::~Network() { shutdown(); }

bool Network::start() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        Logger::error("socket() failed");
        return false;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        Logger::error("bind() failed on port " + std::to_string(port));
        return false;
    }

    if (listen(serverFd, MAX_CLIENTS) < 0) {
        Logger::error("listen() failed");
        return false;
    }

    Logger::info("Listening on port " + std::to_string(port));
    return true;
}

void Network::run(std::atomic<bool>& running) {
    struct pollfd pfd{serverFd, POLLIN, 0};

    while (running) {
        int ret = poll(&pfd, 1, 1000); // 1s timeout so we can check the flag
        if (!running) break;
        if (ret <= 0) continue;

        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);

        if (clientFd < 0) continue;

        std::string clientIp = inet_ntoa(clientAddr.sin_addr);

        if (clientCount >= MAX_CLIENTS) {
            Logger::warn("Rejected: max clients (" + std::to_string(MAX_CLIENTS) + ")", clientIp);
            sendMessage(clientFd, buildError(503));
            close(clientFd);
            continue;
        }

        clientCount++;
        Logger::info("Connected (" + std::to_string(clientCount.load()) + "/" + std::to_string(MAX_CLIENTS) + ")", clientIp);

        {
            std::lock_guard<std::mutex> lock(fdsMtx);
            clientFds.push_back(clientFd);
        }

        std::lock_guard<std::mutex> lock(threadsMtx);
        threads.emplace_back(&Network::handleClient, this, clientFd, clientIp);
    }
}

void Network::handleClient(int clientFd, std::string clientIp) {
    std::string sessionToken;

    auto cleanup = [&]() {
        if (!sessionToken.empty()) {
            auto username = auth.getUsernameForToken(sessionToken);
            if (!username.empty()) game.removePlayer(username);
            auth.destroySession(sessionToken);
        }
        close(clientFd);
        {
            std::lock_guard<std::mutex> lock(fdsMtx);
            clientFds.erase(std::remove(clientFds.begin(), clientFds.end(), clientFd), clientFds.end());
        }
        clientCount--;
        Logger::info("Disconnected (" + std::to_string(clientCount.load()) + "/" + std::to_string(MAX_CLIENTS) + ")", clientIp);
    };

    while (true) {
        json msg = readMessage(clientFd);
        if (msg.is_null()) { cleanup(); return; }

        if (!msg.contains("type") || !msg["type"].is_string()) {
            Logger::warn("Message missing 'type'", clientIp);
            sendMessage(clientFd, buildError(400));
            continue;
        }

        std::string type = msg["type"];

        // ── LOGIN ────────────────────────────────────
        if (type == "LOGIN") {
            auto err = validateLogin(msg);
            if (!err.empty()) {
                Logger::warn("Bad LOGIN: " + err, clientIp);
                sendMessage(clientFd, buildError(400));
                continue;
            }

            std::string username = msg["username"];

            if (!auth.isAllowedUser(username)) {
                Logger::warn("Unknown user: " + username, clientIp);
                sendMessage(clientFd, buildError(403));
                continue;
            }

            auto token = auth.createSession(username);
            if (token.empty()) {
                Logger::warn("Duplicate login: " + username, clientIp);
                sendMessage(clientFd, buildError(409));
                continue;
            }

            sessionToken = token;
            game.addPlayer(username);
            int score = game.getPlayerState(username).score;

            Logger::info("Login: " + username, clientIp);
            sendMessage(clientFd, buildLoginOk(token, score));
        }

        // ── GAME_EVENT ───────────────────────────────
        else if (type == "GAME_EVENT") {
            auto err = validateGameEvent(msg);
            if (!err.empty()) {
                Logger::warn("Bad GAME_EVENT: " + err, clientIp);
                sendMessage(clientFd, buildError(400));
                continue;
            }

            std::string token = msg["token"];
            if (!auth.validateToken(token)) {
                Logger::warn("Invalid token on GAME_EVENT", clientIp);
                sendMessage(clientFd, buildError(401));
                continue;
            }

            uint32_t seq = msg["seq"];
            if (!auth.validateSeq(token, seq)) {
                Logger::warn("Bad seq " + std::to_string(seq), clientIp);
                sendMessage(clientFd, buildError(400));
                continue;
            }

            auto username = auth.getUsernameForToken(token);
            EventData event{msg["gold"], msg["experience"], msg["items_found"]};
            auto result = game.processEvent(username, event);

            if (!result.success)
                Logger::warn("Event rejected (" + username + "): " + result.description, clientIp);

            sendMessage(clientFd, buildEventResult(result.success, result.description, result.score));
        }

        // ── GET_STATE ────────────────────────────────
        else if (type == "GET_STATE") {
            auto err = validateTokenMessage(msg);
            if (!err.empty()) { sendMessage(clientFd, buildError(400)); continue; }

            std::string token = msg["token"];
            if (!auth.validateToken(token)) { sendMessage(clientFd, buildError(401)); continue; }

            auto username = auth.getUsernameForToken(token);
            auto state = game.getPlayerState(username);
            sendMessage(clientFd, buildState(state.score, state.totalGold, state.totalXp, state.totalItems));
        }

        // ── LEADERBOARD ──────────────────────────────
        else if (type == "LEADERBOARD") {
            auto err = validateTokenMessage(msg);
            if (!err.empty()) { sendMessage(clientFd, buildError(400)); continue; }

            std::string token = msg["token"];
            if (!auth.validateToken(token)) { sendMessage(clientFd, buildError(401)); continue; }

            auto entries = db.getLeaderboard(10);
            json scores = json::array();
            for (const auto& e : entries)
                scores.push_back({{"username", e.username}, {"score", e.score}});

            sendMessage(clientFd, buildLeaderboardResult(scores));
        }

        // ── LOGOUT ───────────────────────────────────
        else if (type == "LOGOUT") {
            auto err = validateTokenMessage(msg);
            if (!err.empty()) { sendMessage(clientFd, buildError(400)); continue; }

            auto username = auth.getUsernameForToken(msg["token"]);
            if (!username.empty()) {
                game.removePlayer(username);
                auth.destroySession(msg["token"]);
                Logger::info("Logout: " + username, clientIp);
            }

            sessionToken.clear();
            cleanup();
            return;
        }

        // ── Unknown type ─────────────────────────────
        else {
            Logger::warn("Unknown type: " + type, clientIp);
            sendMessage(clientFd, buildError(400));
        }
    }
}

void Network::shutdown() {
    if (serverFd >= 0) {
        ::shutdown(serverFd, SHUT_RDWR);
        close(serverFd);
        serverFd = -1;
    }

    // Unblock all client recv() calls
    {
        std::lock_guard<std::mutex> lock(fdsMtx);
        for (int fd : clientFds)
            ::shutdown(fd, SHUT_RDWR);
    }

    // Wait for all client threads to finish
    {
        std::lock_guard<std::mutex> lock(threadsMtx);
        for (auto& t : threads)
            if (t.joinable()) t.join();
        threads.clear();
    }

    Logger::info("Server shut down");
}
