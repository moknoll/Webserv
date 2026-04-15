# Simple C++ Webserver

This project implements a basic non-blocking HTTP server in C++ using `poll()` for I/O multiplexing. It currently acts as a simple request-response server that accepts connections, reads input, and replies with a static message.

## 🛠 Compilation

Use the provided `Makefile` to compile the source code:

```bash
make
```

This should generate the executable(s) (e.g., `webserv` or `server`).

> To clean up object files and executables:
> ```bash
> make fclean
> ```

## How to Run

### 1. Start the Server
Run the server executable. It will start listening on the configured port (default is usually `16000` or defined in `sockets.hpp`).

```bash
./server
```

The server will output:
```
Server listening on port 8080
```

### 2. Connect a Client
You can test the server using the provided client, `telnet`, `nc`, or a web browser.

#### Option A: Using the provided Client
Open a **second terminal** and run the client executable:

```bash
./client localhost
```
Follow the prompt to send a message.

#### Option B: Using `telnet` (Netcat)
```bash
telnet localhost 8080
```
Type a message and press Enter.

#### Option C: Using a Browser or `curl`
```bash
curl -v http://localhost:8080
```

## Features
- **Non-blocking I/O:** Uses `fcntl(O_NONBLOCK)` and `poll()`.
- **Socket Management:** Handles `socket`, `bind`, `listen`, and `accept`.
- **Client Handling:** Manages multiple clients simultaneously using a vector of `pollfd`.


## Ressources 
- https://www.cs.columbia.edu/~danr/courses/6761/Fall00/hw/pa1/6761-sockhelp.pdf
- https://developer.mozilla.org/de/docs/Web/HTTP/Guides/Messages
- https://datatracker.ietf.org/doc/html/rfc7230#section-3
- https://www.youtube.com/watch?v=gntyAFoZp-E