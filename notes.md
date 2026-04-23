# Notes

## Project Overview

A C++17 TCP game server built for a Secure Programming course. Clients connect on port 8080 and talk JSON (length-prefixed). There's a SQLite leaderboard, login via an allowlist (`users.txt`), session tokens, and sequenced game events. The server spawns a thread per client (up to 10) and pipes everything through auth, game logic, and the DB. We also have two test clients that share the same protocol code.

---

## User List & Auth

- `users.txt` is just a flat list of allowed usernames (alice, bob, charlie, dave, eve) — loaded once at startup
- On login the server checks if the name is in the allowlist, and that nobody else is already logged in with it
- If it passes, the server generates a cryptographically secure random 64-char hex token (256 bits via libsodium) and hands it back — every request after that uses the token
- Each session tracks a monotonically increasing sequence number, so replayed events get rejected
- Sessions time out after 120s of inactivity, and there's a `cleanExpired()` sweep that prunes stale ones
- Everything is mutex-locked since multiple client threads hit the same session map

## Leaderboard

- Backed by SQLite (`game.db`), just one table: `scores(username, score, updated_at)`
- `updateScore` does an upsert — inserts a new row or overwrites the existing score for that user
- `getLeaderboard` pulls the top 10 by score, sorted descending
- All DB access is behind a mutex so concurrent threads don't step on each other
- The DB file gets `chmod 0600` on creation so only the owner can read/write it

---

## Attack Types in `malicious_client.cpp`

1. **Oversized packet** — sends a 10 KB payload to test message-size limits
2. **Malformed JSON** — sends broken JSON to test parser robustness
3. **SQL injection** — injects SQL via the username field (e.g. `'; DROP TABLE scores;--`)
4. **Inflated values** — sends absurdly high game values (gold=999999)
5. **Fake score field** — injects an extra `score` field directly into a game event
6. **Token spoofing** — uses a fabricated session token to send events
7. **Sequence replay** — re-sends the same sequence number to test replay protection
8. **Rapid-fire** — floods 100 events in a tight loop to test rate limiting
9. **Impersonation** — logs in as an already-connected user
10. **Unknown user** — attempts login with a username not in `users.txt`

---

## Mitigation Status

### Done

- Unique player names enforced — server rejects a login if that username already has an active session
- Session tokens expire after 120s of inactivity
- Monotonic sequence numbers — replayed or out-of-order events get rejected
- Server computes scores itself — clients send gold/xp/items but can't set the score directly
- Bounds checking on event fields (gold, xp, items all capped)
- Username validation — alphanumeric + underscore only, so SQL injection via the name field is dead
- Prepared statements everywhere — no raw string concatenation in SQL
- DB file is `chmod 0600` on creation
- Logging with timestamps and client IP for connects, disconnects, and suspicious stuff
- Score updates store `updated_at` for auditing
- Error responses only carry a generic code — details stay in server logs
- Max connection limit (10 clients), returns 503 if full
- Max packet size (4096 bytes) enforced at the protocol layer
- Token generation uses libsodium's CSPRNG (`randombytes_buf`) — 256-bit tokens, replacing the old `mt19937_64`

### Still needs doing
- **`cleanExpired()` is never actually called** — the function exists but nothing triggers it periodically, so expired sessions just sit there until the token happens to be checked
- **No rate limiting** — the rapid-fire test blasts 100 events and the server happily processes all of them
- **No per-IP connection throttling** — a single IP can reconnect over and over

### Out of scope (for now)

- TLS / packet encryption
- Multi-IP distributed DoS
- Database encryption at rest
