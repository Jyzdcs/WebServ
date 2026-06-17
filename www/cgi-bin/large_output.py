#!/usr/bin/env python3
"""Output 100 KB of data — used to verify large response handling."""
import sys

chunk = "A" * 1024
body = chunk * 100  # 100 KB

print("Content-Type: text/plain")
print("Content-Length: " + str(len(body)))
print("")
sys.stdout.write(body)
sys.stdout.flush()
