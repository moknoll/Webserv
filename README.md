
# Configuration Structure for Parsing

It turns out that to continue implementing the parser correctly, we need much more from the config than I initially thought :-). I did some rough estimates and put together approximately how I see the config object (and what we need from it).

## Proposed Structures

```cpp
struct Location
{
    std::string                 path;                   // "/"  "/upload"
    std::string                 root;                   // "./www"
    std::string                 index;                  // "index.html"
    bool                        autoindex;              // Enabling or disabling directory listing
    size_t                      client_max_body_size;
    std::vector<std::string>    allowed_methods;        // GET, POST, DELETE (if not present, all allowed)
                                                        // List of accepted HTTP methods for the route
    std::string                 redirect;               // HTTP redirection (return 301) — 
};

struct ServerConfig
{
    std::string                     host;
    int                             port;
    std::string                     server_name;
    std::map<int, std::string>      error_pages;            // 404 -> "./www/404.html"
    std::vector<Location>           locations;
}
```

### Questions / Notes
1. Do we need `server_name`?
I'm not sure if it's necessary — if we're not implementing virtual hosts, maybe we don't need it.

2. Fields like `path, root, index` — could they also be in the server section? I'm assuming they are moved into the Location struct.
    - Regarding the subject requirement:

    "Uploading files from the clients to the server is authorized, and storage location is provided"

This suggests the config file should probably look something like the example below.


```config
    server {
    listen 0.0.0.0:8080; # or like this host 127.0.0.1; port 8080;
    
    error_page 404 ./www/404.html;
    error_page 500 ./www/500.html;
    
    client_max_body_size 10M;
    
    location / {
        root ./www;
        index index.html;
        autoindex off;
        methods GET POST;
    }
    
    location /upload {
        root ./www/uploads;
        index index.html;
        autoindex on;
        methods GET POST DELETE;
        client_max_body_size 100M;
    }
}
```
