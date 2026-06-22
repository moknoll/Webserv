#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import cgi
import cgitb

# Включаем отладку для CGI
cgitb.enable()

def print_environment():
    """Выводит все переменные окружения в виде HTML таблицы"""
    print("<h2>Environment Variables:</h2>")
    print("<table border='1' cellpadding='5' style='border-collapse: collapse;'>")
    print("<tr><th>Variable</th><th>Value</th></tr>")
    
    # Сортируем переменные для удобства чтения
    for key in sorted(os.environ.keys()):
        value = os.environ[key]
        # Экранируем HTML спецсимволы для безопасного отображения
        value = value.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
        print(f"<tr><td><b>{key}</b></td><td>{value}</td></tr>")
    
    print("</table>")

def print_post_data():
    """Выводит данные POST запроса"""
    print("<h2>POST Data:</h2>")
    
    request_method = os.environ.get('REQUEST_METHOD', 'GET')
    
    if request_method != 'POST':
        print("<p>This is not a POST request.</p>")
        return
    
    try:
        form = cgi.FieldStorage()
        
        if not form:
            print("<p>No POST data received.</p>")
            return
        
        print("<table border='1' cellpadding='5' style='border-collapse: collapse;'>")
        print("<tr><th>Field</th><th>Value</th></tr>")
        
        for key in form.keys():
            value = form.getvalue(key)
            if isinstance(value, list):
                value = ', '.join(str(v) for v in value)
            value = str(value).replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
            print(f"<tr><td><b>{key}</b></td><td>{value}</td></tr>")
        
        print("</table>")
        
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
        if content_length > 0:
            print("<h3>Raw POST data:</h3>")
            raw_data = sys.stdin.read(content_length)
            raw_data = raw_data.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
            print(f"<pre>{raw_data}</pre>")
            
    except Exception as e:
        print(f"<p style='color: red;'>Error processing POST data: {e}</p>")

def main():
    print("Content-Type: text/html; charset=utf-8\r")
    print("\r")

    print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Environment and POST Data</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            border-bottom: 2px solid #4CAF50;
            padding-bottom: 10px;
        }
        h2 {
            color: #555;
            margin-top: 30px;
        }
        table {
            width: 100%;
            margin-top: 10px;
        }
        th {
            background-color: #4CAF50;
            color: white;
            padding: 10px;
            text-align: left;
        }
        td {
            padding: 8px;
            border-bottom: 1px solid #ddd;
        }
        tr:hover {
            background-color: #f5f5f5;
        }
        .request-info {
            background-color: #e7f3fe;
            padding: 10px;
            border-radius: 5px;
            margin-bottom: 20px;
        }
        pre {
            background-color: #f4f4f4;
            padding: 10px;
            border-radius: 5px;
            overflow-x: auto;
            border: 1px solid #ddd;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📋 CGI Environment & POST Data Viewer</h1>
""")

    # Выводим информацию о запросе
    print("<div class='request-info'>")
    print(f"<strong>Request Method:</strong> {os.environ.get('REQUEST_METHOD', 'Unknown')}<br>")
    print(f"<strong>Content Type:</strong> {os.environ.get('CONTENT_TYPE', 'Not set')}<br>")
    print(f"<strong>Content Length:</strong> {os.environ.get('CONTENT_LENGTH', '0')}<br>")
    print(f"<strong>Query String:</strong> {os.environ.get('QUERY_STRING', 'Empty')}")
    print("</div>")

    print_environment()
    
    print_post_data()

    print("""
    </div>
</body>
</html>
""")

if __name__ == "__main__":
    main()
