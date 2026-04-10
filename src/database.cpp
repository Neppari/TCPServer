#include "database.h"
#include "logger.h"

#include <ctime>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>

Database::~Database() { close(); }

bool Database::init(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        Logger::error("DB open failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    chmod(path.c_str(), S_IRUSR | S_IWUSR); // 0600

    const char* sql =
        "CREATE TABLE IF NOT EXISTS scores ("
        "username TEXT PRIMARY KEY, "
        "score INTEGER NOT NULL DEFAULT 0, "
        "updated_at TEXT NOT NULL)";

    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::error("DB table creation failed: " + std::string(err));
        sqlite3_free(err);
        return false;
    }

    Logger::info("Database initialized: " + path);
    return true;
}

void Database::close() {
    if (db) { sqlite3_close(db); db = nullptr; }
}

static std::string now() {
    auto t = std::time(nullptr);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

bool Database::updateScore(const std::string& username, int score) {
    std::lock_guard<std::mutex> lock(mtx);

    const char* sql =
        "INSERT INTO scores (username, score, updated_at) VALUES (?, ?, ?) "
        "ON CONFLICT(username) DO UPDATE SET score = ?, updated_at = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::error("DB prepare failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    auto ts = now();
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, score);
    sqlite3_bind_text(stmt, 3, ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, score);
    sqlite3_bind_text(stmt, 5, ts.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!ok) Logger::error("DB write failed: " + std::string(sqlite3_errmsg(db)));
    return ok;
}

int Database::getScore(const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);

    const char* sql = "SELECT score FROM scores WHERE username = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    int score = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        score = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return score;
}

std::vector<ScoreEntry> Database::getLeaderboard(int limit) {
    std::lock_guard<std::mutex> lock(mtx);

    const char* sql = "SELECT username, score, updated_at FROM scores ORDER BY score DESC LIMIT ?";
    sqlite3_stmt* stmt = nullptr;
    std::vector<ScoreEntry> results;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return results;
    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back({
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            sqlite3_column_int(stmt, 1),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2))
        });
    }

    sqlite3_finalize(stmt);
    return results;
}
