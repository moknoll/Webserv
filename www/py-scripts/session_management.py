#!/usr/bin/python

import os
import cgi
import sys
from http.cookies import SimpleCookie

# get data from form 
form = cgi.FieldStorage()
first_name = form.getvalue("first_name", "")
action = form.getvalue("action", "")

# get cookie
cookie = SimpleCookie()
cookie.load(os.environ.get("HTTP_COOKIE", ""))
method = os.environ.get("REQUEST_METHOD", "GET")

# clear cookies
if method == "POST" and action == "clear":

    print(f"Set-Cookie: user=\r")
    print("Location: /cgi-bin/session_management.py\r")
    print("\r")
    sys.exit(0)

# if method is  POST and name is correct 
if method == "POST" and first_name and " " not in first_name:
    cookie["user"] = first_name
    # send Set-Cookie
    print(f"Set-Cookie: user={first_name}\r")

# if cookie alredy existe
if "user" in cookie:
    first_name = cookie["user"].value

# ---------- HTML ----------


clear_form = """
    <form action="/cgi-bin/session_management.py" method="POST" style="margin-top: 10px;">
        <input type="hidden" name="action" value="clear">
        <input type="submit" value="🗑️ Clear Cookies" style="padding: 8px 20px; background: #dc3545; color: white; border: none; border-radius: 4px; cursor: pointer;">
    </form>

"""

form = f"""
    <div class="box">
    <h1>Hello {first_name if first_name else "Guest" }!</h1>
    {"<h3>Your cookie is working.</h3>" if first_name else "<h2>Enter your name</h2>"}
        <form action="/cgi-bin/session_management.py" method="POST">
            <input type="text" name="first_name">
            <br><br>
            <input type='submit' value={'Change name' if first_name else 'Submit'}> 
        </form>
        <br><br>
        {clear_form if first_name else ""}
        
"""

body = f"""
    <!doctype html>
    <html lang="en">
    <head>
    <meta charset="UTF-8">
    <title>Session Example</title>
    <link rel="stylesheet" href="/py-scripts/style.css">
    </head>

    <body>
        <nav class="navbar">
            <div class="logo">
                <a href="/">Webserv</a>
            </div>
            <ul class="nav-links">
                <li><a href="/cgi-bin/helloCGI.py">Home</a></li>
                <li><a href="/cgi-bin/upload.py">Upload</a></li>
                <li><a href="/cgi-bin/form-handler.py">Form handler</a></li>
                <li><a href="/cgi-bin/session_management.py">Session management</a></li>
                <li><a href="https://github.com/">GitHub</a></li>
            </ul>
        </nav>
        {form}

    </body>
    </html>
    """

print("Content-Type: text/html\r")
print(f"Content-Length: {len(body.encode("utf-8"))}\r")
print('\r')

sys.stdout.write(body)
