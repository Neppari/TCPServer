#include "protocol.h"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

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

static bool sendRaw(int fd, const void* data, size_t len) {
    return send(fd, data, len, MSG_NOSIGNAL) > 0;
}

static void header(int n, const std::string& name) {
    std::cout << "\n===== Test " << n << ": " << name << " =====\n";
}

static void result(const json& r) {
    if (r.is_null()) std::cout << "<-- [no response / connection closed]\n";
    else             std::cout << "<-- " << r.dump(2) << "\n";
}

int main() {
    std::cout << "=== Malicious Client ===\n";
    int testNum = 0;

    // ── 1. Valid login + event (baseline) ────────────────────────
    header(++testNum, "Valid login + event (baseline)");

    int sock = connectToServer();
    if (sock < 0) { std::cerr << "Connection failed\n"; return 1; }

    sendMessage(sock, buildLogin("bob"));
    auto resp = readMessage(sock);
    result(resp);

    std::string token;
    uint32_t seq = 1;
    if (resp.contains("token")) {
        token = resp["token"];

        sendMessage(sock, buildGameEvent(token, seq++, 100, 200, 1));
        resp = readMessage(sock);
        std::cout << "Valid event: "; result(resp);
    }

    // ── 2. Oversized packet (10 KB) ─────────────────────────────
    header(++testNum, "Oversized packet (10 KB)");
    {
        int s = connectToServer();
        std::string big(10240, 'X');
        uint32_t len = htonl(static_cast<uint32_t>(big.size()));
        sendRaw(s, &len, sizeof(len));
        sendRaw(s, big.data(), big.size());
        resp = readMessage(s);
        result(resp);
        close(s);
    }

    // ── 3. Malformed JSON ────────────────────────────────────────
    header(++testNum, "Malformed JSON");
    {
        int s = connectToServer();
        std::string garbage = R"({"type": broken json!!!)";
        uint32_t len = htonl(static_cast<uint32_t>(garbage.size()));
        sendRaw(s, &len, sizeof(len));
        sendRaw(s, garbage.data(), garbage.size());
        resp = readMessage(s);
        result(resp);
        close(s);
    }

    // ── 4. SQL injection in username ─────────────────────────────
    header(++testNum, "SQL injection in username");
    {
        int s = connectToServer();
        sendMessage(s, buildLogin("alice_DROP_TABLE"));
        resp = readMessage(s);
        result(resp);
        close(s);

        // Also try with special chars (should be rejected by validation)
        s = connectToServer();
        json injectionMsg = {{"type", "LOGIN"}, {"username", "alice'; DROP TABLE scores;--"}};
        sendMessage(s, injectionMsg);
        resp = readMessage(s);
        std::cout << "With special chars: "; result(resp);
        close(s);
    }

    // ── 5. Inflated values ───────────────────────────────────────
    header(++testNum, "Inflated game values (gold=999999)");
    sendMessage(sock, buildGameEvent(token, seq++, 999999, 200, 1));
    resp = readMessage(sock);
    result(resp);

    // ── 6. Fake score field ──────────────────────────────────────
    header(++testNum, "Fake score field in event");
    {
        json fake = {
            {"type", "GAME_EVENT"}, {"token", token}, {"seq", seq++},
            {"gold", 50}, {"experience", 100}, {"items_found", 1},
            {"score", 999999}
        };
        sendMessage(sock, fake);
        resp = readMessage(sock);
        result(resp);
    }

    // ── 7. Token spoofing ────────────────────────────────────────
    header(++testNum, "Fabricated session token");
    {
        int s = connectToServer();
        sendMessage(s, buildGameEvent("TOTALLY_FAKE_TOKEN", 1, 100, 200, 1));
        resp = readMessage(s);
        result(resp);
        close(s);
    }

    // ── 8. Sequence replay ───────────────────────────────────────
    header(++testNum, "Sequence number replay");
    {
        uint32_t replaySeq = seq;
        sendMessage(sock, buildGameEvent(token, replaySeq, 50, 100, 1));
        resp = readMessage(sock);
        std::cout << "First (seq=" << replaySeq << "):  "; result(resp);

        sendMessage(sock, buildGameEvent(token, replaySeq, 50, 100, 1));
        resp = readMessage(sock);
        std::cout << "Replay (seq=" << replaySeq << "): "; result(resp);

        seq = replaySeq + 1;
    }

    // ── 9. Valid event mid-sequence ──────────────────────────────
    header(++testNum, "Valid event (session still works after attacks?)");
    sendMessage(sock, buildGameEvent(token, seq++, 75, 300, 2));
    resp = readMessage(sock);
    result(resp);

    // ── 10. Rapid-fire ───────────────────────────────────────────
    header(++testNum, "Rapid-fire (100 events)");
    {
        int accepted = 0, rejected = 0;
        for (int i = 0; i < 100; i++) {
            sendMessage(sock, buildGameEvent(token, seq++, 10, 20, 0));
            resp = readMessage(sock);
            if (!resp.is_null() && resp.value("success", false))
                accepted++;
            else
                rejected++;
        }
        std::cout << "Accepted: " << accepted << " | Rejected: " << rejected << "\n";
    }

    // ── 11. Impersonation ────────────────────────────────────────
    header(++testNum, "Impersonation (login as already-connected 'alice')");
    {
        int s = connectToServer();
        sendMessage(s, buildLogin("alice"));
        resp = readMessage(s);
        result(resp);
        close(s);
    }

    // ── 12. Unknown user ─────────────────────────────────────────
    header(++testNum, "Unknown user (not in users.txt)");
    {
        int s = connectToServer();
        sendMessage(s, buildLogin("hacker_mcgee"));
        resp = readMessage(s);
        result(resp);
        close(s);
    }

    // ── 13. Final leaderboard ────────────────────────────────────
    header(++testNum, "Leaderboard (final state)");
    sendMessage(sock, buildLeaderboard(token));
    resp = readMessage(sock);
    result(resp);

    // Cleanup
    sendMessage(sock, buildLogout(token));
    close(sock);

    std::cout << "\n=== All " << testNum << " tests complete ===\n";
    return 0;
}
