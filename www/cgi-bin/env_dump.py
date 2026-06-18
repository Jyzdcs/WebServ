#!/usr/bin/env python3
"""Dump all CGI environment variables — used to verify env transmission."""
import os
import sys

print("Content-Type: text/plain")
print("")

keys = [
    "REQUEST_METHOD", "QUERY_STRING", "CONTENT_TYPE", "CONTENT_LENGTH",
    "HTTP_HOST", "SERVER_PROTOCOL", "GATEWAY_INTERFACE",
    "SCRIPT_FILENAME", "PATH_INFO",
]
for k in keys:
    print(k + "=" + os.environ.get(k, "__MISSING__"))
