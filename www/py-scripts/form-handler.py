#!/usr/bin/env python

import cgi
import sys
import os
import json

form_data = cgi.FieldStorage()
filename = "message.json"

name = form_data.getvalue("name")
email = form_data.getvalue("email")
message = form_data.getvalue("message")

list_of_users = []

try:
    with open(filename, "r", encoding='utf-8') as f:
        list_of_users = json.load(f)
except FileNotFoundError:
    list_of_users = []

if name and email and message:
    obj = {"name": name, "email": email, "message": message}
    list_of_users.append(obj)
    with open(filename, "w", encoding='utf-8') as file:
        json.dump(list_of_users, file, indent=4, ensure_ascii=False)


form_html = f"""
    <div class="box">
        <h1>Please, fill out the fields</h1>
        <form action="/cgi-bin/py/form-handler.py" method="POST">
            <input type="text" name="name" placeholder="Name" required>
            <input type="text" name="email" placeholder="Email" required>
            <input type="text" name="message" placeholder="Message" required>
            <br><br>
            <input type='submit' value='Submit'> 
        </form>
        <br><br>
    </div>
"""

table_html = ""
if list_of_users:
    table_rows = ""
    for user in list_of_users:
        table_rows += f"<tr><td>{user.get('name')}</td><td>{user.get('email')}</td><td>{user.get('message')}</td></tr>"
    
    table_html = f"""
    <table border="1" style="margin-top: 20px; width: 100%; border-collapse: collapse;">
        <tr>
            <th>Name</th>
            <th>Email</th>
            <th>Message</th>
        </tr>
        {table_rows}
    </table>
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
                <li><a href="/cgi-bin/py/helloCGI.py">Home</a></li>
                <li><a href="/cgi-bin/py/upload.py">Upload</a></li>
                <li><a href="/cgi-bin/py/form-handler.py">Form handler</a></li>
                <li><a href="/cgi-bin/py/session_management.py">Session management</a></li>
                <li><a href="https://github.com/">GitHub</a></li>
            </ul>
        </nav>
        {form_html}
        {table_html}
    </body>
    </html>
    """

print("Content-Type: text/html\r")
print(f"Content-Length: {len(body.encode('utf-8'))}\r")
print('\r')

sys.stdout.write(body)
