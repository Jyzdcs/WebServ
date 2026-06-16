# HTTP Layer — Interface for Core Server

This folder contains the full HTTP processing pipeline.
You only need these 4 classes to integrate.

---

## Quick start

```cpp
#include "http/RequestParser.hpp"
#include "http/Router.hpp"
#include "http/MethodHandler.hpp"
#include "http/ResponseBuilder.hpp"

// rawString = bytes received from socket (must be a complete request ending with \r\n\r\n)
RequestParser  parser;
HttpRequest    req = parser.parse(rawString);

if (req.method.empty())
{
    // parsing failed — send 400 Bad Request to client
}

Router         router;
LocationConfig loc = router.route(req, serverConfig);

MethodHandler  handler;
HttpResponse   res = handler.handle(req, loc, serverConfig);

ResponseBuilder builder;
std::string     raw = builder.build(res);

send(fd, raw.c_str(), raw.size(), 0);
```

---

## Classes

### `RequestParser`

Parses a raw HTTP/1.1 request string into an `HttpRequest` struct.

```cpp
HttpRequest parse(const std::string& raw);
```

- Returns an empty `HttpRequest` (method == "") if the request is invalid.
- Validates: separator `\r\n\r\n`, HTTP version, `Host` header for HTTP/1.1.
- Rejects tab after `:` in headers (Nginx-aligned).

**You must accumulate socket data until `\r\n\r\n` is found before calling `parse()`.**

---

### `Router`

Finds the best matching `LocationConfig` for a given request URI using longest prefix matching.

```cpp
LocationConfig route(const HttpRequest& req, const ServerConfig& server);
```

- Returns an empty `LocationConfig` (path == "") if no location matches.

---

### `MethodHandler`

Dispatches the request to the correct method handler and returns an `HttpResponse`.

```cpp
HttpResponse handle(const HttpRequest& req, const LocationConfig& loc, const ServerConfig& server);
```

| Method | Behaviour |
|--------|-----------|
| GET    | Serves static files, directory index, autoindex, 301 redirect |
| POST   | Writes body to `upload_store`, returns 201 |
| DELETE | Deletes file via `unlink`, returns 204 |

Common status codes returned:
- `200` OK
- `201` Created (POST upload)
- `204` No Content (DELETE)
- `301` Moved Permanently (redirect)
- `400` Bad Request (path traversal, empty body, etc.)
- `403` Forbidden
- `404` Not Found
- `405` Method Not Allowed
- `500` Internal Server Error
- `501` Not Implemented (CGI — coming soon)

---

### `ResponseBuilder`

Serializes an `HttpResponse` into a raw HTTP/1.1 string ready to `send()`.

```cpp
std::string build(const HttpResponse& res);
```

Output format:
```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 109\r\n
\r\n
<!DOCTYPE html>...
```

---

## Data structures

```cpp
struct HttpRequest {
    std::string                        method;   // "GET", "POST", "DELETE"
    std::string                        uri;      // "/index.html"
    std::string                        version;  // "HTTP/1.1"
    std::map<std::string, std::string> headers;
    std::string                        body;
};

struct HttpResponse {
    int                                status_code;  // 200, 404, ...
    std::string                        status_msg;   // "OK", "Not Found", ...
    std::map<std::string, std::string> headers;
    std::string                        body;
};
```
