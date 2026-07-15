#!/usr/bin/python

import os
import cgi
import sys
from http.cookies import SimpleCookie

# Получаем данные формы
form = cgi.FieldStorage()
first_name = form.getvalue("first_name")

# Загружаем cookie
cookie = SimpleCookie()
cookie.load(os.environ.get("HTTP_COOKIE", ""))

method = os.environ.get("REQUEST_METHOD", "GET")

# Если пришел POST и имя корректное
if method == "POST" and first_name and " " not in first_name:
    cookie["user"] = first_name

    # Отправляем Set-Cookie
    print("Set-Cookie: user=%s" % first_name)

# Если cookie уже существует
if "user" in cookie:
    first_name = cookie["user"].value

# ---------- HTML ----------

if "user" not in cookie:

    body = """
<!doctype html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Session Example</title>
</head>

<body>

<h2>Enter your name</h2>

<form action="/cgi-bin/session_management.py" method="POST">
    First Name:
    <input type="text" name="first_name">
    <br><br>
    <input type="submit" value="Submit">
</form>

</body>
</html>
"""

else:

    body = f"""
<!doctype html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Session Example</title>
</head>

<body>

<h1>Hello {first_name}!</h1>

<p>Your cookie is working.</p>

<form action="/cgi-bin/session_management.py" method="POST">
    <input type="text" name="first_name">
    <input type="submit" value="Change name">
</form>

</body>
</html>
"""

print("Content-Type: text/html\r")
print(f"Content-Length: {len(body.encode("utf-8"))}\r")
print('\r')

sys.stdout.write(body)
