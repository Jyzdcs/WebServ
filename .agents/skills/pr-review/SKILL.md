# SKILL: pr-review

## Trigger

Use this skill when the user asks to review, check, or validate a PR before submitting it, or says:
"review my PR", "check my changes", "pr-review", "/pr-review", "is my PR ready", "validate before merge"

---

## Goal

Analyse the current branch changes against the WebServ subject and evaluation grid,
then produce a structured review report before the PR is created.

---

## Steps

### 1. Read the reference documents

Read both files:
- `docs/subject.md` — mandatory requirements and constraints
- `docs/evaluation.md` — what evaluators will specifically check

### 2. Get the diff

Run the following to see all changes on the current branch vs main:

```bash
git fetch origin main
git diff origin/main...HEAD
```

Also get the list of changed files:

```bash
git diff --name-only origin/main...HEAD
```

And the commit log:

```bash
git log origin/main...HEAD --oneline
```

### 3. Analyse the changes

For each changed file, determine:
- Which subject requirement or evaluation point it addresses
- Whether the implementation is complete or partial
- Whether it introduces any violation of the subject constraints

### 4. Critical constraint check (automatic grade-0 risks)

**Always explicitly check for these — any violation = grade 0:**

| Constraint | How to detect |
|------------|---------------|
| More than one `poll()`/`select()`/`epoll()`/`kqueue()` call site | Search for all occurrences of `poll(`, `select(`, `epoll_wait(`, `kevent(` in changed files |
| `poll()` not checking read AND write simultaneously | Look for `POLLIN \| POLLOUT` or `FD_SET` for both read and write |
| `read`/`recv`/`write`/`send` called without prior `poll()` | Look for raw syscall patterns outside the poll loop |
| `errno` checked after `read`/`recv`/`write`/`send` | Search for `errno` usage near those syscalls |
| `fork()` used outside CGI | Check every `fork()` call |
| `fcntl()` used with forbidden flags (macOS) | Only `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` are allowed |
| C++11 or later features | Look for `auto`, lambdas, range-for, `nullptr`, `std::thread`, etc. |
| External libraries | Check includes for anything beyond standard C++98 + POSIX |

### 5. Produce the report

Output a structured report with the following sections:

---

## PR Review Report

### Branch
`[branch name]`

### Files changed
[list]

### Commits
[list]

---

### ✅ Subject requirements addressed
For each requirement from `docs/subject.md` that this PR fully or partially implements:
- Requirement name
- Status: ✅ Complete / 🟡 Partial / ❌ Missing
- Brief explanation

---

### 🚨 Critical violations (grade 0 risks)
List any detected violations from the constraint table above.
If none: **"None detected — good."**

---

### ⚠️ Evaluation points at risk
Things the PR may not pass during peer evaluation based on `docs/evaluation.md`.

---

### 💬 Code quality notes
Optional observations: memory management, error handling, C++98 compliance, edge cases.

---

### 🔁 Verdict

One of:
- **✅ Ready to PR** — no critical issues found
- **⚠️ Minor issues — review recommended before merge**
- **🚨 Do NOT open PR — critical violation found: [reason]**

---

## Notes

- This skill does not create the PR. It only reviews the readiness.
- If the verdict is "Ready to PR", the user can then run `gh pr create`.
- If teammates are on Claude Code, they run `/project:pr-review` (see `.claude/commands/pr-review.md`).
- If teammates are on Cursor, they invoke this skill directly.
