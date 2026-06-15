# WebServ — Project Checklist

High-level tracker. Each module defines **what** it must deliver, not how it implements it.
Implementation choices belong to the person handling the module.

For detailed subject requirements and evaluation criteria, see `docs/subject.md` and `docs/evaluation.md`.

---

## Coordination

| Module | Owner | Status |
|--------|-------|--------|
| Config Parsing | — | 🔴 Not started |
| Core Server (event loop) | — | 🔴 Not started |
| HTTP Layer | — | 🔴 Not started |

**Integration checkpoints** are moments where the three modules must align.
Schedule them together before starting the next phase.

---

## Phase 0 — Shared setup (team, before splitting)

- [ ] Makefile compiles cleanly with `-Wall -Wextra -Werror -std=c++98`, no unnecessary relink
- [ ] `.gitignore` in place
- [ ] Define the **interface contract** between Core Server and HTTP Layer:
  what data the server passes to the HTTP layer, and what it gets back
  *(agree on this together — it's the only shared API boundary)*
- [ ] `README.md` with the mandatory first line

---

## Phase 1 — Module: Config Parsing

**Delivers:** a parsed, validated representation of the configuration file that the rest of the program can query.

- [ ] Read a config file passed as argument, or fall back to a default path
- [ ] Support multiple server blocks with different host:port pairs
- [ ] Support per-route configuration (methods, root dir, index, listing, redirects, upload, CGI)
- [ ] Validate the config — reject invalid configurations with a clear error, no crash
- [ ] Duplicate host:port → refuse to start
- [ ] Provide representative config files for testing (valid, multi-port, CGI, errors, invalid)

---

## Phase 2 — Module: Core Server

**Delivers:** a non-blocking event loop that accepts connections, reads requests, and writes responses — all going through a single poll() call.

- [ ] Single `poll()` (or equivalent) for all I/O: listening sockets, client sockets, CGI pipes
- [ ] `poll()` monitors read AND write at the same time
- [ ] No read/write on any socket without going through poll() first
- [ ] No use of `errno` after read/recv/write/send to adjust behaviour
- [ ] Accept new client connections
- [ ] Read incoming data and detect when a request is complete
- [ ] Delegate complete requests to the HTTP layer (via the agreed interface)
- [ ] Write responses back to clients, handling partial writes
- [ ] Detect and clean up disconnected clients
- [ ] Timeout idle connections
- [ ] Graceful shutdown on SIGINT

---

## Phase 3 — Module: HTTP Layer

**Delivers:** given a raw request buffer and a server config, return a complete HTTP response.

- [ ] Parse the request line, headers, and body
- [ ] Handle `Transfer-Encoding: chunked` (un-chunk before processing)
- [ ] Route the request to the correct location by longest-prefix matching
- [ ] Return 405 if the method is not allowed for the route
- [ ] Handle HTTP redirections
- [ ] GET: serve static files, directory index, or directory listing
- [ ] POST: enforce body size limit, handle file uploads
- [ ] DELETE: remove the target file
- [ ] Unknown/malformed requests → appropriate error response, no crash
- [ ] Accurate HTTP status codes throughout
- [ ] Default error pages for common codes (400, 403, 404, 405, 413, 500)
- [ ] Custom error pages from config when available
- [ ] CGI: execute scripts, pass the correct environment, handle GET and POST, manage timeouts, no crash on faulty scripts
- [ ] CGI runs only via `fork()` — no other use of `fork()`

---

## Integration checkpoint 1 — After phases 1–3

Meet as a team and verify:

- [ ] Server starts with a config file and listens on the expected ports
- [ ] A GET request from a browser returns a valid response
- [ ] Multi-port setup works — different content per port
- [ ] CGI executes correctly end-to-end
- [ ] No direct push to main — everything via PR + pr-review skill

---

## Phase 4 — Robustness & compliance

- [ ] No crash under any circumstance (malformed requests, huge headers, abrupt disconnects)
- [ ] Stress test with siege — availability ≥ 99.5% on a simple GET
- [ ] No memory leaks under sustained load
- [ ] No hanging connections
- [ ] Behaviour compared with NGINX on key cases (headers, error responses)
- [ ] Test with a real browser (check network panel)
- [ ] `fcntl()` used only with `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` (macOS)
- [ ] No external libraries, no C++11+

---

## Phase 5 — Documentation

- [ ] README complete (see `docs/subject.md` §V for required sections)
- [ ] Config files and default pages included for the evaluation demo
- [ ] All evaluation scenarios can be demonstrated (see `docs/evaluation.md`)

---

## Bonus (only if mandatory is fully complete and stable)

- [ ] Cookie and session management
- [ ] Multiple CGI types

---

## Weekly sync

| Week | Milestone |
|------|-----------|
| — | Phases 0–1 done, interface contract agreed |
| — | Phases 2–3 done, integration checkpoint 1 passed |
| — | Phase 4 done, stress test passing |
| — | Phase 5 done, ready for evaluation |
