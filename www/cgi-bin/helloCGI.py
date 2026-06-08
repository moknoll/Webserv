#!/usr/bin/env python3

body = """
<!DOCTYPE html>
<html>
<head><title>Hello CGI</title></head>
<body>
  <h1>CGI</h1>
</body>
</html>
"""

print("Content-Type: text/html\r")
print("Content-Length: %d\r" % len(body))
print("\r")
print(body)