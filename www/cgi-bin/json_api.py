#!/usr/bin/env python3
"""Return JSON — used to verify non-HTML content-type propagation."""
import os

method = os.environ.get("REQUEST_METHOD", "GET")
query  = os.environ.get("QUERY_STRING", "")

name = "anonymous"
for part in query.split("&"):
    if part.startswith("name="):
        name = part[5:].replace("%20", " ")

body = '{"status":"ok","method":"' + method + '","name":"' + name + '"}'

print("Content-Type: application/json")
print("Content-Length: " + str(len(body)))
print("")
print(body)
