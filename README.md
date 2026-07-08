# webserv

*This project has been created as part of the 42 curriculum by oklimov nmagomad mknoll*

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

### Requirements

Before compiling **webserv**, make sure the following dependencies are installed:

- C++98 compatible compiler (`c++`)
- GNU Make
- Linux or macOS

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

### Testing

Once the server is running, you can test it by opening a web browser and navigating to:

```text
http://localhost:8080
```

Alternatively, you can send HTTP requests using `curl`:

```bash
curl http://localhost:8080
```

## Ressources 
- **CGI**:
  - https://datatracker.ietf.org/doc/html/rfc3875
 
- **HTTP**:
  - https://datatracker.ietf.org/doc/html/rfc7230
  - https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides
  - https://developer.mozilla.org/en-US/docs/Learn_web_development/Getting_started/Web_standards/How_the_web_works

- **Socket Programming**:
  - https://beej.us/guide/bgnet/html/split/
