# WIP — Security Checklist

## Secure Practices In Use

- [x] Authoritative server — scores computed server-side, client values ignored
- [x] Parameterized SQL queries only — no string concatenation
- [x] Input validation — JSON fields bounds-checked (gold 0-500, xp 0-1000, items 0-10)
- [x] Username validation — alphanumeric + underscore only, length capped
- [x] Max packet size enforced (4096 bytes)
- [x] Sequence numbers per session — reject out-of-order or replayed packets
- [x] File-based user allowlist (users.txt) — reject unknown users
- [x] Unique player names enforced — reject duplicate concurrent logins
- [x] Session token expiry after 2 minutes of inactivity
- [x] Session tokens generated with std::random_device (CSPRNG)
- [x] Max concurrent connection limit (10 threads)
- [x] SQLite file permissions set to 0600 (owner-only)
- [x] Generic error codes to clients — internal details logged server-side only
- [x] Timestamped structured logging with client IP
- [x] Score rows include timestamps for audit trails
- [x] Thread-safe shared state (mutex on sessions, game state, database, logger)
- [x] Graceful shutdown — SIGINT handler, thread join, socket cleanup
- [x] SIGPIPE protection — MSG_NOSIGNAL on all sends

## Possible Future Security Features

- [ ] Encrypted transport — libsodium crypto_kx key exchange + crypto_secretbox (XSalsa20-Poly1305)
- [ ] CSPRNG upgrade — replace std::random_device with libsodium randombytes_buf
- [ ] Per-IP rate limiting / connection throttling
- [ ] Database encryption at rest (sqlcipher)
- [ ] Account lockout after repeated failed login attempts
- [ ] Nonce tracking to reject replayed encrypted packets
- [ ] Password hashing for user authentication (argon2id)
- [ ] TLS transport layer (defense in depth)
- [ ] IP allowlist / blocklist
- [ ] Log rotation and remote log shipping
- [ ] HMAC-signed leaderboard exports
