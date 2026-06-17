#!/usr/bin/env python3
"""Return a 302 redirect — used to verify custom Status header parsing."""
import os

target = os.environ.get("QUERY_STRING", "")
location = "https://example.com"
for part in target.split("&"):
    if part.startswith("to="):
        location = part[3:]

print("Status: 302 Found")
print("Location: " + location)
print("Content-Type: text/html")
print("")
print("<html><body>Redirecting to " + location + "</body></html>")
