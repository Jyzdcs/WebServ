#!/usr/bin/env python3
"""Check CGI/1.1 required env variables per RFC 3875 §4.1"""
import os

print("Content-Type: text/plain")
print("")

required = ["REQUEST_METHOD", "QUERY_STRING", "GATEWAY_INTERFACE",
            "SERVER_PROTOCOL", "SERVER_NAME", "SERVER_PORT",
            "SCRIPT_NAME", "REMOTE_ADDR", "PATH_INFO"]

for k in required:
    val = os.environ.get(k, "__MISSING__")
    print(k + "=" + val)
