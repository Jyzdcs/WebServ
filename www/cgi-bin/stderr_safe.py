#!/usr/bin/env python3
"""Write to stderr then respond normally — server must not crash."""
import sys

sys.stderr.write("this is an error log line\n")
sys.stderr.write("another stderr message\n")
sys.stderr.flush()

print("Content-Type: text/plain")
print("")
print("stderr_ok")
