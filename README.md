# TCPServer — Secure Game Server

Authoritative TCP game server with JSON protocol, SQLite leaderboard, and two clients (legitimate + malicious) for a Secure Programming course.

## Prerequisites (WSL/Linux)

```bash
sudo apt update
sudo apt install build-essential cmake libsqlite3-dev
```

## Build

```bash
cmake -S . -B build && cmake --build build
```

## Run

Run from the **project root** (server loads `users.txt` and creates `game.db` in the working directory).

```bash
# Terminal 1 — server
./build/TCPServer

# Terminal 2 — good client
./build/GoodClient

# Terminal 3 — malicious client (run while good client is connected to test threading)
./build/MaliciousClient
```

Press `Ctrl+C` to shut down the server cleanly.

## Project Structure

```
src/
  main.cpp          — entry point, wires components, signal handling
  network.h/cpp     — TCP accept loop, thread-per-client, message dispatch
  protocol.h/cpp    — JSON message framing, builders, validators
  auth.h/cpp        — user allowlist, session tokens, sequence numbers
  database.h/cpp    — SQLite leaderboard (parameterized queries)
  game.h/cpp        — event validation, score computation
  logger.h/cpp      — thread-safe timestamped logging

clients/
  good_client.cpp       — automated legitimate player
  malicious_client.cpp  — sequential attack demo (13 tests)

users.txt           — allowed usernames
```

## Security Features

See [WIP.md](WIP.md) for the full checklist and [threats.md](threats.md) for the threat model.
