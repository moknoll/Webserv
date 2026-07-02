#!/usr/bin/env python

import cgi
import sys

# Set the content type to HTML
# print()

# Get the form data from the request
form = cgi.FieldStorage()

# Extract the form field values
name = form.getvalue('name')
email = form.getvalue('email')
message = form.getvalue('message')

# Print out the form data

body = f"""<html>
<body>
<h1>Form Data</h1>
<p>Name: {name}</p>
<p>Email: {email}</p>
<p>Message: {message}</p>
</body>
</html>"""

print(f"Content-type: text/html\r")
print(f"Content-Length: {len(body)}\r")
print("\r")
sys.stdout.write(body)
