# WebServ

_This project has been created as part of the 42 curriculum by abbouras, kclaudan, arpenel_

---

## Description

WebServ is a fully compliant HTTP/1.1 web server written in C++98, built from scratch as part of the 42 curriculum. It handles static file serving, file uploads, HTTP redirections, directory listing, and CGI script execution — all over a single non-blocking `poll()` event loop.

Key properties:

- Single `poll()` call monitoring all file descriptors (clients, listening sockets, CGI pipes) simultaneously for read and write
- Fully non-blocking I/O — no read or write without going through `poll()` first
- Virtual hosting — multiple server blocks on the same port, routed by `Host` header
- Asynchronous CGI execution with configurable timeout (5 seconds)
- Graceful shutdown on `SIGINT` / `SIGTERM`

---

## Instructions

### Requirements

- `c++` compiler with C++98 support
- Python 3 (for CGI scripts)
- macOS or Linux

### Compilation

```bash
make
```

### Run

```bash
./webserv [config_file]
```

Default config is loaded if no argument is provided:

```bash
./webserv                        # uses config/default.conf
./webserv config/default.conf    # explicit
```

### Stop

```
Ctrl+C   (SIGINT)
```

### Configuration file

NGINX-inspired syntax. Example:

```nginx
server {
    listen 1818;
    host 127.0.0.1;
    server_name mysite.com;
    client_max_body_size 1m;
    error_page 404 www/errors/404.html;

    location / {
        root www;
        index index.html;
        methods GET POST;
        autoindex off;
    }

    location /uploads {
        root www/uploads;
        methods GET POST DELETE;
        upload_store www/uploads;
        autoindex on;
    }

    location /cgi-bin {
        root www;
        methods GET POST;
        cgi_extensions .py /usr/bin/python3;
    }

    location /old {
        redirect http://127.0.0.1:1818/;
    }
}
```

Multiple `server {}` blocks are supported. Two servers on the same port with distinct `server_name` values enable virtual hosting.

### Run tests

```bash
# Unit and integration tests (requires googletest)
bash tests/config/run_config_tests.sh

# End-to-end tests (requires Docker)
docker build -t webserv:test .
docker run --rm webserv:test bash tests/e2e/run_e2e.sh

# Stress test
siege -b http://127.0.0.1:1818/ -t 30s -c 25
```

---

## Resources

- [RFC 7230 — HTTP/1.1 Message Syntax](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 — HTTP/1.1 Semantics](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 3875 — CGI/1.1 Specification](https://datatracker.ietf.org/doc/html/rfc3875)
- [NGINX documentation](https://nginx.org/en/docs/) — used as behavioral reference for config syntax and HTTP compliance
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

### AI usage

AI assistants (Claude via Cursor) were used throughout this project for:

- Code review and HTTP compliance checking against NGINX behavior
- Architectural design of the async CGI integration (`processHttp` / `finishCgi` interface)
- Debugging session issues (CGI pipe handling, virtual hosting routing, C++98 compatibility)
- Writing the end-to-end test suite (`tests/e2e/run_e2e.sh`)
