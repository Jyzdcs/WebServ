# SKILL: run-tests

## Trigger

Use this skill when the user says:
"run tests", "lance les tests", "test", "check tests", "/run-tests", "est-ce que les tests passent"

Or when invoked after implementing a feature to verify nothing is broken.

---

## Goal

1. Run the existing tests relevant to the current module or all modules
2. Decide if the current changes warrant adding a new test
3. If yes, add a minimal integration test — not a unit test

---

## Step 1 — Detect which module is being worked on

Run:
```bash
git diff --name-only origin/main...HEAD
```

Map changed paths to modules:
- `src/config/` or `include/config/` → module **config** → run `tests/config/`
- `src/server/` or `include/server/` → module **server** → run `tests/server/`
- `src/http/` or `include/http/` → module **http** → run `tests/http/`
- Multiple modules or unclear → run all: `tests/run_all.sh`

---

## Step 2 — Run the tests

```bash
# All modules
bash tests/run_all.sh

# Or a specific module
bash tests/run_all.sh config
bash tests/run_all.sh server
bash tests/run_all.sh http
```

If the binary doesn't exist yet:
```bash
make
```

Report the output clearly: how many passed, how many failed, which ones failed.

---

## Step 3 — Decide if a new test is needed

### Add a test if the PR introduces one of these:

| Feature | Module | Test type |
|---------|--------|-----------|
| New config directive (new keyword parsed) | config | Start server with that config, check it works |
| Multi-port or multi-server support | config | Two ports both respond |
| New HTTP method (GET/POST/DELETE) | http | Send that method, check status code |
| File upload | http | POST a file, check it exists on disk |
| Directory listing (autoindex) | http | GET a directory, check HTML response |
| HTTP redirect | http | GET redirected URL, check 301/302 + Location header |
| CGI execution | http | GET/POST a .py or .php script, check 200 |
| Body size limit | http | POST oversized body, check 413 |
| Client timeout | server | Open connection, wait, check it gets closed |
| Simultaneous connections | server | Two concurrent requests, both succeed |

### Do NOT add a test for:

- Internal refactoring (renamed a class, split a function)
- Adding a helper method that's not directly callable via HTTP
- Changing a data structure internally
- Fixing formatting or comments
- Anything already covered by an existing test

---

## Step 4 — Write the test (if needed)

Add the test to the correct file:
- Config feature → `tests/config/test_config.sh`
- Server feature → `tests/server/test_server.sh`
- HTTP feature → `tests/http/test_http.sh`

### Test template

```bash
# --- Feature name ---
start_server "$CONFIGS/relevant.conf"
status=$(http_status -X METHOD "http://localhost:PORT/path")
assert_eq "description of what we're testing: expected code" "EXPECTED" "$status"
stop_server
```

Keep it short. One test = one observable behaviour.
Test the **outcome** (status code, response body, file on disk), never internal state.

---

## Step 5 — Report

Output:
```
Tests run: X
Passed: X
Failed: X

[if new test added]
Added: tests/http/test_http.sh — "POST upload: file created on disk"

[if nothing added]
No new test added (refactoring / already covered).
```

---

## Notes

- `tests/run_all.sh` accepts an optional module argument: `config`, `server`, `http`
- `tests/helpers.sh` provides: `start_server`, `stop_server`, `assert_eq`, `assert_contains`, `http_status`, `http_body`
- The server binary is `./webserv` — build it with `make` if missing
- Tests skip gracefully if the binary doesn't exist yet (`[SKIP]` output)
