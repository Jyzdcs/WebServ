# SKILL: run-tests

## Trigger

Use this skill when the user says:
"run tests", "lance les tests", "test", "check tests", "/run-tests", "est-ce que les tests passent"

Or when invoked after implementing a feature to verify nothing is broken.

---

## Goal

1. Run all existing tests found in `tests/`
2. Decide if the current changes warrant adding a **new** test
3. If yes, write a minimal integration test and add it to the right place

---

## Step 1 — Run existing tests

Look for test scripts in `tests/`:

```bash
find tests/ -name "*.sh" -o -name "*.py" | sort
```

Run each one and collect results. If there are no tests yet, say so clearly and skip to Step 2.

If a `tests/run_all.sh` exists:
```bash
bash tests/run_all.sh
```

Otherwise run individual scripts directly.

Report: how many passed, how many failed, which ones failed.

---

## Step 2 — Detect which module is being worked on

```bash
git diff --name-only origin/main...HEAD
```

Map changed paths to modules:
- `src/config/` or `include/config/` → module **config**
- `src/server/` or `include/server/` → module **server**
- `src/http/` or `include/http/` → module **http**

---

## Step 3 — Decide if a new test is needed

### Add a test if the PR introduces one of these major features:

| Feature | Module | What to test |
|---------|--------|--------------|
| New config directive | config | Start server with that config, verify it starts |
| Multi-port / multi-server | config | Both ports respond |
| New HTTP method (GET/POST/DELETE) | http | Send that method, check status code |
| File upload | http | POST a file, check it appears on disk |
| Directory listing (autoindex) | http | GET a directory, check HTML response |
| HTTP redirect | http | Check 301/302 + Location header |
| CGI execution | http | GET/POST a script, check 200 + output |
| Body size limit | http | POST oversized body, check 413 |
| Client timeout | server | Open connection, wait, check it closes |
| Simultaneous connections | server | Two concurrent requests both succeed |

### Do NOT add a test for:

- Internal refactoring (renamed a class, split a function)
- Helper methods not directly observable from outside
- Data structure changes with no behaviour impact
- Anything already covered by an existing test

---

## Step 4 — Write the test (if needed)

Decide where to put it:
- Config feature → `tests/config/`
- Server feature → `tests/server/`
- HTTP feature → `tests/http/`

Write a **short shell or Python script** that:
1. Starts the server with a config file
2. Sends an HTTP request (with `curl`)
3. Asserts the response (status code, body, or file on disk)
4. Kills the server

One test = one observable behaviour. Test the **outcome**, never internal state.

Ask the user before writing the file.

---

## Step 5 — Report

```
Tests found: X scripts
Passed: X
Failed: X

[if new test added]
→ Added: tests/http/test_upload.sh — "POST upload: file created on disk"

[if nothing added]
→ No new test needed (refactoring / already covered / no major feature).
```
