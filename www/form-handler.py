#!/usr/bin/env python

import cgi
import sys
import os
import json

form = cgi.FieldStorage()
filename = "message.json"

name = form.getvalue("name")
email = form.getvalue("email")
message = form.getvalue("message")

obj = {"name": name, "email": email, "message": message}

if os.path.exists(filename):
	try:
		with open(filename, "r") as file:
			data = json.load(file)
	except json.JSONDecodeError:
		data =[]
else:
	data = []

if in data 
	data.append(obj)

with open(filename, "w") as file:
	json.dump(data, file, indent=4)


body = f"""<html>
<body>
<h1>New Data added!</h1>
<p>Name: {name}</p>
<p>Email: {email}</p>
<p>Message: {message}</p>
</body>
</html>"""

print(f"Content-type: text/html\r")
print(f"Content-Length: {len(body)}\r")
print("\r")
sys.stdout.write(body)
