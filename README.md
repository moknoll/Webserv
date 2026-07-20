# webserv

*This project has been created as part of the 42 curriculum by nmagomad mknoll oklimov*

## Description

**webserv** is an HTTP/1.1 web server built in **C++98**. The goal of this project is to understand how web servers work by implementing one from scratch, without relying on existing web server frameworks.

The server is designed to handle multiple client connections efficiently using **non-blocking I/O** and an **event-driven architecture**. It supports:

- Serving static websites
- Executing CGI scripts
- Handling HTTP requests and responses
- Configurable server settings
- Redirections
- Custom error pages
- File uploads

Through this project, we gained hands-on experience with:

- Network programming
- Socket management
- HTTP protocol implementation
- Process management
- Concurrent server design

Throughout the development of **webserv**, we focused on writing clean, modular, and maintainable C++ code while building a functional and standards-compliant HTTP/1.1 server.

## Instructions

### Compilation

Clone the repository and compile the project using `make`:

```bash
git clone <repository_url>

cd webserv

make
````

After a successful compilation, the `webserv` executable will be generated.

### Usage
Start the server by providing a configuration file:

```bash
./webserv [config.conf]
```
The configuration file is used to define the server behaviour, including ports, routes, allowed methods, error pages, CGI execution and file uploads.

### Configuration file 
A configuration file is required to start the server. It defines how the server handles incoming HTTP requests.

```config
  server {
    listen 8080;
	server_name www.example.com;

	error_page 404 ./www/error_pages/404.html;
	error_page 405 ./www/error_pages/405.html;
	error_page 500 ./www/error_pages/500.html;

	client_max_body_size 100M;

	location / {
		root ./www;
		index index.html;
		allowed_methods GET POST;
	}

	location /upload {
		root ./www/uploads;
		allowed_methods GET POST;
		upload_path ./www/uploads;
	}

	location /autoin {
	root ./src;
	autoindex on;
	allowed_methods GET;
	}

	location /redir {
		return 302 /autoin;
	}

	location /cgi-bin {
		root ./www/;
		cgi_extension .py;
		cgi_path /usr/bin/python3;
		allowed_methods GET POST;
	}
}
```
### Configuration Directives

| Directive | Description |
|-----------|-------------|
| `listen` | Defines the port on which the server listens for incoming client connections. |
| `server_name` | Defines the name of the server. This is useful when multiple servers are configured to determine which server should handle an incoming request. |
| `error_page` | Defines custom error pages that are displayed for specific HTTP error codes. |
| `client_max_body_size` | Defines the maximum allowed size of the request body sent by a client. |
| `location` | Defines a route and the configuration rules applied to requests targeting a specific path. |
| `root` | Defines the root directory from which files are served for a specific location. |
| `index` | Defines the default file that is served when accessing a directory without specifying a file. |
| `autoindex` | Enables or disables automatic directory listing when no index file is available. |
| `allowed_methods` | Defines which HTTP methods are allowed for a specific location, such as `GET` or `POST`. |
| `upload_path` | Defines the directory where uploaded files are stored. |
| `return` | Defines an HTTP redirection by returning a specific status code and target location. |
| `cgi_extension` | Defines which file extensions should be executed as CGI scripts. |
| `cgi_path` | Defines the executable path used to run CGI scripts. |

---
## Testing

Once the server is running, you can test it by opening a web browser and navigating to:

```text
http://localhost:8080
```

Alternatively, you can send HTTP requests using `curl`:

```bash
curl http://localhost:8080
```

### CGI 
The Server supports CGI(Common Gateway Interface) execution, allowing external programms to generate dynamic HTTP responses.
Currently the server supports Python CGI scripts:

```conf
location /cgi-bin {
    root ./www/;
    cgi_extension .py;
    cgi_path /usr/bin/python3;
    allowed_methods GET POST;
}
```
When a request targets a CGI script, the server executes the script and forwards the generated output back to the client as an HTTP response.

```
http://localhost:8080/cgi-bin/script.py
```


### File Upload

File upload can be enabled by configuring a dedicated upload location in the server configuration.

The upload location requires an `upload_path` directive, which defines the directory where uploaded files will be stored.

Example configuration:

```conf
location /upload {
    upload_path ./www/uploads;
    allowed_methods POST;
}
```
### Upload Methods
**1. Uploading from a webserver**
Files can be uploaded using an HTML form with the `multipart/form-data` encoding.
```HTML
<form action="/upload" method="POST" enctype="multipart/form-data">
    <input type="file" name="file">
    <input type="submit" value="Upload">
</form>

```
The uploaded file will be stored in the directory defined by `upload_path`.

**2. Uploading with curl**
Files can be uploaded firectly from the command line. 
```bash
curl -v -X POST -F "file=@photo.jpg" http://localhost:8081/upload
```
This sends the file using the `multipart/form-data` content type, which is commonly used by web browsers for file uploads.

**3.Uploading Raw File Data(application/octet-stream)**
If you do not use multipart/form-data, the server must know the filename by another means. In this implementation, 
the client must provide an X-Filename header containing the desired filename.
In this implementation, the client must provide an X-Filename header containing the desired filename.
```bash
curl -X POST -H "Content-Type: application/octet-stream" -H "X-Filename: photo.jpg" --data-binary @photo.jpg http://localhost:8080/upload
```
## Resources

### CGI (Common Gateway Interface)

- [RFC 3875 - The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)

### HTTP (Hypertext Transfer Protocol)

- [RFC 7230 - HTTP/1.1: Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [MDN Web Docs - HTTP Guides](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides)
- [MDN Web Docs - How the Web Works](https://developer.mozilla.org/en-US/docs/Learn_web_development/Getting_started/Web_standards/How_the_web_works)
- [Nginx Docs](https://nginx.org/en/docs)

### Socket Programming
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split/)

## AI Usage 
**Documentation and Comments**
- README structure and formatting
- Documentation clarity
- Code comments and explanations
  
**Research and Learning**
- Explanation of network programming concetps
- HTTP implemetntaion of nginx
  
**Code Review and Debugging**
- Review code structure
- Identify possible bugs and edge cases
- Discuss possible solutions and improvements


