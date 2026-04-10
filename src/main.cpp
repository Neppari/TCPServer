#include "network.h"
#include "auth.h"
#include "database.h"
#include "game.h"
#include "logger.h"

#include <atomic>
#include <csignal>

static std::atomic<bool> running{true};
static void onSignal(int) { running = false; }

int main() {
    Logger::info("Starting server");

    SessionManager auth;
    if (!auth.loadUsers("users.txt")) {
        Logger::error("Failed to load users.txt — is it in the working directory?");
        return 1;
    }

    Database db;
    if (!db.init("game.db")) {
        Logger::error("Failed to initialize database");
        return 1;
    }

    GameEngine game(db);
    Network server(8080, auth, game, db);

    std::signal(SIGINT, onSignal);

    if (!server.start()) return 1;

    Logger::info("Press Ctrl+C to stop");
    server.run(running);
    server.shutdown();

    Logger::info("Exited cleanly");
    return 0;
}
