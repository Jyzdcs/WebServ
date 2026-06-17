#!/usr/bin/env python3
"""Parse application/x-www-form-urlencoded POST body and reflect values."""
import os
import sys

length = int(os.environ.get("CONTENT_LENGTH", 0))
body   = sys.stdin.read(length) if length > 0 else ""
ctype  = os.environ.get("CONTENT_TYPE", "")

params = {}
for part in body.split("&"):
    if "=" in part:
        k, v = part.split("=", 1)
        params[k] = v.replace("+", " ").replace("%20", " ")

print("Content-Type: text/plain")
print("")
print("CONTENT_TYPE=" + ctype)
for k, v in sorted(params.items()):
    print(k + "=" + v)
