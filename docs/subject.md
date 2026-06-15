# WebServ — Subject (v24.0)

> This is when you finally understand why URLs start with HTTP

## Summary

Write your own HTTP server in C++98, testable with an actual browser.

---

## General Rules

- Program must **never crash** under any circumstances (even out of memory) → grade 0 if it does
- Makefile must compile source files **without unnecessary relinking**
- Makefile must contain at minimum: `$(NAME)`, `all`, `clean`, `fclean`, `re`
- Compile with `c++` and flags: `-Wall -Wextra -Werror`
- Code must comply with **C++ 98** standard (`-std=c++98`)
- Prefer C++ versions of C functions (e.g. `<cstring>` over `<string.h>`)
- **No external libraries**, no Boost

---

## Mandatory Part

### Program

| | |
|---|---|
| **Name** | `webserv` |
| **Usage** | `./webserv [configuration file]` |
| **Language** | C++ 98 |

### Allowed external functions

`execve`, `pipe`, `strerror`, `gai_strerror`, `errno`, `dup`, `dup2`, `fork`, `socketpair`,
`htons`, `htonl`, `ntohs`, `ntohl`, `select`, `poll`, `epoll` (epoll_create, epoll_ctl, epoll_wait),
`kqueue` (kqueue, kevent), `socket`, `accept`, `listen`, `send`, `recv`, `chdir`, `bind`, `connect`,
`getaddrinfo`, `freeaddrinfo`, `setsockopt`, `getsockname`, `getprotobyname`, `fcntl`, `close`,
`read`, `write`, `waitpid`, `kill`, `signal`, `access`, `stat`, `open`, `opendir`, `readdir`, `closedir`

---

## IV.1 — Requirements

### I/O & Poll

- Server must remain **non-blocking at all times**
- Use **only 1 `poll()` (or equivalent)** for all I/O between clients and server (listen included)
- `poll()` must monitor **read AND write simultaneously**
- **Never** do a read or write without going through `poll()` first → grade 0
- **Forbidden**: checking `errno` after `read`/`write`/`recv`/`send` to adjust behaviour → grade 0
- Regular disk files are exempt from non-blocking requirement
- A request must **never hang indefinitely**
- `fork()` is allowed **only for CGI**

### macOS specifics

- `fcntl()` is allowed only with: `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC`
- All other `fcntl()` flags are forbidden

### Features

- [ ] Use a configuration file (argument or default path)
- [ ] Cannot `execve` another web server
- [ ] Compatible with standard web browsers
- [ ] Accurate HTTP response status codes
- [ ] Default error pages if none provided
- [ ] Serve fully static websites
- [ ] Clients can upload files
- [ ] Support at minimum: **GET**, **POST**, **DELETE** methods
- [ ] Handle unknown/malformed requests without crash
- [ ] Listen on **multiple ports**
- [ ] Stress-tested (availability > 99.5% under siege -b)
- [ ] No memory leaks, no hanging connections

---

## IV.2 — Configuration File

Inspired by the NGINX `server` section. Must support:

- [ ] Multiple `host:port` pairs (multiple virtual servers)
- [ ] Default error pages
- [ ] Maximum client request body size (`client_max_body_size`)
- [ ] Per-route configuration:
  - [ ] Accepted HTTP methods list
  - [ ] HTTP redirection
  - [ ] Root directory mapping (e.g. `/kapouet` → `/tmp/www`)
  - [ ] Directory listing enable/disable
  - [ ] Default file for directory requests (e.g. `index.html`)
  - [ ] File upload with configurable storage location
  - [ ] CGI execution based on file extension (`.php`, `.py`, etc.)

---

## IV.3 — CGI Requirements

- [ ] CGI runs in the correct directory (for relative path access)
- [ ] Full request + arguments available to CGI via environment variables
- [ ] Chunked requests must be un-chunked before passing to CGI (EOF = end of body)
- [ ] If CGI returns no `Content-Length`, EOF marks end of data
- [ ] Support at least **one CGI** type (php-cgi, Python, etc.)
- [ ] CGI tested with **GET** and **POST**
- [ ] Server handles CGI errors gracefully (no crash)

---

## V — README Requirements

The `README.md` must include:

- [ ] First line (italicized): *This project has been created as part of the 42 curriculum by \<login1\>[, \<login2\>[, \<login3\>]]*
- [ ] **Description** section: project goal and overview
- [ ] **Instructions** section: compilation, installation, execution
- [ ] **Resources** section: references + how AI was used (which tasks, which parts)
- [ ] Written in **English**

---

## VI — Bonus Part

Only evaluated if mandatory part is **perfect** (no errors, no unexpected behaviours):

- [ ] Cookies and session management
- [ ] Multiple CGI types

---

## Files to Submit

`Makefile`, `*.{h,hpp}`, `*.cpp`, `*.tpp`, `*.ipp`, configuration files
