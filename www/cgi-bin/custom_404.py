#!/usr/bin/env python3
"""Return Status: 404 — used to verify arbitrary status code from CGI."""
print("Status: 404 Not Found")
print("Content-Type: text/html")
print("")
print("<html><body><h1>Custom 404 from CGI</h1></body></html>")
