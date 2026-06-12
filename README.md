## Current Implementation Summary: Cgi.cpp

### CgiContext Structure
**Location:** `src/cgi/Cgi.hpp`

A struct that encapsulates all necessary data for managing a CGI process lifecycle:
- **pid**: Process ID of the forked CGI child process
- **stdin_pipe[2]**: File descriptors for the input pipe (parent writes request body here)
- **stdout_pipe[2]**: File descriptors for the output pipe (child writes response here)
- **env_map**: Map storing CGI environment variables (to be converted to char**)
- **envp**: Environment variables array for execve()
- **argv**: Argument vector for execve()
- **exit_status**: Child process exit code
- **deadline**: Timeout timestamp (for timeout management)
- **response**: HttpResponse object (will store the final CGI response)

---

### Functions in Cgi.cpp

#### 1. **buildCgiContext()**
**Purpose:** Initialize and return a new CgiContext with all pipes and variables set to safe defaults (-1 or NULL).
**Usage:** Called at the start of CGI execution to create a clean context.
**Return:** Initialized CgiContext struct

#### 2. **buildCgiEnv()**
**Purpose:** Build the CGI environment variables map from the HTTP request and location configuration.
**Current Implementation:**
- Extracts REQUEST_METHOD from HTTP request
- Parses QUERY_STRING from URI
- Sets CONTENT_LENGTH from request headers
- Constructs SCRIPT_NAME and PATH_INFO
- Sets PATH_TRANSLATED to the CGI script path
- Sets SERVER_PROTOCOL to "HTTP/1.1"
- Sets REDIRECT_STATUS to "200" (required for PHP-CGI security)

**What's Missing:**
- CONTENT_TYPE header parsing
- REMOTE_ADDR (client IP address)
- SERVER_NAME and SERVER_PORT from Host header
- All HTTP headers converted to HTTP_* format (e.g., Accept → HTTP_ACCEPT)

**RFC 3875 Reference:** See [CGI RFC 3875](https://tools.ietf.org/html/rfc3875) for complete variable specifications.

#### 3. **buildCgiEnvp()**
**Purpose:** Convert the env_map (std::map<string, string>) into a char** array suitable for execve().
**Why Needed:** execve() requires environment variables as a NULL-terminated array of "KEY=VALUE" strings, not a C++ map.
**Example Output:**
```
envp[0] = "REQUEST_METHOD=GET"
envp[1] = "QUERY_STRING=param=value"
envp[2] = "HTTP_HOST=example.com"
...
envp[n] = NULL
```

#### 4. **buildCgiArgv()**
**Purpose:** Build the argument vector for execve() containing the CGI script path and optional arguments.
**Typical argv:**
```
argv[0] = "/path/to/script.cgi"
argv[1] = NULL
```

#### 5. **executeChild()**
**Purpose:** Fork a child process and set up pipes for communication with the CGI script.
**Process:**
1. Create stdin_pipe for sending request body to child
2. Create stdout_pipe for receiving response from child
3. Fork the process
4. **In child process:**
   - Redirect STDIN to stdin_pipe[0] (read end)
   - Redirect STDOUT to stdout_pipe[1] (write end)
   - Close all pipe file descriptors (child doesn't need both ends)
   - Call buildCgiEnvp() and buildCgiArgv()
   - Execute execve() with the CGI script
   - Exit with code 127 if execve() fails
5. **In parent process:**
   - Close stdin_pipe[0] (parent writes, not reads)
   - Close stdout_pipe[1] (parent reads, not writes)
   - Store the child's pid
   - Return success

**Why Pipes?** Pipes allow the parent (webserver) to communicate with the child (CGI script):
- **stdin_pipe:** Parent writes HTTP request body → Child reads it
- **stdout_pipe:** Child writes HTTP response → Parent reads it

---

## To-Do List for Team

### Phase 1: Environment Setup
- [ ] **Complete buildCgiEnv()**
  - [ ] Add CONTENT_TYPE parsing from request headers
  - [ ] Extract REMOTE_ADDR (client IP) from HttpRequest
  - [ ] Extract SERVER_NAME and SERVER_PORT from Host header
  - [ ] Parse all HTTP headers and convert to HTTP_* format (e.g., "Accept: text/html" → "HTTP_ACCEPT=text/html")
  - [ ] Handle header names: replace hyphens with underscores, convert to uppercase
  - [ ] Add getter methods to HttpRequest for port and server address

### Phase 2: Environment Variable Conversion
- [ ] **Implement buildCgiEnvp()**
  - [ ] Convert env_map to char** array (allocate memory)
  - [ ] Format each entry as "KEY=VALUE" string
  - [ ] Ensure NULL-termination of the array
  - [ ] Handle memory management (may need cleanup function)
  - [ ] Test with a simple CGI script

- [ ] **Implement buildCgiArgv()**
  - [ ] Extract CGI script path from Location configuration
  - [ ] Build argv with script path as argv[0]
  - [ ] Add query parameters as additional arguments if needed
  - [ ] Ensure NULL-termination
  - [ ] Handle memory management

### Phase 3: Input/Output Handling
- [ ] **Implement writeRequestBody()**
  - [ ] Read request body from HttpRequest
  - [ ] Write body to stdin_pipe[1] (parent write end)
  - [ ] Handle partial writes (loop until all data sent)
  - [ ] Close stdin_pipe[1] when done (signals EOF to child)
  - [ ] Handle write errors gracefully
  - [ ] **Why stdin?** CGI scripts may expect POST data on stdin. The webserver writes the request body to the pipe, child reads from its stdin.

- [ ] **Implement readChildOutput()**
  - [ ] Read response from stdout_pipe[0] (parent read end) in chunks
  - [ ] Accumulate response data (headers + body)
  - [ ] Parse HTTP response status line and headers
  - [ ] Handle EOF when child closes pipe
  - [ ] **Why stdout?** CGI scripts write their complete HTTP response (including status and headers) to stdout. The webserver reads this output and forwards it to the client.

### Phase 4: Response Building
- [ ] **Implement buildResponse()**
  - [ ] Parse CGI script output (status line, headers, body)
  - [ ] Extract status code from first line (e.g., "200 OK")
  - [ ] Extract headers (e.g., "Content-Type: text/html")
  - [ ] Separate headers from body (blank line delimiter)
  - [ ] Create HttpResponse object with parsed data
  - [ ] Handle malformed responses gracefully

### Phase 5: Timeout & Process Management 
- [ ] **Implement cleanup()**
  - [ ] Close all open pipe file descriptors
  - [ ] Wait for child process (waitpid) with timeout
  - [ ] Kill child if timeout exceeded
  - [ ] Clean up any allocated memory (envp, argv)

- [ ] **Add timeout handling**
  - [ ] Use fcntl() to set O_NONBLOCK on pipes
  - [ ] Set deadline timestamp in CgiContext
  - [ ] In main event loop, check if current_time > deadline
  - [ ] Implement graceful timeout handling (close pipes, kill process)
  - [ ] Reference: Check other 42 webserv implementations for fcntl() usage patterns

- [ ] **Integration with HttpHandler**
  - [ ] Uncomment and test CGI execution in HttpHandler::handleCGI()
  - [ ] Verify buildCgiContext() and buildCgiEnv() are called correctly
  - [ ] Ensure HttpResponse is properly returned


## References
- [RFC 3875: CGI 1.1 Specification](https://tools.ietf.org/html/rfc3875)
- [Linux man pages: fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html)