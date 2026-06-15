# WebServ — Agent Instructions

This file provides context and rules for all AI agents (Cursor, Claude Code, etc.) working on this project.

---

## Project

A custom HTTP/1.1 web server written in **C++98**, built as part of the 42 curriculum.

- Subject: `docs/subject.md`
- Evaluation grid: `docs/evaluation.md`

---

## Hard constraints — never violate these

These are **automatic grade-0** failures in evaluation:

1. **Single poll()** — there must be exactly one `poll()` (or equivalent: `select`, `epoll_wait`, `kevent`) in the entire codebase, inside the main event loop. It must monitor both read AND write simultaneously.
2. **No read/write without poll()** — never call `read`, `recv`, `write`, `send` on a socket without going through `poll()` first.
3. **No errno after read/write** — never check `errno` to adjust server behaviour after a read/recv/write/send call.
4. **fork() for CGI only** — `fork()` must not be used anywhere except CGI execution.
5. **fcntl() flags** — on macOS, only `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` are allowed.
6. **C++98 only** — no C++11 or later. No `auto`, no lambdas, no range-for, no `nullptr`, no `std::thread`.
7. **No external libraries** — standard C++98 + POSIX only. No Boost, no external deps.
8. **No execve another web server** — CGI scripts only, never wrap another HTTP server.

---

## Architecture

```
src/
  config/    — ConfigParser, ServerConfig, RouteConfig
  server/    — Server, PollManager, Socket, Client
  http/      — RequestParser, Router, MethodHandler, CgiHandler, ResponseBuilder

include/
  config/
  server/
  http/

config/      — .conf files (NGINX-style)
www/         — static web root
docs/        — subject and evaluation reference
```

---

## Workflow

- **All changes go through PRs** — no direct push to `main`
- **Before creating a PR**, run the pr-review skill:
  - **Cursor**: invoke the `pr-review` skill
  - **Claude Code**: run `/project:pr-review`
- Branch naming: `feat/`, `fix/`, `refactor/`, `chore/`, `docs/`

---

## Skills available

| Skill | Location | How to invoke |
|-------|----------|---------------|
| `pr-review` | `.agents/skills/pr-review/SKILL.md` | Cursor: skill invocation / Claude Code: `/project:pr-review` |

---

## Configuration file format

NGINX-inspired. Example:

```nginx
server {
    listen 8080;
    server_name localhost;
    client_max_body_size 10M;
    error_page 404 /errors/404.html;

    location / {
        root www;
        index index.html;
        methods GET POST;
        autoindex off;
    }

    location /upload {
        methods POST;
        upload_store /tmp/uploads;
    }

    location /cgi-bin {
        methods GET POST;
        cgi_extension .py /usr/bin/python3;
    }
}
```

---

## Testing references

- NGINX: use to compare headers and response behaviour
- Tools: `curl`, `wget`, `telnet`, `siege`
- Stress test: `siege -b http://localhost:8080/ -t 30s` → availability must be ≥ 99.5%
