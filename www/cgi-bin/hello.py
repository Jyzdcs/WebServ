#!/usr/bin/env python3
import os
import sys

method = os.environ.get("REQUEST_METHOD", "GET")
query  = os.environ.get("QUERY_STRING", "")

name = "World"
for param in query.split("&"):
    if param.startswith("name="):
        name = param.split("=", 1)[1]

body_data = ""
if method == "POST":
    length = int(os.environ.get("CONTENT_LENGTH", 0))
    if length > 0:
        body_data = sys.stdin.read(length)

print("Content-Type: text/html")
print("")
print("<html><body>")
print("<h2>CGI fonctionne !</h2>")
print("<p>Methode : " + method + "</p>")
print("<p>Bonjour " + name + " !</p>")
if body_data:
    print("<p>Body recu : " + body_data + "</p>")
print("</body></html>")
