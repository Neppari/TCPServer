What can go wrong in the system?

The Client(s) should not be trusted, server should handle all validation
Database should be accessible only through the server

Things to protect/Risks:

1. Scoreboard database (can't inflate scores or delete others' scores)
2. Other "players'" session tokens (can't steal session or impersonate others)
3. Server process memory (can't crash the server, DoS) ?
4. Server file system (can't read/write random files)
5. Game state (cheats etc) ?

How to mitigate:

1. Enforce unique player names server-side, don't trust client-supplied identity
2. Session tokens expire
3. Each packet includes an increasing sequence number, reject out of order packets
4. Server calculates game outcomes, client reported values are ignored
5. Packet fields are bound checked
6. Database file permissions set to owner only (600)
7. don't use raw string concatentation
8. Server logs all connections, disconnections and suspicious events with timestamp and client IP address
9. Score submissions should have a timestamp so they can be tracked/audited
10. Error responses to clients only contain generic error codes, details go to logs only
11. Session tokens generated with cryptographically secure random number generator (libsodium?)
12. enforce max connection limit to avoid DoS
13. Packets should have a max size, enforce it and reject larger ones


Threats that are out of scope (for now):

1. Packet interception
2. No DoS handling from multiple IPs at the same time
3. Database encryption (might implement)
4. 