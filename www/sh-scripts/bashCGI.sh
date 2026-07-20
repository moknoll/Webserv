#!/bin/bash
echo "Content-type: text/html"
echo ""
echo "<html><body>"
echo "<h1>CGI Bash Example</h1>"
echo "Query String: ${QUERY_STRING}"
echo "</body></html>"
