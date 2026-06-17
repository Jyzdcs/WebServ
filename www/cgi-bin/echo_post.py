#!/usr/bin/env python3
"""Echo the POST body verbatim — used to verify body transmission."""
import os
import sys

length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(length) if length > 0 else ""

print("Content-Type: text/plain")
print("")
sys.stdout.write(body)
sys.stdout.flush()
