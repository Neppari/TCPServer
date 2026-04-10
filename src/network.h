#pragma once

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <string>

class SessionManager;
class GameEngine;
class Database;

class Network {
public:
    Network(int port, SessionManager& auth, GameEngine& game, Database& db);
    ~Network();

    bool start();
    void run(std::atomic<bool>& running);
    void shutdown();

private:
    static constexpr int MAX_CLIENTS = 10;

    void handleClient(int clientFd, std::string clientIp);

    int serverFd = -1;
    int port;
    std::atomic<int> clientCount{0};

    SessionManager& auth;
    GameEngine& game;
    Database& db;

    std::mutex threadsMtx;
    std::vector<std::thread> threads;

    std::mutex fdsMtx;
    std::vector<int> clientFds;
};
