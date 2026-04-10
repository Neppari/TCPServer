#include "protocol.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static constexpr int  PORT      = 8080;
static constexpr char SERVER[]  = "127.0.0.1";

static int connectToServer() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER, &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

static void print(const std::string& dir, const json& msg) {
    std::cout << dir << " " << msg.dump(2) << "\n\n";
}

int main() {
    std::cout << "=== Good Client ===\n\n";

    int sock = connectToServer();
    if (sock < 0) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }

    // Login
    sendMessage(sock, buildLogin("alice"));
    auto resp = readMessage(sock);
    print("<--", resp);

    if (!resp.contains("token")) {
        std::cerr << "Login failed\n";
        close(sock);
        return 1;
    }

    std::string token = resp["token"];
    uint32_t seq = 1;

    // Scripted game events
    struct { int gold; int xp; int items; const char* label; } events[] = {
        {100, 500, 2,  "Found a treasure chest"},
        {50,  200, 1,  "Completed a quest"},
        {300, 800, 5,  "Defeated a boss"},
        {75,  100, 0,  "Explored a dungeon"},
    };

    for (const auto& e : events) {
        std::cout << "--> GAME_EVENT: " << e.label
                  << " (gold=" << e.gold << " xp=" << e.xp << " items=" << e.items << ")\n";
        sendMessage(sock, buildGameEvent(token, seq++, e.gold, e.xp, e.items));
        resp = readMessage(sock);
        print("<--", resp);
    }

    // Check state
    std::cout << "--> GET_STATE\n";
    sendMessage(sock, buildGetState(token));
    resp = readMessage(sock);
    print("<--", resp);

    // Leaderboard
    std::cout << "--> LEADERBOARD\n";
    sendMessage(sock, buildLeaderboard(token));
    resp = readMessage(sock);
    print("<--", resp);

    // Logout
    std::cout << "--> LOGOUT\n";
    sendMessage(sock, buildLogout(token));

    close(sock);
    std::cout << "Done!\n";
    return 0;
}
